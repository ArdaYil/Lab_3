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

/* Below is the function that will be called when an interrupt is triggered. */
void handle_interrupt(unsigned cause) 
{}

/* Add your code here for initializing interrupts. */
void labinit(void) {
    volatile int *timer_periodl = (volatile int *)(0x04000020 + 0x8);
    volatile int *timer_periodh = (volatile int *)(0x04000020 + 0xC);
    volatile int *timer_control = (volatile int *)(0x04000020 + 0x4);
    volatile int *timer_status  = (volatile int *)(0x04000020);

    // 3,000,000 in hex is 0x2DC6C0
    // Write Period registers (Low then High)
    *timer_periodl = 0xC6C0; // Lower 16 bits
    *timer_periodh = 0x002D; // Upper 16 bits

    // Start Timer with Continuous mode
    // Bit 1: CONT, Bit 2: START. Value = 0110 binary = 0x6
    *timer_control = 0x6;
    
    // Clear any pending status
    *timer_status = 0;
}

void set_leds(int led_mask) {
    /* * 1. Cast the raw address literal to a pointer to a volatile integer.
     * 'volatile' tells the compiler that the value at this address can change 
     * outside the program's control (or has side effects), so it must 
     * perform the write every time this code runs.
     */
    volatile int *led_ptr = (volatile int *)0x04000000;

    /*
     * 2. Dereference the pointer to write the mask value to that address.
     * Only the first 10 bits affect the LEDs, but we write the whole int.
     */
    *led_ptr = led_mask;
}

void set_displays(int display_number, int value) {
    // Lookup table for 7-segment decoding (Active Low)
    // 0 = Segment ON, 1 = Segment OFF
    // Mapping assumed: bit0=A, bit1=B, ... bit6=G
    static const int hex_codes[] = {
        0xC0, // 0: 1100 0000 (A,B,C,D,E,F on)
        0xF9, // 1: 1111 1001 (B,C on)
        0xA4, // 2: 1010 0100 (A,B,D,E,G on)
        0xB0, // 3: 1011 0000 (A,B,C,D,G on)
        0x99, // 4: 1001 1001 (B,C,F,G on)
        0x92, // 5: 1001 0010 (A,C,D,F,G on)
        0x82, // 6: 1000 0010 (A,C,D,E,F,G on)
        0xF8, // 7: 1111 1000 (A,B,C on)
        0x80, // 8: 1000 0000 (All on)
        0x90  // 9: 1001 0000 (A,B,C,D,F,G on)
    };

    // Calculate the specific address for the requested display
    volatile int *display_ptr = (volatile int *)(0x04000050 + (display_number * 0x10));

    // Write the decoded value if it is a valid digit
    if (value >= 0 && value <= 9) {
        *display_ptr = hex_codes[value];
    } else {
        // Turn off all segments if value is invalid (0xFF = 1111 1111)
        *display_ptr = 0xFF;
    }
}

int get_sw(void) {
    // Create a volatile pointer to the switch address
    volatile int *sw_ptr = (volatile int *)0x04000010;
    
    // Read the value from the memory address
    int sw_value = *sw_ptr;
    
    // Mask the result to return only the 10 least significant bits
    // 0x3FF is 11 1111 1111 in binary
    return sw_value & 0x3FF;
}

int get_btn(void) {
    return *(volatile int*) 0x040000d0 & 1;
}

void assignment_1_d() {
    int count = 0;
    int limit = 0xF; // 0xF is 1111 in binary (First 4 LEDs ON)

    // Loop until we reach the limit
    while (1) {
        // Update the LEDs with the current counter value
        set_leds(count);

        // Check if all first 4 LEDs are on
        if (count == limit) {
            break; // Stop the program
        }

        // Increment for the next iteration
        count++;

        // Approximate 1 second delay
        // Using volatile to prevent the compiler from optimizing the empty loop away
        volatile int delay_counter;
        for (delay_counter = 0; delay_counter < 1000000; delay_counter++);
    }
}

void test_toggles() {
    while (1) {
        // 1. Read the current state of the switches
        int current_switch_value = get_sw();

        // 2. Write that value to the LEDs
        // If you toggle a switch on the board, the corresponding LED should light up.
        set_leds(current_switch_value);

        // 3. (Optional) Display the value on the first 7-segment display
        // We use % 10 to ensure we only pass 0-9, as our set_displays function
        // currently only supports decimal digits.
        set_displays(0, current_switch_value % 10);
    }
}

int main() {
    // Declare all variables at the top of the function
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int current_btn;
    int sw_val;
    int selector;
    int value;
    int mytime = 0;        // Time variable for the assembly functions
    char textbuffer[30];   // Buffer for time2string

    // Pointers for Timer registers
    volatile int *timer_status = (volatile int *)(0x04000020);

    // Initialize Timer
    labinit();

    /*
     * Infinite Loop: Digital Clock Logic
     */
    while (1) {
        // 1. Check Button & Switches CONSTANTLY (Every loop iteration)
        current_btn = get_btn();
        
        if (current_btn != 0) {
            sw_val = get_sw();
            
            // Extract Selector (Bits 9 and 8)
            selector = (sw_val >> 8) & 0x03;
            // Extract Value (Bits 0-5)
            value = sw_val & 0x3F;

            if (selector == 1) { // Set Seconds
                seconds = (value < 60) ? value : 59;
            } else if (selector == 2) { // Set Minutes
                minutes = (value < 60) ? value : 59;
            } else if (selector == 3) { // Set Hours
                hours = (value < 24) ? value : 23;
            }
        }

        // 2. Check Timer Timeout (Polling)
        // Bit 0 of Status register is 1 if timeout occurred
        if ((*timer_status & 1) == 1) {
            // Reset the timeout flag (Write 0 to Status)
            *timer_status = 0;

            // Increment the global counter
            timeoutcount++;

            // Only update displays and time once every 10 timeouts (1 Second)
            if (timeoutcount >= 10) {
                timeoutcount = 0;

                // --- Assembly Function Calls ---
                tick(&mytime);
                time2string(textbuffer, mytime);
                display_string(textbuffer);

                // --- 7-Segment Clock Logic ---
                // Increment time logic
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
    }

    return 0;
}


