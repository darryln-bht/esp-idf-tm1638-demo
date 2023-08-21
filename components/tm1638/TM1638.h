/*!
 * @file TM1638.h
 * @brief Arduino library for interface with TM1638 chip. 
 * @n read buttons, switch leds, display on 7segment.
 * @author [Damien](web@varrel.fr)
 * @version  V1.0
 * @date  2022-11-01
 * @url https://github.com/dvarrel/TM1638.git
 * @module https://fr.aliexpress.com/item/32832772646.html
 */

#ifndef _TM1638_H
#define _TM1638_H

#include <inttypes.h>

#ifndef ON
#define ON 1
#endif
#ifndef OFF
#define OFF 0
#endif

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)

typedef enum{
  PULSE1_16,
  PULSE2_16,
  PULSE4_16,
  PULSE10_16,
  PULSE11_16,
  PULSE12_16,
  PULSE13_16,
  PULSE14_16
} pulse_t;

typedef enum{
  S1,S2,S3,S4,
  S5,S6,S7,S8
} button_t;

void TM1638_init(uint8_t clk_pin, uint8_t dio_pin, uint8_t stb_pin);
/**
* @fn getButton
* @param s num of button (S1-S8)
* @return state of button
*/
bool TM1638_getButton(button_t s);
/**
* @fn getButtons
* @return state of 8 buttons
*/
uint8_t TM1638_getButtons();

/**
* @fn writeLed
* @brief put led ON or OFF
* @param num num of led(1-8)
* @param state (true or false)
*/
void TM1638_writeLed(uint8_t num, bool state);

/**
* @fn writeLeds
* @brief set all 8 leds ON or OFF
* @param val 8bits
*/
void TM1638_writeLeds(uint8_t val);

/**
* @fn displayVal
* @brief put value on 7 segment display
* @param digitId num of digit(0-7)
* @param val value(0->F)
*/
void TM1638_displayVal(uint8_t digitId, uint8_t val);
    

/**
* @fn displayDig
* @brief set 7 segment display + dot
* @param digitId num of digit(0-7)
* @param val value 8 bits
*/
void TM1638_displayDig(uint8_t digitId, uint8_t pgfedcba);

/**
* @fn displayClear
* @brief switch off all leds and segment display
*/
void TM1638_displayClear();

/**
* @fn displayTurnOff
* @brief turn off lights
*/
void TM1638_displayTurnOff();

/**
* @fn displayTurnOn
* @brief turn on lights
*/
void TM1638_displayTurnOn();

/**
* @fn displaySetBrightness
* @brief set display brightness
* @param pulse_t (0-7)
*/
void TM1638_displaySetBrightness(pulse_t pulse);

/**
* @fn reset
* @brief switch off all displays-leds
*/
void TM1638_reset();

/**
* @fn test
* @brief blink all displays and leds
*/
void TM1638_test();


#endif