////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:       Lab 04
// File name:       main.c
// Description:     The goal of this assignment is to implement the specification of LAB 4 for ECE 2534
//							Timer3 to measure period of 1ms
//							PmodOLED.c uses SPI1 for communication with the OLED.
// Written by:       Seth Balodi
// Last modified:   12/03/2016

#define _PLIB_DISABLE_LEGACY
#include <stdio.h>                      // for sprintf()
#include <stdbool.h>                    // for data type bool
#include <stdbool.h>                    // for data type bool
#include <plib.h>                       // Peripheral Library
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"
#include "myDebug.h"
//#include "myBoardConfigFall2016.h"

#define USE_OLED_DRAW_GLYPH
#ifdef USE_OLED_DRAW_GLYPH
// forward declaration of OledDrawGlyph() to satisfy the compiler
void OledDrawGlyph(char ch);
#endif

// Return value check macro
#define CHECK_RET_VALUE(a) { \
  if (a == 0) { \
    LATGSET = 0xF << 12; \
    return(EXIT_FAILURE) ; \
  } \
}

#define NUMBER_OF_MILLISECONDS_PER_OLED_UPDATE 100
#define LED_MASK 0xf000

volatile unsigned int timer2_ms_value = 0;

char overall_score[10];
int counter = 0;



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

#define BTN1 (1 << 6)
#define BTN2 (1 << 7)
#define BTN3 (1 << 0)

#define LED1 (1 << 12)
#define LED2 (1 << 13)
#define LED3 (1 << 14)
#define LED4 (1 << 15)


typedef struct {
    int X_axis;
    int Y_axis;
    int Z_axis;
    unsigned int ID;
} Accelerometer;


BYTE diamond[8] = {0x00, 0x00, 0xFF, 0x39, 0x39, 0xE9, 0xCF, 0x00};							
char diamond_char = 0x00;
BYTE blank[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char blank_char = 0x01;



enum states { main_menu, difficulty_easy, difficulty_hard, game_easy,
                game_hard, time_up, scoreboard, game_start_8_bit};
									
enum states systemState;



// The interrupt handler for timer2
// IPL4 medium interrupt priority
// SOFT|AUTO|SRS refers to the shadow register use
void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) {
    DBG_ON(MASK_DBG0);
    timer2_ms_value++; // Increment the millisecond counter.
    DBG_OFF(MASK_DBG0);
    INTClearFlag(INT_T2); // Acknowledge the interrupt source by clearing its flag.
}


//slave select
//int Slave_Select = BIT_12;

Accelerometer * accelerometer_reader(SpiChannel chn){
    static Accelerometer data_container;
    //read ID register
    data_container.ID = read_SPI(chn, 0x0);
    //read x-axis register
    data_container.X_axis = read_SPI(chn, 0x32);
    //read y-axis register
    data_container.Y_axis = read_SPI(chn, 0x34);
    //read z-axis register
    data_container.Z_axis = read_SPI(chn, 0x36) ;
    return &data_container;
};

int read_SPI(SpiChannel chn, int addr){
    PORTFCLR = BIT_12;   // GRAB THE SLAVE
    // read.  Send 1 bit for read command. 0 for multibyte.
    // (read/write << 7) | (multi/single bit << 6) | (address)
    SpiChnGetRov(chn, 1);
    SpiChnPutC(chn,( (BIT_7) | (addr)));
    SpiChnGetRov(chn, 1);
    SpiChnPutC(chn,( 0xff));
    while(SpiChnIsBusy(chn));
    SpiChnGetRov(chn, 1);
    PORTFSET = BIT_12;  // RELEASE THE SLAVE
    return SpiChnGetC(chn);
}

