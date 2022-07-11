

#include <Bluepad32.h>
#include "mappings.h"

#define N64_BUS_PIN 25
#define TEST_PIN 15
#define set_output(pin) (gpio_set_dir_out_masked(1U << pin))
#define set_input(pin) (gpio_set_dir_in_masked(1U << pin))

GamepadPtr myGamepad = nullptr;

void setup() {
  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
  BP32.forgetBluetoothKeys();
}

void onConnectedGamepad(GamepadPtr gp) {
  myGamepad = gp;
  BP32.enableNewBluetoothConnections(false);
}

void onDisconnectedGamepad(GamepadPtr gp) {
  myGamepad = nullptr;
  BP32.enableNewBluetoothConnections(true);
}

void loop() {
  uint8_t bitCount = 0;
  uint8_t dataIn = 0;
  uint32_t dataOut = 0;
  uint32_t n64_buttons = 0;


  //pinMode(3, OUTPUT);
  //while(true) {
  //  gpio_put(TEST_PIN, 1);
  //  delay1us();
  //  gpio_put(TEST_PIN, 0);
  //  delay1us();
  //}

  BP32.update();
  if (myGamepad && myGamepad->isConnected()) {
    uint8_t dpad = myGamepad->dpad();
    uint16_t buttons = myGamepad->buttons();
    int xAxis = myGamepad->axisX();
    int yAxis = myGamepad->axisY();

    if(buttons | BUTTON_A)
      n64_buttons++;
    n64_buttons = n64_buttons << 1;
    if(buttons | BUTTON_B)
      n64_buttons++;
    n64_buttons = n64_buttons << 1;
    if(buttons | BUTTON_Z)
      n64_buttons++;
    n64_buttons = n64_buttons << 1;
    if(buttons | BUTTON_START)
      n64_buttons++;
    n64_buttons = n64_buttons << 1;
    if(dpad | DPAD_UP)
      n64_buttons++;
    n64_buttons = n64_buttons << 1;
    if(dpad | DPAD_DOWN)
      n64_buttons++;
    n64_buttons = n64_buttons << 1;
    if(dpad | DPAD_LEFT)
      n64_buttons++;
    n64_buttons = n64_buttons << 1;
    if(dpad | DPAD_RIGHT)
      n64_buttons++;
    n64_buttons << 3; // 2 unused bits
    if(buttons | BUTTON_L)
      n64_buttons++;
    n64_buttons = n64_buttons << 1;
    if(buttons | BUTTON_R)
      n64_buttons++;
    n64_buttons = n64_buttons << 1;
    if(buttons | BUTTON_C_UP)
      n64_buttons++;
    n64_buttons = n64_buttons << 1;
    if(buttons | BUTTON_C_DOWN)
      n64_buttons++;
    n64_buttons = n64_buttons << 1;
    if(buttons | BUTTON_C_LEFT)
      n64_buttons++;
    n64_buttons = n64_buttons << 1;
    if(buttons | BUTTON_C_RIGHT)
      n64_buttons++;
    n64_buttons = n64_buttons << 1;

    // analog sticks - skip for now
    n64_buttons << 15;
  }

  pinMode(3, OUTPUT);

  //interrupts();
  set_input(N64_BUS_PIN);
  pinMode(2, INPUT);
  bitCount = 8;
  dataIn = 0;
  noInterrupts();

  read_console_command:
    gpio_put(TEST_PIN, 1);
    while(gpio_get(N64_BUS_PIN));
    gpio_put(TEST_PIN, 0);
    while(bitCount--) {
      if(bitCount != 7) {
        delayHalf();
        delay1us();
      }

      dataIn = dataIn << 1;
      if(gpio_get(N64_BUS_PIN)) {
        gpio_put(TEST_PIN, 1);
        dataIn++;
      }


      gpio_put(TEST_PIN, 0);
      while(!gpio_get(N64_BUS_PIN));
      while(gpio_get(N64_BUS_PIN));
      
    }
    delay2us(); // wait for stop bit
    delay1us();
    if(dataIn == 0x00 || dataIn == 0xff) {
      dataOut = CONTROLLER_ID;
      bitCount = 16;
    } else if(dataIn == 0x01) {
      dataOut = n64_buttons;
      bitCount = 32;
    } else {
      delay2us();
      goto end;
    }
    goto write_data;

  write_data:
    //interrupts();
    //gpio_put(N64_BUS_PIN, 1);
    set_output(N64_BUS_PIN);
    //pinMode(2, OUTPUT);
    //noInterrupts();
    while(bitCount--) {
      gpio_put(N64_BUS_PIN, 0);
      delay1us();
      if(dataOut | 0x80000000)
        gpio_put(N64_BUS_PIN, 0);
      else
        gpio_put(N64_BUS_PIN, 0);
      delay2us();
      gpio_put(N64_BUS_PIN, 1);
      delay1us();
      dataOut = dataOut << 1;
    }
    // stop bit
    gpio_put(N64_BUS_PIN, 0);
    delay2us();
    gpio_put(N64_BUS_PIN, 1);
    delay2us();
    goto end;


  end:
    set_input(N64_BUS_PIN);

  
  interrupts();
  

}
