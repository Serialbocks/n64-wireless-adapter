#include "sdkconfig.h"
#ifndef CONFIG_BLUEPAD32_PLATFORM_ARDUINO
#error "Must only be compiled when using Bluepad32 Arduino platform"
#endif  // !CONFIG_BLUEPAD32_PLATFORM_ARDUINO

#include <Arduino.h>
#include <Bluepad32.h>

#include "driver/uart.h"
#include "mappings.h"

#define DATA_REQ_PIN GPIO_NUM_23
#define DATA_READY_PIN GPIO_NUM_22
#define TEST_PIN GPIO_NUM_21
#define MAX_GAMEPADS 1

typedef struct gamepad_t {
  GamepadPtr b32Gamepad = nullptr;
  int16_t xAxisNeutral = 0;
  int16_t yAxisNeutral = 0;
  int16_t xAxisMin = -300;
  int16_t xAxisMax = 300;
  int16_t yAxisMin = -300;
  int16_t yAxisMax = 300;
} gamepad;

uint8_t connectedGamepads = 0;
gamepad gamepads[MAX_GAMEPADS];

const uart_port_t uart_num = UART_NUM_1;
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
static const int RX_BUF_SIZE = 1024;
uint8_t test_data[2] = { 0x08, 0x08 };

static inline void resetController(gamepad* gp) {
  gp->xAxisNeutral = 0;
  gp->yAxisNeutral = 0;

  gp->xAxisMin = -300;
  gp->xAxisMax = 300;
  gp->yAxisMin = -300;
  gp->yAxisMax = 300;
}

void onConnectedGamepad(GamepadPtr gp) {
    if(connectedGamepads < MAX_GAMEPADS) {
      gamepad* gamepad = &gamepads[connectedGamepads];
      gamepad->b32Gamepad = gp;
      resetController(gamepad);
      connectedGamepads++;
    }

    if(connectedGamepads >= MAX_GAMEPADS)
    {
      BP32.enableNewBluetoothConnections(false);
    }

}

void onDisconnectedGamepad(GamepadPtr gp) {
    connectedGamepads--;
    gamepads[connectedGamepads].b32Gamepad = nullptr;
    BP32.enableNewBluetoothConnections(true);
}

static inline uint32_t get_controller_state(gamepad* gp) {
    uint32_t n64_buttons = 0;
    GamepadPtr myGamepad = gp->b32Gamepad;

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
      if(xAxis > gp->xAxisMax)
        gp->xAxisMax = xAxis;
      if(xAxis < gp->xAxisMin)
        gp->xAxisMin = xAxis;
      if(yAxis > gp->yAxisMax)
        gp->yAxisMax = yAxis;
      if(yAxis < gp->yAxisMin)
        gp->yAxisMin = yAxis;

      bool xAxisPositive = xAxis > 0;
      float n64XAxisFactor =xAxisPositive ? ((float)xAxis / gp->xAxisMax) : ((float)xAxis / gp->xAxisMin);
      uint8_t n64XAxis = 0;
      if(n64XAxisFactor > DEADZONE) {
        n64XAxis = (uint8_t)(n64XAxisFactor * 110);
        if(!xAxisPositive) {
          n64XAxis = 0x80 - n64XAxis;
          n64XAxis |= 0x80;
        }
          
          
      }

      bool yAxisPositive = yAxis > 0;
      float n64YAxisFactor = yAxisPositive ? ((float)yAxis / gp->yAxisMax) : ((float)yAxis / gp->yAxisMin);
      uint8_t n64YAxis = 0;
      if(n64YAxisFactor > DEADZONE) {
        n64YAxis = (uint8_t)(n64YAxisFactor * 110);
        if(!yAxisPositive) {
          n64YAxis = 0x80 - n64YAxis;
          n64YAxis |= 0x80;
        }
          
      }

      n64_buttons <<= 8;
      n64_buttons += n64XAxis;
      n64_buttons <<= 8;
      n64_buttons += n64YAxis;

    }

    return n64_buttons;
} 

static inline void setup_uart() {

   const uart_config_t uart_config = {
        .baud_rate = 1000000,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // We won't use a buffer for sending data.
    uart_driver_install(uart_num, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(uart_num, &uart_config);
    uart_set_pin(uart_num, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void setup() {
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
    BP32.forgetBluetoothKeys();
    setup_uart();
    gpio_set_direction(TEST_PIN, GPIO_MODE_OUTPUT);
}

// Arduino loop function. Runs in CPU 1
void loop() {
    BP32.update();
    uint32_t buttonArr[MAX_GAMEPADS] = {0};

    for(int i = 0; i < connectedGamepads; i++) {
      buttonArr[i] = get_controller_state(&(gamepads[i]));
    }

    
    uint8_t data[128];
    int length = 0;
    gpio_set_level(TEST_PIN, 1);
    ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t*)&length));
    
    if(length >= 3) {
      //Console.printf("Length: %d\n", length);

      length = uart_read_bytes(uart_num, data, length, 100);
      uart_flush(uart_num);
      gpio_set_level(TEST_PIN, 0);

      uart_write_bytes(uart_num, (const char*)test_data, 2);
      delayMicroseconds(20);

      uart_flush(uart_num);
      //Console.printf("Data: 0x%02x 0x%02x 0x%02x\n",
      //  data[0], data[1], data[2]);
    }

    gpio_set_level(TEST_PIN, 0);

    //uart_write_bytes(uart_num, (const char*)test_data, 2);
    //delay2us();
    //delay2us();
    //uart_flush(uart_num);
}
