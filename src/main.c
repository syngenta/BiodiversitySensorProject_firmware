/*sdk version 2.5.1*/
#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include "headers/main_aws.h"
#include "headers/main_camera.h"
#include "headers/main_gsm.h"


// Initialization of led1 for GSM
#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);


int readIdx = 0x20000;
int writeIdx = 0;
int counter = 0;
int hr = 0;
int currentTime;
int flashLimit = 0x800000;
extern int camLightTime;

static void my_timer_cb(struct k_timer *mytime)
{

	currentTime = k_uptime_get();
	if (currentTime - camLightTime > 60000)
	{
		printk("time difference over 60 sec = %ld\n", currentTime - camLightTime);
		sys_reboot();
	}
	else
	{
		printk("time difference = %ld\n", currentTime - camLightTime);
	}
}

/*GSM requires a 300 msec of pull down to activate itself */
void gsm_enable()
{
	gpio_pin_configure_dt(&led, GPIO_OUTPUT | GPIO_ACTIVE_LOW);
	gpio_pin_set_dt(&led1, 0);
	K_MSEC(300);
	gpio_pin_set_dt(&led1, 1);
	K_MSEC(100);
}
struct k_timer my_timer;
void main(void)
{
	printk("Starting Board! %s\n", CONFIG_BOARD);
	k_timer_init(&my_timer, my_timer_cb, NULL);
	k_timer_start(&my_timer, K_SECONDS(30), K_SECONDS(30));
	init_camera();
	gsm_enable();
	init_gsm(&GSM_CONNECTED, &GSM_DISCONNECTED);
	init_aws();
	AWS_loop();
}