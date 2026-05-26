#define F_CLK 16000000
#include <Arduino.h>
#include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_ST7735.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <SPI.h>
#include <avr/io.h>
#include <avr/delay.h>

#define AVAILABE_INSTRUCTIONS 7
#define NUM_REGISTERS 16
#define MAX_VALUE 255
#define MIN_VALUE -256

#define PASS 0
#define ADD 1
#define SUB 2
#define INSERT_VAL 3
#define INSERT_REG 4
#define JMP_IF_EQ 5
#define DISPLAY 6

#define DEFAULT 0
#define CURR_INSTR 0
#define CURR_ARG0 1
#define CURR_ARG1 2
#define CURR_ARG2 3

// Adafruit_ST7735 tft = Adafruit_ST7735(10, 9, 8);
hd44780_I2Cexp i2c = hd44780_I2Cexp(0x27, 2, 16);

typedef struct {
    byte instruction;
    short arg0, arg1, arg2;
} Instruction;

volatile byte instructionCount = 0, currentMode = 0;

volatile Instruction instructions[128];
short registers[16];
short outRegister;

volatile byte displayModified = 1;
volatile byte runProgram = 0;
volatile byte currentElement = DEFAULT;

// TODO: ARDUINO -> AVR
void i2cPrintText(int row, char *text) {
    i2c.setCursor(0, row);
    i2c.printf(text);
}

void i2cPrintInt(int row, int nr) {
    i2c.setCursor(0, row);
    i2c.printf("%d", nr);
}

ISR(PCINT2_vect) {
    // PD5 - Scroll through options (by increasing)
    if (PIND & (1 << PD5)) {
        if (currentMode == 0) {
            instructionCount = (instructionCount + 1) % 128;
        } else if (currentMode <= instructionCount) {
            if (currentElement == CURR_INSTR) {
                instructions[currentMode - 1].instruction = (instructions[currentMode - 1].instruction + 1) % AVAILABE_INSTRUCTIONS;
                instructions[currentMode - 1].arg0 = 0;
                instructions[currentMode - 1].arg1 = 0;
                instructions[currentMode - 1].arg2 = 0;
            } else if (currentElement == CURR_ARG0) {
                switch (instructions[currentMode - 1].instruction) {
                    case ADD: case SUB: case INSERT_VAL: case INSERT_REG:
                        instructions[currentMode - 1].arg0 = (instructions[currentMode - 1].arg0 + 1) % NUM_REGISTERS;
                        break;
                    case JMP_IF_EQ:
                        instructions[currentMode - 1].arg0 = (instructions[currentMode - 1].arg0 + 1) % instructionCount;
                        break;
                }
            } else if (currentElement == CURR_ARG1) {
                switch (instructions[currentMode - 1].instruction) {
                    case ADD: case SUB: case INSERT_REG: case JMP_IF_EQ:
                        instructions[currentMode - 1].arg1 = (instructions[currentMode - 1].arg1 + 1) % NUM_REGISTERS;
                        break;
                    case INSERT_VAL:
                        instructions[currentMode - 1].arg1 = (instructions[currentMode - 1].arg1 == MAX_VALUE) ? MIN_VALUE :
                                                            instructions[currentMode - 1].arg1 + 1;
                        break;
                }
            } else if (currentElement == CURR_ARG2) {
                switch (instructions[currentMode - 1].instruction) {
                    case ADD: case SUB: case JMP_IF_EQ:
                        instructions[currentMode - 1].arg2 = (instructions[currentMode - 1].arg2 + 1) % NUM_REGISTERS;
                        break;
                }
            }
        }
    } else if (PIND & (1 << PD6)) { // PD6 - Go backwards through elements
        // Can't go before instruction count
        if (currentMode == 0) return;
        
        if (currentElement > 0) {
            --currentElement;
        } else { // Go back through instructions - find the current instruction element interacted with
            --currentMode;
            if (currentMode == 0) {
                currentElement = DEFAULT;
            } else {
                switch (instructions[currentMode - 1].instruction) {
                    case ADD: case SUB: case JMP_IF_EQ:
                        currentElement = CURR_ARG2;
                        break;
                    case INSERT_VAL: case INSERT_REG:
                        currentElement = CURR_ARG1;
                        break;
                    case DISPLAY:
                        currentElement = CURR_ARG0;
                        break;
                    default:
                        currentElement = CURR_INSTR;
                }
            }
        }
    } else if (PIND & (1 << PD7)) { // PD7 - go forward through elements
        // Moving forward inside runnable program - run again
        if (currentMode == instructionCount + 1) {
            runProgram = true;
            return;
        }

        // No elements - simply go to the next mode
        if (currentMode == 0 || instructions[currentMode + 1].instruction == PASS) {
            ++currentMode;
        } else {
            switch (instructions[currentMode + 1].instruction) {
                case ADD: case SUB: case JMP_IF_EQ:
                    if (currentElement < 3) {
                        ++currentElement;
                    } else {
                        ++currentMode;
                        currentElement = 0;
                    }
                    break;
                case INSERT_VAL: case INSERT_REG:
                    if (currentElement < 2) {
                        ++currentElement;
                    } else {
                        ++currentMode;
                        currentElement = 0;
                    }
                    break;
                case DISPLAY:
                    if (currentElement < 1) {
                        ++currentElement;
                    } else {
                        ++currentMode;
                        currentElement = 0;
                    }
                    break;
            }
        }
    }

    displayModified = true;
}

int runInstruction(volatile Instruction *instruction) {
    switch (instruction->instruction) {
        case PASS:
            break;
        case ADD:
            registers[instruction->arg0] = registers[instruction->arg1] + registers[instruction->arg2];
            break;
        case SUB:
            registers[instruction->arg0] = registers[instruction->arg1] - registers[instruction->arg2];
            break;
        case INSERT_VAL:
            registers[instruction->arg0] = instruction->arg1;
            break;
        case INSERT_REG:
            registers[instruction->arg0] = registers[instruction->arg1];
            break;
        case DISPLAY:
            outRegister = registers[instruction->arg0];
            break;
        case JMP_IF_EQ:
            if (registers[instruction->arg1] == registers[instruction->arg2]) {
                return instruction->arg0;
            }
            break;
    }

    return -1;
}

void setup() {
    Serial.begin(57600);

    i2c.begin(16, 2);

    // tft.initR(INITR_144GREENTAB);
    // tft.fillScreen(ST77XX_RED);

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
    if (displayModified == 1) {
        cli();
        if (currentMode == 0) { // Set instruction count
            i2cPrintText(0, "Instruction cnt:");
            i2cPrintInt(1, instructionCount);
        } else {
            i2cPrintInt(0, currentMode - 1);
        }

        displayModified = 0;
        _delay_ms(200);
        sei();
    }

    if (runProgram == 1) { // Run program
        cli();
        for (int i = 0; i < instructionCount; ++i) {
            runInstruction(instructions + i);
            i2cPrintText(0, "OUT:");
            i2cPrintInt(1, outRegister);
            _delay_ms(500);
        }
        
        runProgram = 0;
        sei();
    }
}