void initialize_accelerometer(SpiChannel SPIchannel){

    //slave!!
    PORTFCLR = BIT_12;
            
    SpiChnPutC(SPIchannel,( (BIT_7 & 0) | (BIT_6 & 0) | (0x2c)));
    SpiChnPutC(SPIchannel,( 0x8 ));    // DATA RATE
    while(SpiChnIsBusy(SPIchannel));
    SpiChnPutC(SPIchannel,( (BIT_7 & 0) | (BIT_6 & 0) | (0x2d)));
    SpiChnPutC(SPIchannel,( 0x8 ));    // MEASURE
    while(SpiChnIsBusy(SPIchannel));
    
    PORTFSET = BIT_12; 
}








void glyph_runner_hard() {
    unsigned int timer2_ms_value_save;
    unsigned int last_oled_update = 0;
    unsigned int ms_since_last_oled_update;
    unsigned int glyph_pos_x = 0, glyph_pos_y = 1;
    int retValue = 0;

    // Initialize GPIO for LEDs
    TRISGCLR = LED_MASK; // For LEDs: configure PortG pins for output
    ODCGCLR = LED_MASK; // For LEDs: configure as normal output (not open drain)
    LATGCLR = LED_MASK; // Initialize LEDs to 0000

    // Initialize Timer1 and OLED for display
    DelayInit();

    // Set up our user-defined characters for the OLED
    retValue = OledDefUserChar(blank_char, blank);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(diamond_char, diamond);
    CHECK_RET_VALUE(retValue);

    // Initialize GPIO for debugging
    DBG_INIT();

    // Initial Timer2
    initTimer2();

    // Configure the system for vectored interrupts
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);

    // Enable the interrupt controller
    INTEnableInterrupts();
    
    TRISFCLR = BIT_12;
    char accel_ID[10] = "";
    char accel_values[10] = "   ";

    int channel = SPI_CHANNEL4;
    
    SpiChnOpen(channel, SPI_OPEN_MSTEN | SPI_OPEN_CKP_HIGH |  
                SPI_OPEN_MODE8  |SPI_OPEN_ENHBUF, 10 );

    initialize_accelerometer(channel);
    
    Accelerometer * accel_data;   //accelerometer instance, container for the values we read in
    
    int number_of_passes = 0;
    int previous_count = 0;
    previous_count = counter;
    

    while (1) {
        
        accel_data = accelerometer_reader(channel);

        sprintf(accel_values, " Y: %d  X: %4d", accel_data->X_axis, accel_data->Y_axis);
        spawn_finish_line();
        OledMoveTo(64,1); 
        OledLineTo(64, 30);
        
        OledSetCursor(0, 0);
        sprintf(overall_score, "Score: %d", counter);
        OledPutString(overall_score);
		OledSetCursor(0, 4);
        OledPutString(accel_values);
        
        
        ms_since_last_oled_update = timer2_ms_value - last_oled_update;
        if (ms_since_last_oled_update >= NUMBER_OF_MILLISECONDS_PER_OLED_UPDATE) {
            DBG_ON(MASK_DBG1);
            timer2_ms_value_save = timer2_ms_value;
            last_oled_update = timer2_ms_value;
            DBG_OFF(MASK_DBG1);
            DBG_ON(MASK_DBG2);
            OledSetCursor(glyph_pos_x, glyph_pos_y);
//            accelerometer_begin();
#ifdef USE_OLED_DRAW_GLYPH
            OledDrawGlyph(blank_char);
#else
            OledPutChar(blank_char);
#endif
            // Update the glyph position
            glyph_pos_x++;
            LATGCLR = LED1;
            LATGCLR = LED2;
            LATGCLR = LED3;
            LATGCLR = LED4;
            
            
            previous_count = counter;
            
            if (glyph_pos_x == 13 || glyph_pos_x == 8) {
                number_of_passes++;
                if (number_of_passes > 3) {
                    systemState = time_up;
                }
                
                if (accel_data->Y_axis > 20 && accel_data->Y_axis < 98) {
                    counter++;
                }
                if (accel_data->Y_axis > 98 && accel_data->Y_axis < 220) {
                    counter++;
                }
                
                
                if (counter > 5) {
                    if (previous_count = counter) {
                        LATGCLR = LED1;
                        LATGCLR = LED2;
                        LATGCLR = LED3;
                        LATGCLR = LED4;
                    }
                    LATGSET =  LED1;
                    if (counter > 8) {
                        LATGSET = LED1;
                        LATGSET = LED2;
                    }
                    if (counter > 12) {
                        LATGSET = LED1;
                        LATGSET = LED2;
                        LATGSET = LED3;
                    }
                    if (counter > 15) {
                        LATGSET = (0xf << 12);
                    }
                }
            }
            if (glyph_pos_x > 15) {
                glyph_pos_x = 0;
            }
            OledSetCursor(glyph_pos_x, glyph_pos_y);
           
            
            
            
            
            
#ifdef USE_OLED_DRAW_GLYPH
            OledDrawGlyph(diamond_char);
            OledUpdate();
#else
            OledPutChar(diamond_char);
#endif
            DBG_OFF(MASK_DBG2);
//            LATGINV = LED_MASK; // Twiddle LEDs
        }
}

    
    
    
}



