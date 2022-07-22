#include "sdkconfig.h"
#ifndef CONFIG_BLUEPAD32_PLATFORM_ARDUINO
#error "Must only be compiled when using Bluepad32 Arduino platform"
#endif  // !CONFIG_BLUEPAD32_PLATFORM_ARDUINO

#include <Arduino.h>
#include <Bluepad32.h>

#include "driver/uart.h"
#include "driver/gpio.h"
#include "xtensa/hal.h"
#include "mappings.h"


#define TEST_PIN GPIO_NUM_21
#define MAX_GAMEPADS 1

#define TXD_PIN GPIO_NUM_17
#define RXD_PIN GPIO_NUM_16

#define GPIO_INTERRUPT_INPUT GPIO_NUM_22
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INTERRUPT_INPUT)

#define PLAYER_LEDS 0x03

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

const uart_port_t uart_num = UART_NUM_2;
static const int RX_BUF_SIZE = 1024;

#define UART_DATA_LEN 17
#define UART_ID_LEN 13
#define N64_UART_00 0x08
#define N64_UART_10 0x0f
#define N64_UART_01 0xe0
uint8_t controller_data[UART_DATA_LEN] = { 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0xfc };
uint8_t controller_id[UART_ID_LEN] = { 0x08, 0x08, 0xe8, 0xe8, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0xfc };

#define POLL_DATA 1
#define POLL_ID 2
#define POLL_DATA_TIMEOUT 80000000
volatile uint8_t pollVal = 0;
volatile unsigned lastPollTimestamp = 0;

#define ESP_INTR_FLAG_DEFAULT 0

static inline void get_controller_state(gamepad* gp);

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    //delayMicroseconds(33);

    if(xthal_get_ccount() - lastPollTimestamp > POLL_DATA_TIMEOUT) {
      pollVal = POLL_ID;
    } else {
      pollVal = POLL_DATA;
    }
    lastPollTimestamp = xthal_get_ccount();

}

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
      delayMicroseconds(10);
      gp->setPlayerLEDs(PLAYER_LEDS);
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

