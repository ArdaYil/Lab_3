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
extern void enable_interrupt(void); // added

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
#define CAUSE_MACHINE_EXTERNAL 11

// Helper function to read Switches
int get_sw(void) {
    volatile int *sw_ptr = (volatile int *)0x04000010;
    return *sw_ptr & 0x3FF;
}

// Helper function to read Button (Bit 1)
int get_btn(void) {
    volatile int *btn_ptr = (volatile int *)0x040000d0;
    return (*btn_ptr >> 1) & 1; 
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
    volatile int *timer_status = (volatile int *)(0x04000020);
    volatile int *btn_edge     = (volatile int *)(0x040000dc);
    
    // Debug Print (Uncomment if needed)
    if (cause != 16) { 
        print("Cause: "); print_dec(cause); print("\n"); 
    }

    // ==========================================
    // 1. CHECK TIMER (Address 0x04000020)
    // ==========================================
    if (cause == 16) {
        if ((*timer_status & 1) == 1) {
            *timer_status = 0; 
            timeoutcount++;

            if (timeoutcount >= 10) {
                timeoutcount = 0;
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
    if (cause == 18) {
        // A. Acknowledge IMMEDIATELY.
        // We write 1s to all bits. This is critical to stop the interrupt line.
        *btn_edge = 0;

        // B. Increment Logic
        // Since Cause 18 fired, we know a button was pressed.
        // We skip checking specific bits because the register read was unreliable (returned 0).
        seconds += 2;
        if (seconds >= 60) {
            seconds -= 60; 
            minutes++;
            if (minutes >= 60) {
                minutes = 0;
                hours++;
                if (hours >= 24) hours = 0;
            }
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
    *timer_control = 0x7;    // Start (1) + Cont (2) + ITO (4) = 7.
    *timer_status = 0;

    // 2. Setup Button Hardware
    // Enable interrupts for ALL 4 buttons (0xF = 1111)
    // This ensures we catch the click even if you press Button 0 or Button 1.
    *btn_mask = 1;   
    
    // Clear any previous edges to prevent immediate interrupt on start
    //*btn_edge = 0xF; 

    enable_interrupt();
}

int main() {
    labinit();

    while (1) {
        prime = nextprime(prime);
    }
}