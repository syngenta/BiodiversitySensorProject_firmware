/*sdk version 2.5.1*/
#include "headers/cam_headers/arducam_mega.h"
#include "headers/main_aws.h"
#include "headers/main_camera.h"
#include "headers/main_gsm.h"
#include "headers/main_epoch.h"
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/sntp.h>
#include <zephyr/sys/printk.h>
#include <zephyr/zephyr.h>

#include <date_time.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/pm.h>
#include <stdio.h>
#include <time.h>

// Flash Sector Size
#define SPI_FLASH_SECTOR_SIZE 4096
// MQTT Payload Size
#define FRAGMENT_SIZE 131072

#define DEV_ID 0x28
#define GRP_ID 0x01

#define CHUNK_SIZE 128

#define STACKSIZE 8192
#define CAMERA_THREAD_PRIORITY 7

/* Defining the mutex as to lock the flash while reading and writing and then unlocking it. */
K_MUTEX_DEFINE(flash_mutex);

#define LED1_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

extern int writeIdx;
extern int flashLimit;
extern int counter;
int writeIdxOld = 0;
int memVar = 0;

int cam_light_time;
static uint8_t battery_percentage = 0;

const struct device *camera_device;
const struct device *flash_dev;
// const struct device *const battery_dev = DEVICE_DT_GET_ANY(maxim_max17048);

static uint8_t image_id_counter = 0;
extern int hr;
int sleep_time = 3; // must be -5:30 hrs  14 for 7:30 pm INDIA ||  1 is for 6:00pm in USA
int wakeup_time = 15; // must be -5:30 hrs  24  for 5:30 am INDIA ||12 is for 5:00am in US

