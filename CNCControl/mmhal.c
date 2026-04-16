#include "mmhal.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"

// #define CNC_VERSION 1

#ifndef CNC_VERSION
#define CNC_VERSION 2
#endif

#define FP_SHIFT 10
#define FP_SCALE (1 << FP_SHIFT)

// Pin numbers arrays
const int step_pins[] = {XSTEP_PIN, YSTEP_PIN, ZSTEP_PIN};
const int dir_pins[] = {XDIR_PIN, YDIR_PIN, ZDIR_PIN};

// Multipliers for each axis, dealing with assymetric stepper directions
#if CNC_VERSION == 1
const int stepper_multipliers[] = {-1, 1, -1};
#elif CNC_VERSION == 2
const int stepper_multipliers[] = {-1, 1, -1};
#else
#error "Invalid CNC_VERSION"
#endif

volatile int mmhal_high_delay_us = 2; // Microseconds for step pulse high time
volatile int mmhal_low_delay_us = 1000;   // Microseconds for step pulse low time

/**
 * @brief Initialize GPIO pins and PWM for spindle control
 */
void mmhal_init()
{
  // TODO - Initialise GPIO pins
  gpio_init(X_MODE0_PIN);
  gpio_init(X_MODE1_PIN);
  gpio_init(X_MODE2_PIN);
  gpio_init(XFLT_PIN);
  gpio_init(Y_MODE0_PIN);
  gpio_init(Y_MODE1_PIN);
  gpio_init(Y_MODE2_PIN);
  gpio_init(YFLT_PIN);
  gpio_init(SPINDLE_PIN);
  gpio_init(ENABLE_PIN);
  gpio_init(VOUT_PIN);
  gpio_init(XLIMIT_PIN);
  gpio_init(YLIMIT_PIN);
  gpio_init(ZLIMIT_PIN);

  gpio_set_dir(X_MODE0_PIN, GPIO_OUT);
  gpio_set_dir(X_MODE1_PIN, GPIO_OUT);
  gpio_set_dir(X_MODE2_PIN, GPIO_OUT);
  gpio_set_dir(XFLT_PIN, GPIO_IN);
  gpio_set_dir(Y_MODE0_PIN, GPIO_OUT);
  gpio_set_dir(Y_MODE1_PIN, GPIO_OUT);
  gpio_set_dir(Y_MODE2_PIN, GPIO_OUT);
  gpio_set_dir(YFLT_PIN, GPIO_IN);
  gpio_set_dir(SPINDLE_PIN, GPIO_OUT);
  gpio_set_dir(ENABLE_PIN, GPIO_OUT);
  gpio_set_dir(VOUT_PIN, GPIO_OUT);
  gpio_set_dir(XLIMIT_PIN, GPIO_IN);
  gpio_set_dir(YLIMIT_PIN, GPIO_IN);
  gpio_set_dir(ZLIMIT_PIN, GPIO_IN);
  
  gpio_put(ENABLE_PIN, 0);
  gpio_put(VOUT_PIN, 1);
  for (int i = 0; i < DIMCOUNT; i++)
  {
    gpio_init(step_pins[i]);
    gpio_set_dir(step_pins[i], GPIO_OUT);
    gpio_init(dir_pins[i]);
    gpio_set_dir(dir_pins[i], GPIO_OUT);
    // TODO - Initialise the pins for X, Y & Z steppers, using the
    // step_pins and dir_pins arrays
  }

  gpio_set_function(SPINDLE_PIN, GPIO_FUNC_PWM);
  uint spindle_slice_num = pwm_gpio_to_slice_num(SPINDLE_PIN);
  pwm_set_wrap(spindle_slice_num, 255);
  pwm_set_enabled(spindle_slice_num, true);

  // TODO - Initialize spindle PWM
}

void mmhal_set_spindle_pwm(uint16_t pwm_level)
{
  // TODO - Implement spindle PWM setting
  pwm_set_gpio_level(SPINDLE_PIN, pwm_level);
}

