#define X_AXIS_MIN -330
#define X_AXIS_MAX 370
#define X_AXIS_NEUTRAL 13

#define Y_AXIS_MIN 344
#define Y_AXIS_MAX -362
#define Y_AXIS_NEUTRAL 10

#define BUTTON_A 0x0002
#define BUTTON_B 0x0001
#define BUTTON_START 0x0080
#define BUTTON_Z 0x0010
#define BUTTON_L 0x0008
#define BUTTON_R 0x0200
#define BUTTON_C_UP 0x0100
#define BUTTON_C_LEFT 0x0004
#define BUTTON_C_RIGHT 0x0040
#define BUTTON_C_DOWN 0x0020

#define DPAD_UP 0x01
#define DPAD_LEFT 0x08
#define DPAD_RIGHT 0x04
#define DPAD_DOWN 0x02

#define CONTROLLER_ID 0x0502

#define delayHalf() asm("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
#define delay1us() asm("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
#define delay2us() asm("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;")
