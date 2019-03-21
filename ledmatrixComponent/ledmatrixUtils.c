#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/io.h>
#include <stdio.h>
#include <errno.h>

#include "ledmatrixUtils.h"
#include "i2c-utils.h"
#include <legato.h>

// private variables
static uint8_t led_matrix_currentDeviceAddress;
static uint8_t led_matrix_offsetAddress;
static uint8_t led_matrix_baseAddress;
static uint32_t led_matrix_deviceId[3]; // Unique device ID(96 bits: Low, Middle, High)

static char led_matrix_i2c_bus[255] = "/dev/i2c-0";

static void i2cSendByte(uint8_t address, uint8_t data)
{
	int i2c_fd = open(led_matrix_i2c_bus, O_RDWR);
	if (i2c_fd < 0) {
		LE_ERROR("i2cSendByte: failed to open %s", led_matrix_i2c_bus);
	}
	if (ioctl(i2c_fd, I2C_SLAVE_FORCE, address) < 0) {
		LE_ERROR("Could not set address to 0x%02x: %s\n",
		       address,
		       strerror(errno));
		return;
	}
	i2c_smbus_write_byte(i2c_fd, data);
	close(i2c_fd);
}
static void i2cSendBytes(uint8_t address, uint8_t *data, uint8_t len)
{
	int i2c_fd = open(led_matrix_i2c_bus, O_RDWR);
	if (i2c_fd < 0) {
		LE_ERROR("i2cSendByte: failed to open %s", led_matrix_i2c_bus);
	}
	if (ioctl(i2c_fd, I2C_SLAVE_FORCE, address) < 0) {
		LE_ERROR("Could not set address to 0x%02x: %s\n",
		       address,
		       strerror(errno));
		return;
	}
	i2c_smbus_write_i2c_block_data(i2c_fd, data[0], len-1, data+1);
	close(i2c_fd);
}
static void i2cSendContinueBytes(uint8_t address, uint8_t *data, uint8_t len)
{
	uint8_t cbytes = I2C_CMD_CONTINUE_DATA;
	int i2c_fd = open(led_matrix_i2c_bus, O_RDWR);
	if (i2c_fd < 0) {
		LE_ERROR("i2cSendByte: failed to open %s", led_matrix_i2c_bus);
	}
	if (ioctl(i2c_fd, I2C_SLAVE_FORCE, address) < 0) {
		LE_ERROR("Could not set address to 0x%02x: %s\n",
		       address,
		       strerror(errno));
		return;
	}
	i2c_smbus_write_i2c_block_data(i2c_fd, cbytes, len+1, data);
	close(i2c_fd);
}
static void i2cReceiveBytes(uint8_t address, uint8_t *data, uint8_t len)
{
	int i2c_fd = open(led_matrix_i2c_bus, O_RDWR);
	if (i2c_fd < 0) {
		LE_ERROR("i2cSendByte: failed to open %s", led_matrix_i2c_bus);
	}
	if (ioctl(i2c_fd, I2C_SLAVE_FORCE, address) < 0) {
		LE_ERROR("Could not set address to 0x%02x: %s\n",
		       address,
		       strerror(errno));
		return;
	}
	i2c_smbus_read_i2c_block_data(i2c_fd, 0, len, data);

	close(i2c_fd);
}

// /*************************************************************
// * Description
// *	Get vendor ID of device.
// * Parameter
// *	Null.
// * Return
// *	Return vendor ID of device.
// *************************************************************/
static uint16_t led_matrix_getDeviceVID(void)
{
	uint8_t data[4] = {0, };
	i2cSendByte(led_matrix_currentDeviceAddress, I2C_CMD_GET_DEV_ID);
	i2cReceiveBytes(led_matrix_currentDeviceAddress, data, 4);
	return (uint16_t)(data[0] + data[1] * 256);
}

