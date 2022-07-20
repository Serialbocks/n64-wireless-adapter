#include "sdkconfig.h"
#ifndef CONFIG_BLUEPAD32_PLATFORM_ARDUINO
#error "Must only be compiled when using Bluepad32 Arduino platform"
#endif  // !CONFIG_BLUEPAD32_PLATFORM_ARDUINO

#include <Arduino.h>
#include <Bluepad32.h>

#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "mappings.h"


#define TEST_PIN GPIO_NUM_21
#define MAX_GAMEPADS 1

#define TXD_PIN GPIO_NUM_17
#define RXD_PIN GPIO_NUM_16

#define GPIO_INTERRUPT_INPUT GPIO_NUM_22
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INTERRUPT_INPUT)

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
uint8_t test_data[17] = { 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xfe };
uint8_t data[125];


#define ESP_INTR_FLAG_DEFAULT 0
static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            gpio_set_level(TEST_PIN, 1);
            gpio_set_level(TEST_PIN, 0);
        }
    }
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

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INTERRUPT_INPUT, gpio_isr_handler, (void*)GPIO_INTERRUPT_INPUT);
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
    //BP32.update();
    //uint32_t buttonArr[MAX_GAMEPADS] = {0};
//
    //for(int i = 0; i < connectedGamepads; i++) {
    //  buttonArr[i] = get_controller_state(&(gamepads[i]));
    //}

  vTaskDelay(1000);
}
