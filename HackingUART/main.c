////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        Lab 2
// File name:       main.c
// Description:     Write a "hamming distance" security sim on the digilent board
//                  
// Resources:       main.c uses Timer2 to measure elapsed time.
//					

#include <stdio.h>                      // for sprintf()
#include <stdlib.h>                     // i think this is for rand()
#include <plib.h>                       // Peripheral Library
#include <stdbool.h>  
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"
#include "myUART.h"


// Digilent board configuration
#pragma config ICESEL       = ICS_PGx1  // ICE/ICD Comm Channel Select
#pragma config DEBUG        = OFF       // Debugger Disabled for Starter Kit
#pragma config FNOSC        = PRIPLL	// Oscillator selection
#pragma config POSCMOD      = XT	    // Primary oscillator mode
#pragma config FPLLIDIV     = DIV_2	    // PLL input divider
#pragma config FPLLMUL      = MUL_20	// PLL multiplier
#pragma config FPLLODIV     = DIV_1	    // PLL output divider
#pragma config FPBDIV       = DIV_8	    // Peripheral bus clock divider
#pragma config FSOSCEN      = OFF	    // Secondary oscillator enable

// Initialize Timer2 so that it rolls over 10 times per second

void Timer2Init() {
    // The period of Timer 2 is (16 * 62500)/(10 MHz) = 1 ms (freq = 10 Hz) // 1 ms
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 6249);
    return;
}

void Timer3Init()
{
    // The period of Timer 3 is 100ms, meaning the F_ROLLOVER = 10Hz
    OpenTimer3(T3_ON | T3_IDLE_CON | T3_SOURCE_INT | T3_PS_1_16 | T3_GATE_OFF, 62499 );
}

bool getBTN1();
bool getBTN2();



/////////////////
// Function: getBTN1
// Checks to see if BTN1 has been pressed

bool getBTN1() {

    enum Button1Position {
        UP, DOWN
    }; // possible states of button 1

    static enum Button1Position button1CurrentPosition = UP; // BTN 1 current state
    static enum Button1Position button1PreviousPosition = UP; // btn1 prev state
    static unsigned int button1History = 0x0; // last 32 samples of BTN1
    // Reminder - "static" vairables retain their values from one call to the next.

    button1PreviousPosition = button1CurrentPosition;

    button1History = button1History << 1; // sample BTN1
    if (PORTG & 0x40) {
        button1History = button1History | 0x01;
    }

    if ((button1History == 0xF) && (button1CurrentPosition == UP)) {
        button1CurrentPosition = DOWN;
    } else if ((button1History == 0x00) && (button1CurrentPosition == DOWN)) {
        button1CurrentPosition = UP;
    }

    if ((button1CurrentPosition == DOWN) && (button1PreviousPosition == UP)) {
        return TRUE; // debounced 0-to-1 transition has been detected
    }
    return FALSE; // 0-to-1 transition has not been detected
}

bool getBTN2() {

    enum Button2Position {
        UP, DOWN
    }; // possible states of button 1

    static enum Button2Position button2CurrentPosition = UP; // BTN 2 current state
    static enum Button2Position button2PreviousPosition = UP; // btn2 prev state
    static unsigned int button2History = 0x0; // last 32 samples of BTN2
    // Reminder - "static" vairables retain their values from one call to the next.

    button2PreviousPosition = button2CurrentPosition;

    button2History = button2History << 1; // sample BTN1
    if (PORTG & 0x80) {
        button2History = button2History | 0x01;
    }

    if ((button2History == 0xF) && (button2CurrentPosition == UP)) {
        button2CurrentPosition = DOWN;
    } else if ((button2History == 0x00) && (button2CurrentPosition == DOWN)) {
        button2CurrentPosition = UP;
    }

    if ((button2CurrentPosition == DOWN) && (button2PreviousPosition == UP)) {
        return TRUE; // debounced 0-to-1 transition has been detected
    }
    return FALSE; // 0-to-1 transition has not been detected
}