// /*************************************************************
// * Description
// *	Get product ID of device.
// * Parameter
// *	Null.
// * Return
// *	Return product ID of device.
// *************************************************************/
static uint16_t led_matrix_getDevicePID(void)
{
	uint8_t data[4] = {0, };
	i2cSendByte(led_matrix_currentDeviceAddress, I2C_CMD_GET_DEV_ID);
	i2cReceiveBytes(led_matrix_currentDeviceAddress, data, 4);
	return (uint16_t)(data[2] + data[3] * 256);
}

// /*************************************************************
// * Description
// *	Change i2c base address of device.
// * Parameter
// *	newAddress: 0x10-0x70, The new i2c base address of device.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_changeDevicebaseAddress(uint8_t newAddress)
{
	uint8_t data[2] = {0, };
	if (!(newAddress >= 0x10 && newAddress <= 0x70)) {
		newAddress = GROVE_TWO_RGB_LED_MATRIX_DEF_I2C_ADDR;
	}
	data[0] = I2C_CMD_SET_ADDR;
	data[1] = newAddress;
	led_matrix_baseAddress = newAddress;
	i2cSendBytes(led_matrix_currentDeviceAddress, data, 2);
	led_matrix_currentDeviceAddress = led_matrix_baseAddress + led_matrix_offsetAddress;
	sleep(0.2);
}

// /*************************************************************
// * Description
// *	Restore the i2c address of device to default.
// * Parameter
// *	Null.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_defaultDeviceAddress(void)
{
	i2cSendByte(led_matrix_currentDeviceAddress, I2C_CMD_RST_ADDR);
	led_matrix_baseAddress = GROVE_TWO_RGB_LED_MATRIX_DEF_I2C_ADDR;
	led_matrix_currentDeviceAddress = led_matrix_baseAddress + led_matrix_offsetAddress;
	sleep(0.2);
}

// /*************************************************************
// * Description
// *	Turn on the indicator LED flash mode.
// * Parameter
// *	Null.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_turnOnLedFlash(void)
{
	i2cSendByte(led_matrix_currentDeviceAddress, I2C_CMD_LED_ON);
}

// /*************************************************************
// * Description
// *	Turn off the indicator LED flash mode.
// * Parameter
// *	Null.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_turnOffLedFlash(void)
{
	i2cSendByte(led_matrix_currentDeviceAddress, I2C_CMD_LED_OFF);
}

// /*************************************************************
// * Description
// *	Enable device auto sleep mode. Send any I2C commands will 
// * wake up all the sleepy devices.
// * Parameter
// *	Null.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_enableAutoSleep(void)
{
	i2cSendByte(led_matrix_currentDeviceAddress, I2C_CMD_AUTO_SLEEP_ON);
}

// /*************************************************************
// * Description
// *	Don't need this function anymore.
// *	(Wake device from sleep mode. It takes about 0.15ms.)
// * Parameter
// *	Null.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_wakeDevice(void)
{
	usleep(200);
}

// /*************************************************************
// * Description
// *	Disable device auto sleep mode.
// * Parameter
// *	Null.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_disableAutoSleep(void)
{
	i2cSendByte(led_matrix_currentDeviceAddress, I2C_CMD_AUTO_SLEEP_OFF);
}

// /*************************************************************
// * Description
// *	Setting the display orientation.
// *	This function can be used before or after display.
// *	DO NOT WORK with displayColorWave(), displayClockwise(), displayColorAnimation()
// * Parameter
// *	orientation: DISPLAY_ROTATE_0, DISPLAY_ROTATE_90, DISPLAY_ROTATE_180,
// * DISPLAY_ROTATE_270, which means the display will rotate 0°, 90°,180° or 270°.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_setDisplayOrientation(enum orientation_type_t orientation)
{
	uint8_t data[2] = {0, };
	data[0] = I2C_CMD_DISP_ROTATE;
	data[1] = (uint8_t)orientation;
	i2cSendBytes(led_matrix_currentDeviceAddress, data, 2);
}

