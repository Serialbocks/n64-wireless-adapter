

#include <Bluepad32.h>
#include "mappings.h"

#define MAX_CONTROLLER_CONNECTIONS 1



GamepadPtr myGamepads[MAX_CONTROLLER_CONNECTIONS] = {};

void setup() {
  pinMode(2, OUTPUT);

  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);

  BP32.forgetBluetoothKeys();

  noInterrupts();
}

void onConnectedGamepad(GamepadPtr gp) {
  bool foundSlot = false;
  for (int i = 0; i < MAX_CONTROLLER_CONNECTIONS; i++) {
    if (myGamepads[i] == nullptr) {
      myGamepads[i] = gp;
      foundSlot = true;
      break;
    }
  }

  if(foundSlot) {
    BP32.enableNewBluetoothConnections(false);
  }
}

void onDisconnectedGamepad(GamepadPtr gp) {
  for (int i = 0; i < MAX_CONTROLLER_CONNECTIONS; i++) {
    if (myGamepads[i] == gp) {
      myGamepads[i] = nullptr;
      break;
    }
  }

  BP32.enableNewBluetoothConnections(true);
}

void loop() {
  
  
  digitalWrite(2, HIGH);
  delay1us();
  digitalWrite(2, LOW);
  delay1us();

  /*for (int i = 0; i < MAX_CONTROLLER_CONNECTIONS; i++) {
    GamepadPtr myGamepad = myGamepads[i];

    if (myGamepad && myGamepad->isConnected()) {

      uint8_t dpad = myGamepad->dpad();
      uint16_t buttons = myGamepad->buttons();
      int xAxis = myGamepad->axisX();
      int yAxis = myGamepad->axisY();
    }
  }*/
}
