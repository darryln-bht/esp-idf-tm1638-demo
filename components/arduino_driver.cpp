#ifndef _TM1638_H
#define _TM1638_H
#include "Arduino.h"

#ifndef ON
#define ON 1
#endif
#ifndef OFF
#define OFF 0
#endif

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

class TM1638{
  private:
    #define INSTRUCTION_WRITE_DATA 0x40
    #define INSTRUCTION_READ_KEY 0x42
    #define INSTRUCTION_ADDRESS_AUTO 0x40
    #define INSTRUCTION_ADDRESS_FIXED 0x44
    #define INSTRUCTION_NORMAL_MODE 0x40
    #define INSTRUCTION_TEST_MODE 0x48

    #define FIRST_DISPLAY_ADDRESS 0xC0

    #define DISPLAY_TURN_OFF 0x80
    #define DISPLAY_TURN_ON 0x88

    uint8_t _digits[16]={
      0b00111111,0b00000110,0b01011011,0b01001111,
      0b01100110,0b01101101,0b01111101,0b00000111,
      0b01111111,0b01101111,0b01110111,0b01111100,
      0b00111001,0b01011110,0b01111001,0b01110001
    };
        
    uint8_t _clk_pin;
    uint8_t _stb_pin;
    uint8_t _dio_pin;
    uint8_t _buttons;
    uint8_t _pulse;
    bool _isOn;

  public:
    TM1638(uint8_t clk_pin, uint8_t dio_pin, uint8_t stb_pin){
      _clk_pin = clk_pin;
      _stb_pin = stb_pin;
      _dio_pin = dio_pin;
      _pulse = PULSE1_16;
      _isOn = false;
      
      pinMode(stb_pin, OUTPUT);
      pinMode(clk_pin, OUTPUT);
      pinMode(dio_pin, OUTPUT);
      digitalWrite(stb_pin, HIGH);
      digitalWrite(clk_pin, HIGH);
      digitalWrite(dio_pin, HIGH);    
    }

    /**
    * @fn getButton
    * @param s num of button (S1-S8)
    * @return state of button
    */
    bool getButton(button_t s);
    /**
    * @fn getButtons
    * @return state of 8 buttons
    */
    uint8_t getButtons();

    /**
    * @fn writeLed
    * @brief put led ON or OFF
    * @param num num of led(1-8)
    * @param state (true or false)
    */
    void writeLed(uint8_t num, bool state);

    /**
    * @fn writeLeds
    * @brief set all 8 leds ON or OFF
    * @param val 8bits
    */
    void writeLeds(uint8_t val);

    /**
    * @fn displayVal
    * @brief put value on 7 segment display
    * @param digitId num of digit(0-7)
    * @param val value(0->F)
    */
    void displayVal(uint8_t digitId, uint8_t val);
        
    
    /**
    * @fn displayDig
    * @brief set 7 segment display + dot
    * @param digitId num of digit(0-7)
    * @param val value 8 bits
    */
    void displayDig(uint8_t digitId, uint8_t pgfedcba);

    /**
    * @fn displayClear
    * @brief switch off all leds and segment display
    */
    void displayClear();

    /**
    * @fn displayTurnOff
    * @brief turn on lights
    */
    void displayTurnOff();

    /**
    * @fn displayTurnOn
    * @brief turn off lights
    */
    void displayTurnOn();

    /**
    * @fn displaySetBrightness
    * @brief set display brightness
    * @param pulse_t (0-7)
    */
    void displaySetBrightness(pulse_t pulse);

    /**
    * @fn reset
    * @brief switch off all displays-leds
    */
    void reset();

    /**
    * @fn test
    * @brief blink all displays and leds
    */
    void test();

  private:
    void writeData(uint8_t data);
    void writeDataAt(uint8_t displayAddress, uint8_t data);
    void setDisplayMode(uint8_t displayMode);
    void setDataInstruction(uint8_t dataInstruction);
};
#endif

#include "Arduino.h"
#include "TM1638.h"

bool TM1638::getButton(button_t s){
  _buttons = getButtons();
  return bitRead(_buttons, s);
}

// buttons K3/KS1-8
uint8_t TM1638::getButtons(){
  digitalWrite(_stb_pin, LOW);
  writeData(INSTRUCTION_READ_KEY);
  //Twait 1µs
  pinMode(_dio_pin, INPUT);
  digitalWrite(_clk_pin, LOW);
  uint8_t data[4];
  for (uint8_t i=0; i<sizeof(data);i++){
    data[i] = shiftIn(_dio_pin, _clk_pin, LSBFIRST);
    delayMicroseconds(1);
  }
  pinMode(_dio_pin, OUTPUT);
  digitalWrite(_stb_pin, HIGH);
  _buttons=0;
  for (uint8_t i=0; i<4;i++){
    _buttons |= data[i]<<i;
  }
  return _buttons;
}