void mmhal_set_microstepping(int x_or_y, mmhal_microstep_mode_t mode)
{
  int mode0_pin;
  const int mode_values[6][3] = {
    {0,0,0},
    {1,0,0},
    {0,1,0},
    {1,1,0},
    {0,0,1},
    {1,0,1}
  };

  mode0_pin = x_or_y ? Y_MODE0_PIN : X_MODE0_PIN;
  // if (x_or_y) {
  //   mode0_pin = Y_MODE0_PIN;
  // }
  // else {
  //   mode0_pin = X_MODE0_PIN;
  // }
  
  gpio_put(mode0_pin, mode_values[mode][0]);
  gpio_put(mode0_pin + 1, mode_values[mode][1]);
  gpio_put(mode0_pin + 2, mode_values[mode][2]);

  // TODO - Implement microstepping mode setting
}

/**
 * @brief Run motors in specified directions
 * @param dirs Array of 3 directions: -1 for negative,
 * 0 for no movement, 1 for positive
 */
void mmhal_step_motors_impl(int dirs[3], int currentCoords[3], bool ignoreLimits)
{
  int xDir = dirs[XDIM] * stepper_multipliers[XDIM];
  int yDir = dirs[YDIM] * stepper_multipliers[YDIM];
  int zDir = dirs[ZDIM] * stepper_multipliers[ZDIM];

  if (!ignoreLimits) {
    if (dirs[XDIM] == 1 && currentCoords[XDIM] >= X_LIMIT) return;
    if (dirs[XDIM] == -1 && currentCoords[XDIM] <= 0) return;
    if (dirs[YDIM] == 1 && currentCoords[YDIM] >= Y_LIMIT) return;
    if (dirs[YDIM] == -1 && currentCoords[YDIM] <= 0) return;
    if (dirs[ZDIM] == 1 && currentCoords[ZDIM] >= Z_LIMIT) return;
    if (dirs[ZDIM] == -1 && currentCoords[ZDIM] <= 0) return;
  }

  

  if (xDir == 1) {
    currentCoords[XDIM] += stepper_multipliers[XDIM];
    gpio_put(XDIR_PIN, 1);
    gpio_put(XSTEP_PIN, 1);
    sleep_us(mmhal_high_delay_us);
    gpio_put(XSTEP_PIN, 0);
    sleep_us(mmhal_low_delay_us);
  }
  else if (xDir == -1) {
    currentCoords[XDIM] -= stepper_multipliers[XDIM];
    gpio_put(XDIR_PIN, 0);
    gpio_put(XSTEP_PIN, 1);
    sleep_us(mmhal_high_delay_us);
    gpio_put(XSTEP_PIN, 0);
    sleep_us(mmhal_low_delay_us);
  }

  if (yDir == 1) {
    currentCoords[YDIM] += stepper_multipliers[YDIM];
    gpio_put(YDIR_PIN, 1);
    gpio_put(YSTEP_PIN, 1);
    sleep_us(mmhal_high_delay_us);
    gpio_put(YSTEP_PIN, 0);
    sleep_us(mmhal_low_delay_us);
  }
  else if (yDir == -1) {
    currentCoords[YDIM] -= stepper_multipliers[YDIM];
    gpio_put(YDIR_PIN, 0);
    gpio_put(YSTEP_PIN, 1);
    sleep_us(mmhal_high_delay_us);
    gpio_put(YSTEP_PIN, 0);
    sleep_us(mmhal_low_delay_us);
  }
  
  if (zDir == 1) {
    currentCoords[ZDIM] += stepper_multipliers[ZDIM];
    gpio_put(ZDIR_PIN, 1);
    gpio_put(ZSTEP_PIN, 1);
    sleep_us(mmhal_high_delay_us);
    gpio_put(ZSTEP_PIN, 0);
    sleep_us(mmhal_low_delay_us);
  }
  else if (zDir == -1) {
    currentCoords[ZDIM] -= stepper_multipliers[ZDIM];
    gpio_put(ZDIR_PIN, 0);
    gpio_put(ZSTEP_PIN, 1);
    sleep_us(mmhal_high_delay_us);
    gpio_put(ZSTEP_PIN, 0);
    sleep_us(mmhal_low_delay_us);
  }
  printf("POS X=%d Y=%d Z=%d\r\n", currentCoords[XDIM], currentCoords[YDIM], currentCoords[ZDIM]);
  
  // TODO - Implement motor stepping logic, using the dirs array
  // to determine which motors to step and in which direction
  // Remember to use the stepper_multipliers array to handle
  // asymmetric stepper directions

  // TODO - Implement the timing for the step pulses, using
  // mmhal_high_delay_us and mmhal_low_delay_us for the pulse timing
}
void bresenham_step(int x1, int y1, int currentCoords[3])
{
    int dx = abs(x1);
    int dy = abs(y1);

    int sx = (x1 > 0) ? 1 : -1;
    int sy = (y1 > 0) ? 1 : -1;

    int diff = dx - dy;

    int x = 0;
    int y = 0;

    while (x != x1 || y != y1)
    {
      if (getchar_timeout_us(0) == 'l') break;
      int D = 2 * diff;

      int step_x = 0;
      int step_y = 0;

      if (D > -dy) {
          diff -= dy;
          x += sx;
          step_x = sx;
      }

      if (D < dx) {
          diff += dx;
          y += sy;
          step_y = sy;
      }
      int step_dir[3] = {step_x, step_y, 0};
      mmhal_step_motors_impl(step_dir, currentCoords, false);
      // printf("%d, %d\r\n", step_x, step_y);
    }
}

