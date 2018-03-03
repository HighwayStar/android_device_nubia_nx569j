/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2014 The Linux Foundation. All rights reserved.
 * Copyright (C) 2016 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cutils/log.h>
#include <cutils/properties.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>

#include <sys/ioctl.h>
#include <sys/types.h>

#include <hardware/lights.h>

#define LOG_TAG "lightHAL"

/******************************************************************************/

#define HOME_MASK	16
#define LEFT_MASK	8
#define RIGHT_MASK	32

#define AW_POWER_OFF	0
#define AW_CONST_ON 	1
#define AW_FADE_AUTO	3

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static struct light_state_t g_attention;
static struct light_state_t g_notification;
static struct light_state_t g_battery;
static struct light_state_t g_buttons;

#define BREATH_SOURCE_NOTIFICATION  0x01
#define BREATH_SOURCE_BATTERY       0x02
#define BREATH_SOURCE_BUTTONS       0x04
#define BREATH_SOURCE_ATTENTION     0x08
#define BREATH_SOURCE_NONE          0xFF

static int active_states = 0;

static int last_state = BREATH_SOURCE_NONE;


char const*const LCD_FILE
        = "/sys/class/leds/lcd-backlight/brightness";


char const*const BREATH_RED_LED
        = "/sys/class/leds/nubia_led/blink_mode";

char const*const BREATH_RED_OUTN
        = "/sys/class/leds/nubia_led/outn";

char const*const BREATH_RED_GRADE
        = "/sys/class/leds/nubia_led/grade_parameter";

char const *const BREATH_RED_FADE
        = "/sys/class/leds/nubia_led/fade_parameter";

char const*const BATTERY_CAPACITY
        = "/sys/class/power_supply/battery/capacity";

char const*const BATTERY_CHARGING_STATUS
        = "/sys/class/power_supply/battery/status";

int btn_state = 0;
int is_charging = 0;

/**
 * Device methods
 */

static void init_globals(void)
{
    // Init the mutex
    pthread_mutex_init(&g_lock, NULL);
}

static int write_int(char const* path, int value)
{
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[20];
        int bytes = sprintf(buffer, "%d\n", value);
        int amt = write(fd, buffer, bytes);
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
        if (already_warned == 0) {
            ALOGE("write_int failed to open %s\n", path);
            already_warned = 1;
        }
        return -errno;
    }
}

static int write_str(char const* path, char* value)
{
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[PAGE_SIZE];
        int bytes = sprintf(buffer, "%s\n", value);
        int amt = write(fd, buffer, bytes);
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
        if (already_warned == 0) {
            ALOGE("write_str failed to open %s\n", path);
            already_warned = 1;
        }
        return -errno;
    }
}

static int rgb_to_brightness(struct light_state_t const* state)
{
    int color = state->color & 0x00ffffff;
    return (
      ((color >> 16) & 0xff)
    + ((color >> 8 ) & 0xff)
    + ( color        & 0xff)
    ) / 3;
}

static int set_light_backlight(struct light_device_t* dev,
        struct light_state_t const* state)
{
    int err = 0;
    int brightness = rgb_to_brightness(state);
    pthread_mutex_lock(&g_lock);
    err = write_int(LCD_FILE, brightness);
    pthread_mutex_unlock(&g_lock);
    return err;
}

static int set_breath_light_locked(int event,
        struct light_state_t const* state)
{
    int onMS, offMS;
    char buffer[25];
    int brightness;

    if (state == NULL) {
	ALOGV("disabling buttons backlight\n");
        write_int(BREATH_RED_OUTN, AW_POWER_OFF);
        write_int(BREATH_RED_LED, AW_POWER_OFF);
        return 0;
    }

    brightness = rgb_to_brightness(state);
    ALOGD("set_speaker_light_locked mode=%d brightnsS=%d\n", state->flashMode, brightness);

    if(state->flashMode != LIGHT_FLASH_TIMED) {
            if(event == BREATH_SOURCE_BATTERY && brightness != 0){  //battery charging
                char charging_status[15];
                FILE* fp = fopen(BATTERY_CHARGING_STATUS, "rb");
                fgets(charging_status, 14, fp);
                fclose(fp);
                if (strstr(charging_status, "Charging") != NULL) {
                    is_charging = 1;
                } else {
                    return 0;
                }
            } else {
                if(brightness == 28 || brightness == 0)    // battery charged or disconnected
                    is_charging = 0;
		}
		return 0;
    }else {
        onMS = state->flashOnMS;
        offMS = state->flashOffMS;
	    switch(onMS){
		case 5000:
		    onMS = 5;
		    break;
		case 2000:
		    onMS = 4;
		    break;
		case 1000:
		    onMS = 3;
		    break;
		case 500:
		    onMS = 2;
		    break;
		case 250:
		    onMS = 1;
		    break;
		default:
		    onMS = 1;
	    }

	    switch(offMS){
		case 5000:
		    offMS = 5;
		    break;
		case 2000:
		    offMS = 4;
		    break;
		case 1000:
		    offMS = 3;
		    break;
		case 500:
		    offMS = 2;
		    break;
		case 250:
		    offMS = 1;
		    break;
		case 1:
		    offMS = 0;
		    break;
		default:
		    offMS = 0;
	    }
	}
    snprintf(buffer, sizeof(buffer), "%d %d %d\n", offMS, onMS, onMS);
    ALOGD("offMS=%d onMS=%d onMS=%d\n", offMS, onMS, onMS);
    write_int(BREATH_RED_OUTN, HOME_MASK);
    write_str(BREATH_RED_FADE, buffer);
    write_str(BREATH_RED_GRADE, "0 255");
    write_int(BREATH_RED_LED, AW_FADE_AUTO);

