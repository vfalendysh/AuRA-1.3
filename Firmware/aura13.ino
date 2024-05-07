#include <EEPROM.h>
#include <Rotary.h>
#include <U8g2lib.h>
#include "AccelStepper.h" 

// enables demo / test mode in settings menu. endlesly runs programs when enabled; 1 or 0
#define DEMO_AVAILABLE 1

// changes rotary encoder direction; 1 or -1
#define ENCODER_DIRECTION 1

// continuous must rotate counter clock when looking on a tank cap side
// set to -1 to invert or 1 to normal
#define MOTOR_DIRECTION -1

#include "Pins.h"
#include "Menus.h"

/**
 * Stepper
 */
AccelStepper stepper(1, PIN_MOTOR_STEP, PIN_MOTOR_DIR);

/**
 * Rotary encoder
 */
Rotary rotary = Rotary(PIN_ENC_A, PIN_ENC_B);

volatile boolean rotaryButtonUp = false;
volatile boolean rotaryButtonDown = false;
volatile boolean rotaryButtonPressed = false;
volatile boolean rotaryButtonLongPressed = false;
volatile boolean rotaryButtonWasPressed = false;
unsigned long rotaryButtonPressedMillis = 0;

/**
 * Machine state
 */
#define STATE_MENU 0
#define STATE_RUN 1
#define STATE_DONE 2
#define STATE_STOPPED 3

#define PROGRAMM_MODE_R1 MENU_ROOT_R1
#define PROGRAMM_MODE_R2 MENU_ROOT_R2
#define PROGRAMM_MODE_R3 MENU_ROOT_R3
#define PROGRAMM_MODE_R4 MENU_ROOT_R4
#define PROGRAMM_MODE_TIMER MENU_ROOT_TIMER

byte machineState = STATE_MENU;
bool presetSelected = false;

/**
 * Counters
 */
unsigned int finishedCount = 0; 
unsigned int interruptedCount = 0;

/**
 * Settings
 */
#define EEPROM_SETTINGS_SAVED 0
#define EEPROM_SETTINGS_SOUND 1
#define EEPROM_SETTINGS_TANK 2
#define EEPROM_SETTINGS_FINISHED_BYTE1 3
#define EEPROM_SETTINGS_FINISHED_BYTE0 4
#define EEPROM_SETTINGS_INTERRUPTED_BYTE1 5
#define EEPROM_SETTINGS_INTERRUPTED_BYTE0 6
#if (DEMO_AVAILABLE == 1)
  #define EEPROM_SETTINGS_DEMO 7
#endif  

// Preset settings
#define EEPROM_SETTINGS_PRESETS_OFFSET 10

#define PRESET_MODE 0
#define PRESET_MINUTES 1
#define PRESET_SECONDS 2
#define PRESET_SPEED 3

#define MAX_MINUTES 90

volatile byte presets[PRESETS_COUNT][PRESETS_SETTINGS_COUNT] = {
  {0, 10, 0, 50},
  {0, 10, 0, 50},
  {0, 10, 0, 50},
  {0, 10, 0, 50},
  {0, 10, 0, 50},
};

/**
 * Tanks
 */
#define TANK_MODEL_COUNT 3
#define TANK_J1500 0
#define TANK_J2500 1
#define TANK_P4 2

/**
 * Sound
 */
byte settingsSound = 1;
byte settingsTank = TANK_J1500;

/**
 * Demo / test mode
 */
#if (DEMO_AVAILABLE == 1)
  byte settingsDemo = 0;
#endif  

/**
 * Menus
 */
byte menuMode = MENU_MODE_ROOT;
int8_t menuPosition = 0;
int8_t menuPresetsPosition = 0;
int8_t menuSettingsPosition = 0;
int8_t menuSettingsPresetsPosition = 0;
byte programmMode = 0;

/**
 * Programm times in seconds
 */
int programmDurationSeconds = 300;
int programmTimeSeconds = 0;
int programmTimeLeftSeconds = programmDurationSeconds;

/**
 * Motor vars
 */
bool runMotor = false;
bool runMotorBackwards = false;
int  runMotorBackwardsSwitchTime = 0;
bool motorAccelerating = false;
bool slowDown = false;

#define MOTOR_SPEEDS_COUNT 12
byte motorSpeedIndex = 9;
byte motorRPMs[MOTOR_SPEEDS_COUNT] = {5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 60, 75};

float motorSpeedMultipliers[TANK_MODEL_COUNT][MOTOR_SPEEDS_COUNT] = {
  // Jobo 1500
  {19, 19, 19, 19.25, 19.25, 19.5, 19.5, 19.5, 19.75, 20.0, 20.25, 20.5},

  // Jobo 2500
  {27, 27, 27, 27.34, 27.34, 27.69, 27.69, 27.69, 28.00, 28.4, 28.75, 29.1},

  // Paterson 4
  {21.0, 21.0, 21.5, 21.8, 22.0, 22.2, 22.2, 22.5, 22.7, 23.0, 23.0, 23.0}
};

int motorMaxSpeed;
int motorTargetSpeed;
float motorCurrentSpeed = 0;

float motorAcceleration = 0.2;
float motorDeceleration = 0.16;

/**
 * Screen
 */
U8G2_SSD1306_128X64_NONAME_2_HW_I2C u8g2(U8G2_R0, PIN_OLED_CLK, PIN_OLED_DAT);

/**
 * Setup
 */
