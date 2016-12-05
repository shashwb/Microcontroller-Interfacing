// ECE 2534:        myUART.h
// Purpose:         User-generated UART helper function prototypes.
// Resources:       UART only
// Written by:      JST
// Last modified:   26 September 2014 (by JST)

#include <plib.h>

#ifndef MYUART_H
#define MYUART_H

void initUART(UART_MODULE uart, unsigned int sourceClock, unsigned int dataRate);
unsigned char UARTReceiveByte(UART_MODULE uart);
void UARTSendByte(UART_MODULE uart, char byte_sent);
void UARTSendString(UART_MODULE uart, const char* string);

#endif