void glyph_runner() {
    unsigned int timer2_ms_value_save;
    unsigned int last_oled_update = 0;
    unsigned int ms_since_last_oled_update;
    unsigned int glyph_pos_x = 0, glyph_pos_y = 1;
    int retValue = 0;

    // Initialize GPIO for LEDs
    TRISGCLR = LED_MASK; // For LEDs: configure PortG pins for output
    ODCGCLR = LED_MASK; // For LEDs: configure as normal output (not open drain)
    LATGCLR = LED_MASK; // Initialize LEDs to 0000

    // Initialize Timer1 and OLED for display
    DelayInit();

    // Set up our user-defined characters for the OLED
    retValue = OledDefUserChar(blank_char, blank);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(diamond_char, diamond);
    CHECK_RET_VALUE(retValue);

    // Initialize GPIO for debugging
    DBG_INIT();

    // Initial Timer2
    initTimer2();

    // Configure the system for vectored interrupts
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);

    // Enable the interrupt controller
    INTEnableInterrupts();
    
    TRISFCLR = BIT_12;
    char accel_ID[10] = "";
    char accel_values[10] = "   ";

    int channel = SPI_CHANNEL4;
    
    SpiChnOpen(channel, SPI_OPEN_MSTEN | SPI_OPEN_CKP_HIGH |  
                SPI_OPEN_MODE8  |SPI_OPEN_ENHBUF, 10 );

    initialize_accelerometer(channel);
    
    Accelerometer * accel_data;   //accelerometer instance, container for the values we read in
    
    
    int number_of_passes = 0;

    while (1) {
        
        accel_data = accelerometer_reader(channel);

        sprintf(accel_values, " Y: %d  X: %4d", accel_data->X_axis, accel_data->Y_axis);
        spawn_finish_line();
        
        OledSetCursor(0, 0);
        sprintf(overall_score, "Score: %d", counter);
        OledPutString(overall_score);
		OledSetCursor(0, 4);
        OledPutString(accel_values);
        
        
        ms_since_last_oled_update = timer2_ms_value - last_oled_update;
        if (ms_since_last_oled_update >= NUMBER_OF_MILLISECONDS_PER_OLED_UPDATE) {
            DBG_ON(MASK_DBG1);
            timer2_ms_value_save = timer2_ms_value;
            last_oled_update = timer2_ms_value;
            DBG_OFF(MASK_DBG1);
            DBG_ON(MASK_DBG2);
            OledSetCursor(glyph_pos_x, glyph_pos_y);
            
            
            
//            accelerometer_begin();
#ifdef USE_OLED_DRAW_GLYPH
            OledDrawGlyph(blank_char);
#else
            OledPutChar(blank_char);
#endif
            // Update the glyph position
            glyph_pos_x++;
            LATGCLR = LED1;
            LATGCLR = LED2;
            LATGCLR = LED3;
            LATGCLR = LED4;
            
            int previous_count = 0;
            previous_count = counter;
            
            if (glyph_pos_x == 13) {
                number_of_passes++;
                if (number_of_passes > 3) {
                    systemState = time_up;
                }
                
                if (accel_data->Y_axis > 20 && accel_data->Y_axis < 98) {
                    counter++;
                }
                if (accel_data->Y_axis > 98 && accel_data->Y_axis < 220) {
                    counter++;
                }
                
                
                if (counter > 5) {
                    if (previous_count = counter) {
                        LATGCLR = LED1;
                        LATGCLR = LED2;
                        LATGCLR = LED3;
                        LATGCLR = LED4;
                    }
                    LATGSET =  LED1;
                    if (counter > 8) {
                        LATGSET = LED1;
                        LATGSET = LED2;
                    }
                    if (counter > 12) {
                        LATGSET = LED1;
                        LATGSET = LED2;
                        LATGSET = LED3;
                    }
                    if (counter > 15) {
                        LATGSET = (0xf << 12);
                    }
                }
            }
            if (glyph_pos_x > 15) {
                glyph_pos_x = 0;
            }
            OledSetCursor(glyph_pos_x, glyph_pos_y);
            
            
            
            
            
#ifdef USE_OLED_DRAW_GLYPH
            OledDrawGlyph(diamond_char);
            OledUpdate();
#else
            OledPutChar(diamond_char);
#endif
            DBG_OFF(MASK_DBG2);
//            LATGINV = LED_MASK; // Twiddle LEDs
        }
}

    
    
    
}