void camera_start(void)
{
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT | GPIO_ACTIVE_LOW);
	int rc; // to store return values of functions
	int epochArr[10] = {0};

	while (1)
	{
		uint8_t time_arr[10];
		setting_epoch();
		get_epoch(epochArr);
		printk("hr is %d\n:", hr);
		/*this loop is for the sleep at night time*/

		if (hr != 0)
		{

			while ((sleep_time < wakeup_time && hr >= sleep_time && hr < wakeup_time) ||
				   (sleep_time > wakeup_time && (hr >= sleep_time || hr < wakeup_time)))
			{
				printk("\nIt's sleep time : ");
				printk("%d", hr);
				k_sleep(K_SECONDS(30));
				cam_light_time = k_uptime_get();
				setting_epoch();
				get_epoch(epochArr);
			}
		}

		printf("Attempting to take picture...\n\n ");

		gpio_pin_set_dt(&led1, 1);
		K_MSEC(200);
		cam_light_time = k_uptime_get();

		arducam_mega_take_picture(camera_device, CAM_IMAGE_MODE_HD, CAM_IMAGE_PIX_FMT_JPG);

		image_id_counter = ((image_id_counter) % 255) + 1;

		gpio_pin_set_dt(&led1, 0);
		K_MSEC(200);

		// Initializing camera buffer 128bytes
		uint8_t cam_buff[CHUNK_SIZE];

		// Initialization variables for storing total bytes read from camera buffer
		// and number of times loop executed to read 128 bytes
		uint8_t bytes_read;
		uint16_t number_of_times_loop = 0;

		do
		{

			/* Erasing flash with sector size defined above */
			if (writeIdx % SPI_FLASH_SECTOR_SIZE == 0)
			{

				if (writeIdx >= flashLimit)
				{
					writeIdx = 0;
				}

				k_mutex_lock(&flash_mutex, K_FOREVER);
				rc = flash_erase(flash_dev, writeIdx, SPI_FLASH_SECTOR_SIZE);
				k_mutex_unlock(&flash_mutex);

				if (rc != 0)
				{
					printk("Flash erase failed with error code %d\n", rc);
					return;
				}

				// Leaving 1024 bytes at staring of each 128KB for overhead detiles of images later
				if (writeIdx % FRAGMENT_SIZE == 0)
				{

					writeIdx = writeIdx + CHUNK_SIZE;
				}
			}

			/* reading CHUNK_SIZE bytes of data from the camera and storing it in */

			bytes_read = arducam_mega_read_image_buf(camera_device, cam_buff, CHUNK_SIZE);
			const size_t leng = sizeof(cam_buff);
			k_mutex_lock(&flash_mutex, K_FOREVER);
			rc = flash_write(flash_dev, writeIdx, cam_buff, leng);
			k_mutex_unlock(&flash_mutex);
			if (rc != 0)
			{
				printk("Flash write failed with error code %d\n", rc);
				return;
			} 

			// Increasing flash memory 128bytes to store next 128bytes of camera buffer
			writeIdx = writeIdx + CHUNK_SIZE;
			number_of_times_loop++;
			if (number_of_times_loop >= 6400)
				sys_reboot();

		} while (bytes_read > 0);

		// Calculating value of memory address (i1) in flash at which
		// last byte of data of camera buffer is stored
		int next_sector_offset = (writeIdx % FRAGMENT_SIZE);

		// Increasing the index of flash memory to starting of next 16KB
		writeIdx = writeIdx + (FRAGMENT_SIZE - next_sector_offset);

		// Dividing i1 with 	128 as to take just 1 byte of memory in flash
		// it is valid for upto value 32640
		uint16_t next_sector_offset_enc = next_sector_offset / 512;

		// Initializing the id of first chunk to 1
		int FRAG_ID = 1;

		// Check whether the write index is jump to 0 location again
		if (writeIdxOld > writeIdx)
		{

			memVar = flashLimit;
		}
		else
		{

			memVar = 0;
		}

		// Calculating total fragment of 16KB made for stoing an image
		int TOTAL_FRAG = (memVar + writeIdx - writeIdxOld) / FRAGMENT_SIZE;

		// Initializing a variable to get the index of overhead which is at stating of each 16KB
		int TOTAL_FRAG_LOOP = TOTAL_FRAG;
		printk("TOTAL FRAG %d \n", TOTAL_FRAG);

		// Reassiging the current write index value to another variable to calculate
		// the difference of memory consumed for next time
		writeIdxOld = writeIdx;

		// Loop to get index of overhead loccation
		while (TOTAL_FRAG_LOOP > 0)
		{

			int OV_loc = writeIdx - (TOTAL_FRAG_LOOP * FRAGMENT_SIZE);

			// Check when some part of image is saved at end of flash
			// and rest of part is saved at top
			// in that case the overhead location will be negative
			if (OV_loc < 0)
			{

				OV_loc = flashLimit + OV_loc;
			}

			uint8_t arr_oh[32] = {0}; // array to store the 16 bytes of overhead
			arr_oh[0] = next_sector_offset_enc;
			arr_oh[1] = DEV_ID;
			arr_oh[2] = FRAG_ID;
			arr_oh[3] = TOTAL_FRAG;

			// appending the timestamp as 10 bytes into the overhead array to be written
			// in the memory

			for (int i = 0; i < 10; i++)
				arr_oh[i + 4] = epochArr[i];

			// set the image id counter
			arr_oh[14] = image_id_counter;
			arr_oh[15] = GRP_ID;
			const size_t leng = sizeof(arr_oh);

			// offset writeIdx_oh must be a multiple of 4 since the minimum write
			k_mutex_lock(&flash_mutex, K_FOREVER);
			rc = flash_write(flash_dev, OV_loc, arr_oh, leng);
			k_mutex_unlock(&flash_mutex);
			if (rc != 0)
			{
				printk("Flash write for sector offset failed with error code %d\n", rc);
				return;
			}
			TOTAL_FRAG_LOOP = TOTAL_FRAG_LOOP - 1;
			FRAG_ID = FRAG_ID + 1; // increasing image ID
		}
		printf("Going to sleep for 20 sec.\n");
		k_sleep(K_SECONDS(20));
	}
}

void init_camera(void)
{
	// Initializing flash memory
	flash_dev = DEVICE_DT_GET(DT_ALIAS(spi_flash0));

	if (!device_is_ready(flash_dev))
	{
		printk("ERROR: %s: device not ready.\n", flash_dev->name);
		return;
	}

	// Initializing camera device
	camera_device = device_get_binding(DT_NODE_FULL_NAME(DT_NODELABEL(arducam_mega)));
	if (camera_device == 0)
	{
		printk("ERROR: Camera device not found!\n");
		return;
	}

	printk("Camera Module Initialsied successfully!!\n");
}

// start the thread immediately
K_THREAD_DEFINE(camera_thread_id, STACKSIZE, camera_start, NULL, NULL, NULL, CAMERA_THREAD_PRIORITY,
				0, 0);