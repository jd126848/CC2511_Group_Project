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

// -------------------------
// Machine state (minimal)
// -------------------------
typedef struct {
    int absolute_mode;   // G90 = 1, G91 = 0
    int units_mm;        // G21 = 1, G20 = 0
    float feedrate;
} MachineState;

MachineState state;

// -------------------------
// Utility: remove comments
// -------------------------
void strip_comment(char *line)
{
    char *p = strchr(line, ';');
    if (p) *p = '\0';
}

// -------------------------
// Utility: trim whitespace
// -------------------------
void trim(char *line)
{
    // left trim
    while (isspace((unsigned char)*line)) {
        memmove(line, line + 1, strlen(line));
    }

    // right trim
    int len = strlen(line);
    while (len > 0 && isspace((unsigned char)line[len - 1])) {
        line[len - 1] = '\0';
        len--;
    }
}

// -------------------------
// Extract G or M code
// -------------------------
int get_gcode(char *line)
{
    int g;
    if (sscanf(line, "G%d", &g) == 1) return g;
    return -1;
}

int get_mcode(char *line)
{
    int m;
    if (sscanf(line, "M%d", &m) == 1) return m;
    return -1;
}

void process_line(char *line)
{
    strip_comment(line);
    trim(line);

    if (strlen(line) == 0)
        return;

    int g = get_gcode(line);
    int m = get_mcode(line);

    // -------------------------
    // G-code handling
    // -------------------------
    if (g != -1)
    {
        if (g == 0 || g == 1)
        {
            handle_linear_motion(line);
        }
        else if (g == 2 || g == 3)
        {
            handle_arcs(line);
        }
        else if (g == 4)
        {
            float p;
            if (sscanf(line, "G4 P%f", &p) == 1)
                printf("Dwell for %.2f\n", p);
        }
        else if (g == 20 || g == 21 || g == 90 || g == 91 || g == 28)
        {
            handle_modes(line);
        }

        return;
    }

    // -------------------------
    // M-code handling
    // -------------------------
    if (m != -1)
    {
        handle_spindle(line);
    }
}

// =======================================================
// GROUP HANDLERS
// =======================================================

// -------------------------
// Linear motion (G0 / G1)
// -------------------------
void handle_linear_motion(char *line)
{
    int x, y, z;
    float f = state.feedrate;

    int has_xyz = sscanf(line, "%*s X%d Y%d Z%d", &x, &y, &z);

    if (has_xyz >= 2)
    {
        if (strstr(line, "F")) {
            sscanf(line, "%*s X%d Y%d Z%d F%f", &x, &y, &z, &f);
        }

        if (strstr(line, "G0")) {
            // rapid move
            printf("Rapid move to %d %d %d\n", x, y, z);
        } else {
            // linear move
            printf("Linear move to %d %d %d F%.2f\n", x, y, z, f);
        }
    }
}

// -------------------------
// Arc motion (G2 / G3)
// -------------------------
void handle_arcs(char *line)
{
    int x, y, i, j;

    if (sscanf(line, "%*s X%d Y%d I%d J%d", &x, &y, &i, &j) == 4)
    {
        if (strstr(line, "G2")) {
            printf("CW Arc to %d %d center %d %d\n", x, y, i, j);
        } else {
            printf("CCW Arc to %d %d center %d %d\n", x, y, i, j);
        }
    }
}

// -------------------------
// Modes (G20/21/90/91/28)
// -------------------------
void handle_modes(char *line)
{
    int g;
    if (sscanf(line, "G%d", &g) != 1) return;

    switch (g)
    {
        case 20:
            state.units_mm = 0;
            printf("Units: inches\n");
            break;

        case 21:
            state.units_mm = 1;
            printf("Units: mm\n");
            break;

        case 90:
            state.absolute_mode = 1;
            printf("Absolute mode\n");
            break;

        case 91:
            state.absolute_mode = 0;
            printf("Relative mode\n");
            break;

        case 28:
            printf("Homing cycle\n");
            break;
    }
}

// -------------------------
// Spindle (M3 / M5)
// -------------------------
void handle_spindle(char *line)
{
    int m;
    if (sscanf(line, "M%d", &m) != 1) return;

    if (m == 3) {
        printf("Spindle ON (CW)\n");
    }
    else if (m == 5) {
        printf("Spindle OFF\n");
    }
}

void calibrate_position() {
  while (gpio_get(XLIMIT_PIN)) {
    mmhal_step_motors(-1,0,0);
  }
  while (gpio_get(YLIMIT_PIN)) {
    mmhal_step_motors(-1,0,0);
  }
  while (gpio_get(ZLIMIT_PIN)) {
    mmhal_step_motors(-1,0,0);
  }
}



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
    process_line(command);
    command_complete = false;
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
