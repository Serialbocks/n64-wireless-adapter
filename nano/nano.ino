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

#define N64_CONT_1_DATA PIN2_MASK
#define N64_CONT_2_DATA PIN3_MASK

#define DATA_PIN PIN6_MASK
#define DATA_READY_PIN PIN5_MASK
#define DATA_REQ_PIN PIN4_MASK

#define TEST_PIN PIN7_MASK

#define CONTROLLER_ID 0x0502
#define MAX_GAMEPADS 2

// MACROS
#define gpio_b_read(PIN) ( !!( PINB & PIN ) ) // 1 cycle
#define gpio_b_write(PIN, val) ( (val) ? PORTB |= PIN : PORTB &= ~PIN )
#define gpio_b_set_input(PIN) DDRB &= ~PIN; PORTB |= PIN;
#define gpio_b_set_output(PIN) DDRB |= PIN; PORTB &= ~PIN;

#define gpio_c_read(PIN) ( !!( PINC & PIN ) ) // 1 cycle
#define gpio_c_write(PIN, val) ( (val) ? PORTC |= PIN : PORTC &= ~PIN )
#define gpio_c_set_input(PIN) DDRC &= ~PIN;
#define gpio_c_set_output(PIN) DDRC |= PIN; PORTC |= PIN;

#define gpio_d_read(PIN) ( !!( PIND & PIN ) ) // 1 cycle
#define gpio_d_write(PIN, val) ( (val) ? PORTD |= PIN : PORTD &= ~PIN )
#define gpio_d_set_input(PIN) DDRD &= ~PIN;
#define gpio_d_set_output(PIN) DDRD |= PIN;


#define delay1() asm("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"); // delays ~1 microsecond. Not exactly 16 no-ops, because we need some cycles to do other things
#define delayHalf() asm("nop\nnop\nnop\n");
#define delay2() delay1() delay1();

volatile uint32_t buttonArr[MAX_GAMEPADS] = {};
volatile bool interrupted = false;

void cont1_isr();
void cont2_isr();

void setup() {
  // Set TCNT1 to use system clock with no divide
  TCCR1B &= 0xF8;
  TCCR1B |= 0x01;

  // N64 pin init
  gpio_d_set_input(N64_CONT_1_DATA);
  gpio_d_set_input(N64_CONT_2_DATA);

  // Data pins init
  gpio_d_set_output(DATA_REQ_PIN);
  gpio_d_write(DATA_REQ_PIN, 0);
  gpio_d_set_input(DATA_READY_PIN);
  gpio_d_set_input(DATA_PIN);
  gpio_d_set_output(TEST_PIN);
  gpio_d_write(TEST_PIN, 0);

  attachInterrupt(0, cont1_isr, FALLING);
  //attachInterrupt(1, cont2_isr, FALLING);
}

static inline void getButtonData() {
  // Request and read data from esp32
  interrupted = false;
  for(int i = 0; i < MAX_GAMEPADS; i++) {
    uint32_t buttons = 0;
    int bitCount = 32;
    gpio_d_write(DATA_REQ_PIN, 1);
    while(bitCount) {
      uint32_t count = 0;
      while(!gpio_d_read(DATA_READY_PIN) && count++ < 5000 && !interrupted);
      buttons <<= 1;
      buttons += gpio_d_read(DATA_PIN);
      gpio_d_write(DATA_REQ_PIN, 0);

      if(count >= 5000 || interrupted) return;
      while(gpio_d_read(DATA_READY_PIN));
      bitCount--;
    }
    if(!interrupted)
      buttonArr[i] = buttons;
  }
}

static inline void handle_controller(uint8_t n64DataPin, uint8_t contIndex) {
  uint8_t bitCount = 7;
  uint8_t dataIn = 0;
  uint32_t buttons = 0;

  while(gpio_d_read(n64DataPin));
  delay2();
  read_console_command:
    dataIn <<= 1;
    delay1();
    delayHalf();

    gpio_d_write(TEST_PIN, 1);
    dataIn += gpio_d_read(n64DataPin);
    gpio_d_write(TEST_PIN, 0);
    bitCount--;
    while(!gpio_d_read(n64DataPin));
    while(gpio_d_read(n64DataPin));

    if(bitCount)
      goto read_console_command;

    if(dataIn == 0x3f || dataIn == 0x00) {
      bitCount = 16;
      buttons = CONTROLLER_ID;
    } else {
      bitCount = 32;
      buttons = buttonArr[contIndex];
    }
    gpio_d_set_output(n64DataPin);
    gpio_d_write(n64DataPin, 1);
    delay2();
    delay2();
    delay2();
  
  sendBits:
    gpio_d_write(n64DataPin, 0);
    delayHalf();
    gpio_d_write(n64DataPin, buttons & 0x80000000);
    delay2();
    gpio_d_write(n64DataPin, 1);
    //delay1();
    bitCount--;
    buttons <<= 1;
    if(bitCount)
      goto sendBits;

    gpio_d_write(n64DataPin, 0);
    delay2();
    gpio_d_write(n64DataPin, 1);
    delay1();

    gpio_d_set_input(n64DataPin);

}

void cont1_isr() {
  interrupted = true;
  handle_controller(N64_CONT_1_DATA, 0);
  bitSet(EIFR, 0);
}

void cont2_isr() {
  interrupted = true;
  handle_controller(N64_CONT_2_DATA, 1);
  bitSet(EIFR, 0);
}

void loop() {
  getButtonData();
  if(interrupted) {
    for(int i = 0; i < 300; i++) {
      delay1();
    }
  }
    delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); delay2(); 
}
