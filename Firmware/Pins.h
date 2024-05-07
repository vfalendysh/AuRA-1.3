/**
 * Stepper motor
 */
#define PIN_MOTOR_EN 8 
#define PIN_MOTOR_STEP 7
#define PIN_MOTOR_DIR 4

#define PIN_FAN_PWM A3

/**
 * Buzzer
 */
#define PIN_BUZZER 12

/**
 * Fan
 */
#define PIN_FAN A3

/**
 * Rotary encoder
 */
#if ENCODER_DIRECTION == 1
  #define PIN_ENC_A 9
  #define PIN_ENC_B 10
#else
  #define PIN_ENC_A 10
  #define PIN_ENC_B 9
#endif  

#define PIN_ENC_C 11

/**
 * Oled screen
 */
#define PIN_OLED_CLK 5
#define PIN_OLED_DAT 4
#define PIN_OLED_CS 16