enum states {mainStart, SS_HD, SS_STATS, HD_4_BIT,HD_8_BIT,STATS_4_BIT,STATS_8_BIT, HD_GAME_4, HD_GAME_8, STATS_GAME_4, STATS_GAME_8};
    /*
     state machine states for this lab
     * mainStart - opening OledDisplay for five seconds. after 5 seconds, go to SS_HD
     * 
     * SS_HD - Security Sim selection page, with the arrow pointing to HD
     *  - BTN1 press will toggle to state "SS_STATS"
     *  - BTN2 press will enter the HD menu, known as state "HD_4BIT and HD8BIT"
     * 
     * SS_STATS - Security Sim selection page, with the arrow pointing to Stats
     *  - BTN1 press will toggle state to "SS_HD"
     *  - BTN2 will enter the Stats menu, known as state "STATS_4_BIT" and "STATS_8_BIT"
     * 
     * HD_4_BIT - Hamming distance game selection page, arrow on 4-bit
     *  - BTN1 press will toggle state to "HD_8_BIT"
     *  - BTN2 press will set state to "HD_GAME_4"
     * 
     * HD_8_BIT - Hamming distance game selection page, arrow on 8-bit
     *  - BTN1 press will toggle state to "HD_4_BIT"
     *  - BTN2 press will set state to "HD_GAME_8"
     * 
     * STATS_4_BIT - Statistics for the game, 4-bit play
     *  - BTN1 press will toggle state to "STATS_8_BIT"
     *  - BTN2 press will set state to "STATS_GAME_4"
     * 
     * STATS_8_BIT - Statistics for the game, 8-bit play
     *  - BTN1 press will toggle state to "STATS_4_BIT"
     *  - BTN2 press will set state to "STATS_GAME_8"
     * 
     * HD_GAME_4
     *  - 4 bit game
     * 
     * 
     * 
     */

enum states currentState;

char top_score1_4,top_score2_4,top_score3_4,top_score1_8,top_score2_8,top_score3_8;

bool gameStart = FALSE;
bool gameFinish = FALSE;