void TM1638::reset(){
  setDisplayMode(DISPLAY_TURN_ON | _pulse);
  setDataInstruction(INSTRUCTION_WRITE_DATA| INSTRUCTION_ADDRESS_AUTO);
  digitalWrite(_stb_pin, LOW);
  writeData(FIRST_DISPLAY_ADDRESS);
  for(uint8_t i=0;i<16;i++)
    writeData(0);
  digitalWrite(_stb_pin, HIGH);
}

void TM1638::displayVal(uint8_t digitId, uint8_t val){
  if (digitId>7 | val>15 | val<0) return;
  setDisplayMode(DISPLAY_TURN_ON | _pulse);
  setDataInstruction(INSTRUCTION_WRITE_DATA| INSTRUCTION_ADDRESS_FIXED);
  writeDataAt(FIRST_DISPLAY_ADDRESS+14-(digitId*2), _digits[val]);
}

void TM1638::displayDig(uint8_t digitId, uint8_t pgfedcba){
  if (digitId>7) return;
  setDisplayMode(DISPLAY_TURN_ON | _pulse);
  setDataInstruction(INSTRUCTION_WRITE_DATA| INSTRUCTION_ADDRESS_FIXED);
  writeDataAt(FIRST_DISPLAY_ADDRESS+14-(digitId*2), pgfedcba);
}

void TM1638::displayClear(){
  setDisplayMode(DISPLAY_TURN_ON | _pulse);
  setDataInstruction(INSTRUCTION_WRITE_DATA | INSTRUCTION_ADDRESS_FIXED);
  for (uint8_t i=0;i<15;i+=2){
    writeDataAt(FIRST_DISPLAY_ADDRESS+i,0x00);
  }
}

void TM1638::writeLed(uint8_t num,bool state){
  if (num<1 |num>8) return;
  setDisplayMode(DISPLAY_TURN_ON | _pulse);
  setDataInstruction(INSTRUCTION_WRITE_DATA | INSTRUCTION_ADDRESS_FIXED);
  writeDataAt(FIRST_DISPLAY_ADDRESS + (num*2-1), state);
}

void TM1638::writeLeds(uint8_t val){
  setDisplayMode(DISPLAY_TURN_ON | _pulse);
  setDataInstruction(INSTRUCTION_WRITE_DATA | INSTRUCTION_ADDRESS_FIXED);
  for(uint8_t i=1;i<9;i++){
    writeDataAt(FIRST_DISPLAY_ADDRESS + (i*2-1), val & 0x01);
    val >>= 1; 
  }
}

void TM1638::displayTurnOn(){
  setDisplayMode(DISPLAY_TURN_ON | _pulse);
  _isOn = true;
}

void TM1638::displayTurnOff(){
  setDisplayMode(DISPLAY_TURN_OFF | _pulse);
  _isOn = false;
}

void TM1638::displaySetBrightness(pulse_t newpulse){
  if (newpulse<PULSE1_16 | newpulse>PULSE14_16) return;
  _pulse = newpulse;
  uint8_t data = (_isOn) ? DISPLAY_TURN_ON : DISPLAY_TURN_OFF;
  data |= _pulse;
  setDisplayMode(data);
}

void TM1638::writeData(uint8_t data){
  shiftOut(_dio_pin,_clk_pin,LSBFIRST,data);
} 

void TM1638::writeDataAt(uint8_t displayAddress, uint8_t data){
    digitalWrite(_stb_pin, LOW);
    writeData(displayAddress);
    writeData(data);
    digitalWrite(_stb_pin, HIGH);
    delayMicroseconds(1);
}

void TM1638::setDisplayMode(uint8_t displayMode){
  digitalWrite(_stb_pin, LOW);
  writeData(displayMode);
  digitalWrite(_stb_pin, HIGH);
  delayMicroseconds(1);
}
void TM1638::setDataInstruction(uint8_t dataInstruction){
  digitalWrite(_stb_pin, LOW);
  writeData(dataInstruction);
  digitalWrite(_stb_pin, HIGH);
  delayMicroseconds(1);  
}

void TM1638::test(){
  uint8_t val=0;
  for(uint8_t i=0;i<5;i++){
    setDisplayMode(DISPLAY_TURN_ON | _pulse);
    setDataInstruction(INSTRUCTION_WRITE_DATA| INSTRUCTION_ADDRESS_AUTO);
    digitalWrite(_stb_pin, LOW);
    writeData(FIRST_DISPLAY_ADDRESS);
    for(uint8_t i=0;i<16;i++)
      writeData(val);
    digitalWrite(_stb_pin, HIGH);
    delay(1000);
    val = ~val;
  }

}

