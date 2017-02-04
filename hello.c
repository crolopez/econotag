/*
  Simple test program for Econotag development board
  Activates or deactivates the LEDs configured through UART commands

  Author: Cristóbal López Peñalver
  February 2, 2017
*/

#include <stdint.h>
#include <stdio.h>
// BSP headers
#include "system.h"
#include "math.h"

// The red led is in GPIO 44
#define RED_LED gpio_pin_44
// The red led is in GPIO 45
#define GREEN_LED gpio_pin_45
// S3 switch
#define s3_out gpio_pin_22
#define s3_in gpio_pin_26
// S2 switch
#define s2_out gpio_pin_23
#define s2_in gpio_pin_27

// Application delay
uint32_t const delay = 0xB0000;
// Leds state. Only switch on when set to 1
uint8_t red_led = 1;
uint8_t green_led = 2;

void pause(uint32_t delay) {
    uint32_t i;
    // Active waiting. No sleep support
    for (i = 0; i < delay; i++);
}

void uart_callback() {
    switch (getchar()) {
    case 'g':
        printf("Green led is on.\r\n");
        green_led = 1;
        break;
    case 'G':
        printf("Green led is off\r\n");
        green_led = 2;
        break;
    case 'R':
        printf("Red led is off.\r\n");
        red_led = 2;
        break;
    case 'r':
        printf("Red led is on.\r\n");
        red_led = 1;
        break;
    default:
        printf("\r\n****************\r\nTurn the red led on/off (r/R)\r\nTurn the green led on/off (g/G)\r\n****************\r\n");
        break;
    }
}

void gpio_init(void) {
    // Configure the 44 (red led) and 45 (green led) as output
    gpio_set_port_dir_output(gpio_port_1, (1 << (RED_LED - 32)) | (1 << (GREEN_LED - 32)));

    // Configure the S3 and S2 buttons
    gpio_set_port_dir_input(gpio_port_0, (1 << s2_in) | (1 << s3_in));
    gpio_set_port_dir_output(gpio_port_0, (1 << s2_out) | (1 << s3_out));
    // Init the buttons
    gpio_set_port(gpio_port_0, (1 << s2_out) | (1 << s3_out));

    // Turn off the leds
    gpio_clear_pin(RED_LED);
    gpio_clear_pin(GREEN_LED);
}

int main() {
    // Configure the GPIOs
    gpio_init();
    // Set the uart callback to handle the leds and the printed messages
    uart_set_receive_callback(uart_1, uart_callback);

    // This loop will make the activated leds blink
    while (1) {
        if (green_led == 1) {
            gpio_set_pin(GREEN_LED);
        } else if (green_led == 2) {
            gpio_clear_pin(GREEN_LED);
        }

        if (red_led == 1) {
            gpio_set_pin(RED_LED);
        } else if (red_led == 2) {
            gpio_clear_pin(RED_LED);
        }

        pause(delay);
        gpio_clear_pin(RED_LED);
        gpio_clear_pin(GREEN_LED);
        pause(delay);
    }

    return 0;
}
