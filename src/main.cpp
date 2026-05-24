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

void setup() {
    Serial.begin(57600);

    auto res = i2c.begin(16, 2);
    Serial.printf("%d\n", res);
    res = i2c.clear();

    i2c.printf("1234567890abcdef");
    Serial.printf("%d\n", res);

    i2c.setCursor(0, 1);

    i2c.printf("ghijklmnopqrstuv");


    tft.initR(INITR_144GREENTAB);
    tft.fillScreen(ST77XX_RED);
}

void loop() {
    
}