void mmhal_step_motors(int dx, int dy, int dz, int currentCoords[3])
{
  int dirs[3] = {dx, dy, dz};
  if ((dx == 0 && dy == 0) || (dx == 0 && dz == 0) || (dy == 0 && dz == 0)) {
    int steps, step_dir[3] = {0,0,0};
    if (dirs[XDIM] !=0) {
      steps = dirs[XDIM];
      step_dir[XDIM] = (steps > 0) ? 1 : -1;
    }
    else if (dirs[YDIM] !=0) {
      steps = dirs[YDIM];
      step_dir[YDIM] = (steps > 0) ? 1 : -1;
    }
    else if (dirs[ZDIM] !=0) {
      steps = dirs[ZDIM];
      step_dir[ZDIM] = (steps > 0) ? 1 : -1;
    }
    for (size_t i = 0; i < abs(steps); i++) {
      if (getchar_timeout_us(0) == 'l') break;
      mmhal_step_motors_impl(step_dir, currentCoords, false);
    }
  }
  else {
    bresenham_step(dirs[XDIM], dirs[YDIM], currentCoords);
    int step_dir[3] = {0,0,0};
    step_dir[ZDIM] = (dirs[ZDIM] > 0) ? 1 : -1;
    for (size_t i = 0; i < abs(dirs[ZDIM]); i++)
    {
      if (getchar_timeout_us(0) == 'l') break;
      mmhal_step_motors_impl(step_dir, currentCoords, false);
    }
  } 
}

void mmhal_move_arc(int x1, int y1, int i, int j, bool CW, int currentCoords[3])
{
  int cx = i;
  int cy = j;

  int x = -cx;
  int y = -cy;

  int xpos = 0;
  int ypos = 0;

  int ex = x1 - cx;
  int ey = y1 - cy;

  int r2 = x*x + y*y;

  int dirs[8][2] = {
    {1,0},{1,1},{0,1},{-1,1},
    {-1,0},{-1,-1},{0,-1},{1,-1}
  };

  for (int iter = 0; iter < 10000; iter++)
  {
    int px = cx + x;
    int py = cy + y;
    if (getchar_timeout_us(0) == 'l') break;
    mmhal_step_motors(px - xpos, py - ypos, 0, currentCoords);

    xpos = px;
    ypos = py;

    // stop near target
    if (abs(x - ex) <= 1 && abs(y - ey) <= 1)
      break;

    int best_x = x;
    int best_y = y;
    int best_err = 1e9;

    for (int k = 0; k < 8; k++)
    {
      if (getchar_timeout_us(0) == 'l') break;
      int nx = x + dirs[k][0];
      int ny = y + dirs[k][1];

      int err = abs(nx*nx + ny*ny - r2);

      int step_cross = x*dirs[k][1] - y*dirs[k][0];

      if (CW && step_cross > 0) continue;
      if (!CW && step_cross < 0) continue;

      if (err < best_err) {
        best_err = err;
        best_x = nx;
        best_y = ny;
      }
    }

    //prevent stall
    if (best_x == x && best_y == y) {
      if (CW) {
        int tmp = x;
        x = y;
        y = -tmp;
      } else {
        int tmp = x;
        x = -y;
        y = tmp;
      }
      continue;
    }

    x = best_x;
    y = best_y;
  }
}