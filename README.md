Nintendo recently began manufacturing Nintendo 64 controllers for the Nintendo Switch. These controllers are completely wireless and act like any other switch controller.

This project includes the firmware and 3d printable enclosure for a wireless receiver/adapter allowing use of this controller on original Nintnedo 64 hardware.

The design uses an ESP32 microcontroller to receive the controller signal. Unfortunately, due to the nature of the multi-threaded processor when wireless functions are enabled, the precise timing for the Nintendo 64 controller protocol could not be achieved. Therefore, an additional ATMEGA328 (Arduino Nano) is needed using bit-banging to implement the protocol.