void setup() {
  /**
   * Settings from memory
   */
  if (EEPROM.read(EEPROM_SETTINGS_SAVED) == 1) {
    settingsSound = EEPROM.read(EEPROM_SETTINGS_SOUND);
    
    #if (DEMO_AVAILABLE == 1)
      settingsDemo = EEPROM.read(EEPROM_SETTINGS_DEMO);     
    #endif
      
    settingsTank = EEPROM.read(EEPROM_SETTINGS_TANK);

    finishedCount = EEPROM.read(EEPROM_SETTINGS_FINISHED_BYTE1) * 256 + EEPROM.read(EEPROM_SETTINGS_FINISHED_BYTE0);
    interruptedCount = EEPROM.read(EEPROM_SETTINGS_INTERRUPTED_BYTE1) * 256 + EEPROM.read(EEPROM_SETTINGS_INTERRUPTED_BYTE0);

    for (byte i = 0; i < PRESETS_COUNT; i++) {
      presets[i][PRESET_MODE] = EEPROM.read(EEPROM_SETTINGS_PRESETS_OFFSET + i * PRESETS_SETTINGS_COUNT + PRESET_MODE);
      presets[i][PRESET_MINUTES] = EEPROM.read(EEPROM_SETTINGS_PRESETS_OFFSET + i * PRESETS_SETTINGS_COUNT + PRESET_MINUTES);
      presets[i][PRESET_SECONDS] = EEPROM.read(EEPROM_SETTINGS_PRESETS_OFFSET + i * PRESETS_SETTINGS_COUNT + PRESET_SECONDS);
      presets[i][PRESET_SPEED] = EEPROM.read(EEPROM_SETTINGS_PRESETS_OFFSET + i * PRESETS_SETTINGS_COUNT + PRESET_SPEED);
    }
  }

  // check settings
  if (settingsSound > 1) settingsSound = 1;
  
  #if (DEMO_AVAILABLE == 1)
    if (settingsDemo > 1) settingsDemo = 1;
  #endif
  
  if (settingsTank >= TANK_MODEL_COUNT) settingsTank = TANK_J1500;
  if (finishedCount > 65530) finishedCount = 0;
  if (interruptedCount == 65530) interruptedCount = 0;
  
  for (byte i = 0; i < PRESETS_COUNT; i++) {
    if (presets[i][PRESET_MODE] > 2) presets[i][PRESET_MODE] = 0;
    if (presets[i][PRESET_MINUTES] > MAX_MINUTES) {
      presets[i][PRESET_MINUTES] = 10;
      presets[i][PRESET_SECONDS] = 0;
    }
    if (presets[i][PRESET_SECONDS] >= 60) presets[i][PRESET_SECONDS] = 0;

    if (presets[i][PRESET_MINUTES] * MAX_MINUTES + presets[i][PRESET_SECONDS] < 60) {
      presets[i][PRESET_MINUTES] = 10;
      presets[i][PRESET_SECONDS] = 0;
    }

    if (presets[i][PRESET_SPEED] >= MOTOR_SPEEDS_COUNT) presets[i][PRESET_SPEED] = motorSpeedIndex;
  }

  motorMaxSpeed = 3000;
  motorTargetSpeed = motorRPMs[motorSpeedIndex] * motorSpeedMultipliers[settingsTank][motorSpeedIndex];

  /**
   * Stepper setup
   */
  stepper.setMaxSpeed(motorMaxSpeed);
  stepper.setEnablePin(PIN_MOTOR_EN);
  stepper.setPinsInverted(false, false, true);

  /**
   * Buzzer
   */
  pinMode(PIN_BUZZER, OUTPUT);

  /**
   * Fan 
   */
  pinMode(PIN_FAN, OUTPUT);
  
  /**
   * Rotary encoder
   */
  pinMode(PIN_ENC_C, INPUT_PULLUP);

  u8g2.begin();
  u8g2.setFontPosTop();
  
  digitalWrite(PIN_FAN, HIGH);
  displayIntro();
  delay(2500);  
  digitalWrite(PIN_FAN, LOW);
  
  updateScreen();

  /**
   * Time interruprs
   */
  // TIMER 1 for interrupt frequency 16000 Hz:
  cli(); // stop interrupts
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; // initialize counter value to 0
  // set compare match register for 16000 Hz increments
  OCR1A = 999; // = 16000000 / (1 * 16000) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12, CS11 and CS10 bits for 1 prescaler
  TCCR1B |= (0 << CS12) | (0 << CS11) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei(); // allow interrupts
}

/**
 * Runs motor on timer interrupt
 * timer1 interrupt 8kHz toggles pin 9
 */
ISR(TIMER1_COMPA_vect){
  //generates pulse wave of frequency 8kHz/2 = 4kHz (takes two cycles for full wave- toggle high then toggle low)
  if (machineState == STATE_RUN && runMotor) {
    stepper.runSpeed();
  }
}

/**
 * Main loop
 */
void loop() {

  if (machineState == STATE_MENU) {
    handleRotaryEncoder();
  
    if (rotaryButtonPressed) {
      handleMenuSelectActions();
      if (machineState == STATE_MENU) updateScreen();
      rotaryButtonPressed = false;
      delay(200);
    } else if (rotaryButtonLongPressed) {
      handleMenuBackActions();
      if (machineState == STATE_MENU) updateScreen();
      rotaryButtonLongPressed = false;
      delay(200);
    } else if (rotaryButtonUp || rotaryButtonDown) {
      handleMenuScrollActions();
      updateScreen();
      rotaryButtonUp = rotaryButtonDown = false;
    }
   
  } else if (machineState == STATE_RUN) {
    runProgramm();
  } else if (machineState == STATE_DONE || machineState == STATE_STOPPED) {
    
    handleRotaryEncoder();
    
    if (rotaryButtonPressed || rotaryButtonLongPressed || rotaryButtonUp || rotaryButtonDown) {
      if (presetSelected) {
        machineState = STATE_MENU;
        menuMode = MENU_MODE_PRESETS;
        //menuPresetsPosition++;
        //if (menuPresetsPosition >= PRESETS_COUNT) menuPresetsPosition = 0;
      } else {
        machineState = STATE_MENU;
        menuMode = MENU_MODE_ROOT;
      }
      updateScreen();
      rotaryButtonPressed = rotaryButtonLongPressed = rotaryButtonUp = rotaryButtonDown = false;
      delay(200);
    }
  }
}

/**
 * Get speed by index
 */
int getSpeedByIndex(byte i) {
  return motorRPMs[i] * motorSpeedMultipliers[settingsTank][i];
}

/**
 * Get deceleration coeficient by index
 */
