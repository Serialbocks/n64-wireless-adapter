/*
 * Author: Serialbocks
 * 
*/

#define PIN2_MASK 0b00000100
#define PIN3_MASK 0b00001000
#define PIN4_MASK 0b00010000
#define PIN5_MASK 0b00100000
#define PIN6_MASK 0b01000000

#define N64_PIN PIN6_MASK
#define DATA_PIN PIN3_MASK
#define DATA_READY_PIN PIN4_MASK
#define DATA_REQ_PIN PIN5_MASK

#define CONTROLLER_ID 0x0502

// MACROS
#define gpio_read(PIN) ( !!( PIND & PIN ) ) // 1 cycle
#define gpio_write(PIN, val) ( (val) ? PORTD |= PIN : PORTD &= ~PIN )
#define gpio_set_input(PIN) DDRD &= ~PIN; PORTD |= PIN;
#define gpio_set_output(PIN) DDRD |= PIN; PORTD |= PIN;

#define delay1() asm("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"); // delays ~1 microsecond. Not exactly 16 no-ops, because we need some cycles to do other things
#define delayHalf() asm("nop\nnop\nnop\n");
#define delay2() delay1() delay1();

uint32_t buttons = 0;

void setup() {
  // Set TCNT1 to use system clock with no divide
  TCCR1B &= 0xF8;
  TCCR1B |= 0x01;

  // Disable interrupts so we have complete control over each CPU cycle
  asm("cli\n");

  // N64 pin init
  gpio_set_input(N64_PIN);

  // Data pins init
  gpio_set_output(DATA_REQ_PIN);
  gpio_write(DATA_REQ_PIN, 0);
  gpio_set_input(DATA_READY_PIN);
  gpio_set_input(DATA_PIN);


}

void loop() {
  uint8_t bitCount = 8;
  uint8_t dataIn = 0;
  while(!gpio_read(N64_PIN)); // Console off
  while(gpio_read(N64_PIN)); // Idle

  read_console_command:
    dataIn <<= 1;
    delay1();
    buttons += gpio_read(N64_PIN);
    bitCount--;
    while(!gpio_read(N64_PIN));
    while(gpio_read(N64_PIN));

    if(bitCount)
      goto read_console_command;

    if(dataIn == 0xff || dataIn == 0x01) {
      bitCount = 16;
      buttons = CONTROLLER_ID;
    } else {
      bitCount = 32;
    }

  gpio_set_output(N64_PIN);
  gpio_write(N64_PIN, 1);
  delay2();
  delay2();
  delay2();
  
  sendBits:
    gpio_write(N64_PIN, 0);
    delayHalf();
    gpio_write(N64_PIN, buttons & 0x80000000);
    delay2();
    gpio_write(N64_PIN, 1);
    //delay1();
    bitCount--;
    buttons <<= 1;
    if(bitCount)
      goto sendBits;

  gpio_write(N64_PIN, 0);
  delay2();
  gpio_write(N64_PIN, 1);
  delay1();

  gpio_set_input(N64_PIN);

  // Request and read data from esp32
  buttons = 0;
  bitCount = 32;
  gpio_write(DATA_REQ_PIN, 1);
  while(bitCount) {
    while(!gpio_read(DATA_READY_PIN));
    buttons <<= 1;
    buttons += gpio_read(DATA_PIN);
    gpio_write(DATA_REQ_PIN, 0);
    while(gpio_read(DATA_READY_PIN));
    bitCount--;
  }

  
  bitCount = 32;



}
