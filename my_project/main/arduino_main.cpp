#include "sdkconfig.h"
#ifndef CONFIG_BLUEPAD32_PLATFORM_ARDUINO
#error "Must only be compiled when using Bluepad32 Arduino platform"
#endif  // !CONFIG_BLUEPAD32_PLATFORM_ARDUINO

#include <Arduino.h>
#include <Bluepad32.h>

#include "mappings.h"

#define N64_PIN GPIO_NUM_32
#define DATA_REQ_PIN GPIO_NUM_23
#define DATA_READY_PIN GPIO_NUM_22
#define DATA_PIN GPIO_NUM_21

GamepadPtr myGamepad = nullptr;
int16_t xAxisNeutral = 0;
int16_t yAxisNeutral = 0;
int16_t xAxisMin = -300;
int16_t xAxisMax = 300;
int16_t yAxisMin = -300;
int16_t yAxisMax = 300;

static inline void resetController() {
  xAxisNeutral = 0;
  yAxisNeutral = 0;

  xAxisMin = -300;
  xAxisMax = 300;
  yAxisMin = -300;
  yAxisMax = 300;
}

void onConnectedGamepad(GamepadPtr gp) {
    myGamepad = gp;
    BP32.enableNewBluetoothConnections(false);
    resetController();
}

void onDisconnectedGamepad(GamepadPtr gp) {
    myGamepad = nullptr;
    BP32.enableNewBluetoothConnections(true);
}

static inline uint32_t get_controller_state() {
    uint32_t n64_buttons = 0;

    BP32.update();
    if (myGamepad && myGamepad->isConnected()) {
      uint8_t dpad = myGamepad->dpad();
      uint16_t buttons = myGamepad->buttons();
      int xAxis = myGamepad->axisX();
      int yAxis = -myGamepad->axisY();

      if(buttons & BUTTON_A)
        n64_buttons++;
      n64_buttons <<= 1;
      if(buttons & BUTTON_B)
        n64_buttons++;
      n64_buttons <<= 1;
      if(buttons & BUTTON_Z)
        n64_buttons++;
      n64_buttons <<= 1;
      if(buttons & BUTTON_START)
        n64_buttons++;
      n64_buttons <<= 1;
      if(dpad & DPAD_UP)
        n64_buttons++;
      n64_buttons <<= 1;
      if(dpad & DPAD_DOWN)
        n64_buttons++;
      n64_buttons <<= 1;
      if(dpad & DPAD_LEFT)
        n64_buttons++;
      n64_buttons <<= 1;
      if(dpad & DPAD_RIGHT)
        n64_buttons++;
      n64_buttons <<= 3; // 2 unused bits
      if(buttons & BUTTON_L)
        n64_buttons++;
      n64_buttons <<= 1;
      if(buttons & BUTTON_R)
        n64_buttons++;
      n64_buttons <<= 1;
      if(buttons & BUTTON_C_UP)
        n64_buttons++;
      n64_buttons <<= 1;
      if(buttons & BUTTON_C_DOWN)
        n64_buttons++;
      n64_buttons <<= 1;
      if(buttons & BUTTON_C_LEFT)
        n64_buttons++;
      n64_buttons <<= 1;
      if(buttons & BUTTON_C_RIGHT)
        n64_buttons++;

      // analog sticks
      if(xAxis > xAxisMax)
        xAxisMax = xAxis;
      if(xAxis < xAxisMin)
        xAxisMin = xAxis;
      if(yAxis > yAxisMax)
        yAxisMax = yAxis;
      if(yAxis < yAxisMin)
        yAxisMin = yAxis;

      bool xAxisPositive = xAxis > 0;
      float n64XAxisFactor =xAxisPositive ? ((float)xAxis / xAxisMax) : ((float)xAxis / xAxisMin);
      int8_t n64XAxis = 0;
      if(n64XAxisFactor > DEADZONE) {
        n64XAxis = (int8_t)(n64XAxisFactor * 127);
        if(!xAxisPositive)
          n64XAxis *= -1;
      }

      bool yAxisPositive = yAxis > 0;
      float n64YAxisFactor = yAxisPositive ? ((float)yAxis / yAxisMax) : ((float)yAxis / yAxisMin);
      int8_t n64YAxis = 0;
      if(n64YAxisFactor > DEADZONE) {
        n64YAxis = (int8_t)(n64YAxisFactor * 127);
        if(!yAxisPositive)
          n64YAxis *= -1;
      }

      n64_buttons <<= 8;
      n64_buttons |= n64XAxis;
      n64_buttons <<= 8;
      n64_buttons |= n64YAxis;

    }

    return n64_buttons;
} 

void setup() {
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
    BP32.forgetBluetoothKeys();
    gpio_set_direction(N64_PIN, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_direction(DATA_REQ_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(DATA_READY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(DATA_PIN, GPIO_MODE_OUTPUT);
}

// Arduino loop function. Runs in CPU 1
void loop() {
    while(!gpio_get_level(DATA_REQ_PIN));
    uint32_t buttons = get_controller_state();
    
    int dataCount = 32;
    while(dataCount) {
        gpio_set_level(DATA_PIN, !!(buttons & 0x80000000));
        gpio_set_level(DATA_READY_PIN, 1);
        delay2us();
        delay2us();
        gpio_set_level(DATA_READY_PIN, 0);
        delay2us();
        delay2us();
        dataCount--;
        buttons <<= 1;
    }
}