float getAccelerationByIndex(byte i) {
  return motorRPMs[i] / (settingsTank == TANK_J1500 ? 175.0 : (settingsTank == TANK_J2500 ? 125.0 : 150.0)); // bigger number - smaller steps, slower acceleration
}

/**
 * Get deceleration coeficient by index
 */
float getDecelerationByIndex(byte i) {
  return motorRPMs[i] / (settingsTank == TANK_J1500 ? 200.0 : (settingsTank == TANK_J2500 ? 125.0 : 150.0)); // bigger number - smaller steps, slower deceleration
}

/**
 * Beep
 */
void beep(int d) {
  tone(PIN_BUZZER, 3520);
  delay(d);
  noTone(PIN_BUZZER);
}

/**
 * Run programm
 */
void runProgramm() {
  unsigned long programmStartMillis = millis();
  unsigned long programmLeftMillis = programmDurationSeconds * 1000L;
  unsigned long programmEndMillis = programmStartMillis + programmLeftMillis;
  unsigned long lastScreenUpdateSecond;

  runMotorBackwards = false;
  runMotorBackwardsSwitchTime = 0;
  runMotor = false;
  slowDown = false;
  programmTimeSeconds = 0;
  lastScreenUpdateSecond = 1;

  programmTimeLeftSeconds = programmDurationSeconds;
 
  if (programmMode != PROGRAMM_MODE_TIMER) {

    if ((settingsTank == TANK_J2500) && (motorSpeedIndex > MOTOR_SPEEDS_COUNT - 3)) motorSpeedIndex = MOTOR_SPEEDS_COUNT - 3; // 50 max
    
    motorCurrentSpeed = 0;
    motorTargetSpeed = getSpeedByIndex(motorSpeedIndex);
    stepper.enableOutputs();
    setMotorState();

    // run fun only when motor is moving
    digitalWrite(PIN_FAN, HIGH);
  }

  updateScreen();
  
  for (;;) {
    
    unsigned long currentMillis = millis();

    programmTimeLeftSeconds = programmLeftMillis / 1000L;
    programmTimeSeconds = programmDurationSeconds - programmTimeLeftSeconds;
    
    if (currentMillis > programmEndMillis) {
      
      if (programmMode != PROGRAMM_MODE_TIMER) {
        runMotor = false;
        digitalWrite(PIN_FAN, LOW);
        stepper.disableOutputs();
      }

      finishedCount++;
      EEPROM.write(EEPROM_SETTINGS_FINISHED_BYTE1, finishedCount >> 8);
      EEPROM.write(EEPROM_SETTINGS_FINISHED_BYTE0, finishedCount % 256);
      
      machineState = STATE_DONE;
      updateScreen();

      if (settingsSound == 1) {
        beep(1000);
        delay(100);
        beep(1000);
      }

      #if (DEMO_AVAILABLE == 1)
        if (settingsDemo == 1) {
          delay(1000);
          machineState = STATE_RUN;
          programmTimeLeftSeconds = programmDurationSeconds;
          programmTimeSeconds = 0;
          programmMode++;
          if (programmMode >= PROGRAMM_MODE_TIMER) {
            programmMode = PROGRAMM_MODE_R1; 
          }
          updateScreen();
        }  
      #endif   
      
      break;
    }
    
    programmLeftMillis = programmEndMillis - currentMillis;

    handleRotaryEncoder();

    if (programmMode != PROGRAMM_MODE_TIMER) {
      /**
       * Increase RPM
       */
      if (rotaryButtonUp) {
        if (motorSpeedIndex < MOTOR_SPEEDS_COUNT - 1) {
          motorSpeedIndex += 1;

          if ((settingsTank == TANK_J2500) && (motorSpeedIndex > MOTOR_SPEEDS_COUNT - 3)) motorSpeedIndex = MOTOR_SPEEDS_COUNT - 3; // 50 max
          
          motorTargetSpeed = getSpeedByIndex(motorSpeedIndex);
          updateScreen();
        }
        rotaryButtonUp = false;
      }
  
      /**
       * Decrease RPM
       */
      if (rotaryButtonDown) {
        if (motorSpeedIndex > 0) {
          motorSpeedIndex -= 1;
          motorTargetSpeed = getSpeedByIndex(motorSpeedIndex);
          updateScreen();
        }
        rotaryButtonDown = false;
      }
  
      setMotorState();
    }

    /**
     * Stop programm
     */
    if (rotaryButtonLongPressed) {
      runMotor = false;
      digitalWrite(PIN_FAN, LOW);
      stepper.disableOutputs();
      machineState = STATE_STOPPED;
      
      interruptedCount++;
      EEPROM.write(EEPROM_SETTINGS_INTERRUPTED_BYTE1, interruptedCount >> 8);
      EEPROM.write(EEPROM_SETTINGS_INTERRUPTED_BYTE0, interruptedCount % 256);
      
      updateScreen();
      rotaryButtonLongPressed = false;
      delay(200);
      break; 
    }

    /**
     * Check programm times
     */
    if (programmTimeSeconds != lastScreenUpdateSecond) {
      
      updateScreen();
      
      lastScreenUpdateSecond = programmTimeSeconds;

      if (settingsSound == 1) {
        if (programmTimeLeftSeconds == 60) {
          beep(100);
        } else if (programmTimeLeftSeconds == 30) {
          beep(100); delay(100); beep(100);
        }
      }
    }
  }
}

void setMotorStateAccelR2R3R4() {
  if (motorCurrentSpeed < motorTargetSpeed) {
    motorCurrentSpeed += getAccelerationByIndex(motorSpeedIndex);
    if (motorCurrentSpeed > motorTargetSpeed) motorCurrentSpeed = motorTargetSpeed;
  } else if (motorCurrentSpeed > motorTargetSpeed) {
    motorCurrentSpeed -= getDecelerationByIndex(motorSpeedIndex);
    if (motorCurrentSpeed < 0) motorCurrentSpeed = 0;
  }
}

/**
 * Set motor state
 */
