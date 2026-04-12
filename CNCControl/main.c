/**************************************************************
 * main.c
 * rev 1.0 13-Dec-2025 Bruce
 * milling_basic
 * ***********************************************************/

#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdio.h> // printf
#include <string.h>
#include "hardware/pwm.h"
#include "mmhal.h"
#include <ctype.h>

#define COMMAND_BUFFER_SIZE 64
#define NUMSPEEDS 5

const int spindle_pwm_levels[NUMSPEEDS] = {0, 64, 128, 192, 256};

bool manual_mode = true; // Start in manual mode for testing

char command[COMMAND_BUFFER_SIZE];
unsigned int command_index = 0;
bool command_complete = false;


// -------------------------
// Machine state
// -------------------------
typedef struct {
  bool absolute_mode;   // G90 = 1, G91 = 0
  bool units_mm;        // G21 = 1, G20 = 0
  float feedrate;
  float current_coords[3];
  float home_coords[3];
  int corner_coords[3];
  int spindle_speed;
  int microsteps;
} MachineState;

MachineState state;


int distance_to_steps(float distance) {
  if (state.units_mm) {
    return distance*state.microsteps/STEPS_PER_MM;
  }
  else {
    return distance*25.4*state.microsteps/STEPS_PER_MM; // 25.4mm/inch
  }
}


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

// =======================================================
// GROUP HANDLERS
// =======================================================

// -------------------------
// Linear motion (G0 / G1)
// -------------------------
void handle_linear_motion(char *line, int g)
{
  float x = state.current_coords[XDIM], y = state.current_coords[YDIM], z = state.current_coords[ZDIM], f = state.feedrate;
  // float f = state.feedrate;

  // int has_xyz = sscanf(line, "%*s X%d Y%d Z%d", &x, &y, &z);

  char *ptr = line;
  while (*ptr) {
    if (*ptr == 'X') sscanf(ptr+1, "%f", &x);
    if (*ptr == 'Y') sscanf(ptr+1, "%f", &y);
    if (*ptr == 'Z') sscanf(ptr+1, "%f", &z);
    if (*ptr == 'F') {sscanf(ptr+1, "%f", &f); state.feedrate = f;}
    ptr++;
  }

  if (g == 0) {
    // rapid move
    printf("Rapid move to %f %f %f\n", x, y, z);
  }
  else {
    // linear move
    printf("Linear move to %f %f %f F%.2f\n", x, y, z, f);
    mmhal_step_motors(x-state.current_coords[XDIM],y-state.current_coords[YDIM],z-state.current_coords[ZDIM]);
  }

  // if (has_xyz >= 2)
  // {
  //     if (strstr(line, "F")) {
  //         sscanf(line, "%*s X%d Y%d Z%d F%f", &x, &y, &z, &f);
  //     }

  //     if (strstr(line, "G0")) {
  //         // rapid move
  //         printf("Rapid move to %d %d %d\n", x, y, z);
  //     } else {
  //         // linear move
  //         printf("Linear move to %d %d %d F%.2f\n", x, y, z, f);
  //     }
  // }
}

