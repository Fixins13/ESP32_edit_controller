#include "driver/rtc_io.h"
#include "BleKeyboard.h"

// Ext wakeup pin mask
#define BUTTON_PIN_BITMASK  0x10A007010

// Debounce delay for the buttons
#define KEY_DEBOUNCE_DELAY  150

// IDLE time for sleep
#define IDLE_TIME_DURATION  2000



// Push button pin assignmetn
#define I_KEY               32
#define O_KEY               25
#define PERIOD_KEY          27
#define COMMA_KEY           14
#define SHIFT_KEY           4

// Rotary encoder pin assignment
#define KY40_CLK  33
#define KY40_DT   26
#define KY40_SW   15


BleKeyboard bleKeyboard;

long startTime = 0, currentTime = 0;

// Button and respective charactor array
uint8_t buttonCharacterCombinationArray[][2] {  {I_KEY, 'i'},
                                                {O_KEY, 'o'},
                                                {PERIOD_KEY, '.'},
                                                {COMMA_KEY, ','},
};

// Encoder breakout board SW is pulled up, so seperately defined
uint8_t KY40SWCharacter = ' ';

// Button and respective function array
uint8_t buttonFunctionCombinationArray[][3] {{SHIFT_KEY, KEY_LEFT_SHIFT, 0},
};

static uint8_t prevNextCode = 0;
static uint16_t store = 0;

// Rotary encoder handler
int8_t readRotary() {
  static int8_t rotEncTable[] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0};

  prevNextCode <<= 2;
  if (digitalRead(KY40_DT)) prevNextCode |= 0x02;
  if (digitalRead(KY40_CLK)) prevNextCode |= 0x01;
  prevNextCode &= 0x0f;

  // If valid then store as 16 bit data.
  if  (rotEncTable[prevNextCode] ) {
    store <<= 4;
    store |= prevNextCode;
    //if (store==0xd42b) return 1;
    //if (store==0xe817) return -1;
    if ((store & 0xff) == 0x2b) return -1;
    if ((store & 0xff) == 0x17) return 1;
  }
  return 0;
}

void setup() {

  for (int i = 0; i < (sizeof(buttonFunctionCombinationArray) / sizeof(buttonFunctionCombinationArray[0])); i++) {
    pinMode(buttonFunctionCombinationArray[i][0], INPUT_PULLDOWN);
    rtc_gpio_pulldown_en((gpio_num_t)buttonFunctionCombinationArray[i][0]);
    rtc_gpio_hold_en((gpio_num_t)buttonFunctionCombinationArray[i][0]);
  }

  for (int i = 0; i < (sizeof(buttonCharacterCombinationArray) / sizeof(buttonCharacterCombinationArray[0])); i++) {
    pinMode(buttonCharacterCombinationArray[i][0], INPUT_PULLDOWN);
    rtc_gpio_pulldown_en((gpio_num_t)buttonCharacterCombinationArray[i][0]);
    rtc_gpio_hold_en((gpio_num_t)buttonCharacterCombinationArray[i][0]);
  }

  //Setup Rotary encoder
  pinMode(KY40_CLK, INPUT);
  pinMode(KY40_CLK, INPUT_PULLUP);
  pinMode(KY40_DT, INPUT);
  pinMode(KY40_DT, INPUT_PULLUP);
  pinMode(KY40_SW, INPUT);
  pinMode(KY40_SW, INPUT_PULLUP);

  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  
  bleKeyboard.begin();
  delay(500);
  
  startTime = millis();
}

void loop() {

  if (bleKeyboard.isConnected()) {

    static int8_t val;
    if (val = readRotary() ) {
      for (int i = 0; i < (sizeof(buttonFunctionCombinationArray) / sizeof(buttonFunctionCombinationArray[0])); i++) {
        if (digitalRead(buttonFunctionCombinationArray[i][0]) && !buttonFunctionCombinationArray[i][2]) {
          bleKeyboard.press(buttonFunctionCombinationArray[i][1]);
          buttonFunctionCombinationArray[i][2] = 1;
          delay(100);
        }
        else if (!digitalRead(buttonFunctionCombinationArray[i][0]) && buttonFunctionCombinationArray[i][2]) {
          bleKeyboard.release(buttonFunctionCombinationArray[i][1]);
          buttonFunctionCombinationArray[i][2] = 0;
          delay(100);
        }
      }

      if (val == 1) {
        bleKeyboard.write(KEY_RIGHT_ARROW);
      }
      else if (val == -1) {
        bleKeyboard.write(KEY_LEFT_ARROW);
      }
      startTime = millis();
    }

    for (int i = 0; i < (sizeof(buttonCharacterCombinationArray) / sizeof(buttonCharacterCombinationArray[0])); i++) {
      if (digitalRead(buttonCharacterCombinationArray[i][0])) {
        bleKeyboard.write(buttonCharacterCombinationArray[i][1]);
        delay(KEY_DEBOUNCE_DELAY);
        startTime = millis();
      }
    }

    if (!digitalRead(KY40_SW)) {
      bleKeyboard.write(KY40SWCharacter);
      delay(KEY_DEBOUNCE_DELAY);
      startTime = millis();
    }
  }

  currentTime = millis();
  int duration = (int)abs(currentTime - startTime);
  if ((duration / 1000) > IDLE_TIME_DURATION) {
    delay(1000);
    esp_deep_sleep_start();
  }

}