void setMotorState() {

  int sec = programmTimeSeconds % 60;
  
  if (programmTimeLeftSeconds < 2) {
    slowDown = true;
    motorTargetSpeed = 0;
  }

  switch (programmMode) {
    /**
     * Easy
     */
    case PROGRAMM_MODE_R1: {
      byte mod = sec % 30;
      if (programmTimeSeconds <= 30) {
        // fast first 30 seconds
        motorTargetSpeed = getSpeedByIndex(motorSpeedIndex);
      } else {
        motorTargetSpeed = getSpeedByIndex(mod >= 25 ? motorSpeedIndex : motorSpeedIndex / 5);
          if (mod == 24) motorTargetSpeed = 0;
          if ((mod == 25) && (programmTimeSeconds - runMotorBackwardsSwitchTime > 0)) {
            runMotorBackwards = !runMotorBackwards;
            runMotorBackwardsSwitchTime = programmTimeSeconds;
          }
      }
      
      if (motorCurrentSpeed < motorTargetSpeed) {
        // accelerate
        motorCurrentSpeed += getAccelerationByIndex(motorSpeedIndex);
        if (motorCurrentSpeed > motorTargetSpeed) motorCurrentSpeed = motorTargetSpeed;
      } else if (motorCurrentSpeed > motorTargetSpeed && motorCurrentSpeed > 0) {
        // decelerate
        motorCurrentSpeed -= motorTargetSpeed == 0 ? getDecelerationByIndex(0) : getDecelerationByIndex(motorSpeedIndex);
        if (motorCurrentSpeed < 0) motorCurrentSpeed = 0;
      }

      stepper.setSpeed((runMotorBackwards ? -1 : 1) * motorCurrentSpeed * MOTOR_DIRECTION);
      
      runMotor = true;
        
      break;
    }

    /**
     * Continuous
     */
    case PROGRAMM_MODE_R2: {
     
      setMotorStateAccelR2R3R4();
      stepper.setSpeed(motorCurrentSpeed * MOTOR_DIRECTION);
      
      runMotor = true;
        
      break;
    }

    /**
     * Oscilation 1:1
     */
    case PROGRAMM_MODE_R3: {
      byte mod = sec % 15;

      motorTargetSpeed = slowDown ? 0 : (mod < 8 ? (mod > 6 ? 0 : getSpeedByIndex(motorSpeedIndex)) : (mod > 13 ? 0 : getSpeedByIndex(motorSpeedIndex)));
      
      setMotorStateAccelR2R3R4();
      stepper.setSpeed((mod < 8 ? motorCurrentSpeed : - motorCurrentSpeed) * MOTOR_DIRECTION);
      
      runMotor = true;
            
      break;
    }
    
    /**
     * Progress 1:5
     */
    case PROGRAMM_MODE_R4: {
      byte mod = sec % 10;

      motorTargetSpeed = slowDown ? 0 : (mod < 7 ? (mod > 5 ? 0 : getSpeedByIndex(motorSpeedIndex)) : (mod > 8 ? 0 : getSpeedByIndex(motorSpeedIndex)));
     
      setMotorStateAccelR2R3R4(); 
      stepper.setSpeed((mod < 7 ? motorCurrentSpeed : - motorCurrentSpeed) * MOTOR_DIRECTION);
      
      runMotor = true;
            
      break;
    }
  }
}

/**
 * Handle rotary encoder
 */
void handleRotaryEncoder() {
  
  unsigned char result = rotary.process();

  if (result == DIR_CW) {
     rotaryButtonDown = true;
  } else if (result == DIR_CCW) {
     rotaryButtonUp = true;
  }
  
  if (!digitalRead(PIN_ENC_C)) {
    if (!rotaryButtonWasPressed) {
      rotaryButtonWasPressed = true;
      rotaryButtonPressedMillis = millis();
      unsigned long rotarryButtonPressedDuration = millis() - rotaryButtonPressedMillis;
      if (rotarryButtonPressedDuration > 500) {
        rotaryButtonLongPressed = true;
      }
    }
  } else {
    if (rotaryButtonWasPressed) {
      unsigned long rotarryButtonPressedDuration = millis() - rotaryButtonPressedMillis;
      if (rotarryButtonPressedDuration > 500) {
        rotaryButtonLongPressed = true;
      } else {
        rotaryButtonPressed = true;
      }

      rotaryButtonWasPressed = false;
    }
  }
}

/**
 * Handle menu scroll action
 */
