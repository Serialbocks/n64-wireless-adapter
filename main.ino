/*
 * Author: Serialbocks
 * 
*/
#include "button_config.h"

#define PIN2_MASK 0b00000100
#define PIN6_MASK 0b01000000

// MACROS
#define READ_PIND2 ( ( PIND & ( 1 << PIND2 ) ) >> PIND2 ) // 1 cycle
#define READ_PIND6 ( ( PIND & ( 1 << PIND6 ) ) >> PIND6 ) // 1 cycle
#define delay1() asm("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"); // delays ~1 microsecond. Not exactly 16 no-ops, because we need some cycles to do other things
#define delayHalf() asm("nop\nnop\nnop\n");
#define delay2() delay1() delay1();

#define setInputMode() DDRD &= ~PIN2_MASK; PORTD |= PIN2_MASK;
#define setOutputMode() DDRD |= PIN2_MASK; PORTD |= PIN2_MASK;
#define setGCInputMode() DDRD &= ~PIN6_MASK; PORTD |= PIN6_MASK;
#define setGCOutputMode() DDRD |= PIN6_MASK; PORTD |= PIN6_MASK;
#define setPIND2(val) ( (val) ? PORTD |= PIN2_MASK : PORTD &= ~PIN2_MASK )
#define setPIND6(val) ( (val) ? PORTD |= PIN6_MASK : PORTD &= ~PIN6_MASK )
#define checkGCLine() if(!READ_PIND6) goto gamecube_sample_fsm;

// CONSTANTS
#define N64_POLL_BYTE 0x01
#define RETRY_TIMEOUT 128
#define GC_RETRY_TIMEOUT 32
#define TIME_BETWEEN_POLL 5000

