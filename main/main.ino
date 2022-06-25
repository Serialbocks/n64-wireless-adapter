// Copyright 2021 - 2022, Ricardo Quesada
// SPDX-License-Identifier: Apache 2.0 or LGPL-2.1-or-later

/*
 * This example shows how to use the Gamepad API.
 *
 * Supported on boards with NINA W10x like:
 *  - Arduino MKR WiFi 1010,
 *  - UNO WiFi Rev.2,
 *  - Nano RP2040 Connect,
 *  - Nano 33 IoT,
 *  - etc.
 */
#include <Bluepad32.h>

GamepadPtr myGamepads[BP32_MAX_GAMEPADS] = {};

void setup() {
  // Initialize serial
  Serial.begin(9600);
  while (!Serial) {
    // wait for serial port to connect.
    // You don't have to do this in your game. This is only for debugging
    // purposes, so that you can see the output in the serial console.
    ;
  }

  String fv = BP32.firmwareVersion();
  Serial.print("Firmware version installed: ");
  Serial.println(fv);

  // BP32.pinMode(27, OUTPUT);
  // BP32.digitalWrite(27, 0);

  // This call is mandatory. It setups Bluepad32 and creates the callbacks.
  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);

  // "forgetBluetoothKeys()" should be called when the user performs
  // a "device factory reset", or similar.
  // Calling "forgetBluetoothKeys" in setup() just as an example.
  // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
  // But might also fix some connection / re-connection issues.
  BP32.forgetBluetoothKeys();

  //BP32.setDebug(true);
}

// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedGamepad(GamepadPtr gp) {
  bool foundEmptySlot = false;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myGamepads[i] == nullptr) {
      Serial.print("CALLBACK: Gamepad is connected, index=");
      Serial.println(i);
      myGamepads[i] = gp;
      foundEmptySlot = true;

      // Optional, once the gamepad is connected, request further info about the
      // gamepad.
      GamepadProperties properties = gp->getProperties();
      char buf[80];
      sprintf(buf,
              "BTAddr: %02x:%02x:%02x:%02x:%02x:%02x, VID/PID: %04x:%04x, "
              "flags: 0x%02x",
              properties.btaddr[0], properties.btaddr[1], properties.btaddr[2],
              properties.btaddr[3], properties.btaddr[4], properties.btaddr[5],
              properties.vendor_id, properties.product_id, properties.flags);
      Serial.println(buf);
      break;
    }
  }
  if (!foundEmptySlot) {
    Serial.println(
        "CALLBACK: Gamepad connected, but could not found empty slot");
  }
}

void onDisconnectedGamepad(GamepadPtr gp) {
  bool foundGamepad = false;

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myGamepads[i] == gp) {
      Serial.print("CALLBACK: Gamepad is disconnected from index=");
      Serial.println(i);
      myGamepads[i] = nullptr;
      foundGamepad = true;
      break;
    }
  }

  if (!foundGamepad) {
    Serial.println(
        "CALLBACK: Gamepad disconnected, but not found in myGamepads");
  }
}

void loop() {
  // This call fetches all the gamepad info from the NINA (ESP32) module.
  // Just call this function in your main loop.
  // The gamepad pointers (the ones received in the callbacks) gets updated
  // automatically.
  BP32.update();

  // It is safe to always do this before using the gamepad API.
  // This guarantees that the gamepad is valid and connected.
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    GamepadPtr myGamepad = myGamepads[i];

    if (myGamepad && myGamepad->isConnected()) {
      // There are different ways to query whether a button is pressed.
      // By query each button individually:
      //  a(), b(), x(), y(), l1(), etc...
      if (myGamepad->a()) {
        static int colorIdx = 0;
        // Some gamepads like DS4 and DualSense support changing the color LED.
        // It is possible to change it by calling:
        switch (colorIdx % 3) {
          case 0:
            // Red
            myGamepad->setColorLED(255, 0, 0);
            break;
          case 1:
            // Green
            myGamepad->setColorLED(0, 255, 0);
            break;
          case 2:
            // Blue
            myGamepad->setColorLED(0, 0, 255);
            break;
        }
        colorIdx++;
      }

      if (myGamepad->y()) {
        // Disable new gamepad connections
        //Serial.println("Bluetooth new connections disabled");
        BP32.enableNewBluetoothConnections(false);
      }

      // Another way to query the buttons, is by calling buttons(), or
      // miscButtons() which return a bitmask.
      // Some gamepads also have DPAD, axis and more.
      char buffer[150];
      snprintf(buffer, sizeof(buffer) - 1,
               "idx=%d, type=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4li, %4li",
               i,                      // Gamepad Index
               myGamepad->getModel(),
               myGamepad->dpad(),      // DPAD
               myGamepad->buttons(),   // bitmask of pressed buttons
               myGamepad->axisX(),     // (-511 - 512) left X Axis
               myGamepad->axisY()     // (-511 - 512) left Y axiss
      );
      Serial.println(buffer);

      // You can query the axis and other properties as well. See Gamepad.h
      // For all the available functions.
    }
  }

  delay(75);
}
