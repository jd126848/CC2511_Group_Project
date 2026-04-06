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
#define NUMSPEEDS 5
#define CNCVERSION 1

const int spindle_pwm_levels[NUMSPEEDS] = {0, 64, 128, 192, 256};
int spindle_speed = 0;

int microsteps = 1;

int current_coords[3];

int next_coords[3];
int feed_rate; 

bool manual_mode = true; // Start in manual mode for testing


char command[COMMAND_BUFFER_SIZE];
unsigned int command_index = 0;
bool command_complete = false;

void handle_manual_mode() {
  int ch = getchar_timeout_us(0);
  if (ch != PICO_ERROR_TIMEOUT) {
    printf("%c\r\n", ch);
    switch (ch) {
      case 'a':
        mmhal_step_motors(-1,0,0);
        break;
      case 'd':
        mmhal_step_motors(1,0,0);
        break;
      case 'w':
        mmhal_step_motors(0,1,0);
        break;
      case 's':
        mmhal_step_motors(0,-1,0);
        break;
      case 'q':
        mmhal_step_motors(0,0,1);
        break;
      case 'e':
        mmhal_step_motors(0,0,-1);
        break;
      case '=':
      case '+':
        if (spindle_speed < NUMSPEEDS -1) {
          spindle_speed++;
          mmhal_set_spindle_pwm(spindle_pwm_levels[spindle_speed]);
        }
        printf("Speed: %d\r\n", spindle_speed);
        break;
      case '-':
      case '_':
        if (spindle_speed > 0) {
          spindle_speed--;
          mmhal_set_spindle_pwm(spindle_pwm_levels[spindle_speed]);
        }
        printf("Speed: %d", spindle_speed);
        break;
      case '0':
        mmhal_set_microstepping(0, MMHAL_MS_MODE_1);
        mmhal_set_microstepping(1, MMHAL_MS_MODE_1);
        microsteps = 1;
        break;
      case '1':
        mmhal_set_microstepping(0, MMHAL_MS_MODE_2);
        mmhal_set_microstepping(1, MMHAL_MS_MODE_2);
        microsteps = 2;
        break;
      case '2':
        mmhal_set_microstepping(0, MMHAL_MS_MODE_4);
        mmhal_set_microstepping(1, MMHAL_MS_MODE_4);
        microsteps = 4;
        break;
      case '3':
        mmhal_set_microstepping(0, MMHAL_MS_MODE_8);
        mmhal_set_microstepping(1, MMHAL_MS_MODE_8);
        microsteps = 8;
        break;
      case '4':
        mmhal_set_microstepping(0, MMHAL_MS_MODE_16);
        mmhal_set_microstepping(1, MMHAL_MS_MODE_16);
        microsteps = 16;
        break;
      case '5':
        mmhal_set_microstepping(0, MMHAL_MS_MODE_32);
        mmhal_set_microstepping(1, MMHAL_MS_MODE_32);
        microsteps = 32;
        break;
      case '\033': // escape character
        manual_mode = false;
        break;
      default:
        // printf("%d", ch);
        break;
    }      
  }   
}

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

void handle_command_mode() {
  process_input();
  if (command_complete) {
    if (3 == sscanf(command, "G00 X%i Y%i Z%i", &next_coords[XDIM], &next_coords[YDIM], &next_coords[ZDIM])) {
      // Move to coords x,y,z no microstepping
    }
    else if (3 <= sscanf(command, "G01 X%i Y%i Z%i F%i", &next_coords[XDIM], &next_coords[YDIM], &next_coords[ZDIM], &feed_rate)) {
      // Move to coords x,y,z (F is optional)
    }
    else if (1 == sscanf(command, "G04 P%f")) {

    }
    else if ()
  }
}

int main(void) {
  // Initialise components and variables
  stdio_init_all();
  mmhal_init();

  while (true) {
    //  Repeated code here
    if (manual_mode) {
      handle_manual_mode();
      }
    else {
      handle_command_mode();
    }
  }
}
