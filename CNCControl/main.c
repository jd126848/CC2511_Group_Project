/**************************************************************
 * main.c
 * rev 1.0 13-Dec-2025 Bruce
 * milling_basic
 * ***********************************************************/

#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdio.h> // printf
#include "hardware/pwm.h"
#include "mmhal.h"

#define COMMAND_BUFFER_SIZE 64

bool manual_mode = true; // Start in manual mode for testing


char command[COMMAND_BUFFER_SIZE];
unsigned int command_index = 0;
bool command_complete = false;


void process_input() {
  int ch = getchar_timeout_us(0);
  if ( ch != PICO_ERROR_TIMEOUT ) {
    switch (ch) {
      case '\r':
      case '\n':
        command[command_index] = 0;
        printf("\n");
        command_index = 0;
        command_complete = true;
        break;
      
      case '\b':
      case '\177':
        printf("%c", ch);
        command_index--;
        break;

      default:
        if (command_index != COMMAND_BUFFER_SIZE - 1) {
          command[command_index] = ch;
          printf("%c", ch);
          command_index++;
        }
        break;
    } 
  }
}


int main(void) {
  // Initialise components and variables
  stdio_init_all();
  mmhal_init();

  while (true) {
    //  Repeated code here
    if (manual_mode) {
      process_input();
      // handle_manual_mode();
    }
    else {
      // handle_command_mode();
    }
  }
}