// /*************************************************************
// * Description
// *	Setting the display offset of x-axis and y-axis.
// *	This function can be used before or after display.
// *	DO NOT WORK with displayColorWave(), displayClockwise(), displayColorAnimation(),
// *	displayNumber(when number<0 or number>=10), displayString(when more than one character)
// * Parameter
// *	offset_x: The display offset value of horizontal x-axis, range from -8 to 8.
// *	offset_y: The display offset value of horizontal y-axis, range from -8 to 8.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_setDisplayOffset(int offset_x, int offset_y)
{
	// convert to positive
	offset_x +=8;
	offset_y +=8;

	if (offset_x < 0)
		offset_x = 0;
	else if (offset_x > 16)
		offset_x = 16;

	if (offset_y < 0)
		offset_y = 0;
	else if (offset_y > 16)
		offset_y = 16;

	uint8_t data[3] = {0, };
	data[0] = I2C_CMD_DISP_OFFSET;
	data[1] = (uint8_t)offset_x;
	data[2] = (uint8_t)offset_y;
	i2cSendBytes(led_matrix_currentDeviceAddress, data, 3);
}

// /*************************************************************
// * Description
// *	Display a bar on RGB LED Matrix.
// * Parameter
// *	bar: 0 - 32. 0 is blank and 32 is full.
// *	duration_time: Set the display time(ms) duration. Set it to 0 to not display.
// *	forever_flag: Set it to true to display forever, and the duration_time will not work.
// *		      Or set it to false to display one time.
// *	color: Set the color of the display, range from 0 to 255. See COLORS for more details.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_displayBar(uint8_t bar,
				  uint16_t duration_time,
				  bool forever_flag,
				  uint8_t color)
{
	uint8_t data[6] = {0, };

	data[0] = I2C_CMD_DISP_BAR;
	if (bar > 32) 
		bar = 32;
	data[1] = bar;
	data[2] = (uint8_t)(duration_time & 0xff);
	data[3] = (uint8_t)((duration_time >> 8) & 0xff);
	data[4] = forever_flag;
	data[5] = color;
	i2cSendBytes(led_matrix_currentDeviceAddress, data, 6);
}

// /*************************************************************
// * Description
// *	Display a colorful bar on RGB LED Matrix.
// * Parameter
// *	bar: 0 - 32. 0 is blank and 32 is full.
// *	duration_time: Set the display time(ms) duration. Set it to 0 to not display.
// *	forever_flag: Set it to true to display forever, and the duration_time will not work.
// *		      Or set it to false to display one time.
// * Return
// *    Null.
// *************************************************************/
static void led_matrix_displayColorBar(uint8_t bar,
				       uint16_t duration_time,
				       bool forever_flag)
{
	uint8_t data[5] = {0, };
	data[0] = I2C_CMD_DISP_COLOR_BAR;
	if (bar > 32)
		bar = 32;
	data[1] = bar;
	data[2] = (uint8_t)(duration_time & 0xff);
	data[3] = (uint8_t)((duration_time >> 8) & 0xff);
	data[4] = forever_flag;
	i2cSendBytes(led_matrix_currentDeviceAddress, data, 5);
}

// /*************************************************************
// * Description
// *	Display a wave on RGB LED Matrix.
// * Parameter
// *	color: Set the color of the display, range from 0 to 255. See COLORS for more details.
// *	duration_time: Set the display time(ms) duration. Set it to 0 to not display.
// *	forever_flag: Set it to true to display forever, and the duration_time will not work.
// *		      Or set it to false to display one time.
// * Return
// *    Null.
// *************************************************************/
static void led_matrix_displayColorWave(uint8_t color,
					uint16_t duration_time,
					bool forever_flag)
{
	uint8_t data[5] = {0, };
	data[0] = I2C_CMD_DISP_COLOR_WAVE;
	data[1] = color;
	data[2] = (uint8_t)(duration_time & 0xff);
	data[3] = (uint8_t)((duration_time >> 8) & 0xff);
	data[4] = forever_flag;
	i2cSendBytes(led_matrix_currentDeviceAddress, data, 5);
}

