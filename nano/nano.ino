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

#define DATA_PIN PIN6_MASK

#define SEND_DATA_PIN PIN4_MASK

#define CONTROLLER_ID 0x050000

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

void setup() {
  // Set TCNT1 to use system clock with no divide
  TCCR1B &= 0xF8;
  TCCR1B |= 0x01;

  asm("cli;");

  // N64 pin init
  gpio_d_set_input(DATA_PIN);
  gpio_d_set_output(SEND_DATA_PIN);

  // Data pins init
  gpio_d_write(SEND_DATA_PIN, 0);
  
}

void loop() {
  uint8_t bitCount = 0;
  uint8_t dataIn = 0;
  uint32_t dataOut = CONTROLLER_ID;

  while(!gpio_d_read(DATA_PIN));
  
  read_bit:
    while(gpio_d_read(DATA_PIN));
    delay2();
    if(gpio_d_read(DATA_PIN)) {
      dataIn++;
    }
    bitCount++;
    while(!gpio_d_read(DATA_PIN));
    if(bitCount < 8) {
      dataIn <<= 1;
      goto read_bit;
    }


  delay2();
  delay2();
  delay2();
  delay2();
  delay2();

  if(dataIn == 0x01) {
    goto signal_send_data;
  } else if(dataIn == 0x00 || dataIn == 0xff) {
    goto send_controller_status;
  }

  send_controller_status:
    bitCount = 0;
    gpio_d_set_output(DATA_PIN);

  send_controller_bit:
    gpio_d_write(DATA_PIN, 0);
    delay1();
    gpio_d_write(DATA_PIN, !!(dataOut & 0x800000));
    delay2();
    gpio_d_write(DATA_PIN, 1);
    delay1();
    bitCount++;
    if(bitCount < 24) {
      dataOut <<= 1;
      goto send_controller_bit;
    }

    // Stop bit
    gpio_d_write(DATA_PIN, 0);
    delay2();
    gpio_d_write(DATA_PIN, 1);

    gpio_d_set_input(DATA_PIN);
    return;


  signal_send_data:

  
  gpio_d_write(SEND_DATA_PIN, 1);
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();
  delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();delay2();

  gpio_d_write(SEND_DATA_PIN, 0);
}