void spawn_finish_line() {
        OledMoveTo(0,1);
        OledLineTo(0,30);
        OledMoveTo(1,1);
        OledLineTo(1,30);
        OledMoveTo(1,0);
        OledLineTo(125,0);
        OledMoveTo(1,1);
        OledLineTo(125,1);      
        OledMoveTo(1,31);
        OledLineTo(125,31);
        OledMoveTo(1, 30);
        OledLineTo(125,30);  
        OledMoveTo(100,1); 
        OledLineTo(100, 30);
}


void accelerometer_begin() {
    TRISFCLR = BIT_12;
    char accel_ID[10] = "";
    char accel_values[10] = "   ";

    int channel = SPI_CHANNEL4;
    
    SpiChnOpen(channel, SPI_OPEN_MSTEN | SPI_OPEN_CKP_HIGH |  
                SPI_OPEN_MODE8  |SPI_OPEN_ENHBUF, 10 );

    initialize_accelerometer(channel);
    
    Accelerometer * accel_data;   //accelerometer instance, container for the values we read in

    
    while (1){
        // Grab new data
        accel_data = accelerometer_reader(channel);
        
        sprintf(accel_values, " Y: %d  X: %4d", accel_data->X_axis, accel_data->Y_axis);
        spawn_finish_line();
		OledSetCursor(0, 4);
        
        glyph_runner();
        OledPutString(accel_values);

    }

}


void initialization();
bool getInput_BTN1();
bool getInput_BTN2();
bool Timer2Input();

void Timer2Init();
void Timer3Init();
bool rollOver();

//enum states { main_menu, difficulty_easy, difficulty_hard, game_easy,
//                game_hard, time_up, scoreboard, game_start_8_bit};
//									
//enum states systemState;



