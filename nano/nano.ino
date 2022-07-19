/*
 * Author: Serialbocks
 * 
*/

#define PIN0_MASK 0b00000001
#define PIN1_MASK 0b00000010
#define PIN2_MASK 0b00000100
#define PIN3_MASK 0b00001000
#define PIN4_MASK 0b00010000
#define PIN5_MASK 0b00100000
#define PIN6_MASK 0b01000000
#define PIN7_MASK 0b10000000

#define N64_CONT_1_DATA PIN0_MASK
#define N64_CONT_1_ACTIVE PIN1_MASK
#define N64_CONT_2_DATA PIN2_MASK
#define N64_CONT_2_ACTIVE PIN3_MASK
#define N64_CONT_3_DATA PIN4_MASK
#define N64_CONT_3_ACTIVE PIN5_MASK
#define N64_CONT_4_DATA PIN6_MASK
#define N64_CONT_4_ACTIVE PIN7_MASK

#define DATA_PIN PIN6_MASK
#define DATA_READY_PIN PIN5_MASK
#define DATA_REQ_PIN PIN4_MASK

#define CONTROLLER_ID 0x0502
#define MAX_GAMEPADS 1

// MACROS
#define gpio_b_read(PIN) ( !!( PINB & PIN ) ) // 1 cycle
#define gpio_b_write(PIN, val) ( (val) ? PORTB |= PIN : PORTB &= ~PIN )
#define gpio_b_set_input(PIN) DDRB &= ~PIN; PORTB |= PIN;
#define gpio_b_set_output(PIN) DDRB |= PIN; PORTB |= PIN;

#define gpio_c_read(PIN) ( !!( PINC & PIN ) ) // 1 cycle
#define gpio_c_write(PIN, val) ( (val) ? PORTC |= PIN : PORTC &= ~PIN )
#define gpio_c_set_input(PIN) DDRC &= ~PIN;
#define gpio_c_set_output(PIN) DDRC |= PIN; PORTC |= PIN;

#define gpio_d_read(PIN) ( !!( PIND & PIN ) ) // 1 cycle
#define gpio_d_write(PIN, val) ( (val) ? PORTD |= PIN : PORTD &= ~PIN )
#define gpio_d_set_input(PIN) DDRD &= ~PIN; PORTD |= PIN;
#define gpio_d_set_output(PIN) DDRD |= PIN; PORTD |= PIN;


#define delay1() asm("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"); // delays ~1 microsecond. Not exactly 16 no-ops, because we need some cycles to do other things
#define delayHalf() asm("nop\nnop\nnop\n");
#define delay2() delay1() delay1();

uint32_t buttonArr[MAX_GAMEPADS] = {};
uint32_t buttons = 0;

void setup() {
  // Set TCNT1 to use system clock with no divide
  TCCR1B &= 0xF8;
  TCCR1B |= 0x01;

  // Disable interrupts so we have complete control over each CPU cycle
  asm("cli\n");

  // N64 pin init
  gpio_c_set_input(N64_CONT_1_DATA);
  gpio_c_set_input(N64_CONT_1_ACTIVE);
  gpio_c_set_input(N64_CONT_2_DATA);
  gpio_c_set_input(N64_CONT_2_ACTIVE);
  gpio_c_set_input(N64_CONT_3_DATA);
  gpio_c_set_input(N64_CONT_3_ACTIVE);
  gpio_c_set_input(N64_CONT_4_DATA);
  gpio_c_set_input(N64_CONT_4_ACTIVE);

  // Data pins init
  gpio_d_set_output(DATA_REQ_PIN);
  gpio_d_write(DATA_REQ_PIN, 0);
  gpio_d_set_input(DATA_READY_PIN);
  gpio_d_set_input(DATA_PIN);

}

static void getButtonData() {
    // Request and read data from esp32
  for(int i = 0; i < MAX_GAMEPADS; i++) {
    uint32_t buttons = 0;
    int bitCount = 32;
    gpio_d_write(DATA_REQ_PIN, 1);
    while(bitCount) {
      uint32_t count = 0;
      while(!gpio_d_read(DATA_READY_PIN) && count++ < 5000);
      buttons <<= 1;
      buttons += gpio_d_read(DATA_PIN);
      gpio_d_write(DATA_REQ_PIN, 0);

      if(count >= 5000) return;
      while(gpio_d_read(DATA_READY_PIN));
      bitCount--;
    }
    buttonArr[i] = buttons;
  }
}

void loop() {
  uint8_t n64DataPin = 0;
  uint8_t n64ActivePin = 0;
  uint8_t bitCount = 8;
  uint8_t dataIn = 0;

  
  for(int i = MAX_GAMEPADS - 1; i >= 0; i--) {
    switch(i) {
      case 0:
        n64DataPin = N64_CONT_1_DATA;
        n64ActivePin = N64_CONT_1_ACTIVE;
        break;
      case 1:
        n64DataPin = N64_CONT_2_DATA;
        n64ActivePin = N64_CONT_2_ACTIVE;
        break;
      case 2:
        n64DataPin = N64_CONT_3_DATA;
        n64ActivePin = N64_CONT_3_ACTIVE;
        break;
      case 3:
        n64DataPin = N64_CONT_4_DATA;
        n64ActivePin = N64_CONT_4_ACTIVE;
        break;
      default:
        break;
    }

    if(!gpio_c_read(n64ActivePin)) {
      // This controller is not plugged in
      continue;
    }
    
    bitCount = 8;
    dataIn = 0;
      while(gpio_c_read(n64DataPin)); // Idle

  read_console_command:
    dataIn <<= 1;
    delay1();
    dataIn += gpio_c_read(n64DataPin);
    bitCount--;
    while(!gpio_c_read(n64DataPin));
    while(gpio_c_read(n64DataPin));

    if(bitCount)
      goto read_console_command;

    if(dataIn == 0xff || dataIn == 0x01) {
      bitCount = 16;
      buttons = CONTROLLER_ID;
    } else {
      getButtonData();
      bitCount = 32;
      buttons = buttonArr[i];
    }
    gpio_c_set_output(n64DataPin);
    gpio_c_write(n64DataPin, 1);
    delay2();
    delay2();
    delay2();
  
  sendBits:
    gpio_c_write(n64DataPin, 0);
    delayHalf();
    gpio_c_write(n64DataPin, buttons & 0x80000000);
    delay2();
    gpio_c_write(n64DataPin, 1);
    //delay1();
    bitCount--;
    buttons <<= 1;
    if(bitCount)
      goto sendBits;

    gpio_c_write(n64DataPin, 0);
    delay2();
    gpio_c_write(n64DataPin, 1);
    delay1();
  
    gpio_c_set_input(n64DataPin);
    
  }

  
}
