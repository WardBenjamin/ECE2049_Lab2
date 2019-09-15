/************** ECE2049 DEMO CODE ******************/
/**************  13 March 2019   ******************/
/***************************************************/

#include <msp430.h>
#include <stdlib.h>
#include <stdio.h>
#include "notes.h"

/* Peripherals.c and .h are where the functions that implement
 * the LEDs and keypad, etc are. It is often useful to organize
 * your code by putting like functions together in files.
 * You include the header associated with that file(s)
 * into the main file of your project. */
#include "peripherals.h"

typedef struct Alien
{
    unsigned char key;
    int x;
    int y;
    bool visible;
} Alien;

typedef enum State
{
    WELCOME_SCREEN,
    WAIT_FOR_START,
    START_COUNT_DOWN,
    COUNT_DOWN_SCREEN,
    NEXT_LEVEL_SCREEN,
    START_LEVEL,
    PLAYING_GAME,
    LOSS_SCREEN
} State;

// Function Prototypes
void drawWelcome();
void drawLoss();
void drawNextLevel(int);
bool drawCountdown(int, int);
void drawAliens(Alien*, int);

// Write  a  function  to  configure  the  4  lab  board  buttons,  S1  through  S4

void configureButtons() {
    // P7.0, P3.6, P2.2, P7.4
    // Configure P2.2
    P2SEL &= ~(BIT2); // Select pin for DI/O
    P2DIR &= ~(BIT2); // Set pin as input
    P2REN |= (BIT2); // Enable pull-up resistor

    // Configure P3.6
    P3SEL &= ~(BIT6); // Select pin for DI/O
    P3DIR &= ~(BIT6); // Set pin as input
    P3REN |= (BIT6); // Enable pull-up resistor

    // Configure P7.0 and P7.4
    P7SEL &= ~(BIT4 | BIT0); // Select pins for DI/O
    P7DIR &= ~(BIT4 | BIT0); // Set pins as input
    P7REN |= (BIT4 | BIT0); // Enable pull-up resistors
}


// Write  a  function  that  returns  the  state  of  the  lab  board  buttons  with  1=pressed  and  0=not pressed.

int getButtonState() {
    char ret = 0x00;
    // P2.2
    if(P2IN & BIT2) ret |= BIT0; // Button 1
    // P3.6
    if(P3IN & BIT6) ret |= BIT1; // Button 2
    // P7.0
    if(P7IN & BIT0) ret |= BIT2; // Button 3
    // P7.4
    if(P7IN & BIT4) ret |= BIT3; // Button 4
    return ret;
}

// Write a complete C function to configure and light 2 user LEDs on the MSP430F5529 Launchpad board based on the char argument
// passed. If BIT0 of the argument = 1, LED1 is lit and if BIT0=0 then LED1 is off. Similarly, if BIT1 of the argument = 1, LED2
// is lit and if BIT1=0 then LED2 is off.
void configureUserLEDs(char outputState) {
    // P4.7, P1.0
    // Configure P1.1
    P1SEL &= ~(BIT0); // Select pins for DI/O
    P1DIR |= BIT0; // Set pin as output
    P1OUT &= ~(BIT0); // Set off by default

        // Configure P4.7
    P4SEL &= ~(BIT7); // Select pins for DI/O
    P4DIR |= BIT7; // Set pin as output
    P4OUT &= ~(BIT7); // Set off by default

    if(outputState | BIT0) P1OUT |= BIT0;
    if(outputState | BIT1) P4OUT |= BIT7;
}

// What data structure(s) will you use to store pitch, duration and the corresponding LED? What length songs will you eventually
// want to play? Given how you choose to save your notes, etc., how much memory will that require?

// Total of 3 bytes
typedef struct Note
{
    int pitch; // Period in ACLK ticks
    int duration; // Duration in milliseconds
    int delay; // Silence after the note in milliseconds
} Note;


// Declare globals here

// Main
void main(void)
{
    int i;
    int levelBottom = 85; // The lowest the top of the alien character can go before you lose the level
    unsigned char keyPressed = 0, lastKeyPressed = 0;
    unsigned long int mainCounter = 0, auxCounter = 0, auxCounter2 = 0;
    State state = WELCOME_SCREEN;
    Note notes[] = {
      {NOTE_D5, 100, 80},
      {NOTE_F5, 100, 80},
      {NOTE_D6, 200, 250},

      {NOTE_D5, 100, 80},
      {NOTE_F5, 100, 80},
      {NOTE_D6, 200, 250},

      {NOTE_E6, 200, 200},
      {NOTE_F6, 100, 100},
      {NOTE_E6, 100, 80},
      {NOTE_F6, 100, 80},
      {NOTE_E6, 100, 80},
      {NOTE_C6, 100, 80},
      {NOTE_A5, 100, 300},

      {NOTE_D5, 200,100},
      {NOTE_F5, 100,100},
      {NOTE_G5, 100,100},
      {NOTE_A5, 100,500},

      {NOTE_A5, 200,100},
      {NOTE_D5, 200,100},
      {NOTE_F5, 100,100},
      {NOTE_G5, 100,100},
      {NOTE_E5, 100, 500},

      {NOTE_D5, 100,80},
      {NOTE_F5, 100,80},
      {NOTE_D6, 200,250},

      {NOTE_D5, 100,80},
      {NOTE_F5, 100,80},
      {NOTE_D6, 200,250},

      {NOTE_E6, 200,200},
      {NOTE_F6, 100,100},
      {NOTE_E6, 100,80},
      {NOTE_F6, 100,80},
      {NOTE_E6, 100,80},
      {NOTE_C6, 100,80},
      {NOTE_A5, 100,300},

      {NOTE_D5, 200,100},
      {NOTE_F5, 100,100},
      {NOTE_G5, 100,100},
      {NOTE_A5, 300, 100},
      {NOTE_A5, 200, 100},
      {NOTE_D5, 2000, 2000}
    };

    int noteCounter = 0, noteCount = 41;
    Note currentNote = notes[0];
    BuzzerOnFreq(currentNote.pitch);
    auxCounter2 = currentNote.duration;
    bool shouldPlay = false;

    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer

    initLeds();
    configDisplay();
    configKeypad();

    while (1)
    {
        keyPressed = getKey();


        mainCounter++;
        if (auxCounter2 > 0)
        {
            auxCounter2--;
            if(!auxCounter2) {
                if(shouldPlay) {
                    noteCounter++;
                    currentNote = notes[noteCounter % noteCount];
                    BuzzerOnFreq(currentNote.pitch);
                    auxCounter2 = currentNote.duration * 7;
                } else {
                    BuzzerOff();
                    auxCounter2 = currentNote.delay * 7;
                }
                shouldPlay = !shouldPlay;
            }
        }
        lastKeyPressed = keyPressed;
    }
}