/*
	Main Method
*/
int main()
{
//	TRISFCLR = BIT_12;
    
    bool next_1 = FALSE;											//booleans that hold the value of whether the debounced signal of a button-down are registered
	bool next_2 = FALSE;
	
	//CODE FROM LAB 1
	char OLED_buf[17];	

    unsigned int timeCount = 0; 								//how long has the program been running?
    unsigned int timer2_current = 0, timer2_previous = 0;
    
    initialization();
    
    while (1)
    {
		//OledClearBuffer();
        switch (systemState)
        {	
		/*
			This is the Main Menu 
		*/	
        case main_menu:
            
			OledSetCursor(0, 0);          // upper-left corner of display
			OledPutString("Difficulty");
            OledSetCursor(0, 1);
            OledPutString("Hello");
			OledSetCursor(0, 2);
			OledPutString("-->   Easy");
			OledSetCursor(0, 3);          // column 0, row 3 of display
			OledPutString("      Hard");
				
			next_1 = getInput_BTN1();								//recieve the debounced signals of the two button inputs, takes about 5 seconds, unfortunately
			next_2 = getInput_BTN2();
			
			if ((PORTG & BTN2) && (next_2 == TRUE)) {		// if a debounced BTN2 is pressed
				systemState = game_easy;
				OledClearBuffer();
				break;
			}			
			//if we toggle selection
			if ((PORTG & BTN1) && (next_1 == TRUE)) {		// if a debounced BTN1 is pressed
				systemState = difficulty_hard;
				OledClearBuffer();
				break;
			}
            break;
			
					

			
		/*
			Alternative of Main Menu with Stat selected
		*/		
		case difficulty_hard:
			//toggle back to the prevoius state
			OledSetCursor(0, 0);          // upper-left corner of display
			OledPutString("Choose Difficulty");
			OledSetCursor(0, 2);
			OledPutString("      Easy");
			OledSetCursor(0, 3);          // column 0, row 3 of display
			OledPutString("-->   Hard");

			
			next_1 = getInput_BTN1();
			next_2 = getInput_BTN2();
			
			if ((PORTG & BTN1) && (next_1 == TRUE)) {
				systemState = main_menu;
				OledClearBuffer();
				break;
			}
			//NEED TO WRITE CONDITIONAL TO MOVE ON TO NEXT STATE
			if ((PORTG & BTN2) && (next_2 == TRUE)) {
				systemState = game_hard;
				OledClearBuffer();
				break;
			}
			break;
			

		case game_easy:   
            glyph_runner();
			break;
			
	
		case game_hard:
			glyph_runner_hard();
			break;
			
			

			
		case time_up:
			OledSetCursor(0, 0);          // upper-left corner of display
			OledPutString("TIME IS UP");
			OledSetCursor(0, 1);
			OledPutString("wasn't that super fun?!");
			next_1 = getInput_BTN1();
			next_2 = getInput_BTN2();
			
			if ((PORTG & BTN1) && (next_1 == TRUE)) {
				systemState = scoreboard;
				OledClearBuffer();
				break;
			}
			if (PORTG & BTN2 && (next_2 == TRUE)) {
				systemState = scoreboard;
				OledClearBuffer();
				break;
			}

			break;
			
			
			
		case scoreboard:
			OledSetCursor(0, 0);          // upper-left corner of display
			OledPutString("Your score: ");
			OledSetCursor(0, 2);          // column 0, row 3 of display
            
            sprintf(overall_score, "Score: %d", counter);
            OledPutString(overall_score);
            
			OledPutString("5");
			next_1 = getInput_BTN1();
			next_2 = getInput_BTN2();
			
			
			if ((PORTG & BTN1) && (next_1 == TRUE)) {
				systemState = main_menu;
				OledClearBuffer();
				break;
			}
			if (PORTG & BTN2 && (next_2 == TRUE)) {
				systemState = main_menu;
				OledClearBuffer();
				break;
			}

			break;

		
        default:
            // Should never happen; set all LEDs and trap CPU
//            LATGSET = (0xf << 12);
            while(1)
            { // Do nothing; infinite loop to help with debugging     
            }
            break;
        } // end switch

		OledUpdate();
		
		
   } // end while
   
    
   return EXIT_SUCCESS;           // This return should never occur
} // end main  


