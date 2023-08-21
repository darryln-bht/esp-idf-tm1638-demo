#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "TM1638.h"

// logging tags
static const char* TAG_main  = "main ";
static const char* TAG_blink = "blink";
static const char* TAG_disp  = "disp ";
static const char* TAG_leds  = "leds ";

// task handles
static TaskHandle_t blink_task_handle;
static TaskHandle_t disp_task_handle;
static TaskHandle_t leds_task_handle;

// task sleep periods
#define MAIN_TASK_SLEEP 1000
#define DISP_TASK_SLEEP 375
#define BLINK_TASK_SLEEP 500
#define LEDS_TASK_SLEEP 200

// blinky led pin
#define BLINK_GPIO GPIO_NUM_5
#define BLINK_PRINT FALSE
#define BUTTON_GPIO GPIO_NUM_0

// tm1638 pins
#define CONFIG_TM1638_CLK_GPIO     GPIO_NUM_33  
#define CONFIG_TM1638_DIO_GPIO     GPIO_NUM_2
#define CONFIG_TM1638_STB_GPIO     GPIO_NUM_32

void blink_task( void *pvParameters ) {
    uint8_t s_led_state = 0;
    ESP_LOGI(TAG_blink, "blink_task start");

    // configure LED
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    // configure user button
    gpio_config_t cfg = { 0 };
    gpio_reset_pin(BUTTON_GPIO);
    cfg.pin_bit_mask = (1 << (BUTTON_GPIO));
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&cfg);

    while(1) {
        // stop blinking while user button pressed
        if (gpio_get_level(BUTTON_GPIO) == 0) {
            // turn off led
            gpio_set_level(BLINK_GPIO, 0);
            ESP_LOGI(TAG_blink, "user button pressed");
            // wait for button release
            do {
                vTaskDelay(50);
            } while (gpio_get_level(BUTTON_GPIO) != 1);
            ESP_LOGI(TAG_blink, "user button released");
            // restart blink with led on
            s_led_state = 1;
        }
        gpio_set_level(BLINK_GPIO, s_led_state);
        // toggle led state
        s_led_state = !s_led_state;
        // don't delay if user button pressed
        if (gpio_get_level(BUTTON_GPIO) == 0)
            continue;
        vTaskDelay(BLINK_TASK_SLEEP / portTICK_PERIOD_MS);
     }
    vTaskDelete( NULL );
}

void leds_task( void *pvParameters ) {
    static uint8_t buttons0=0;
    static uint8_t buttons1=0;
    ESP_LOGI(TAG_leds, "leds_task start");
    while(1) {
        buttons0 = TM1638_getButtons();
        if (buttons0 != 0xff && buttons0 != 0) {
            // one or more buttons is pressed
            ESP_LOGI(TAG_leds, "buttons pressed %02x", buttons0);
            // accummulate button presses
            buttons1 |= buttons0;
            // light corresponding LEDs
            TM1638_writeLeds(buttons1);
            // test for all buttons
            if (buttons1 == 0xff) {
                ESP_LOGI(TAG_leds, "all buttons have been pressed");
                // hang out for a bit
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                // clear the leds
                buttons0 = buttons1 = 0;
                TM1638_writeLeds(buttons1);
            }
            // wait until all buttons released
            while (TM1638_getButtons())
                vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        vTaskDelay(LEDS_TASK_SLEEP / portTICK_PERIOD_MS);
    }
    vTaskDelete( NULL );
}

void disp_task( void *pvParameters ) {
    static uint8_t val[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};

    ESP_LOGI(TAG_disp, "disp_task start");

    // init the driver
    TM1638_init(
        CONFIG_TM1638_CLK_GPIO, 
        CONFIG_TM1638_DIO_GPIO,
        CONFIG_TM1638_STB_GPIO);
    TM1638_reset();
    // segment test and blink
    TM1638_test();    
    TM1638_displayTurnOn();
    TM1638_displaySetBrightness(PULSE1_16);

    // create leds task
    xTaskCreate(leds_task, "LEDS", 2048, ( void * ) 0, 0, &leds_task_handle );

    while(1) {
        // output 8 digits
        for (uint8_t i=0; i < 8; i++){
            TM1638_displayVal(7-i, val[i]);
            // incr digit values and wrap
            val[i] = val[i]+1;
            val[i] &= 0x0f;
        }
        vTaskDelay(DISP_TASK_SLEEP / portTICK_PERIOD_MS);
    }
    vTaskDelete( NULL );
}

void app_main(void) {
    ESP_LOGI(TAG_main, "app_main start");

    xTaskCreate(blink_task, "BLINK", 2048, ( void * ) 0, 0, &blink_task_handle );
    xTaskCreate(disp_task, "DISP", 2048, ( void * ) 0, 0, &disp_task_handle );

    while(1)
        vTaskDelay(MAIN_TASK_SLEEP / portTICK_PERIOD_MS);

}

