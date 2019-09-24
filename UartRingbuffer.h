/*
* UartRingbuffer.c
* Created on: 10-Jul-2019
* Author: Controllerstech
* Edited by embryonic.dk
*/

#ifndef UARTRINGBUFFER_H_
#define UARTRINGBUFFER_H_

#include "stm32f7xx_hal.h"

/* change the size of the buffer */
#define UART_BUFFER_SIZE 64

typedef struct
{
  unsigned char buffer[UART_BUFFER_SIZE];
  volatile unsigned int head;
  volatile unsigned int tail;
} ring_buffer;

/* reads the data in the rx_buffer and increment the tail count in rx_buffer */
int Uart_read(void);

/* writes the data to the tx_buffer and increment the head count in tx_buffer */
void Uart_write(int c);

/* Writes a byte array to the tx_buffer*/
void Uart_sendArray (char arr[],int numBytes);

/* function to send the string to the uart */
void Uart_sendstring(const char *s);

/* Print a number with any base
 * base can be 10, 8 etc*/
void Uart_printbase (long n, uint8_t base);

/* Initialize the ring buffer */
void Ringbuf_init(UART_HandleTypeDef handle);

/* checks if the data is available to read in the rx_buffer */
int IsDataAvailable(void);

/* get the position of the given string within the incoming data.
 * It returns the position, where the string ends */
uint16_t get_pos (char *string);

//flush all available characters from the RX buffer
void flush(void);

/* the ISR for the uart. put it in the IRQ handler */
void Uart_isr (UART_HandleTypeDef *huart);

/* once you hit 'enter' (\r\n), it copies the entire string to the buffer*/
void Get_string (char *buffer);

/* keep waiting until the given string has arrived and stores the data in the buffertostore
 * on success return 1
 * or else returns 0
 * @ usage :  while (!(wait_until("\r\n", buffer)));
*/
int wait_until (char *string, char *buffertostore);

#endif /* UARTRINGBUFFER_H_ */
