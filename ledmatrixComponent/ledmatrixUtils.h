#ifndef __led_matrix_h_
#define __led_matrix_h_
#include <stdint.h>
#include <stdbool.h>

#define I2C_CMD_CONTINUE_DATA			0x81

#define GROVE_TWO_RGB_LED_MATRIX_DEF_I2C_ADDR	0x65 // The device i2c address in default
#define GROVE_TWO_RGB_LED_MATRIX_VID		0x2886 // Vender ID of the device
#define GROVE_TWO_RGB_LED_MATRIX_PID		0x8005 // Product ID of the device

#define I2C_CMD_GET_DEV_ID			0x00 // This command gets device ID information
#define I2C_CMD_DISP_BAR			0x01 // This command displays LED bar
#define I2C_CMD_DISP_EMOJI			0x02 // This command displays emoji
#define I2C_CMD_DISP_NUM			0x03 // This command displays number
#define I2C_CMD_DISP_STR			0x04 // This command displays string
#define I2C_CMD_DISP_CUSTOM			0x05 // This command displays user-defined pictures
#define I2C_CMD_DISP_OFF			0x06 // This command cleans the display
#define I2C_CMD_DISP_ASCII			0x07 // not use
#define I2C_CMD_DISP_FLASH			0x08 // This command displays pictures which are stored in flash
#define I2C_CMD_DISP_COLOR_BAR			0x09 // This command displays colorful led bar
#define I2C_CMD_DISP_COLOR_WAVE			0x0a // This command displays built-in wave animation
#define I2C_CMD_DISP_COLOR_CLOCKWISE		0x0b // This command displays built-in clockwise animation
#define I2C_CMD_DISP_COLOR_ANIMATION		0x0c // This command displays other built-in animation
#define I2C_CMD_DISP_COLOR_BLOCK		0x0d // This command displays an user-defined color
#define I2C_CMD_STORE_FLASH			0xa0 // This command stores frames in flash
#define I2C_CMD_DELETE_FLASH			0xa1 // This command deletes all the frames in flash

#define I2C_CMD_LED_ON				0xb0 // This command turns on the indicator LED flash mode
#define I2C_CMD_LED_OFF				0xb1 // This command turns off the indicator LED flash mode
#define I2C_CMD_AUTO_SLEEP_ON			0xb2 // This command enable device auto sleep mode
#define I2C_CMD_AUTO_SLEEP_OFF			0xb3 // This command disable device auto sleep mode (default mode)

#define I2C_CMD_DISP_ROTATE			0xb4 // This command setting the display orientation
#define I2C_CMD_DISP_OFFSET			0xb5 // This command setting the display offset

#define I2C_CMD_SET_ADDR			0xc0 // This command sets device i2c address
#define I2C_CMD_RST_ADDR			0xc1 // This command resets device i2c address
#define I2C_CMD_TEST_TX_RX_ON			0xe0 // This command enable TX RX pin test mode
#define I2C_CMD_TEST_TX_RX_OFF			0xe1 // This command disable TX RX pin test mode
#define I2C_CMD_TEST_GET_VER			0xe2 // This command use to get software version
#define I2C_CMD_GET_DEVICE_UID			0xf1 // This command use to get chip id

#ifndef _GROVE_TWO_LED_MATRIX_H_
	enum orientation_type_t
	{
		DISPLAY_ROTATE_0 = 0,
		DISPLAY_ROTATE_90 = 1,
		DISPLAY_ROTATE_180 = 2,
		DISPLAY_ROTATE_270 = 3,
	};
#endif

enum COLORS
{
	red = 0x00,
	orange = 0x12,
	yellow = 0x18,
	green = 0x52,
	cyan = 0x7f,
	blue = 0xaa,
	purple =0xc3,
	pink = 0xdc,
	white = 0xfe,
	black = 0xff,
};

struct led_matrix {
	uint16_t (*getDeviceVID)(void);
	uint16_t (*getDevicePID)(void);
	void (*changeDeviceBaseAddress)(uint8_t);
	void (*defaultDeviceAddress)(void);
	void (*turnOnLedFlash)(void);
	void (*turnOffLedFlash)(void);
	void (*enableAutoSleep)(void);
	void (*wakeDevice)(void);
	void (*disableAutoSleep)(void);
	void (*setDisplayOrientation)(enum orientation_type_t);
	void (*setDisplayOffset)(int, int);
	void (*displayBar)(uint8_t, uint16_t, bool, uint8_t);
	void (*displayColorBar)(uint8_t, uint16_t, bool);
	void (*displayColorWave)(uint8_t, uint16_t, bool);
	void (*displayClockwise)(bool, bool, uint16_t, bool);
	void (*displayColorAnimation)(uint8_t, uint16_t, bool);
	void (*displayEmoji)(uint8_t, uint16_t, bool);
	void (*displayNumber)(int16_t, uint16_t, bool, uint8_t);
	void (*displayString)(char *, uint16_t, bool, uint8_t);
	void (*displayFrames)(uint8_t *, uint16_t, bool, uint8_t);
	void (*displayFrames64)(uint64_t *, uint16_t, bool, uint8_t);
	void (*displayColorBlock)(uint32_t, uint16_t, bool);
	void (*stopDisplay)(void);
	void (*storeFrames)(void);
	void (*deleteFrames)(void);
	void (*displayFramesFromFlash)(uint16_t, bool, uint8_t, uint8_t);
	void (*enableTestMode)(void);
	void (*disableTestMode)(void);
	uint32_t (*getTestVersion)(void);
	void (*resetDevice)(void);
	void (*getDeviceId)(void);
};

/**
 * Function: Create led_matrix object contains all led matrix functionality
 * Param:	- i2c_bus: string base, represent I2C bus (ex: /dev/i2c-0)
 * 		- base: led matrix I2C address range from 0x10 to 0x70
 * 			leave 0 for default address
 * 		- screenNumber: range from 1 to 16, represent index of this led
 * 			matrix
 **/
struct led_matrix *led_matrix_create(const char *i2c_bus,
				     uint8_t base, 
				     uint8_t screenNumber);

/**
 * Function: Release led matrix object
 * Param:	- led_matrix: led matrix object created by led_matrix_create
 **/
int led_matrix_release(struct led_matrix *led_matrix);

#endif