// /*************************************************************
// * Description
// *	Display a clockwise(or anti-clockwise) animation on RGB LED Matrix.
// * Parameter
// *	is_cw: Set it true to display a clockwise animation, while set it false to display a anti-clockwise
// *	is_big: Set it true to display a 8*8 animation, while set it false to display a 4*4 animation
// *	duration_time: Set the display time(ms) duration. Set it to 0 to not display.
// *	forever_flag: Set it to true to display forever, and the duration_time will not work.
// *		      Or set it to false to display one time.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_displayClockwise(bool is_cw,
					bool is_big,
					uint16_t duration_time,
					bool forever_flag)
{
	uint8_t data[6] = {0, };
	data[0] = I2C_CMD_DISP_COLOR_CLOCKWISE;
	data[1] = is_cw;
	data[2] = is_big;
	data[3] = (uint8_t)(duration_time & 0xff);
	data[4] = (uint8_t)((duration_time >> 8) & 0xff);
	data[5] = forever_flag;
	i2cSendBytes(led_matrix_currentDeviceAddress, data, 6);
}

// /*************************************************************
// * Description
// *	Display other built-in animations on RGB LED Matrix.
// * Parameter
// *	index: the index of animations, 
// *			0. big clockwise 
// *			1. small clockwise
// *			2. rainbow cycle
// *			3. fire
// *			4. walking child
// *			5. broken heart
// *    duration_time: Set the display time(ms) duration. Set it to 0 to not display.
// *	forever_flag: Set it to true to display forever, and the duration_time will not work.
// *		      Or set it to false to display one time.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_displayColorAnimation(uint8_t index,
					     uint16_t duration_time,
					     bool forever_flag)
{
	uint8_t data[6] = {0, };
	uint8_t from, to;
	data[0] = I2C_CMD_DISP_COLOR_ANIMATION;
	switch(index) {
	case 0:
		from = 0;
		to = 28;
		break;
	case 1:
		from = 29;
		to = 41;
		break;
	case 2:				// rainbow cycle
		from = 255;
		to = 255;
		break;
	case 3: 			// fire 
		from = 254;
		to = 254;
		break;
	case 4: 			// walking
		from = 42;
		to = 43;
		break;
	case 5:				// broken heart
		from = 44;
		to = 52;
		break;
	default:
		break;
	}
	data[1] = from;
	data[2] = to;
	data[3] = (uint8_t)(duration_time & 0xff);
	data[4] = (uint8_t)((duration_time >> 8) & 0xff);
	data[5] = forever_flag;
	i2cSendBytes(led_matrix_currentDeviceAddress, data, 6);
}

// /*************************************************************
// * Description
// *	Display emoji on LED matrix.
// * Parameter
// *	emoji: Set a number from 0 to 29 for different emoji.	
// *		0	smile	10	heart		20	house
// *		1	laugh	11	small heart	21	tree
// *		2	sad	12	broken heart	22	flower
// *		3	mad	13	waterdrop	23	umbrella
// *		4	angry	14	flame		24	rain
// *		5	cry	15	creeper		25	monster
// *		6	greedy	16	mad creeper	26	crab
// *		7	cool	17	sword		27	duck
// *		8	shy	18	wooden sword	28	rabbit
// *		9	awkward	19	crystal sword	29	cat
// *		30	up	31	down		32 left
// *		33	right	34	smile face 3
// *	duration_time: Set the display time(ms) duration. Set it to 0 to not display.
// *	forever_flag: Set it to true to display forever, and the duration_time will not work. 
// *		      Or set it to false to display one time.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_displayEmoji(uint8_t emoji,
			     uint16_t duration_time,
			     bool forever_flag)
{
	uint8_t data[5] = {0, };

	data[0] = I2C_CMD_DISP_EMOJI;
	data[1] = emoji;
	data[2] = (uint8_t)(duration_time & 0xff);
	data[3] = (uint8_t)((duration_time >> 8) & 0xff);
	data[4] = forever_flag;
	i2cSendBytes(led_matrix_currentDeviceAddress, data, 5);
}