void initialization()
{
    // Initialize GPIO for BTN1-2 and LED1-4
    TRISGSET = 0xC0;     // For BTN1: configure PortG bit for input
    TRISGCLR = 0xf000;   // For LED1-4: configure PortG pin for output
    ODCGCLR  = 0xf000;   // For LED1-4: configure as normal output (not open drain)
    
    unsigned int ini_timeCount = 0; // Elapsed time since initialization of program
    unsigned int ini_timer2_current=0, ini_timer2_previous=0;

    // Initialize PmodOLED, also Timer1 and SPI1
    DelayInit();
    OledInit();

    // Set up Timer2 to roll over every 500 ms
    initTimer2();
    // Set up Timer3 to roll over every 1ms
    Timer3Init();
    
    while(PORTG & (1<<7)) // blocking_check if BTN2 is still pushed
    {}
    
    LATGSET=0xf<<12;
    
    while(ini_timeCount<10)
    {
        if (Timer2Input())
        {
           // Timer2 has rolled over, so increment count of elapsed time
            ini_timeCount++;
            LATGINV=0xf<<12;           
        }
       
    }
    LATGCLR=0xf<<12;
    INTClearFlag(INT_T3);
    // Send a welcome message to the OLED display
    OledClearBuffer();
    OledSetCursor(0, 0);          // upper-left corner of display
    OledPutString("ECE 2534");
    OledSetCursor(0, 2);          // column 0, row 2 of display
    OledPutString("Lab 4");
    OledSetCursor(0, 3);          // column 0, row 3 of display
    OledPutString("Accel Hero!");
    OledUpdate();
	DelayMs(5000);		
	systemState = main_menu;
	OledClearBuffer(); 
}
		
void initTimer2() {
    // Configure Timer 2 to request a real-time interrupt once per millisecond.
    // The period of Timer 2 is (16 * 625)/(10 MHz) = 1 s.
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 624);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);
}

bool Timer2Input()
{
    int timer2_current;                 // current reading from Timer2
    static int timer2_previous = 0;     // previous reading from Timer2
                                        //  (note:  static value is retained
                                        //  from previous call)  
    
    timer2_current = ReadTimer2();
    if(timer2_previous > timer2_current)
    {
        timer2_previous = timer2_current;
        return TRUE;
    } else
    {
        timer2_previous = timer2_current;
        return FALSE;
    }
}

bool getInput_BTN1()
{
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

    if ((button1History == 0x0F) && (button1CurrentPosition == UP)) {
        button1CurrentPosition = DOWN;
    } else if ((button1History == 0x00) && (button1CurrentPosition == DOWN)) {
        button1CurrentPosition = UP;
    }

    if ((button1CurrentPosition == DOWN) && (button1PreviousPosition == UP)) {
        return TRUE; // debounced 0-to-1 transition has been detected
    }
    return FALSE; // 0-to-1 transition has not been detected
}

bool getInput_BTN2()
{
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

    if ((button2History == 0x0F) && (button2CurrentPosition == UP)) {
        button2CurrentPosition = DOWN;
    } else if ((button2History == 0x00) && (button2CurrentPosition == DOWN)) {
        button2CurrentPosition = UP;
    }

    if ((button2CurrentPosition == DOWN) && (button2PreviousPosition == UP)) {
        return TRUE; // debounced 0-to-1 transition has been detected
    }
    return FALSE; // 0-to-1 transition has not been detected
}

void Timer3Init() 
{
    // The period of Timer 3 is (1 * 10000)/(10 MHz) = 1 ms 
    OpenTimer3(T3_ON | T3_IDLE_CON | T3_SOURCE_INT | T3_PS_1_1 | T3_GATE_OFF, 62499);
    return;
}

bool rollOver() 
{
    if (INTGetFlag(INT_T3))   //has 1 ms passed?
    {  
        INTClearFlag(INT_T3); //clear flag so we don't respond until it sets again
        return TRUE;
    }
    return FALSE;
}