static inline void get_controller_state(gamepad* gp) {
    GamepadPtr myGamepad = gp->b32Gamepad;

    if (myGamepad && myGamepad->isConnected()) {
      uint8_t dataIndex = 0;
      uint8_t dpad = myGamepad->dpad();
      uint16_t buttons = myGamepad->buttons();
      int xAxis = myGamepad->axisX();
      int yAxis = -myGamepad->axisY();

      // Uart data hack: http://www.qwertymodo.com/hardware-projects/n64/n64-controller
      controller_data[dataIndex] = N64_UART_00;
      if(buttons & BUTTON_A)
        controller_data[dataIndex] |= N64_UART_10;
      if(buttons & BUTTON_B)
        controller_data[dataIndex] |= N64_UART_01;
      dataIndex++;
      
      controller_data[dataIndex] = N64_UART_00;
      if(buttons & BUTTON_Z)
        controller_data[dataIndex] |= N64_UART_10;
      if(buttons & BUTTON_START)
        controller_data[dataIndex] |= N64_UART_01;
      dataIndex++;

      controller_data[dataIndex] = N64_UART_00;
      if(dpad & DPAD_UP)
        controller_data[dataIndex] |= N64_UART_10;
      if(dpad & DPAD_DOWN)
        controller_data[dataIndex] |= N64_UART_01;
      dataIndex++;
      
      controller_data[dataIndex] = N64_UART_00;
      if(dpad & DPAD_LEFT)
        controller_data[dataIndex] |= N64_UART_10;
      if(dpad & DPAD_RIGHT)
        controller_data[dataIndex] |= N64_UART_01;
      dataIndex++;

      // 2 unused bits
      controller_data[dataIndex] = N64_UART_00;
      dataIndex++;

      controller_data[dataIndex] = N64_UART_00;
      if(buttons & BUTTON_L)
        controller_data[dataIndex] |= N64_UART_10;
      if(buttons & BUTTON_R)
        controller_data[dataIndex] |= N64_UART_01;
      dataIndex++;

      controller_data[dataIndex] = N64_UART_00;
      if(buttons & BUTTON_C_UP)
        controller_data[dataIndex] |= N64_UART_10;
      if(buttons & BUTTON_C_DOWN)
        controller_data[dataIndex] |= N64_UART_01;
      dataIndex++;

      controller_data[dataIndex] = N64_UART_00;
      if(buttons & BUTTON_C_LEFT)
        controller_data[dataIndex] |= N64_UART_10;
      if(buttons & BUTTON_C_RIGHT)
        controller_data[dataIndex] |= N64_UART_01;
      dataIndex++;

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
        n64XAxis = (uint8_t)(n64XAxisFactor * 85);
        if(!xAxisPositive) {
          n64XAxis = 0x80 - n64XAxis;
          n64XAxis |= 0x80;
        }
      }

      bool yAxisPositive = yAxis > 0;
      float n64YAxisFactor = yAxisPositive ? ((float)yAxis / gp->yAxisMax) : ((float)yAxis / gp->yAxisMin);
      uint8_t n64YAxis = 0;
      if(n64YAxisFactor > DEADZONE) {
        n64YAxis = (uint8_t)(n64YAxisFactor * 85);
        if(!yAxisPositive) {
          n64YAxis = 0x80 - n64YAxis;
          n64YAxis |= 0x80;
        }
      }
      
      for(int i = 0; i < 4; i++) {
        controller_data[dataIndex] = N64_UART_00;
        if(n64XAxis & 0x80) {
          controller_data[dataIndex] |= N64_UART_10;
        }
        if(n64XAxis & 0x40) {
          controller_data[dataIndex] |= N64_UART_01;
        }
        n64XAxis <<= 2;
        dataIndex++;
      }

      for(int i = 0; i < 4; i++) {
        controller_data[dataIndex] = N64_UART_00;
        if(n64YAxis & 0x80) {
          controller_data[dataIndex] |= N64_UART_10;
        }
        if(n64YAxis & 0x40) {
          controller_data[dataIndex] |= N64_UART_01;
        }
        n64YAxis <<= 2;
        dataIndex++;
      }
    }
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

static inline void enable_interrupts() {
    gpio_isr_handler_add(GPIO_INTERRUPT_INPUT, gpio_isr_handler, (void*)GPIO_INTERRUPT_INPUT);
}

static inline void disable_interrupts() {
    gpio_isr_handler_remove(GPIO_INTERRUPT_INPUT);
}

static inline void setup_gpio_interrupt() {
    gpio_config_t io_conf;
    //interrupt of falling edge
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = (gpio_pullup_t)1;
    gpio_config(&io_conf);

    //change gpio interrupt type for one pin
    gpio_set_intr_type(GPIO_INTERRUPT_INPUT, GPIO_INTR_ANYEDGE);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    enable_interrupts();
}

void setup() {
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
    BP32.forgetBluetoothKeys();
    gpio_set_direction(TEST_PIN, GPIO_MODE_OUTPUT);
    setup_uart();
    setup_gpio_interrupt();
}


// Arduino loop function. Runs in CPU 1
void loop() {
    if(pollVal) {

      disable_interrupts();
      if(pollVal == POLL_DATA) {
        uart_write_bytes(uart_num, (const char*)controller_data, UART_DATA_LEN);
      } else if(pollVal == POLL_ID) {
        uart_write_bytes(uart_num, (const char*)controller_id, UART_ID_LEN);
      }

      uart_wait_tx_done(uart_num, 10000);
      uart_flush(uart_num);
      pollVal = 0;
      enable_interrupts();
      BP32.update();
      for(int i = 0; i < connectedGamepads; i++) {
        get_controller_state(&(gamepads[i]));
      }
    }
    delayMicroseconds(1);
}