// GLOBALS
static const unsigned char device_id[] = {0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char button_state[BUTTON_STATE_LEN + GET_ORIGIN_LEN];

void setup() {
  // Set TCNT1 to use system clock with no divide
  TCCR1B &= 0xF8;
  TCCR1B |= 0x01;

  // Disable interrupts so we have complete control over each CPU cycle
  asm("cli\n");

  // Set Pin 2 as an output with pull-up resistor enabled
  setOutputMode();
  setPIND2(1);

  // Set Pin 6 as input with pull-up resistor enabled
  setGCInputMode();

}

/*
 * This is where the magic happens: the implementation of all of the state machines. 
 * The reason I use goto's here instead of function calls is because a function call takes more CPU cycles than we can afford!
 * The resolution of the controller signals is at 1 MHZ, and our clock is 16 MHZ, so 16 clock cycles is enough for us to miss a signal.
 * 
 * That said, goto's can be clean and easy to follow, especially when mapped to a FSM.
 * 
 */
void loop() {
  unsigned char n64PollByte;
  unsigned char n64PollBitCount;
  unsigned long n64InputSR;
  unsigned long n64InputValues;
  unsigned char n64InputBitCount;
  unsigned int gcBitCount;
  unsigned int gcTotalBits;
  unsigned char* gcOutputValues;
  unsigned long i;
  unsigned int retries;
  unsigned int previousRetries;

  /*
  ------------------------------------------------
  Poll N64 Controller FSM
  -----------------------------------------------
  */
  poll_n64_controller_fsm:
    setOutputMode();
    setGCInputMode();
    n64PollByte = N64_POLL_BYTE;
    n64PollBitCount = 0;
    setPIND2(1);
  write_bit:
    setPIND2(0);
    checkGCLine();
    delay1();
    checkGCLine();
    setPIND2(n64PollByte >> 7);
    delay2();
    checkGCLine();
    setPIND2(1);
    n64PollByte = n64PollByte << 1;
    n64PollBitCount++;
    delay1();
    checkGCLine();
    if(n64PollBitCount <= 7)
      goto write_bit;

    // stop bit
    setPIND2(0);
    delay1();
    checkGCLine();
    setPIND2(1);
    delay2();
    checkGCLine();
    goto sample_n64_controller_fsm;

  delay_between_n64_poll:
    for(i = 0; i < TIME_BETWEEN_POLL; i++) {
      delayHalf();
      checkGCLine();
    }
    goto poll_n64_controller_fsm;

  /*
   ------------------------------------------------
   Sample N64 Controller FSM
   -----------------------------------------------
  */
  sample_n64_controller_fsm:
    setInputMode();
    n64InputSR = 0;
    n64InputBitCount = 0;
    retries = 0;
    while( READ_PIND2 && ++retries < RETRY_TIMEOUT && READ_PIND6 );
    if(retries >= RETRY_TIMEOUT) goto poll_n64_controller_fsm;
    checkGCLine();
    goto sample;

  sample:
    // delay 1.5
    delay1();
    delayHalf();

    // sample
    n64InputSR <<= 1;
    n64InputSR |= READ_PIND2;
    n64InputBitCount++;

    checkGCLine();

    // wait_high
    retries = 0;
    while( !READ_PIND2 && ++retries < RETRY_TIMEOUT && READ_PIND6 );
    if(retries >= RETRY_TIMEOUT) goto poll_n64_controller_fsm;
    checkGCLine();

    // wait_low
    retries = 0;
    while( READ_PIND2 && ++retries < RETRY_TIMEOUT && READ_PIND6 );
    if(retries >= RETRY_TIMEOUT) goto poll_n64_controller_fsm;
    checkGCLine();

    if(n64InputBitCount < 32)
      goto sample;

    goto map_buttons;

  map_buttons:
    // delay 4 to wait to take the line back
    checkGCLine();
    delay2();
    checkGCLine();
    delay2();
    checkGCLine();
    n64InputValues = n64InputSR;
    setOutputMode();

    // Remap buttons
    map_buttons(button_state, n64InputValues);

    checkGCLine();
    
    goto delay_between_n64_poll;
    
    
  /*
   ------------------------------------------------
   Gamecube Sample FSM
   -----------------------------------------------
  */
  gamecube_sample_fsm:
    gcBitCount = 0;
    previousRetries = 0;
    setGCInputMode();
    goto wait_high_gc;

  wait_high_gc:
    retries = 0;
    while( !READ_PIND6 && ++retries < RETRY_TIMEOUT );
    if(retries >= RETRY_TIMEOUT) goto poll_n64_controller_fsm;
    goto wait_low_gc;

  wait_low_gc:
    retries = 0;
    gcBitCount++;
    while( READ_PIND6 && ++retries < GC_RETRY_TIMEOUT );
    if(retries < GC_RETRY_TIMEOUT)  {
      previousRetries = retries;
      goto wait_high_gc;
    }

    // A somewhat hacky way of determining the command the GC adapter is sending to us. We simply count the bits.
    // If the bit count is more than 9, it's polling for button state (25 bits). Otherwise if it is 9 bits, we can determine 
    // between "Get Origin" and "Device ID" (both 9 bit) commands by remembering if the last bit sent was a 1.
    if(gcBitCount > 9) {
      gcTotalBits = BUTTON_STATE_LEN; // Button State
      gcOutputValues = button_state;
    } else if(previousRetries > 2) {
      gcTotalBits = BUTTON_STATE_LEN + GET_ORIGIN_LEN; // Get Origin
      gcOutputValues = button_state;
    } else {
      gcTotalBits = DEVICE_ID_LEN; // Device ID
      gcOutputValues = device_id;
    }

    goto send_gamecube_button_state_fsm;


   /*
   ------------------------------------------------
   Send Gamecube Button State FSM
   -----------------------------------------------
  */
  send_gamecube_button_state_fsm:
    gcBitCount = 0;
    setGCOutputMode();
    goto write_bit_gc;

  write_bit_gc:
    setPIND6(0);
    delay1();
    setPIND6(gcOutputValues[gcBitCount]);
    delay2();
    setPIND6(1);
    gcBitCount++;
    delay1();
    if(gcBitCount <= gcTotalBits)
      goto write_bit_gc;

    // stop bit is just a 1 
    setPIND6(0);
    delay1();
    asm("nop\nnop\nnop\nnop\nnop\n");
    setPIND6(1);
    delay2();
    delay1();
    goto poll_n64_controller_fsm;
}
