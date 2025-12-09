#include <stdio.h>

/* main.c

   This file written 2024 by Artur Podobas and Pedro Antunes

   For copyright and licensing, see file COPYING */


/* Below functions are external and found in other files. */
extern void print(const char*);
extern void print_dec(unsigned int);
extern void display_string(char*);
extern void time2string(char*,int);
extern void tick(int*);
extern void delay(int);
extern int nextprime( int );

int mytime = 0x5957;
char textstring[] = "text, more text, and even more text!";

int timeoutcount = 0;

int prime = 1234567;

// Global Time Variables
int hours = 0;
int minutes = 0;
int seconds = 0;
char textbuffer[30];   // Buffer for time2string

// Interrupt Cause Constants
// Cause 11 is the Standard RISC-V Machine External Interrupt
#define CAUSE_MACHINE_EXTERNAL 11

// Helper function to read Switches
int get_sw(void) {
    volatile int *sw_ptr = (volatile int *)0x04000010;
    return *sw_ptr & 0x3FF;
}

// Helper function to read Button (Bit 1)
int get_btn(void) {
    volatile int *btn_ptr = (volatile int *)0x040000d0;
    return !((*btn_ptr >> 1) & 1);
}

void set_displays(int display_number, int value) {
    static const int hex_codes[] = {
        0xC0, 0xF9, 0xA4, 0xB0, 0x99, 
        0x92, 0x82, 0xF8, 0x80, 0x90
    };
    volatile int *display_ptr = (volatile int *)(0x04000050 + (display_number * 0x10));
    if (value >= 0 && value <= 9) {
        *display_ptr = hex_codes[value];
    } else {
        *display_ptr = 0xFF;
    }
}

/* * INTERRUPT HANDLER 
 * The assembly code calls this function with the 'cause' argument.
 */
void handle_interrupt(unsigned cause) {
    print_dec(cause);

    volatile int *timer_status = (volatile int *)(0x04000020);
    volatile int *btn_edge     = (volatile int *)(0x040000dc);
    
    // We check the devices regardless of the specific cause number, 
    // because on some boards they map to 16/17 (Local) and on others to 11 (External).
    // This makes the handler robust for both configurations.
    
    // ==========================================
    // 1. CHECK TIMER (Address 0x04000020)
    // ==========================================
    if (cause == 16) {
        if ((*timer_status & 1) == 1) {
            // Acknowledge Timer Interrupt
            *timer_status = 0; 
            timeoutcount++;

            if (timeoutcount >= 10) {
                timeoutcount = 0;
                // Standard 1-second increment
                seconds++;
                if (seconds >= 60) {
                    seconds = 0;
                    minutes++;
                    if (minutes >= 60) {
                        minutes = 0;
                        hours++;
                        if (hours >= 24) hours = 0;
                    }
                }
            }
        }
    }

    // ==========================================
    // 2. CHECK BUTTON (Address 0x040000dc)
    // ==========================================
    // Check if Button 2 (Bit 1) triggered it via the Edge Capture register
    if (cause == 18) {
        if (1) {
            // A. Increment by 2 seconds
            seconds += 2;

            // B. Handle Overflow
            if (seconds >= 60) {
                seconds -= 60; 
                minutes++;
                if (minutes >= 60) {
                    minutes = 0;
                    hours++;
                    if (hours >= 24) hours = 0;
                }
            }
            // C. Clear the Edge Capture (Interrupt Acknowledge)
            // Write 1 to Bit 1 to clear it
            *btn_edge = 2; 
        }
    }

    // ==========================================
    // 3. UPDATE DISPLAYS
    // ==========================================
    tick(&mytime);
    time2string(textbuffer, mytime);
    display_string(textbuffer);

    set_displays(0, seconds % 10);
    set_displays(1, seconds / 10);
    set_displays(2, minutes % 10);
    set_displays(3, minutes / 10);
    set_displays(4, hours % 10);
    set_displays(5, hours / 10);
}

/* Initialize Interrupts and Timer */
void labinit(void) {
    // Timer Pointers
    volatile int *timer_periodl = (volatile int *)(0x04000020 + 0x8);
    volatile int *timer_periodh = (volatile int *)(0x04000020 + 0xC);
    volatile int *timer_control = (volatile int *)(0x04000020 + 0x4);
    volatile int *timer_status  = (volatile int *)(0x04000020);

    // Button Pointers
    volatile int *btn_mask = (volatile int *)(0x040000d0 + 0x8); 
    volatile int *btn_edge = (volatile int *)(0x040000d0 + 0xC);

    // 1. Setup Timer Hardware (100ms)
    *timer_periodl = 0xC6C0; 
    *timer_periodh = 0x002D; 
    *timer_control = 0x7;    // Start (1) + Cont (2) + ITO (4) = 7. Correct.
    *timer_status = 0;

    // 2. Setup Button Hardware
    *btn_mask = 2;   // Enable interrupt for Button 2 (Bit 1)
    *btn_edge = 0xF; // Clear any previous edges

    // 3. Enable RISC-V CPU Interrupts
    // We enable multiple bits to ensure we catch the interrupt regardless of mapping:
    // Bit 11 (0x800): Machine External Interrupt (Standard RISC-V)
    // Bit 16 (0x10000): Local Int 0 (Timer)
    // Bit 17 (0x20000): Local Int 1 (Often Buttons, but maybe not on your board)
    // Bit 18 (0x40000): Local Int 2 (You suspected the button is here)
    // Value: 0x40000 + 0x20000 + 0x10000 + 0x800 = 0x70800
    asm volatile("csrs mie, %0" :: "r"(0x70800));

    // Enable Global Interrupts (Bit 3 in mstatus)
    asm volatile("csrs mstatus, %0" :: "r"(0x8));
}

int main() {
    labinit();

    while (1) {
        prime = nextprime(prime);
    }
}