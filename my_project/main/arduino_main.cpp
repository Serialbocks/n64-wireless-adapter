#include "sdkconfig.h"
#ifndef CONFIG_BLUEPAD32_PLATFORM_ARDUINO
#error "Must only be compiled when using Bluepad32 Arduino platform"
#endif  // !CONFIG_BLUEPAD32_PLATFORM_ARDUINO

#include <Arduino.h>
#include <Bluepad32.h>
#include "soc/rtc_wdt.h"
#include "driver/adc.h"



#include "mappings.h"

#define N64_PIN GPIO_NUM_32

GamepadPtr myGamepad = nullptr;

void onConnectedGamepad(GamepadPtr gp) {
  myGamepad = gp;
  BP32.enableNewBluetoothConnections(false);
}

void onDisconnectedGamepad(GamepadPtr gp) {
  myGamepad = nullptr;
  BP32.enableNewBluetoothConnections(true);
}

void setup() {
  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
  BP32.forgetBluetoothKeys();
  gpio_set_direction(N64_PIN, GPIO_MODE_INPUT_OUTPUT);
}

// Arduino loop function. Runs in CPU 1
void loop() {
    BP32.update();



    gpio_set_level(N64_PIN, 1);
    delay1us();
    gpio_set_level(N64_PIN, 0);
    delay1us();
    

}
