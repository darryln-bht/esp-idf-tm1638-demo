#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "TM1638.h"
#include "TM1638_platform.h"

static const char* TAG = "TM1638";

#define BLINK_GPIO GPIO_NUM_5
#define BLINK_PERIOD 500
#define BLINK_PRINT FALSE
#define BUTTON_GPIO GPIO_NUM_0

// tm1638 pins
#define CONFIG_TM1638_CLK_GPIO     GPIO_NUM_33  
#define CONFIG_TM1638_DIO_GPIO     GPIO_NUM_2
#define CONFIG_TM1638_STB_GPIO     GPIO_NUM_32

static TM1638_Handler_t tm1638;

static uint8_t s_led_state = 0;
static uint16_t loopcount = 0;

static void configure_led(void) {
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

static void configure_button(void) {
    gpio_config_t cfg = { 0 };
    gpio_reset_pin(BUTTON_GPIO);
    cfg.pin_bit_mask = (1 << (BUTTON_GPIO));
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&cfg);
}

static void configure_tm1638_module(void) {
    TM1638_Platform_Init(&tm1638, 
        CONFIG_TM1638_STB_GPIO, 
        CONFIG_TM1638_CLK_GPIO, 
        CONFIG_TM1638_DIO_GPIO);
    TM1638_Init(&tm1638, TM1638DisplayTypeComCathode);
    TM1638_ConfigDisplay(&tm1638, 7, TM1638DisplayStateON);
}

static void update_led(void) {
    gpio_set_level(BLINK_GPIO, s_led_state);
    // toggle
    s_led_state = !s_led_state;
}

static void update_tm1638(void) {
    // Display a hex digit
    uint8_t digit = loopcount & 0xf;
    TM1638_SetSingleDigit_HEX(&tm1638, digit, digit & 7);
}

void app_main(void) {

    configure_led();
    configure_button();
    configure_tm1638_module();

    while (1) {
        // block when button pressed
        if (gpio_get_level(BUTTON_GPIO) == 0) {
            do
                vTaskDelay(100);
            while (gpio_get_level(BUTTON_GPIO) != 1);
        }
        update_led();
        update_tm1638();
        vTaskDelay(BLINK_PERIOD / portTICK_PERIOD_MS);
        loopcount++;
    }
}

