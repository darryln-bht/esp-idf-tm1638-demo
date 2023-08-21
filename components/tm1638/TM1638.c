#include <stdbool.h>
#include <inttypes.h>

#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "TM1638.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1
#define LSBFIRST 0
#define MSBFIRST 1

#define INSTRUCTION_WRITE_DATA 0x40
#define INSTRUCTION_READ_KEY 0x42
#define INSTRUCTION_ADDRESS_AUTO 0x40
#define INSTRUCTION_ADDRESS_FIXED 0x44
#define INSTRUCTION_NORMAL_MODE 0x40
#define INSTRUCTION_TEST_MODE 0x48

#define FIRST_DISPLAY_ADDRESS 0xC0

#define DISPLAY_TURN_OFF 0x80
#define DISPLAY_TURN_ON 0x88

static uint8_t _digits[16]={
  0b00111111,0b00000110,0b01011011,0b01001111,
  0b01100110,0b01101101,0b01111101,0b00000111,
  0b01111111,0b01101111,0b01110111,0b01111100,
  0b00111001,0b01011110,0b01111001,0b01110001
};

static SemaphoreHandle_t _mutex;
static uint8_t _clk_pin;
static uint8_t _stb_pin;
static uint8_t _dio_pin;
static uint8_t _buttons;
static uint8_t _pulse;
static bool _isOn;

// declare private functions
static void _setDisplayMode(uint8_t displayMode);
static void _setDataInstruction(uint8_t dataInstruction);
static void _writeData(uint8_t data);
static void _writeDataAt(uint8_t displayAddress, uint8_t data);
static void _setDataInstruction(uint8_t dataInstruction);
static uint8_t _shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);
static void _shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);

#define CLK_HI_US 1
#define CLK_LO_US 2
#define CLK_BYTE_US 5

static void delayUs(uint32_t us) {
    ets_delay_us(us);
}

static uint8_t _shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder) {
    uint8_t value = 0;
    uint8_t i;

    for(i = 0; i < 8; ++i) {
        if(bitOrder == LSBFIRST)
            value |= gpio_get_level(dataPin) << i;
        else
            value |= gpio_get_level(dataPin) << (7 - i);
        gpio_set_level(clockPin, HIGH);
        delayUs(CLK_HI_US);
        gpio_set_level(clockPin, LOW);
        delayUs(CLK_LO_US);
    }
    return value;
}

static void _shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val) {
    uint8_t i;

    for(i = 0; i < 8; i++) {
        if(bitOrder == LSBFIRST)
            gpio_set_level(dataPin, !!(val & (1 << i)));
        else
            gpio_set_level(dataPin, !!(val & (1 << (7 - i))));

        gpio_set_level(clockPin, HIGH);
        delayUs(CLK_HI_US);
        gpio_set_level(clockPin, LOW);
        delayUs(CLK_LO_US);
    }
}


bool TM1638_getButton(button_t s){
  _buttons = TM1638_getButtons();
  return bitRead(_buttons, s);
}

// buttons K3/KS1-8
uint8_t TM1638_getButtons(){
  xSemaphoreTake(_mutex, portMAX_DELAY);
  gpio_set_level(_stb_pin, LOW);
  _writeData(INSTRUCTION_READ_KEY);
  gpio_set_direction(_dio_pin, GPIO_MODE_INPUT);
  gpio_set_level(_clk_pin, LOW);
  uint8_t data[4];
  for (uint8_t i=0; i<sizeof(data);i++){
    data[i] = _shiftIn(_dio_pin, _clk_pin, LSBFIRST);
      delayUs(CLK_BYTE_US);

  }
  gpio_set_direction(_dio_pin, GPIO_MODE_OUTPUT);
  gpio_set_level(_stb_pin, HIGH);
  _buttons=0;
  for (uint8_t i=0; i<4;i++){
    _buttons |= data[i]<<i;
  }
  xSemaphoreGive(_mutex);
  return _buttons;
}

void TM1638_reset(){
  xSemaphoreTake(_mutex, portMAX_DELAY);
  _setDisplayMode(DISPLAY_TURN_ON | _pulse);
  _setDataInstruction(INSTRUCTION_WRITE_DATA| INSTRUCTION_ADDRESS_AUTO);
  gpio_set_level(_stb_pin, LOW);
  _writeData(FIRST_DISPLAY_ADDRESS);
  for(uint8_t i=0;i<16;i++)
    _writeData(0);
  gpio_set_level(_stb_pin, HIGH);
  xSemaphoreGive(_mutex);
}

void TM1638_displayVal(uint8_t digitId, uint8_t val){
  if ((digitId>7) || (val>15)) return;
  xSemaphoreTake(_mutex, portMAX_DELAY);
  _setDisplayMode(DISPLAY_TURN_ON | _pulse);
  _setDataInstruction(INSTRUCTION_WRITE_DATA| INSTRUCTION_ADDRESS_FIXED);
  _writeDataAt(FIRST_DISPLAY_ADDRESS+14-(digitId*2), _digits[val]);
  xSemaphoreGive(_mutex);
}

void TM1638_displayDig(uint8_t digitId, uint8_t pgfedcba){
  if (digitId>7) return;
  xSemaphoreTake(_mutex, portMAX_DELAY);
  _setDisplayMode(DISPLAY_TURN_ON | _pulse);
  _setDataInstruction(INSTRUCTION_WRITE_DATA| INSTRUCTION_ADDRESS_FIXED);
  _writeDataAt(FIRST_DISPLAY_ADDRESS+14-(digitId*2), pgfedcba);
  xSemaphoreGive(_mutex);
}