void handleMenuScrollActions() {
  int maxSeconds = 60 * MAX_MINUTES;

  switch (menuMode) {
    case MENU_MODE_ROOT: {
      if (rotaryButtonUp) { menuPosition++; if (menuPosition >= MENU_ROOT_SIZE) menuPosition = 0; } 
      else { menuPosition--; if (menuPosition < 0) menuPosition = MENU_ROOT_SIZE - 1; }
      break;
    }

    case MENU_MODE_R1_MINUTES: 
    case MENU_MODE_R2_MINUTES:
    case MENU_MODE_R3_MINUTES:
    case MENU_MODE_R4_MINUTES:
    case MENU_MODE_TIMER_MINUTES: {
      if (rotaryButtonUp) { programmDurationSeconds += 60; if (programmDurationSeconds > maxSeconds) programmDurationSeconds = maxSeconds; } 
      else { programmDurationSeconds -= 60; if (programmDurationSeconds < 60) programmDurationSeconds = 60; }
      break;
    }

    case MENU_MODE_R1_SECONDS: 
    case MENU_MODE_R2_SECONDS:
    case MENU_MODE_R3_SECONDS:
    case MENU_MODE_R4_SECONDS:
    case MENU_MODE_TIMER_SECONDS: {
      if (rotaryButtonUp) { programmDurationSeconds += 1; if (programmDurationSeconds > maxSeconds) programmDurationSeconds = maxSeconds; } 
      else { programmDurationSeconds -= 1; if (programmDurationSeconds < 60) programmDurationSeconds = 60; }
      break;
    }

    case MENU_MODE_R1_RPM:
    case MENU_MODE_R2_RPM:
    case MENU_MODE_R3_RPM: 
    case MENU_MODE_R4_RPM: {
      if (rotaryButtonUp && motorSpeedIndex < MOTOR_SPEEDS_COUNT - 1) { 
        motorSpeedIndex += 1; 
        if ((settingsTank == TANK_J2500) && (motorSpeedIndex > MOTOR_SPEEDS_COUNT - 3)) motorSpeedIndex = MOTOR_SPEEDS_COUNT - 3; // 50 max
      } 
      else if (rotaryButtonDown && motorSpeedIndex > 0) { motorSpeedIndex -= 1; }
    }

    case MENU_MODE_PRESETS: {
      if (rotaryButtonUp) { menuPresetsPosition++; if (menuPresetsPosition >= PRESETS_COUNT) menuPresetsPosition = 0; } 
      else { menuPresetsPosition--; if (menuPresetsPosition < 0) menuPresetsPosition = PRESETS_COUNT - 1; }
    }

    case MENU_MODE_SETTINGS: {
      if (rotaryButtonUp) { menuSettingsPosition++; if (menuSettingsPosition >= MENU_SETTINGS_SIZE) menuSettingsPosition = 0; } 
      else { menuSettingsPosition--; if (menuSettingsPosition < 0) menuSettingsPosition = MENU_SETTINGS_SIZE - 1; }

      if (menuSettingsPosition >= MENU_MODE_SETTINGS_PRESET1 && menuSettingsPosition <= MENU_MODE_SETTINGS_PRESET5) {
        menuSettingsPresetsPosition = menuSettingsPosition - MENU_MODE_SETTINGS_PRESET1;
      }
      break;
    }

    case MENU_MODE_SETTINGS_PRESETX_MODE: {
      if (rotaryButtonUp) { 
        presets[menuSettingsPresetsPosition][PRESET_MODE]++; 
        if (presets[menuSettingsPresetsPosition][PRESET_MODE] > 3) presets[menuSettingsPresetsPosition][PRESET_MODE] = 0; 
      } 
      else { 
        if (presets[menuSettingsPresetsPosition][PRESET_MODE] == 0) {
          presets[menuSettingsPresetsPosition][PRESET_MODE] = 3; 
        } else {
          presets[menuSettingsPresetsPosition][PRESET_MODE]--; 
        }
      }
      break;
    }

    case MENU_MODE_SETTINGS_PRESETX_MINUTES: {
      if (rotaryButtonUp) { 
        presets[menuSettingsPresetsPosition][PRESET_MINUTES]++; 
        if (presets[menuSettingsPresetsPosition][PRESET_MINUTES] > MAX_MINUTES) {
          presets[menuSettingsPresetsPosition][PRESET_MINUTES] = MAX_MINUTES; 
          presets[menuSettingsPresetsPosition][PRESET_SECONDS] = 0; 
        }
      } 
      else { 
        presets[menuSettingsPresetsPosition][PRESET_MINUTES]--; 
        if (presets[menuSettingsPresetsPosition][PRESET_MINUTES] < 1) {
          presets[menuSettingsPresetsPosition][PRESET_MINUTES] = 1;
          presets[menuSettingsPresetsPosition][PRESET_SECONDS] = 0;
        }
      }
      break;
    }

    case MENU_MODE_SETTINGS_PRESETX_SECONDS: {
      if (rotaryButtonUp) { 
        if (presets[menuSettingsPresetsPosition][PRESET_MINUTES] < MAX_MINUTES) {
          presets[menuSettingsPresetsPosition][PRESET_SECONDS]++; 
        }
        if (presets[menuSettingsPresetsPosition][PRESET_SECONDS] >= 60) {
          presets[menuSettingsPresetsPosition][PRESET_MINUTES]++; 
          if (presets[menuSettingsPresetsPosition][PRESET_MINUTES] > MAX_MINUTES) presets[menuSettingsPresetsPosition][PRESET_MINUTES] = MAX_MINUTES; 
          presets[menuSettingsPresetsPosition][PRESET_SECONDS] = 0; 
        }
      } 
      else { 
        if (presets[menuSettingsPresetsPosition][PRESET_SECONDS] == 0) {
          presets[menuSettingsPresetsPosition][PRESET_MINUTES]--; 
          if (presets[menuSettingsPresetsPosition][PRESET_MINUTES] == 0) {
            presets[menuSettingsPresetsPosition][PRESET_MINUTES] = 1;
            presets[menuSettingsPresetsPosition][PRESET_SECONDS] = 0; 
          } else {
            presets[menuSettingsPresetsPosition][PRESET_SECONDS] = 59; 
          }
        } else {
          presets[menuSettingsPresetsPosition][PRESET_SECONDS]--; 
        }
      }
      break;
    }

    case MENU_MODE_SETTINGS_PRESETX_SPEED: {
      if (rotaryButtonUp) { 
        if (presets[menuSettingsPresetsPosition][PRESET_SPEED] < MOTOR_SPEEDS_COUNT - 1) {
          presets[menuSettingsPresetsPosition][PRESET_SPEED]++; 
        } else {
          presets[menuSettingsPresetsPosition][PRESET_SPEED] = MOTOR_SPEEDS_COUNT - 1;
        }
      } 
      else { 
        if (presets[menuSettingsPresetsPosition][PRESET_SPEED] > 0) {
          presets[menuSettingsPresetsPosition][PRESET_SPEED]--; 
        } else {
          presets[menuSettingsPresetsPosition][PRESET_SPEED] = 0;
        }
      }
    }
  }
}

/**
 * Menu back on double click
 */