    return 0;
}

static int set_light_buttons(struct light_device_t* dev,
        struct light_state_t const* state)
{

    int lcd_on, err = 0;
    int brightness = rgb_to_brightness(state);
    char buffer[25];

    if(brightness > 200)
	brightness = 200;

    pthread_mutex_lock(&g_lock);

    if(brightness == 0 && is_charging == 1){    // buttons on & charging
        write_int(BREATH_RED_OUTN, AW_POWER_OFF);
        write_int(BREATH_RED_LED, AW_POWER_OFF);

        write_int(BREATH_RED_OUTN, HOME_MASK);
        write_str(BREATH_RED_FADE, "1 0 0");
        write_str(BREATH_RED_GRADE, "10 255");
        write_int(BREATH_RED_LED, AW_CONST_ON);
        btn_state = 0;
    } else if(brightness != 0 && !btn_state) {                // turn buttons on
        ALOGE(" Button led on");
        write_str(BREATH_RED_FADE, "1 0 0");
        write_str(BREATH_RED_GRADE, "10 255");
        char prop[PROPERTY_VALUE_MAX];
        int rc;
        rc = property_get("perist.sy.disablebtn", prop , "1");

        //mid
        write_int(BREATH_RED_OUTN, HOME_MASK);
        write_int(BREATH_RED_LED, AW_CONST_ON);

        if(!rc || (strncmp(prop, "1", PROP_VALUE_MAX) >=0)){
            //right
            write_int(BREATH_RED_OUTN, RIGHT_MASK);
            write_int(BREATH_RED_LED, AW_CONST_ON);

            //left
            write_int(BREATH_RED_OUTN, LEFT_MASK);
            write_int(BREATH_RED_LED, AW_CONST_ON);
        }
        btn_state = 1;
    } else if(brightness == 0 && btn_state){
        ALOGE(" disable led");
        write_int(BREATH_RED_OUTN, 0);
        write_str(BREATH_RED_FADE, "1 0 0");
        write_str(BREATH_RED_GRADE, "10 255");
        write_int(BREATH_RED_LED, AW_POWER_OFF);
        btn_state = 0;
    }

    pthread_mutex_unlock(&g_lock);

    return 0;
}

static int set_light_battery(struct light_device_t* dev,
        struct light_state_t const* state)
{
    pthread_mutex_lock(&g_lock);
    g_battery = *state;
    set_breath_light_locked(BREATH_SOURCE_BATTERY, &g_battery);
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int set_light_notifications(struct light_device_t* dev,
        struct light_state_t const* state)
{
    pthread_mutex_lock(&g_lock);
    g_notification = *state;
    set_breath_light_locked(BREATH_SOURCE_NOTIFICATION, &g_notification);
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int set_light_attention(struct light_device_t* dev,
        struct light_state_t const* state)
{
    pthread_mutex_lock(&g_lock);
    g_attention = *state;
    set_breath_light_locked(BREATH_SOURCE_ATTENTION, &g_attention);
    pthread_mutex_unlock(&g_lock);
    return 0;
}

/** Close the lights device */
static int close_lights(struct light_device_t *dev)
{
    if (dev) {
        free(dev);
    }
    return 0;
}


/**
 * Module methods
 */

/** Open a new instance of a lights device using name */
static int open_lights(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    int (*set_light)(struct light_device_t* dev,
            struct light_state_t const* state);

    if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
        set_light = set_light_backlight;
    else if (0 == strcmp(LIGHT_ID_BUTTONS, name))
        set_light = set_light_buttons;
    else if (0 == strcmp(LIGHT_ID_BATTERY, name))
        set_light = set_light_battery;
    else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name))
        set_light = set_light_notifications;
    else if (0 == strcmp(LIGHT_ID_ATTENTION, name))
        set_light = set_light_attention;
    else
        return -EINVAL;

    pthread_once(&g_init, init_globals);

    struct light_device_t *dev = malloc(sizeof(struct light_device_t));
    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = (int (*)(struct hw_device_t*))close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t*)dev;
    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open =  open_lights,
};

/*
 * The lights Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "Lights Module for Nubia Z11mini&miniS",
    .author = "SY",
    .methods = &lights_module_methods,
};
