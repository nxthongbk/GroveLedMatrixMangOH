#include "legato.h"
#include "ledmatrixUtils.h"

COMPONENT_INIT
{
	struct led_matrix *lmatrix = led_matrix_create("/dev/i2c-0", 0, 0);
	uint16_t pid = lmatrix->getDevicePID();
	uint16_t vid = lmatrix->getDeviceVID();
	LE_INFO("PID: %x; VID: %x\n", pid, vid);

	// Display number
	for (int i = 0; i < 10; i++) {
		lmatrix->displayNumber(i, 1000, true, red);
		sleep(1);
	}

	// Display emoji
	for(int i = 0; i < 35; i++) {
		lmatrix->displayEmoji(i, 5000, true);
		sleep(5);
	}

	led_matrix_release(lmatrix);
}