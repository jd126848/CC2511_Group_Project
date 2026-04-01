#include "mmhal.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#ifndef CNC_VERSION
#define CNC_VERSION 2
#endif

// Pin numbers arrays
const int step_pins[] = {XSTEP_PIN, YSTEP_PIN, ZSTEP_PIN};
const int dir_pins[] = {XDIR_PIN, YDIR_PIN, ZDIR_PIN};

// Multipliers for each axis, dealing with assymetric stepper directions
#if CNC_VERSION == 1
const int stepper_multipliers[] = {-1, 1, -1};
#elif CNC_VERSION == 2
const int stepper_multipliers[] = {1, -1, 1};
#else
#error "Invalid CNC_VERSION"
#endif

volatile int mmhal_high_delay_us = 2000; // Microseconds for step pulse high time
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
  
  gpio_put(ENABLE_PIN, 0);
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
  const mode_values[6][3] = {
    {0,0,0},
    {1,0,0},
    {0,1,0},
    {1,1,0},
    {0,0,1},
    {1,0,1}
};

  if (x_or_y) {
    mode0_pin = 7;
  }
  else {
    mode0_pin = 2;
  }
  
  gpio_put(mode0_pin, mode_values[mode][0]);
  gpio_put(mode0_pin, mode_values[mode][1]);
  gpio_put(mode0_pin, mode_values[mode][2]);
  printf("Microstepping Mode: %d", mode);

  // TODO - Implement microstepping mode setting
}

/**
 * @brief Run motors in specified directions
 * @param dirs Array of 3 directions: -1 for negative,
 * 0 for no movement, 1 for positive
 */
void mmhal_step_motors_impl(int dirs[])
{
  printf("%d x, %d y", dirs[XDIM], dirs[YDIM]);
  dirs[XDIM] *= stepper_multipliers[XDIM];
  dirs[YDIM] *= stepper_multipliers[YDIM];
  dirs[ZDIM] *= stepper_multipliers[ZDIM];

  if (dirs[XDIM] == 1) {
    gpio_put(XDIR_PIN, 1);
    gpio_put(XSTEP_PIN, 1);
    sleep_us(mmhal_high_delay_us);
    gpio_put(XSTEP_PIN, 0);
    sleep_us(mmhal_low_delay_us);
  }
  if (dirs[XDIM] == -1) {
    gpio_put(XDIR_PIN, 0);
    gpio_put(XSTEP_PIN, 1);
    sleep_us(mmhal_high_delay_us);
    gpio_put(XSTEP_PIN, 0);
    sleep_us(mmhal_low_delay_us);
  }
  if (dirs[YDIM] == 1) {
    gpio_put(YDIR_PIN, 1);
    gpio_put(YSTEP_PIN, 1);
    sleep_us(mmhal_high_delay_us);
    gpio_put(YSTEP_PIN, 0);
    sleep_us(mmhal_low_delay_us);
  }
  if (dirs[YDIM] == -1) {
    gpio_put(YDIR_PIN, 0);
    gpio_put(YSTEP_PIN, 1);
    sleep_us(mmhal_high_delay_us);
    gpio_put(YSTEP_PIN, 0);
    sleep_us(mmhal_low_delay_us);
  }
  if (dirs[ZDIM] == 1) {
    gpio_put(ZDIR_PIN, 1);
    gpio_put(ZSTEP_PIN, 1);
    sleep_us(mmhal_high_delay_us);
    gpio_put(ZSTEP_PIN, 0);
    sleep_us(mmhal_low_delay_us);
  }
  if (dirs[ZDIM] == -1) {
    gpio_put(ZDIR_PIN, 0);
    gpio_put(ZSTEP_PIN, 1);
    sleep_us(mmhal_high_delay_us);
    gpio_put(ZSTEP_PIN, 0);
    sleep_us(mmhal_low_delay_us);
  }
  
  
  // TODO - Implement motor stepping logic, using the dirs array
  // to determine which motors to step and in which direction
  // Remember to use the stepper_multipliers array to handle
  // asymmetric stepper directions

  // TODO - Implement the timing for the step pulses, using
  // mmhal_high_delay_us and mmhal_low_delay_us for the pulse timing
}

void mmhal_step_motors(int x_dir, int y_dir, int z_dir)
{
  int dirs[3] = {x_dir, y_dir, z_dir};
  for (size_t i = 0; i < 200; i++)
  {
    /* code */
  }
  
  mmhal_step_motors_impl(dirs);
}