// -------------------------
// Arc motion (G2 / G3)
// -------------------------
void handle_arcs(char *line)
{
  float x = 0, y = 0, i = 0, j = 0;
  char *ptr = line;
  while (*ptr) {
    if (*ptr == 'X') sscanf(ptr+1, "%f", &x);
    if (*ptr == 'Y') sscanf(ptr+1, "%f", &y);
    if (*ptr == 'I') sscanf(ptr+1, "%f", &i);
    if (*ptr == 'J') sscanf(ptr+1, "%f", &j);
    ptr++;
  }


  // if (sscanf(line, "%*s X%f Y%f I%f J%f", &x, &y, &i, &j) == 4)
  // {
  if (strstr(line, "G2")) {
      printf("CW Arc to %f %f center %f %f\n", x, y, i, j);
  } else {
      printf("CCW Arc to %f %f center %f %f\n", x, y, i, j);
  }
  // }
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
    switch (g)
    {
    case 0: case 1:
      handle_linear_motion(line, g);
      break;

    case 2: case 3:
      handle_arcs(line);
      break;

    case 4: {
      float p;
      if (sscanf(line, "G4 P%f", &p) == 1)
          printf("Dwell for %.2f\n", p);
      break;
    }

    case 20:
      state.units_mm = false;
      printf("Units: inches\n");
      break;

    case 21:
      state.units_mm = true;
      printf("Units: mm\n");
      break;

    case 90:
      state.absolute_mode = true;
      printf("Absolute mode\n");
      break;

    case 91:
      state.absolute_mode = false;
      printf("Relative mode\n");
      break;

    case 28:
      printf("Homing cycle\n");
      break;

    default:
      break;
    }
      // if (g == 0 || g == 1)
      // {
      //     handle_linear_motion(line);
      // }
      // else if (g == 2 || g == 3)
      // {
      //     handle_arcs(line);
      // }
      // else if (g == 4)
      // {
      //     float p;
      //     if (sscanf(line, "G4 P%f", &p) == 1)
      //         printf("Dwell for %.2f\n", p);
      // }
      // else if (g == 20 || g == 21 || g == 90 || g == 91 || g == 28)
      // {
      //     handle_modes(line);
      // }

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








void calibrate_position() {
  while (gpio_get(XLIMIT_PIN)) {
    mmhal_step_motors(-1,0,0);
  }
  state.corner_coords[XDIM] = 0;
  printf("X-Limit Reached\r\n");

  while (gpio_get(YLIMIT_PIN)) {
    mmhal_step_motors(0,-1,0);
  }
  state.corner_coords[YDIM] = 0;
  printf("Y-Limit Reached\r\n");

  while (gpio_get(ZLIMIT_PIN)) {
    mmhal_step_motors(0,0,-1);
  }
  state.corner_coords[ZDIM] = 0;
  printf("Z-Limit Reached\r\n");
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
        mmhal_step_motors(0,0,-1);
        break;
      case 'e':
        mmhal_step_motors(0,0,1);
        break;
      case '=':
      case '+':
        if (state.spindle_speed < NUMSPEEDS -1) {
          state.spindle_speed++;
          mmhal_set_spindle_pwm(spindle_pwm_levels[state.spindle_speed]);
        }
        printf("Speed: %d\r\n", state.spindle_speed);
        break;
      case '-':
      case '_':
        if (state.spindle_speed > 0) {
          state.spindle_speed--;
          mmhal_set_spindle_pwm(spindle_pwm_levels[state.spindle_speed]);
        }
        printf("Speed: %d\r\n", state.spindle_speed);
        break;
      case '0':
        mmhal_set_microstepping(0, MMHAL_MS_MODE_1);
        mmhal_set_microstepping(1, MMHAL_MS_MODE_1);
        state.microsteps = 1;
        printf("Microstepping Mode: 0");
        break;
      case '1':
        mmhal_set_microstepping(0, MMHAL_MS_MODE_2);
        mmhal_set_microstepping(1, MMHAL_MS_MODE_2);
        state.microsteps = 2;
        printf("Microstepping Mode: 1");
        break;
      case '2':
        mmhal_set_microstepping(0, MMHAL_MS_MODE_4);
        mmhal_set_microstepping(1, MMHAL_MS_MODE_4);
        state.microsteps = 4;
        printf("Microstepping Mode: 2");
        break;
      case '3':
        mmhal_set_microstepping(0, MMHAL_MS_MODE_8);
        mmhal_set_microstepping(1, MMHAL_MS_MODE_8);
        state.microsteps = 8;
        printf("Microstepping Mode: 3");
        break;
      case '4':
        mmhal_set_microstepping(0, MMHAL_MS_MODE_16);
        mmhal_set_microstepping(1, MMHAL_MS_MODE_16);
        state.microsteps = 16;
        printf("Microstepping Mode: 4");
        break;
      case '5':
        mmhal_set_microstepping(0, MMHAL_MS_MODE_32);
        mmhal_set_microstepping(1, MMHAL_MS_MODE_32);
        state.microsteps = 32;
        printf("Microstepping Mode: 5");
        break;
      case '\033': // escape character
        manual_mode = false;
        printf("Command Mode:\r\n");
        break;
      case 'c':
        calibrate_position();
        printf("%d,%d,%d", state.corner_coords[XDIM], state.corner_coords[YDIM], state.corner_coords[ZDIM]);
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
      case '\033': // escape character
        manual_mode = true;
        printf("Manual Mode:\r\n");
        break;
      case '\r':
      case '\n':
        command[command_index] = 0;
        printf("\n");
        command_index = 0;
        command_complete = true;
        printf("%s", command);
        break;
      
      case '\b':
      case '\177':
        if (command_index > 0) {
          printf("%c", ch);
          command_index--;
        }
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
  state.absolute_mode = true;
  state.units_mm = true;
  state.corner_coords[XDIM] = 0;
  state.corner_coords[YDIM] = 0;
  state.corner_coords[ZDIM] = 0;
  state.home_coords[XDIM] = X_LIMIT / 2;
  state.home_coords[YDIM] = Y_LIMIT / 2;
  state.home_coords[ZDIM] = Z_LIMIT / 2;
  state.spindle_speed = 0;
  state.microsteps = 1;
  
  stdio_init_all();
  mmhal_init();

  while (!stdio_usb_connected) {
    sleep_ms(100);
  }
  printf("hi\r\n");
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
