// ECE 2534:        myUART.c
// Purpose:         User-generated UART helper functions.
// Resources:       UART only
// Written by:      JST
// Last modified:   26 September 2014 (by JST)

#include <plib.h>
#include "myUART.h"

///////////////////////////////////////////////////////////////////////////////
//
// Function:    initUART
// Description: Initializes UART on the PIC32 for transmission and reception
//              using a data frame of  8 data bits, no parity, and 1 stop bit.
//              Also turns on the UART.
//
// Parameters:   uart - The Cerebot UART module.
//               sourceClock - The Cerebot peripheral clock frequency
//               dataRate - the baud rate of the UART channel
// Return value: N/A

void initUART(UART_MODULE uart, unsigned int sourceClock, unsigned int dataRate)
{
    UARTConfigure(uart, UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetLineControl(uart, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTSetDataRate(uart, sourceClock, dataRate);
    UARTEnable(uart, (UART_ENABLE | UART_PERIPHERAL | UART_RX | UART_TX));
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    UARTReceiveByte
// Description: Receives a byte from UART.

// Parameters:   uart - The Cerebot UART module.
// Return value: byte_received - the byte received.

unsigned char UARTReceiveByte(UART_MODULE uart)
{
    unsigned char byte_received;
    while(!UARTReceivedDataIsAvailable(uart))
    {
        // DO NOTHING
    }
    byte_received = UARTGetDataByte(uart);
    return byte_received;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    UARTSendByte
// Description:	Transmits a byte via UART.
//
// Parameters:   uart - The Cerebot UART module.
//               byte_sent - The byte transmitted via UART.
// Return value: N/A

void UARTSendByte(UART_MODULE uart, char byte_sent)
{
    while(!UARTTransmitterIsReady(uart))
    {
        // DO NOTHING
    }
    UARTSendDataByte(uart,byte_sent);
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    UARTSendString
// Description: Transmits a string via UART.
//
// Parameters:   uart - The Cerebot UART module.
//               string - A pointer to the string being transmitted.
// Return value: N/A

void UARTSendString(UART_MODULE uart, const char* string)
{
    int i;
    for (i=0; string[i] != NULL; i++)
    {
        while(!UARTTransmitterIsReady(uart))
        {
            // DO NOTHING
        }
        UARTSendDataByte(uart, string[i]);
    }
}
