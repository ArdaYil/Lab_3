#include <stdio.h>

/* main.c

   This file written 2024 by Artur Podobas and Pedro Antunes

   For copyright and licensing, see file COPYING */

/* Below functions are external and found in other files. */
extern void print(const char*);
extern void print_dec(unsigned int);
extern void display_string(char*);
extern void time2string(char*, int);
extern void tick(int*);
extern void delay(int);
extern int nextprime(int);

int mytime = 0x5957;
char textstring[] = "text, more text, and even more text!";

int timeoutcount = 0;

int prime = 1234567;

// Global Time Variables
int hours = 0;
int minutes = 0;
int seconds = 0;
char textbuffer[30];  // Buffer for time2string

// Helper function to read the Second Button (Bit 1)
int get_btn(void) {
    return *(volatile int*)0x040000d0 & 1;
}

// Helper function to read Switches
int get_sw(void) {
    volatile int* sw_ptr = (volatile int*)0x04000010;
    return *sw_ptr & 0x3FF;
}

void set_displays(int display_number, int value) {
    static const int hex_codes[] = {
        0xC0, 0xF9, 0xA4, 0xB0, 0x99,
        0x92, 0x82, 0xF8, 0x80, 0x90};
    volatile int* display_ptr = (volatile int*)(0x04000050 + (display_number * 0x10));
    if (value >= 0 && value <= 9) {
        *display_ptr = hex_codes[value];
    } else {
        *display_ptr = 0xFF;
    }
}

// INTERRUPT HANDLER
void handle_interrupt(unsigned int cause) {
    volatile int* timer_status = (volatile int*)(0x04000020);
    int current_btn;
    int sw_val;
    int selector;
    int value;

    // Check if the interrupt was caused by the Timer (Bit 0 of Status is 1)
    if ((*timer_status & 1) == 1) {
        // Acknowledge the interrupt by clearing the status register
        *timer_status = 0;

        // Increment the global counter
        timeoutcount++;

        // Only update displays and time once every 10 timeouts (1 Second)
        if (timeoutcount >= 10) {
            timeoutcount = 0;

            // --- Button Logic ---
            current_btn = get_btn();
            if (current_btn != 0) {
                sw_val = get_sw();
                selector = (sw_val >> 8) & 0x03;
                value = sw_val & 0x3F;

                if (selector == 1) {  // Set Seconds
                    seconds = (value < 60) ? value : 59;
                } else if (selector == 2) {  // Set Minutes
                    minutes = (value < 60) ? value : 59;
                } else if (selector == 3) {  // Set Hours
                    hours = (value < 24) ? value : 23;
                }
            }

            // --- 7-Segment Clock Logic ---
            seconds++;
            if (seconds >= 60) {
                seconds = 0;
                minutes++;
                if (minutes >= 60) {
                    minutes = 0;
                    hours++;
                    if (hours >= 24) {
                        hours = 0;
                    }
                }
            }

            // Update the display registers
            set_displays(0, seconds % 10);
            set_displays(1, seconds / 10);
            set_displays(2, minutes % 10);
            set_displays(3, minutes / 10);
            set_displays(4, hours % 10);
            set_displays(5, hours / 10);
        }
    }

    tick(&mytime);
}

/* Initialize Interrupts and Timer */
void labinit(void) {
    volatile int* timer_periodl = (volatile int*)(0x04000020 + 0x8);
    volatile int* timer_periodh = (volatile int*)(0x04000020 + 0xC);
    volatile int* timer_control = (volatile int*)(0x04000020 + 0x4);
    volatile int* timer_status = (volatile int*)(0x04000020);

    // 1. Setup Timer Hardware (3,000,000 cycles = 100ms)
    *timer_periodl = 0xC6C0;
    *timer_periodh = 0x002D;

    // Start Timer + Continuous + Interrupt Enable (ITO)
    *timer_control = 0x7;

    // Clear pending status
    *timer_status = 0;
}

int main() {
    labinit();

    while (1) {
        print("Prime: ");
        prime = nextprime(prime);
        print_dec(prime);
        print("\n");
    }
}