void handleMenuBackActions() {
  
  switch (menuMode) {
    case MENU_MODE_R1_MINUTES:
    case MENU_MODE_R2_MINUTES:
    case MENU_MODE_R3_MINUTES:
    case MENU_MODE_R4_MINUTES:
    case MENU_MODE_TIMER_MINUTES: 
    case MENU_MODE_PRESETS:
    case MENU_MODE_SETTINGS:
    case MENU_MODE_ABOUT:{
      menuMode = MENU_MODE_ROOT; break;
    }
    
    case MENU_MODE_R1_SECONDS: menuMode = MENU_MODE_R1_MINUTES; break;
    case MENU_MODE_R2_SECONDS: menuMode = MENU_MODE_R2_MINUTES; break;
    case MENU_MODE_R3_SECONDS: menuMode = MENU_MODE_R3_MINUTES; break;
    case MENU_MODE_R4_SECONDS: menuMode = MENU_MODE_R4_MINUTES; break;
    case MENU_MODE_TIMER_SECONDS: menuMode = MENU_MODE_TIMER_MINUTES; break; 

    case MENU_MODE_R1_RPM: menuMode = MENU_MODE_R1_SECONDS; break;
    case MENU_MODE_R2_RPM: menuMode = MENU_MODE_R2_SECONDS; break;
    case MENU_MODE_R3_RPM: menuMode = MENU_MODE_R3_SECONDS; break;
    case MENU_MODE_R4_RPM: menuMode = MENU_MODE_R4_SECONDS; break;
  }
}

/**
 * Handle menu click
 */
void handleMenuSelectActions() {
  switch (menuMode) {
    case MENU_MODE_ROOT: {
      switch (menuPosition) {
        case MENU_ROOT_R1: menuMode = MENU_MODE_R1_MINUTES; programmMode = menuPosition; break;  
        case MENU_ROOT_R2: menuMode = MENU_MODE_R2_MINUTES; programmMode = menuPosition; break;
        case MENU_ROOT_R3: menuMode = MENU_MODE_R3_MINUTES; programmMode = menuPosition; break;
        case MENU_ROOT_R4: menuMode = MENU_MODE_R4_MINUTES; programmMode = menuPosition; break;
        case MENU_ROOT_TIMER: menuMode = MENU_MODE_TIMER_MINUTES; programmMode = menuPosition; break;
        case MENU_ROOT_PRESETS: menuMode = MENU_MODE_PRESETS; menuPresetsPosition = 0; break;
        case MENU_ROOT_SETTINGS: menuMode = MENU_MODE_SETTINGS; menuSettingsPosition = 0; break;
        case MENU_ROOT_ABOUT: menuMode = MENU_MODE_ABOUT; break;
      }
      
      break;
    }
    case MENU_MODE_R1_MINUTES: menuMode = MENU_MODE_R1_SECONDS; break;
    case MENU_MODE_R2_MINUTES: menuMode = MENU_MODE_R2_SECONDS; break;
    case MENU_MODE_R3_MINUTES: menuMode = MENU_MODE_R3_SECONDS; break;
    case MENU_MODE_R4_MINUTES: menuMode = MENU_MODE_R4_SECONDS; break;
    case MENU_MODE_TIMER_MINUTES: menuMode = MENU_MODE_TIMER_SECONDS; break;
    
    case MENU_MODE_R1_SECONDS: menuMode = MENU_MODE_R1_RPM; break;
    case MENU_MODE_R2_SECONDS: menuMode = MENU_MODE_R2_RPM; break;
    case MENU_MODE_R3_SECONDS: menuMode = MENU_MODE_R3_RPM; break;
    case MENU_MODE_R4_SECONDS: menuMode = MENU_MODE_R4_RPM; break;

    case MENU_MODE_R1_RPM:
    case MENU_MODE_R2_RPM:
    case MENU_MODE_R3_RPM:
    case MENU_MODE_R4_RPM:
    case MENU_MODE_TIMER_SECONDS: presetSelected = false; machineState = STATE_RUN; break;
    
    case MENU_MODE_PRESETS: {
      programmMode = presets[menuPresetsPosition][PRESET_MODE];
      programmDurationSeconds = presets[menuPresetsPosition][PRESET_MINUTES] * MAX_MINUTES + presets[menuPresetsPosition][PRESET_SECONDS];
      motorSpeedIndex = presets[menuPresetsPosition][PRESET_SPEED];
      if ((settingsTank == TANK_J2500) && (motorSpeedIndex > MOTOR_SPEEDS_COUNT - 3)) motorSpeedIndex = MOTOR_SPEEDS_COUNT - 3; // 50 max
      
      presetSelected = true;
      machineState = STATE_RUN;
      break; 
    }
    
    case MENU_MODE_SETTINGS: {
      switch (menuSettingsPosition) {
        case MENU_SETTINGS_TANK: {
          settingsTank++;
          if (settingsTank >= TANK_MODEL_COUNT) settingsTank = TANK_J1500;
          motorMaxSpeed = motorRPMs[MOTOR_SPEEDS_COUNT - 1] * motorSpeedMultipliers[settingsTank][MOTOR_SPEEDS_COUNT - 1];
          motorTargetSpeed = motorRPMs[motorSpeedIndex] * motorSpeedMultipliers[settingsTank][motorSpeedIndex];
          break;
        }
        case MENU_SETTINGS_PRESET1:
        case MENU_SETTINGS_PRESET2: 
        case MENU_SETTINGS_PRESET3: 
        case MENU_SETTINGS_PRESET4: 
        case MENU_SETTINGS_PRESET5: {
          menuMode = MENU_MODE_SETTINGS_PRESETX_MODE;
          menuSettingsPresetsPosition = menuSettingsPosition - MENU_SETTINGS_PRESET1;
          break;
        }
        case MENU_SETTINGS_SOUND: {
          settingsSound = settingsSound == 1 ? 0 : 1;
          break;
        }
        #if (DEMO_AVAILABLE == 1)
          case MENU_SETTINGS_DEMO: {
            settingsDemo = settingsDemo == 1 ? 0 : 1;
            break;
          }
        #endif  
        case MENU_SETTINGS_SAVE: {
          EEPROM.write(EEPROM_SETTINGS_SAVED, 1);
          EEPROM.write(EEPROM_SETTINGS_SOUND, settingsSound);
          EEPROM.write(EEPROM_SETTINGS_TANK, settingsTank);
          #if (DEMO_AVAILABLE == 1)
            EEPROM.write(EEPROM_SETTINGS_DEMO, settingsDemo);
          #endif  

          for (byte i = 0; i < PRESETS_COUNT; i++) {
            EEPROM.write(EEPROM_SETTINGS_PRESETS_OFFSET + i * PRESETS_SETTINGS_COUNT + PRESET_MODE, presets[i][PRESET_MODE]);
            EEPROM.write(EEPROM_SETTINGS_PRESETS_OFFSET + i * PRESETS_SETTINGS_COUNT + PRESET_MINUTES, presets[i][PRESET_MINUTES]);
            EEPROM.write(EEPROM_SETTINGS_PRESETS_OFFSET + i * PRESETS_SETTINGS_COUNT + PRESET_SECONDS, presets[i][PRESET_SECONDS]);
            EEPROM.write(EEPROM_SETTINGS_PRESETS_OFFSET + i * PRESETS_SETTINGS_COUNT + PRESET_SPEED, presets[i][PRESET_SPEED]);
          }

          if (settingsSound == 1) {
            beep(100);
            delay(100);
            beep(100);
          }
          menuMode = MENU_MODE_ROOT;
          break;
        }
        case MENU_SETTINGS_CANCEL: menuMode = MENU_MODE_ROOT; break;
      }
      break;
    }

    case MENU_MODE_SETTINGS_PRESETX_MODE: menuMode = MENU_MODE_SETTINGS_PRESETX_MINUTES; break;
    case MENU_MODE_SETTINGS_PRESETX_MINUTES: menuMode = MENU_MODE_SETTINGS_PRESETX_SECONDS; break;
    case MENU_MODE_SETTINGS_PRESETX_SECONDS: menuMode = MENU_MODE_SETTINGS_PRESETX_SPEED; break;
    case MENU_MODE_SETTINGS_PRESETX_SPEED: menuMode = MENU_MODE_SETTINGS; break;
    case MENU_MODE_ABOUT: menuMode = MENU_MODE_ROOT; break;
  }
}

