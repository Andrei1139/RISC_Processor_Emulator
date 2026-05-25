#define F_CLK 16000000
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <SPI.h>

Adafruit_ST7735 tft = Adafruit_ST7735(10, 9, 8);
hd44780_I2Cexp i2c = hd44780_I2Cexp(0x27, 2, 16);

typedef struct {
    byte instruction;
    short arg1, arg2;
} Instruction;

unsigned int instructionCount = 0, currentMode = 0;

Instruction instructions[128];

// TODO: de inlocuit cod Arduino cu instructiuni AVR
void i2c_display(int row, char *text) {
    i2c.setCursor(0, row);
    i2c.printf(text);
}

// PD5 - scroll through options (by increasing)
ISR(PCINT21_vec) {

}

// PD6 - go backward through elements
ISR(PCINT22_vec) {

}

// PD7 - go forward through elements
ISR(PCINT23_vec) {

}

void setup() {
    Serial.begin(57600);

    i2c.begin(16, 2);

    tft.initR(INITR_144GREENTAB);
    tft.fillScreen(ST77XX_RED);
}

void loop() {
    if (currentMode == 0) { // Set instruction count

    }
}