int main() {
    char buf[17]; // Temporary string for OLED display
    unsigned int timeCount = 0; // Elapsed time since initialization of program
    unsigned int timer2_current = 0, timer2_previous = 0;
    currentState = mainStart;
    
    // Initialize GPIO for BTN1 and LED1
    TRISGSET = 0x40; // For BTN1: configure PortG bit for input
   // ODCGSET = 0x40;
  
    TRISGSET = 0x80; // for btn2 input configuration
    //ODCGSET = 0x80;
    
    //temp char for checking for 1/0
    char temp_char;
    
    
    short HD_NUM_WRONG = 0;
    //Initializing LEDS
    TRISGCLR = 0x1000;
    TRISGCLR = 0x2000;
    TRISGCLR = 0x4000;
    TRISGCLR = 0x8000;
    ODCGCLR = 0x1000;

    top_score1_4 = 'NO HIGH SCORES';
    top_score1_8 = 'NO HIGH SCORES';
    top_score2_4 = 'NO HIGH SCORES';
    top_score2_8 = 'NO HIGH SCORES';
    top_score3_4 = 'NO HIGH SCORES';
    top_score3_8 = 'NO HIGH SCORES';

    // Initialize PmodOLED, also Timer1 and SPI1
    DelayInit();
    OledInit();

    // Set up Timer2 and Timer3 to roll over every 100 ms
    Timer2Init();
    Timer3Init();

  
    currentState = mainStart;
    bool btn1Press;
    bool btn2Press;
    
    
    char newchar[8];
    int bit_count = 0; // variable for counting the bits.
    char sec_code[8];
    
    

    
    
    // score
    char game_score[17];
    
    sec_code[0] = '0';
    sec_code[1] = '1';
    sec_code[2] = '1';
    sec_code[3] = '0';
    sec_code[4] = '0';
    sec_code[5] = '1';
    sec_code[6] = '0';
    sec_code[7] = '0';
    
    // UART INITIALIZATION
  //  UARTConfigure(UART1, UART_ENABLE_PINS_TX_RX_ONLY);
    
    //I'll remove the number 8, the word NONE and the number 1 from the option strings and replace them with *
  //  UARTSetLineControl(UART1, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    
    //change: I'll remove the two numbers and replace them with *
   // UARTSetDataRate(UART1, 10000000, 9600);
    
    // notice it is being enabled only in Receive mode
   // UARTEnable(UART1, (UART_ENABLE | UART_PERIPHERAL | UART_RX));
    
    initUART(UART1, 10000000, 9600);
    
    
    while (1)
    {
       
        
        switch(currentState)
        {
            case(mainStart):
                DelayMs(200);               // just to slow the program down
                OledClearBuffer();
                OledSetCursor(0,0);
                OledPutString("ECE 2534     ");
                OledSetCursor(0,1);
                OledPutString("Lab 2        ");
                OledSetCursor(0,2);
                OledPutString("Security Sim ");     // main start screen
                
                DelayMs(5000);                      // 5 second delay
                currentState = SS_HD;               // switch to new menu state
                break;
            case(SS_HD):                            // main menu state 1
                btn1Press = getBTN1();
                btn2Press = getBTN2();
                OledSetCursor(0,0);
                OledPutString("Security Sim");
                OledSetCursor(0,1);
                OledPutString("->   HD      ");
                OledSetCursor(0,2);
                OledPutString("     Stats   ");
                OledSetCursor(0,3);
                OledPutString("             ");
               
                if(btn1Press == TRUE && (PORTG & (1<<6)))       // checks that a debounced read on button 1, and pure input from btn1
                {
                    currentState = SS_STATS;
                    break;
                }
                
                
                if (btn2Press == TRUE && (PORTG & (1<<7)))      
                {
                    currentState = HD_4_BIT;
                    break;
                }
                else
                    break;
                
            case(SS_STATS):
                btn1Press = getBTN1();
                btn2Press = getBTN2();
                OledSetCursor(0,0);
                OledPutString("Security Sim ");
                OledSetCursor(0,1);
                OledPutString("     HD      ");
                OledSetCursor(0,2);
                OledPutString("->   Stats   ");
                
                if(btn1Press == TRUE && (PORTG & (1<<6)) )
                {
                    currentState = SS_HD;
                    break;
                }
                
                
                if (btn2Press == TRUE && (PORTG & (1<<7)))
                {
                    currentState = STATS_4_BIT;
                    break;
                }
                else
                    break;
                
            case(HD_4_BIT):
                btn1Press = getBTN1();
                btn2Press = getBTN2();
                OledSetCursor(0,0);
                OledPutString("HD           ");
                OledSetCursor(0,1);
                OledPutString("->   4-Bit   ");
                OledSetCursor(0,2);
                OledPutString("     8-Bit   ");
                
                if(btn1Press == TRUE && (PORTG & (1<<6)) )
                {
                    currentState = HD_8_BIT;
                    break;
                }
                
                
                if (btn2Press == TRUE && (PORTG & (1<<7)))
                {
                    currentState = HD_GAME_4;
                    break;
                }
                else
                    break;
                
            case(HD_8_BIT):
                btn1Press = getBTN1();
                btn2Press = getBTN2();
                OledSetCursor(0,0);
                OledPutString("HD           ");
                OledSetCursor(0,1);
                OledPutString("     4-Bit   ");
                OledSetCursor(0,2);
                OledPutString("->   8-Bit   ");
                
                if(btn1Press == TRUE && (PORTG & (1<<6)) )
                {
                    currentState = HD_4_BIT;
                    break;
                }
                
                
                if (btn2Press == TRUE && (PORTG & (1<<7)))
                {
                    currentState = HD_GAME_8;
                    break;
                }
                else
                    break;
                
            case(STATS_4_BIT):
                btn1Press = getBTN1();
                btn2Press = getBTN2();
                OledSetCursor(0,0);
                OledPutString("Statistics      ");
                OledSetCursor(0,1);
                OledPutString("->   4-Bit   ");
                OledSetCursor(0,2);
                OledPutString("     8-Bit   ");
                
                if(btn1Press == TRUE && (PORTG & (1<<6)) )
                {
                    currentState = STATS_8_BIT;
                    break;
                }
                
                
                if (btn2Press == TRUE && (PORTG & (1<<7)))
                {
                    currentState = STATS_GAME_4;
                    break;
                }
                else
                    break;
                
            case(STATS_8_BIT):
                btn1Press = getBTN1();
                btn2Press = getBTN2();
                OledSetCursor(0,0);
                OledPutString("Statistics   ");
                OledSetCursor(0,1);
                OledPutString("     4-Bit   ");
                OledSetCursor(0,2);
                OledPutString("->   8-Bit   ");
                
                if(btn1Press == TRUE && (PORTG & (1<<6)) )
                {
                    currentState = STATS_4_BIT;
                    break;
                }
                
                
                if (btn2Press == TRUE && (PORTG & (1<<7)))
                {
                    currentState = STATS_GAME_8;
                    break;
                }
                else
                    break;
            case(HD_GAME_4):                // 4 bit game state
                if(gameStart == FALSE)
                {
                    
                    OledClearBuffer();
                    OledSetCursor(0,0);
                    OledPutString("****         ");
                    OledSetCursor(0,1);
                    OledPutString("             ");
                    OledSetCursor(0,2);
                    OledPutString("             ");
                    gameStart = TRUE;
                    gameFinish = FALSE;
                    bit_count = 0;
                    HD_NUM_WRONG = 0;
                    
                    
                    
                    // set timer for game here
                }
                
                
                while(gameFinish == FALSE && bit_count < 4)     // while loop for receiving the UART info
                {
                    temp_char = UARTReceiveByte(UART1);
                    OledSetCursor(bit_count,0);
                    OledPutChar(temp_char);
                    OledSetCursor(0,1);
                    OledPutChar(temp_char);
                    UARTSendByte(UART1,temp_char);
                    if (temp_char == '1' || (temp_char == '0'))
                    {
                        newchar[bit_count] = temp_char;
                    }
                    else
                        bit_count = bit_count - 1;
                    
                    
                    
                    
                   
                    if(bit_count == 3)
                    {
                        if(newchar[0] == sec_code[0])
                            HD_NUM_WRONG = HD_NUM_WRONG;
                        else
                            HD_NUM_WRONG = HD_NUM_WRONG + 1;
                        
                        if(newchar[1] == sec_code[1])
                            HD_NUM_WRONG = HD_NUM_WRONG;
                        else
                            HD_NUM_WRONG = HD_NUM_WRONG + 1;
                        
                        if(newchar[2] == sec_code[2])
                            HD_NUM_WRONG = HD_NUM_WRONG;
                        else
                            HD_NUM_WRONG = HD_NUM_WRONG + 1;
                        
                        if(newchar[3] == sec_code[3])
                            HD_NUM_WRONG = HD_NUM_WRONG;
                        else
                            HD_NUM_WRONG = HD_NUM_WRONG + 1;
                        
                    }
                    bit_count = bit_count + 1;
                    
                    if (HD_NUM_WRONG > 0)
                    {
                         gameStart = FALSE;
                         ledLighter(HD_NUM_WRONG);
                    }

                }
                gameFinish = TRUE;
               
                
                
                
                
                if (btn2Press == TRUE && (PORTG & (1<<7)) && gameFinish == TRUE)
                {
                    currentState = SS_HD;
                    gameFinish = FALSE;
                    gameStart = FALSE;
                    break;
                }
                else
                    break;
                
               
                
                break;
				
				
				
				
				
            case(HD_GAME_8):
                if(gameStart == FALSE)
                {
                    
                    OledClearBuffer();
                    OledSetCursor(0,0);
                    OledPutString("********     ");
                    OledSetCursor(0,1);
                    OledPutString("             ");
                    OledSetCursor(0,2);
                    OledPutString("             ");
                    gameStart = TRUE;
                    gameFinish = FALSE;
                    bit_count = 0;
                    HD_NUM_WRONG = 0;
                    
                    
                    // set timer for game here
                }
                
                
                while(gameFinish == FALSE && bit_count < 8)     // while loop for receiving the UART info
                {
                    temp_char = UARTReceiveByte(UART1);
                    OledSetCursor(bit_count,0);
                    OledPutChar(temp_char);
                    OledSetCursor(0,1);
                    OledPutChar(temp_char);
                    UARTSendByte(UART1,temp_char);
                    if (temp_char == '1' || (temp_char == '0'))
                    {
                        newchar[bit_count] = temp_char;
                    }
                    else
                        bit_count = bit_count - 1;
                    
                    
                    
                    
                   
                    if(bit_count == 7)
                    {
                        if(newchar[0] == sec_code[0])
                            HD_NUM_WRONG = HD_NUM_WRONG;
                        else
                            HD_NUM_WRONG = HD_NUM_WRONG + 1;
                        
                        if(newchar[1] == sec_code[1])
                            HD_NUM_WRONG = HD_NUM_WRONG;
                        else
                            HD_NUM_WRONG = HD_NUM_WRONG + 1;
                        
                        if(newchar[2] == sec_code[2])
                            HD_NUM_WRONG = HD_NUM_WRONG;
                        else
                            HD_NUM_WRONG = HD_NUM_WRONG + 1;
                        
                        if(newchar[3] == sec_code[3])
                            HD_NUM_WRONG = HD_NUM_WRONG;
                        else
                            HD_NUM_WRONG = HD_NUM_WRONG + 1;
                        
                        if(newchar[4] == sec_code[4])
                            HD_NUM_WRONG = HD_NUM_WRONG;
                        else
                            HD_NUM_WRONG = HD_NUM_WRONG + 1;
                        
                        if(newchar[5] == sec_code[5])
                            HD_NUM_WRONG = HD_NUM_WRONG;
                        else
                            HD_NUM_WRONG = HD_NUM_WRONG + 1;
                        
                        if(newchar[6] == sec_code[6])
                            HD_NUM_WRONG = HD_NUM_WRONG;
                        else
                            HD_NUM_WRONG = HD_NUM_WRONG + 1;
                        
                        if(newchar[7] == sec_code[7])
                            HD_NUM_WRONG = HD_NUM_WRONG;
                        else
                            HD_NUM_WRONG = HD_NUM_WRONG + 1;
                        
                    }
                    bit_count = bit_count + 1;
                    
                    if (HD_NUM_WRONG > 0)
                    {
                         gameStart = FALSE;
                         ledLighter(HD_NUM_WRONG);
                    }
                       
                    

                }
                gameFinish = TRUE;
               
                
                
                
                
                if (btn2Press == TRUE && (PORTG & (1<<7)) && gameFinish == TRUE)
                {
                    currentState = SS_HD;
                    gameFinish = FALSE;
                    gameStart = FALSE;
                    break;
                }
                else
                    break;
                
               
                
                break;
            
            case(STATS_GAME_4):
                OledSetCursor(0,0);
                OledPutString("4-Bit Stats");
                OledSetCursor(0,1);
                OledPutString("1.           ");
                OledSetCursor(2,1);
                
                OledSetCursor(0,2);
                OledPutString("2.           ");
                OledSetCursor(2,2);
                
                OledSetCursor(0,3);
                OledPutString("3.           ");
                OledSetCursor(2,3);
                
                
                
                
                if (btn2Press == TRUE && (PORTG & (1<<7)))
                {
                    currentState = SS_HD;
                    break;
                }
                else
                    break;
                
                
            case(STATS_GAME_8):
                OledSetCursor(0,0);
                OledPutString("8-Bit Stats");
                OledSetCursor(0,1);
                OledPutString("1.           ");
                OledSetCursor(2,1);
                
                OledSetCursor(0,2);
                OledPutString("2.           ");
                OledSetCursor(2,2);
                
                OledSetCursor(0,3);
                OledPutString("3.           ");
                OledSetCursor(2,3);
                
                
                 
                
                if (btn2Press == TRUE && (PORTG & (1<<7)))
                {
                    currentState = SS_HD;
                    break;
                }
                else
                    break;
                
                        
                        
    
        }

    }
    
    return EXIT_SUCCESS; // This return should never occur
} // end main  

void ledLighter(short number_wrong)
{
    LATGCLR = 1 << 12;
    LATGCLR = 1 << 13;
    LATGCLR = 1 << 14;
    LATGCLR = 1 << 15;
    
    if (number_wrong == 1)
    {
       LATGSET = 1 << 12; // led 1 0001
    }
    else if (number_wrong == 2)
    {
        LATGSET = 1 << 13; // led 2 0010
    }
    else if (number_wrong == 3) // led 2,3 0011
    {
        LATGSET = 1 << 13; 
        LATGSET = 1 << 12;
    }
    
    else if (number_wrong == 4) // led 3 0100
    {
        LATGSET = 1 << 14; 
    }
    
    else if (number_wrong == 5) // leds:  0101
    {
        LATGSET = 1 << 12; 
        LATGSET = 1 << 14;
    }
    
    else if (number_wrong == 6) // leds:  0110
    {
        LATGSET = 1 << 13; 
        LATGSET = 1 << 14;
    }
    else if (number_wrong == 7) // leds 0111
    {
        LATGSET = 1 << 12;
        LATGSET = 1 << 13;
        LATGSET = 1 << 14;
    }
    
    else if (number_wrong == 8) // leds 1000
    {
        LATGSET = 1 << 15;
    }
}