/**
 * Convert seconds to mm:ss strings
 */
String getTimeFromSeconds(int s) {
  int minutes = s / 60;
  int seconds = s % 60;
  return (minutes < 10 ? "0" : "") + String(minutes) + ":" + (seconds < 10 ? "0" : "") + String(seconds);
}

/**
 * Update screen
 */
void updateScreen() {
  
  u8g2.firstPage();
  
  do {
    switch (machineState) {
      case STATE_RUN: screenProgress(getTimeFromSeconds(programmTimeLeftSeconds)); break;
      case STATE_MENU: {
        switch (menuMode) {
          case MENU_MODE_ROOT: screenRootMenu(); break;
         
          case MENU_MODE_R1_MINUTES:
          case MENU_MODE_R2_MINUTES:
          case MENU_MODE_R3_MINUTES:
          case MENU_MODE_R4_MINUTES:
          case MENU_MODE_TIMER_MINUTES: 
          case MENU_MODE_R1_SECONDS:
          case MENU_MODE_R2_SECONDS:
          case MENU_MODE_R3_SECONDS:
          case MENU_MODE_R4_SECONDS:
          case MENU_MODE_TIMER_SECONDS: screenTimeSettings(); break;
  
          case MENU_MODE_R1_RPM:
          case MENU_MODE_R2_RPM:
          case MENU_MODE_R3_RPM:
          case MENU_MODE_R4_RPM: screenRPMSettings(); break;

          case MENU_MODE_PRESETS: screenPresetsMenu(); break;
  
          case MENU_MODE_SETTINGS: screenSettingsMenu(); break;
          
          case MENU_MODE_SETTINGS_PRESETX_MODE: 
          case MENU_MODE_SETTINGS_PRESETX_MINUTES: 
          case MENU_MODE_SETTINGS_PRESETX_SECONDS: 
          case MENU_MODE_SETTINGS_PRESETX_SPEED: screenSettingsPresetsMenu(); break; 
          
          case MENU_MODE_ABOUT: screenAbout(); break;
        }
        break;
      }
      case STATE_DONE: screenProgress("Done!"); break;
      case STATE_STOPPED: screenProgress(":("); break;
    }
    
  } while (u8g2.nextPage());
}

/**
 * Draw centered string
 */
void drawCenterString(String s) {
  u8g2.setFont(u8g2_font_inb19_mr);
  u8g2.setCursor((127 - u8g2.getStrWidth(s.c_str())) / 2, 7); 
  u8g2.print(s);
}

/**
 * Draw bottom string
 */
void drawBottomString(String s) {
  u8g2.setFont(u8g2_font_t0_16_tr);
  u8g2.setCursor((127 - u8g2.getStrWidth(s.c_str())) / 2, 38);
  u8g2.print(s);
}

/**
 * Draw icon
 */
void drawIcon(byte i) {
  u8g2.drawXBMP(48, 3, 32, 32, menuIcons[i]);
}

/**
 * Show intro
 */
void displayIntro() {
  u8g2.firstPage();
  do {
    screenAbout();
  } while (u8g2.nextPage());
}

/**
 * Show screen when running program or stopped
 */
void screenProgress(String centerString) {
  drawCenterString(centerString);

  String s = 
    getTimeFromSeconds(programmDurationSeconds) + (programmMode != PROGRAMM_MODE_TIMER ? " " + String(motorRPMs[motorSpeedIndex]) + "rpm" : "");

  u8g2.setFont(u8g2_font_t0_16_tr);
  u8g2.setCursor(28, 38);
  u8g2.print(s);

  u8g2.drawXBMP(8, 38, 12, 12, smallIcons[programmMode]);  
}

/** 
 * Show about message
 */
void screenAbout() {
  drawCenterString("AuRA");
  drawBottomString("1.3/" + String(finishedCount));
}

/**
 * Draw menu
 */
void screenRootMenu() {
  drawIcon(menuPosition);
  drawBottomString(menuRoot[menuPosition]);
}

/**
 * Draw settings menu
 */
