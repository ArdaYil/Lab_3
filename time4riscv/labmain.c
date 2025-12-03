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

/* Below is the function that will be called when an interrupt is triggered. */
void handle_interrupt(unsigned cause) 
{}

/* Add your code here for initializing interrupts. */
void labinit(void)
{}

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

/* Your code goes into main as well as any needed functions. */
int main() {
    // Call labinit()
    labinit();

   int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int ms_tick;
    int current_btn;
    int sw_val;
    int selector;
    int value;
    volatile int delay;

    /*
     * Infinite Loop: Digital Clock Logic
     */
    while (1) {
        // We simulate 1 second by looping 1000 times with a small delay.
        for (ms_tick = 0; ms_tick < 1000; ms_tick++) {
            
            // Get current button state
            current_btn = get_btn();

            // 1. Check Button & Update Time CONSTANTLY
            // Because get_btn now inverts logic (Active Low), 
            // it returns 1 when pressed.
            if (current_btn != 0) {
                sw_val = get_sw();
                
                // Extract Selector (Bits 9 and 8)
                // 01 (1) = Seconds, 10 (2) = Minutes, 11 (3) = Hours
                selector = (sw_val >> 8) & 0x03;
                
                // Extract Value (Bits 0-5)
                value = sw_val & 0x3F;

                if (selector == 1) { // Set Seconds
                    if (value < 60) seconds = value;
                    else seconds = 59; 
                } else if (selector == 2) { // Set Minutes
                    if (value < 60) minutes = value;
                    else minutes = 59;
                } else if (selector == 3) { // Set Hours
                    if (value < 24) hours = value;
                    else hours = 23;
                }
            }

            // 2. Refresh Displays continuously
            set_displays(0, seconds % 10);
            set_displays(1, seconds / 10);
            set_displays(2, minutes % 10);
            set_displays(3, minutes / 10);
            set_displays(4, hours % 10);
            set_displays(5, hours / 10);

            // 3. Small Delay (approx 1ms)
            for(delay = 0; delay < 1000; delay++);
        }

        // 4. Increment the time (After approx 1 second)
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
    }

    return 0;
}