// /*************************************************************
// * Description
// *	Display a number(-32768 ~ 32767) on LED matrix.
// * Parameter
// *	number: Set the number you want to display on LED matrix. Number(except 0-9)
// *		will scroll horizontally, the shorter you set the duration time, 
// *		the faster it scrolls. The number range from -32768 to +32767, if 
// *		you want to display larger number, please use displayString().
// *	duration_time: Set the display time(ms) duration. Set it to 0 to not display.
// *	forever_flag: Set it to true to display forever, or set it to false to display one time.
// *	color: Set the color of the display, range from 0 to 255. See COLORS for more details.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_displayNumber(int16_t number,
				     uint16_t duration_time,
				     bool forever_flag,
				     uint8_t color)
{
	uint8_t data[7] = {0, };

	data[0] = I2C_CMD_DISP_NUM;
	data[1] = (uint8_t)((uint16_t)number & 0xff);
	data[2] = (uint8_t)(((uint16_t)number >> 8) & 0xff);
	data[3] = (uint8_t)(duration_time & 0xff);
	data[4] = (uint8_t)((duration_time >> 8) & 0xff);
	data[5] = forever_flag;
	data[6] = color;
	i2cSendBytes(led_matrix_currentDeviceAddress, data, 7);
}

// /*************************************************************
// * Description
// *	Display a string on LED matrix.
// * Parameter
// *	str: The string pointer, the maximum length is 28 bytes. String will 
// *	     scroll horizontally when its length is more than 1. The shorter 
// *	     you set the duration time, the faster it scrolls.
// *	duration_time: Set the display time(ms) duration. Set it to 0 to not display.
// *	forever_flag: Set it to true to display forever, or set it to false to display one time.
// *	color: Set the color of the display, range from 0 to 255. See COLORS for more details.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_displayString(char *str,
				     uint16_t duration_time,
				     bool forever_flag,
				     uint8_t color)
{
	uint8_t data[36] = {0, };
	uint8_t len = strlen(str);

	if (len >= 28)
		len = 28;
	for (uint8_t i = 0; i < len; i++)
		data[i + 6] = str[i];
	data[0] = I2C_CMD_DISP_STR;
	data[1] = forever_flag;
	data[2] = (uint8_t)(duration_time & 0xff);
	data[3] = (uint8_t)((duration_time >> 8) & 0xff);
	data[4] = len;
	data[5] = color;
	if (len > 25) {
		i2cSendBytes(led_matrix_currentDeviceAddress, data, 31);
		usleep(1000); // 1ms
		i2cSendContinueBytes(led_matrix_currentDeviceAddress, data+31, (len - 25));
	} else {
		i2cSendBytes(led_matrix_currentDeviceAddress, data, (len + 6));
	}
}

