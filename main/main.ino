

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
  uint32_t n64_buttons = 0;

  BP32.update();
  if (myGamepad && myGamepad->isConnected()) {
    uint8_t dpad = myGamepad->dpad();
    uint16_t buttons = myGamepad->buttons();
    int xAxis = myGamepad->axisX();
    int yAxis = myGamepad->axisY();

    if(buttons | BUTTON_A)
      n64_buttons++;
    n64_buttons << 1;
    if(buttons | BUTTON_B)
      n64_buttons++;
    n64_buttons << 1;
    if(buttons | BUTTON_Z)
      n64_buttons++;
    n64_buttons << 1;
    if(buttons | BUTTON_START)
      n64_buttons++;
    n64_buttons << 1;
    if(dpad | DPAD_UP)
      n64_buttons++;
    n64_buttons << 1;
    if(dpad | DPAD_DOWN)
      n64_buttons++;
    n64_buttons << 1;
    if(dpad | DPAD_LEFT)
      n64_buttons++;
    n64_buttons << 1;
    if(dpad | DPAD_RIGHT)
      n64_buttons++;
    n64_buttons << 3; // 2 unused bits
    if(buttons | BUTTON_L)
      n64_buttons++;
    n64_buttons << 1;
    if(buttons | BUTTON_R)
      n64_buttons++;
    n64_buttons << 1;
    if(buttons | BUTTON_C_UP)
      n64_buttons++;
    n64_buttons << 1;
    if(buttons | BUTTON_C_DOWN)
      n64_buttons++;
    n64_buttons << 1;
    if(buttons | BUTTON_C_LEFT)
      n64_buttons++;
    n64_buttons << 1;
    if(buttons | BUTTON_C_RIGHT)
      n64_buttons++;
    n64_buttons << 1;

    // analog sticks - skip for now
    n64_buttons << 15;
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
    delay2us(); // wait for stop bit
    delay1us();
    if(dataIn == 0x00 || dataIn == 0xff) {
      dataOut = CONTROLLER_ID;
      bitCount = 16;
    } else if(dataIn == 0x01) {
      dataOut = n64_buttons;
      bitCount = 32;
    } else {
      goto end;
    }
    goto write_data;

  write_data:
    pinMode(N64_BUS_PIN, OUTPUT);
    while(bitCount--) {
      digitalWrite(N64_BUS_PIN, LOW);
      delay1us();
      if(dataOut | 0x80000000)
        digitalWrite(N64_BUS_PIN, HIGH);
      else
        digitalWrite(N64_BUS_PIN, LOW);
      delay2us();
      digitalWrite(N64_BUS_PIN, HIGH);
      delay1us();
      dataOut << 1;
    }
    // stop bit
    digitalWrite(N64_BUS_PIN, LOW);
    delay2us();
    digitalWrite(N64_BUS_PIN, HIGH);
    delay2us();
    goto end;


  end:
    pinMode(N64_BUS_PIN, INPUT);

  
  interrupts();
  

}
