

#include <Bluepad32.h>
#include "mappings.h"

#define MAX_CONTROLLER_CONNECTIONS 1

GamepadPtr myGamepad = nullptr;

void setup() {
  pinMode(2, OUTPUT);

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
  BP32.update();
  if (myGamepad && myGamepad->isConnected()) {
    uint8_t dpad = myGamepad->dpad();
    uint16_t buttons = myGamepad->buttons();
    int xAxis = myGamepad->axisX();
    int yAxis = myGamepad->axisY();
  }
  
  noInterrupts();
  digitalWrite(2, HIGH);
  delayHalf();
  digitalWrite(2, LOW);
  delay2us();
  digitalWrite(2, HIGH);
  delay1us();
  digitalWrite(2, LOW);
  delay2us();
  digitalWrite(2, HIGH);
  delay2us();
  digitalWrite(2, LOW);
  delay2us();
  interrupts();
  

}