// /*************************************************************
// * Description
// *	Display user-defined frames on LED matrix.
// * Parameter
// *	buffer: The data pointer. 1 frame needs 64bytes data. Frames will switch
// *		automatically when the frames_number is larger than 1. The shorter 
// *		you set the duration_time, the faster it switches.
// *	duration_time: Set the display time(ms) duration. Set it to 0 to not display.
// *	forever_flag: Set it to true to display forever, or set it to false to display one time.
// *	frames_number: the number of frames in your buffer. Range from 1 to 5.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_displayFrames(uint8_t *buffer,
				     uint16_t duration_time,
				     bool forever_flag,
				     uint8_t frames_number)
{
	uint8_t data[72] = {0, };
	// max 5 frames in storage
	if (frames_number > 5)
		frames_number = 5;
	else if (frames_number == 0)
		return;
	data[0] = I2C_CMD_DISP_CUSTOM;
	data[1] = 0x0;
	data[2] = 0x0;
	data[3] = 0x0;
	data[4] = frames_number;
	for (int i = frames_number-1; i >= 0; i--) {
		data[5] = i;
		for (int j = 0; j < 64; j++)
			data[8+j] = buffer[j+i*64];
		if (i == 0) {
			// display when everything is finished.
			data[1] = (uint8_t)(duration_time & 0xff);
			data[2] = (uint8_t)((duration_time >> 8) & 0xff);
			data[3] = forever_flag;
		}
		i2cSendBytes(led_matrix_currentDeviceAddress, data, 24);
		usleep(1000);
		i2cSendContinueBytes(led_matrix_currentDeviceAddress, data+24, 24);
		usleep(1000);
		i2cSendContinueBytes(led_matrix_currentDeviceAddress, data+48, 24);
	}
}
static void led_matrix_displayFrames64(uint64_t *buffer,
				       uint16_t duration_time,
				       bool forever_flag,
				       uint8_t frames_number)
{
	uint8_t data[72] = {0, };
	// max 5 frames in storage

	if (frames_number > 5)
		frames_number = 5;
	else if (frames_number == 0)
		return;

	data[0] = I2C_CMD_DISP_CUSTOM;
	data[1] = 0x0;
	data[2] = 0x0;
	data[3] = 0x0;
	data[4] = frames_number;

	for (int i = frames_number - 1; i >= 0; i--) {
		data[5] = i;
		// different from uint8_t buffer
		for (int j = 0; j < 8; j++) {
			for (int k = 7; k >= 0; k--) {
				data[8+j*8+(7-k)] = ((uint8_t *)buffer)[j*8+k+i*64];
			}
		}
		if (i == 0) {
			// display when everything is finished.
			data[1] = (uint8_t)(duration_time & 0xff);
			data[2] = (uint8_t)((duration_time >> 8) & 0xff);
			data[3] = forever_flag;
		}
		i2cSendBytes(led_matrix_currentDeviceAddress, data, 24);
		usleep(1000);
		i2cSendContinueBytes(led_matrix_currentDeviceAddress, data+24, 24);
		usleep(1000);
		i2cSendContinueBytes(led_matrix_currentDeviceAddress, data+48, 24);
	}
}

// /*************************************************************
// * Description
// *	Display color block on LED matrix with a given uint32_t rgb color.
// * Parameter
// *	rgb: uint32_t rgb color, such as 0xff0000(red), 0x0000ff(blue)
// *	duration_time: Set the display time(ms) duration. Set it to 0 to not display.
// *	forever_flag: Set it to true to display forever, or set it to false to display one time.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_displayColorBlock(uint32_t rgb,
					 uint16_t duration_time,
					 bool forever_flag)
{
	uint8_t data[7] = {0, };
	data[0] = I2C_CMD_DISP_COLOR_BLOCK;
	// red green blue
	data[1] = (uint8_t)((rgb >> 16) & 0xff);
	data[2] = (uint8_t)((rgb >> 8) & 0xff);
	data[3] = (uint8_t)(rgb & 0xff);
	data[4] = (uint8_t)(duration_time & 0xff);
	data[5] = (uint8_t)((duration_time >> 8) & 0xff);
	data[6] = forever_flag;
	i2cSendBytes(led_matrix_currentDeviceAddress, data, 7);
}

// /*************************************************************
// * Description
// *	Display nothing on LED Matrix.
// * Parameter
// *	Null.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_stopDisplay(void)
{
	i2cSendByte(led_matrix_currentDeviceAddress, I2C_CMD_DISP_OFF);
}

// /*************************************************************
// * Description
// *	Store the frames(you send to the device) to flash. It takes about 200ms.
// * Parameter
// *	Null.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_storeFrames(void)
{
	i2cSendByte(led_matrix_currentDeviceAddress, I2C_CMD_STORE_FLASH);
	usleep(200000); // 200ms
}

// /*************************************************************
// * Description
// *	Delete all the frames in the flash of device. It takes about 200ms.
// * Parameter
// *	Null.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_deleteFrames(void)
{
	i2cSendByte(led_matrix_currentDeviceAddress, I2C_CMD_DELETE_FLASH);
	usleep(200000); // 200ms
}