void TM1638_displayClear(){
  xSemaphoreTake(_mutex, portMAX_DELAY);
  _setDisplayMode(DISPLAY_TURN_ON | _pulse);
  _setDataInstruction(INSTRUCTION_WRITE_DATA | INSTRUCTION_ADDRESS_FIXED);
  for (uint8_t i=0;i<15;i+=2){
    _writeDataAt(FIRST_DISPLAY_ADDRESS+i,0x00);
  }
  xSemaphoreGive(_mutex);
}

void TM1638_writeLed(uint8_t num,bool state){
  if ((num<1) ||(num>8)) return;
  xSemaphoreTake(_mutex, portMAX_DELAY);
  _setDisplayMode(DISPLAY_TURN_ON | _pulse);
  _setDataInstruction(INSTRUCTION_WRITE_DATA | INSTRUCTION_ADDRESS_FIXED);
  _writeDataAt(FIRST_DISPLAY_ADDRESS + (num*2-1), state);
  xSemaphoreGive(_mutex);
}

void TM1638_writeLeds(uint8_t val){
  xSemaphoreTake(_mutex, portMAX_DELAY);
  _setDisplayMode(DISPLAY_TURN_ON | _pulse);
  _setDataInstruction(INSTRUCTION_WRITE_DATA | INSTRUCTION_ADDRESS_FIXED);
  for(uint8_t i=1;i<9;i++){
    _writeDataAt(FIRST_DISPLAY_ADDRESS + (i*2-1), val & 0x01);
    val >>= 1; 
  }
  xSemaphoreGive(_mutex);
}

void TM1638_displayTurnOn(){
  xSemaphoreTake(_mutex, portMAX_DELAY);
  _setDisplayMode(DISPLAY_TURN_ON | _pulse);
  _isOn = true;
  xSemaphoreGive(_mutex);
}

void TM1638_displayTurnOff(){
  xSemaphoreTake(_mutex, portMAX_DELAY);
    _setDisplayMode(DISPLAY_TURN_OFF | _pulse);
  _isOn = false;
  xSemaphoreGive(_mutex);
}

void TM1638_displaySetBrightness(pulse_t newpulse){
  if ((newpulse<PULSE1_16) || (newpulse>PULSE14_16)) return;
  xSemaphoreTake(_mutex, portMAX_DELAY);
  _pulse = newpulse;
  uint8_t data = (_isOn) ? DISPLAY_TURN_ON : DISPLAY_TURN_OFF;
  data |= _pulse;
  _setDisplayMode(data);
  xSemaphoreGive(_mutex);
}

static void _writeData(uint8_t data){
  _shiftOut(_dio_pin,_clk_pin,LSBFIRST,data);
} 

static void _writeDataAt(uint8_t displayAddress, uint8_t data){
    gpio_set_level(_stb_pin, LOW);
    _writeData(displayAddress);
    _writeData(data);
    gpio_set_level(_stb_pin, HIGH);
      delayUs(CLK_BYTE_US);
}

static void _setDisplayMode(uint8_t displayMode){
  gpio_set_level(_stb_pin, LOW);
  _writeData(displayMode);
  gpio_set_level(_stb_pin, HIGH);
    delayUs(CLK_BYTE_US);
}

static void _setDataInstruction(uint8_t dataInstruction){
  gpio_set_level(_stb_pin, LOW);
  _writeData(dataInstruction);
  gpio_set_level(_stb_pin, HIGH);
    delayUs(CLK_BYTE_US);  
}

void TM1638_test(){
  uint8_t val=0;
  xSemaphoreTake(_mutex, portMAX_DELAY);
  for(uint8_t i=0;i<5;i++){
    _setDisplayMode(DISPLAY_TURN_ON | _pulse);
    _setDataInstruction(INSTRUCTION_WRITE_DATA| INSTRUCTION_ADDRESS_AUTO);
    gpio_set_level(_stb_pin, LOW);
    _writeData(FIRST_DISPLAY_ADDRESS);
    for(uint8_t i=0;i<16;i++)
      _writeData(val);
    gpio_set_level(_stb_pin, HIGH);
    vTaskDelay(100);
    val = ~val;
  }
  xSemaphoreGive(_mutex);
}

void TM1638_init(uint8_t clk_pin, uint8_t dio_pin, uint8_t stb_pin) {
  _clk_pin = clk_pin;
  _stb_pin = stb_pin;
  _dio_pin = dio_pin;
  _pulse = PULSE1_16;
  _isOn = false;

  _mutex = xSemaphoreCreateMutex();

  gpio_reset_pin(stb_pin);  
  gpio_set_direction(stb_pin, GPIO_MODE_OUTPUT);
  gpio_reset_pin(clk_pin);  
  gpio_set_direction(clk_pin, GPIO_MODE_OUTPUT);
  gpio_reset_pin(dio_pin);  
  gpio_set_direction(dio_pin, GPIO_MODE_OUTPUT);
  gpio_set_level(stb_pin, HIGH);
  gpio_set_level(clk_pin, HIGH);
  gpio_set_level(dio_pin, HIGH);    
}