void drawWelcome()
{
    // *** Intro Screen ***
    Graphics_clearDisplay(&g_sContext);                // Clear the display

    // Write some text to the display
    Graphics_drawStringCentered(&g_sContext, "SPACE", AUTO_STRING_LENGTH, 48,
                                25, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "INVADERS", AUTO_STRING_LENGTH, 48,
                                35, TRANSPARENT_TEXT);

    Graphics_drawStringCentered(&g_sContext, "Press *", AUTO_STRING_LENGTH, 48,
                                70, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "to Start", AUTO_STRING_LENGTH, 48,
                                80, TRANSPARENT_TEXT);

    // Draw a box around everything because it looks nice
    Graphics_Rectangle box = { .xMin = 5, .xMax = 91, .yMin = 5, .yMax = 91 };
    Graphics_drawRectangle(&g_sContext, &box);

    // We are now done writing to the display.  However, if we stopped here, we would not
    // see any changes on the actual LCD.  This is because we need to send our changes
    // to the LCD, which then refreshes the display.
    // Since this is a slow operation, it is best to refresh (or "flush") only after
    // we are done drawing everything we need.
    Graphics_flushBuffer(&g_sContext);
}

void drawLoss()
{
    // *** Intro Screen ***
    Graphics_clearDisplay(&g_sContext);                // Clear the display

    // Write some text to the display
    Graphics_drawStringCentered(&g_sContext, "You Lost.", AUTO_STRING_LENGTH,
                                48, 25, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, ">:(", AUTO_STRING_LENGTH, 48, 35,
    TRANSPARENT_TEXT);

    Graphics_drawStringCentered(&g_sContext, "Press *", AUTO_STRING_LENGTH, 48,
                                70, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "to Try Again", AUTO_STRING_LENGTH,
                                48, 80, TRANSPARENT_TEXT);

    // Draw a box around everything because it looks nice
    Graphics_Rectangle box = { .xMin = 5, .xMax = 91, .yMin = 5, .yMax = 91 };
    Graphics_drawRectangle(&g_sContext, &box);

    Graphics_flushBuffer(&g_sContext);
}

void drawNextLevel(int level)
{

    Graphics_clearDisplay(&g_sContext);

    char buffer[10];
    snprintf(buffer, 9, "Level %d", level);

    // Write some text to the display
    Graphics_drawStringCentered(&g_sContext, buffer, AUTO_STRING_LENGTH, 48, 35,
                                TRANSPARENT_TEXT);

    Graphics_drawStringCentered(&g_sContext, "Press *", AUTO_STRING_LENGTH, 48,
                                70, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "to Begin", AUTO_STRING_LENGTH, 48,
                                80, TRANSPARENT_TEXT);

    // Draw a box around everything because it looks nice
    Graphics_Rectangle box = { .xMin = 5, .xMax = 91, .yMin = 5, .yMax = 91 };
    Graphics_drawRectangle(&g_sContext, &box);

    Graphics_flushBuffer(&g_sContext);
}

bool drawCountdown(int mainCounter, int auxCounter)
{
    if (auxCounter == mainCounter)
    {
        Graphics_clearDisplay(&g_sContext); // Clear the display
        Graphics_drawStringCentered(&g_sContext, "3", AUTO_STRING_LENGTH, 48,
                                    50, TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
    }
    else if (mainCounter == auxCounter + 4000)
    {
        Graphics_clearDisplay(&g_sContext); // Clear the display
        Graphics_drawStringCentered(&g_sContext, "2", AUTO_STRING_LENGTH, 48,
                                    50, TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
    }
    else if (mainCounter == auxCounter + 8000)
    {
        Graphics_clearDisplay(&g_sContext); // Clear the display
        Graphics_drawStringCentered(&g_sContext, "1", AUTO_STRING_LENGTH, 48,
                                    50, TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
    }
    else if (mainCounter == auxCounter + 12000)
    {
        Graphics_clearDisplay(&g_sContext); // Clear the display
        Graphics_flushBuffer(&g_sContext);
        return true;
    }
    return false;
}

void drawAliens(Alien* aliens, int alienCount)
{
    int i;
    for (i = 0; i < alienCount; i++)
    {
        Alien currentAlien = aliens[i];
        if (currentAlien.visible && currentAlien.y >= 0)
        {
            unsigned char* str = &(currentAlien.key);
            Graphics_drawStringCentered(&g_sContext, str, 1, currentAlien.x,
                                        currentAlien.y, TRANSPARENT_TEXT);
        }
    }
    Graphics_flushBuffer(&g_sContext);
}