// /*************************************************************
// * Description
// *	Display frames which is stored in the flash of device.
// * Parameter
// *	duration_time: Set the display time(ms) duration. Set it to 0 to not display. If there
// *		       are more than 1 frame to display, frames will switch automatically. The  
// *		       shorter you set the duration_time, the faster it switches.
// *	forever_flag: Set it to true to display forever, or set it to false to display one time.
// *	from: the index of frames in your flash. Range from 1 to 5. 
// *	to: the index of frames in your flash. Range from 1 to 5.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_displayFramesFromFlash(uint16_t duration_time,
					      bool forever_flag,
					      uint8_t from,
					      uint8_t to)
{
	uint8_t data[6] = {0, };
	uint8_t temp = 0;

	// 1 <= from <= to <= 5
	if (from < 1)
		from = 1;
	else if (from > 5)
		from =5;
	if (to < 1)
		to = 1;
	else if (to > 5)
		to = 5;

	if (from > to) {
		temp = from;
		from = to;
		to = temp;
	}
	data[0] = I2C_CMD_DISP_FLASH;
	data[1] = (uint8_t)(duration_time & 0xff);
	data[2] = (uint8_t)((duration_time >> 8) & 0xff);
	data[3] = forever_flag;
	data[4] = from-1;
	data[5] = to-1;
	i2cSendBytes(led_matrix_currentDeviceAddress, data, 6);

	usleep(200000); // 200ms
}

// /*************************************************************
// * Description
// *	Enable TX and RX pin test mode.
// * Parameter
// *	Null.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_enableTestMode(void)
{
	i2cSendByte(led_matrix_currentDeviceAddress, I2C_CMD_TEST_TX_RX_ON);
}

// /*************************************************************
// * Description
// *	Enable TX and RX pin test mode.
// * Parameter
// *	Null.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_disableTestMode(void)
{
	i2cSendByte(led_matrix_currentDeviceAddress, I2C_CMD_TEST_TX_RX_OFF);
}

// /*************************************************************
// * Description
// *	Get software vresion.
// * Parameter
// *	Null.
// * Return
// *	Return software version.
// *************************************************************/
static uint32_t led_matrix_getTestVersion(void)
{
	uint8_t data[3] = {0, };

	i2cSendByte(led_matrix_currentDeviceAddress, I2C_CMD_TEST_GET_VER);
	i2cReceiveBytes(led_matrix_currentDeviceAddress, data, 3);
	return (uint32_t)(data[2] + (uint32_t)data[1] * 256 + (uint32_t)data[0] * 256 * 256);
}

// /*************************************************************
// * Description
// *	Reset device.
// * Parameter
// *	Null.
// * Return
// *	Null.
// *************************************************************/
static void led_matrix_resetDevice(void)
{

}

// /*************************************************************
// * Description
// *	Get device id.
// * Parameter
// *	Null.
// * Return
// *	Null.

// *************************************************************/
static void led_matrix_getdeviceId(void)
{
	i2cSendByte(led_matrix_currentDeviceAddress, I2C_CMD_GET_DEVICE_UID);
	i2cReceiveBytes(led_matrix_currentDeviceAddress, (uint8_t *)led_matrix_deviceId, 12);
}

#define I2C_HUB_PORT_RASPI	0x08
#define I2C_HUB_PORT_IOT	0x01
#define I2C_HUB_PORT_GPIO	0x04
#define I2C_HUB_PORT_USB_HUB	0x02
#define I2C_HUB_PORT_ALL	0x0F

/**
 * Function: Configure I2C hub to enable I2C bus that connected to led matrix
 * Params: 	- hub_address: I2C address of hub
 * 		- port: Bus ports
 **/