void screenSettingsMenu() {
  switch(menuSettingsPosition) {
    case MENU_SETTINGS_TANK: {
      drawIcon(ICON_TANK);
      drawBottomString(menuSettings[menuSettingsPosition] + (settingsTank == 0 ? "Jobo 1500" : (settingsTank == 1 ? "Jobo 2500" : "Paterson")));
      break;
    }
    
    case MENU_SETTINGS_PRESET1: 
    case MENU_SETTINGS_PRESET2: 
    case MENU_SETTINGS_PRESET3: 
    case MENU_SETTINGS_PRESET4: 
    case MENU_SETTINGS_PRESET5: 
    {
      menuSettingsPresetsPosition = menuSettingsPosition - MENU_SETTINGS_PRESET1;
      screenSettingsPresetsMenu();
      break;
    }

    case MENU_SETTINGS_SOUND: {
      drawIcon(ICON_SOUND);
      drawBottomString(menuSettings[menuSettingsPosition] + (settingsSound == 1 ? "On" : "Off"));
      break;
    }

    #if (DEMO_AVAILABLE == 1)
      case MENU_SETTINGS_DEMO: {
        drawCenterString("Demo");
        drawBottomString(menuSettings[menuSettingsPosition] + (settingsDemo == 1 ? "On" : "Off"));
        break;
      }
    #endif   
   
    case MENU_SETTINGS_SAVE: {
      drawIcon(ICON_OK);
      drawBottomString(menuSettings[menuSettingsPosition]);
      break;
    }
    case MENU_SETTINGS_CANCEL: {
      drawIcon(ICON_CANCEL);
      drawBottomString(menuSettings[menuSettingsPosition]);
      break;
    }
  }
}

/**
 * Show times settings
 */
void screenTimeSettings() {
  switch (menuMode) {
    case MENU_MODE_R1_MINUTES:
    case MENU_MODE_R2_MINUTES:
    case MENU_MODE_R3_MINUTES:
    case MENU_MODE_R4_MINUTES:
    case MENU_MODE_TIMER_MINUTES: {
      u8g2.drawRBox(20, 4, 37, 28, 3);
      break;
    }

    case MENU_MODE_R1_SECONDS:
    case MENU_MODE_R2_SECONDS:
    case MENU_MODE_R3_SECONDS: 
    case MENU_MODE_R4_SECONDS: 
    case MENU_MODE_TIMER_SECONDS: {
      u8g2.drawRBox(68, 4, 37, 28, 3);
      break;
    }
  }

  u8g2.setFontMode(1);
  u8g2.setDrawColor(2);
  drawCenterString(getTimeFromSeconds(programmDurationSeconds));

  u8g2.setFontMode(1);
  u8g2.setDrawColor(1);
  drawBottomString(menuRoot[programmMode]);
}

/**
 * Show times settings
 */
void screenRPMSettings() {
  u8g2.drawRBox(76, 4, 37, 28, 3);
  u8g2.setFontMode(1);
  u8g2.setDrawColor(2);
  if (motorSpeedIndex == 0) {
    drawCenterString("RPM: " + String(motorRPMs[motorSpeedIndex]));
  } else {
    drawCenterString("RPM:" + String(motorRPMs[motorSpeedIndex]));
  }

  u8g2.setFontMode(1);
  u8g2.setDrawColor(1);
  drawBottomString(menuRoot[programmMode]);
}

/**
 * Show presets
 */
void screenPresetsMenu() {
  u8g2.drawRBox(42, 4, 40, 28, 3);
  u8g2.setFontMode(1);
  u8g2.setDrawColor(2);
  drawCenterString("P" + String(menuPresetsPosition + 1));
  u8g2.setFontMode(1);
  u8g2.setDrawColor(1);
  
  u8g2.setFont(u8g2_font_t0_16_tr);
  u8g2.setCursor(28, 38);
  
  byte spd = presets[menuPresetsPosition][PRESET_SPEED];
  if ((settingsTank == TANK_J2500) && (spd > MOTOR_SPEEDS_COUNT - 3)) spd = MOTOR_SPEEDS_COUNT - 3; // 50 max

  u8g2.print(
    getTimeFromSeconds(presets[menuPresetsPosition][PRESET_MINUTES] * MAX_MINUTES + presets[menuPresetsPosition][PRESET_SECONDS]) + " " +
    String(motorRPMs[spd]) + "rpm"
  );

  u8g2.drawXBMP(8, 38, 12, 12, smallIcons[presets[menuPresetsPosition][PRESET_MODE]]);
}

/**
 * Show presets settings
 */
void screenSettingsPresetsMenu()
{
  if (menuMode == MENU_MODE_SETTINGS && menuSettingsPosition >= MENU_SETTINGS_PRESET1 && menuSettingsPosition <= MENU_SETTINGS_PRESET5) {
    u8g2.drawRBox(42, 4, 40, 28, 3);
    u8g2.setFontMode(1);
    u8g2.setDrawColor(2);
    drawCenterString(menuSettings[menuSettingsPosition]);
    u8g2.setFontMode(1);
    u8g2.setDrawColor(1);
  } else {
    drawCenterString(menuSettings[menuSettingsPosition]);
  }

  String s = 
    getTimeFromSeconds(presets[menuSettingsPresetsPosition][PRESET_MINUTES] * 60 + presets[menuSettingsPresetsPosition][PRESET_SECONDS]) + " " +
    String(motorRPMs[presets[menuSettingsPresetsPosition][PRESET_SPEED]]) + "rpm";

  if (menuMode == MENU_MODE_SETTINGS_PRESETX_MINUTES) u8g2.drawRFrame(25, 34, 22, 20, 3);
  else if (menuMode == MENU_MODE_SETTINGS_PRESETX_SECONDS) u8g2.drawRFrame(49, 34, 22, 20, 3);
  else if (menuMode == MENU_MODE_SETTINGS_PRESETX_SPEED) u8g2.drawRFrame(73, 34, 47, 20, 3);
  
  u8g2.setFont(u8g2_font_t0_16_tr);
  u8g2.setCursor(28, 38);
  u8g2.print(s);

  if (menuMode == MENU_MODE_SETTINGS_PRESETX_MODE) u8g2.drawRFrame(4, 34, 20, 20, 3);

  u8g2.drawXBMP(8, 38, 12, 12, smallIcons[presets[menuSettingsPresetsPosition][PRESET_MODE]]);
}
