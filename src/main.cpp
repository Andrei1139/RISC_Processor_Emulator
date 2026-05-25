#define F_CLK 16000000
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <SPI.h>
#include <avr/io.h>

#define PASS 0
#define ADD 1
#define SUB 2
#define INSERT 3
#define DISPLAY 4


Adafruit_ST7735 tft = Adafruit_ST7735(10, 9, 8);
hd44780_I2Cexp i2c = hd44780_I2Cexp(0x27, 2, 16);

typedef struct {
    byte instruction;
    short arg0, arg1, arg2;
} Instruction;

unsigned int instructionCount = 0, currentMode = 0;

Instruction instructions[128];
short registers[16];
short outRegister;

// TODO: de inlocuit cod Arduino cu instructiuni AVR
void i2cPrintText(int row, char *text) {
    i2c.setCursor(0, row);
    i2c.printf(text);
}

void i2cPrintInt(int row, int nr) {
    i2c.setCursor(0, row);
    i2c.printf("%d", nr);
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

void runInstruction(Instruction *instruction) {
    switch (instruction->instruction) {
        case PASS:
            break;
        case ADD:
            registers[instruction->arg0] = registers[instruction->arg1] + registers[instruction->arg2];
            break;
        case SUB:
            registers[instruction->arg0] = registers[instruction->arg1] - registers[instruction->arg2];
            break;
        case INSERT:
            registers[instruction->arg0] = instruction->arg1;
            break;
        case DISPLAY:
            outRegister = registers[instruction->arg0];
            break;
    }
}

void setup() {
    Serial.begin(57600);

    i2c.begin(16, 2);

    tft.initR(INITR_144GREENTAB);
    tft.fillScreen(ST77XX_RED);

    // Enable input pins
    DDRD &= ~(1 << PD5);
    DDRD &= ~(1 << PD6);
    DDRD &= ~(1 << PD7);

    // Enable interrupts for the second PCINT family
    PCICR |= (1 << PCIE2);
    
    // Enable individual interrupts
    PCMSK2 |= (1 << PCINT21);
    PCMSK2 |= (1 << PCINT22);
    PCMSK2 |= (1 << PCINT23);

    sei();
}

void loop() {
    if (currentMode == 0) { // Set instruction count
        i2cPrintText(0, "Instruction cnt:");
        i2cPrintInt(1, instructionCount);
    } else if (currentMode == instructionCount + 1) { // Run program
        for (int i = 0; i < instructionCount; ++i) {
            runInstruction(instructions + i);
            i2cPrintText(0, "OUT:");
            i2cPrintInt(1, outRegister);
        }
    }
}