int i2c_hub_select_port(uint8_t hub_address, uint8_t port)
{
	int result = 0;
	int i2c_fd = open(led_matrix_i2c_bus, O_RDWR);
	if (i2c_fd < 0) {
		LE_ERROR("i2cSendByte: failed to open %s", led_matrix_i2c_bus);
	}
	if (ioctl(i2c_fd, I2C_SLAVE_FORCE, hub_address) < 0) {
		LE_ERROR("Could not set address to 0x%02x: %s\n",
		       hub_address,
		       strerror(errno));
		return -1;
	}
	const int writeResult = i2c_smbus_write_byte(i2c_fd, port);
	if (writeResult < 0) {
		LE_ERROR("smbus write failed with error %d\n", writeResult);
		result = -1;
	} else {
		result = 0;
	}
	close(i2c_fd);

	return result;
}

struct led_matrix *led_matrix_create(const char *i2c_bus,
				     uint8_t base, 
				     uint8_t screenNumber)
{
	struct led_matrix *res = malloc(sizeof(struct led_matrix));
	if (res == NULL) {
		LE_ERROR("led_matrix_create: failed to create led matrix");
		return NULL;
	}
	res->getDeviceVID		= &led_matrix_getDeviceVID;
	res->getDevicePID		= &led_matrix_getDevicePID;
	res->changeDeviceBaseAddress	= &led_matrix_changeDevicebaseAddress;
	res->defaultDeviceAddress	= &led_matrix_defaultDeviceAddress;
	res->turnOnLedFlash		= &led_matrix_turnOnLedFlash;
	res->turnOffLedFlash		= &led_matrix_turnOffLedFlash;
	res->enableAutoSleep		= &led_matrix_enableAutoSleep;
	res->wakeDevice			= &led_matrix_wakeDevice;
	res->disableAutoSleep		= &led_matrix_disableAutoSleep;
	res->setDisplayOrientation	= &led_matrix_setDisplayOrientation;
	res->setDisplayOffset		= &led_matrix_setDisplayOffset;
	res->displayBar			= &led_matrix_displayBar;
	res->displayColorBar		= &led_matrix_displayColorBar;
	res->displayColorWave		= &led_matrix_displayColorWave;
	res->displayClockwise		= &led_matrix_displayClockwise;
	res->displayColorAnimation	= &led_matrix_displayColorAnimation;
	res->displayEmoji		= &led_matrix_displayEmoji;
	res->displayNumber		= &led_matrix_displayNumber;
	res->displayString		= &led_matrix_displayString;
	res->displayFrames		= &led_matrix_displayFrames;
	res->displayFrames64		= &led_matrix_displayFrames64;
	res->displayColorBlock		= &led_matrix_displayColorBlock;
	res->stopDisplay		= &led_matrix_stopDisplay;
	res->storeFrames		= &led_matrix_storeFrames;
	res->deleteFrames		= &led_matrix_deleteFrames;
	res->displayFramesFromFlash	= &led_matrix_displayFramesFromFlash;
	res->enableTestMode		= &led_matrix_enableTestMode;
	res->disableTestMode		= &led_matrix_disableTestMode;
	res->getTestVersion		= &led_matrix_getTestVersion;
	res->resetDevice		= &led_matrix_resetDevice;
	res->getDeviceId		= &led_matrix_getdeviceId;

	if (!(screenNumber >= 1 && screenNumber <= 16))
		screenNumber = 1;
	if (!(base >= 0x10 && base <= 0x70))
		base = GROVE_TWO_RGB_LED_MATRIX_DEF_I2C_ADDR;

	led_matrix_offsetAddress = screenNumber - 1;
	led_matrix_baseAddress = base;
	led_matrix_currentDeviceAddress = led_matrix_offsetAddress + led_matrix_baseAddress;

	memcpy(led_matrix_i2c_bus, i2c_bus, strlen(i2c_bus));
	led_matrix_i2c_bus[strlen(i2c_bus)] = 0;
	
	i2c_hub_select_port(0x71, I2C_HUB_PORT_ALL);

	return res;
}
int led_matrix_release(struct led_matrix *led_matrix)
{
	free(led_matrix);
	return 0;
}
