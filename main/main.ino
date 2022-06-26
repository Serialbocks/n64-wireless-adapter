

#include <Bluepad32.h>
#include "mappings.h"

#define N64_BUS_PIN 2

GamepadPtr myGamepad = nullptr;

void setup() {
  pinMode(N64_BUS_PIN, INPUT);
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

  BP32.update();
  if (myGamepad && myGamepad->isConnected()) {
    uint8_t dpad = myGamepad->dpad();
    uint16_t buttons = myGamepad->buttons();
    int xAxis = myGamepad->axisX();
    int yAxis = myGamepad->axisY();

    if(buttons | BUTTON_A)
      dataOut++;
    dataOut << 1;
    if(buttons | BUTTON_B)
      dataOut++;
    dataOut << 1;
    if(buttons | BUTTON_Z)
      dataOut++;
    dataOut << 1;
    if(buttons | BUTTON_START)
      dataOut++;
    dataOut << 1;
  }

  
  
  noInterrupts();
  pinMode(N64_BUS_PIN, INPUT);
  
  console_off:
    while(!digitalRead(N64_BUS_PIN));
    goto idle;

  idle:
    while(digitalRead(N64_BUS_PIN));
    goto read_console_command;

  read_console_command:
    bitCount = 8;
    dataIn = 0;
    while(bitCount--) {
      delay2us();
      dataIn += digitalRead(N64_BUS_PIN);
      dataIn << 1;
      while(!digitalRead(N64_BUS_PIN));
      while(digitalRead(N64_BUS_PIN));
    }
    if(dataIn == 0x00 || dataIn == 0xff) {
      dataOut = CONTROLLER_ID;
      bitCount = 16;
    } else if(dataIn == 0x01) {
      goto end;
    } else {
      goto end;
    }
    goto write_data;

  write_data:



  end:
  interrupts();
  

}
