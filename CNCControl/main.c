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



int Xcoord;
int Ycoord;
int Zcoord;

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
      int ch = getchar_timeout_us(0);
      
      if (ch != PICO_ERROR_TIMEOUT) {
        printf("%c\r\n", ch);
        switch (ch)
        {
        case 'a':
          // for (int i = 0; i < 100; i++)
          // {
          //   printf("%d", i);
          //   gpio_put(XSTEP_PIN, 1);
          //   sleep_us(mmhal_high_delay_us);
          //   gpio_put(XSTEP_PIN, 0);
          //   sleep_us(mmhal_low_delay_us);
          // }
          
          
          mmhal_step_motors(-1,0,0);
          break;
        case 'd':
          for (size_t i = 0; i < 50; i++)
          {
            /* code */
            mmhal_step_motors(-1,0,0);
          }
          
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
          printf("Speed: %d", spindle_speed);
          break;
        case '-':
        case '_':
          if (spindle_speed > 0) {
            spindle_speed--;
            mmhal_set_spindle_pwm(spindle_pwm_levels[spindle_speed]);
          }
          printf("Speed: %d", spindle_speed);
          break;
        default:
          // printf("%d", ch);
          break;
        }
      }

      // handle_manual_mode();
    }
    else {
      process_input();
      // handle_command_mode();
    }
  }
}
