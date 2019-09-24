/*
* UartRingbuffer.c
* Created on: 10-Jul-2019
* Author: Controllerstech
* Edited by embryonic.dk
*/
#include "UartRingbuffer.h"
#include <string.h>

/**************** no changes after this **********************/
ring_buffer rx_buffer = { { 0 }, 0, 0};
ring_buffer tx_buffer = { { 0 }, 0, 0};
ring_buffer *_rx_buffer;
ring_buffer *_tx_buffer;
UART_HandleTypeDef _handle;

void store_char(unsigned char c, ring_buffer *buffer);

void Ringbuf_init(UART_HandleTypeDef handle)
{
	_rx_buffer = &rx_buffer;
	_tx_buffer = &tx_buffer;
	_handle = handle;

	/* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
	__HAL_UART_ENABLE_IT(&_handle, UART_IT_ERR);

	/* Enable the UART Data Register not empty Interrupt */
	__HAL_UART_ENABLE_IT(&_handle, UART_IT_RXNE);
}

void store_char(unsigned char c, ring_buffer *buffer)
{
	int i = (unsigned int)(buffer->head + 1) % UART_BUFFER_SIZE;
	// if we should be storing the received character into the location
	// just before the tail (meaning that the head would advance to the
	// current location of the tail), we're about to overflow the buffer
	// and so we don't write the character or advance the head.
	if(i != buffer->tail)
	{
		buffer->buffer[buffer->head] = c;
		buffer->head = i;
	}
}

int Uart_read(void)
{
	// if the head isn't ahead of the tail, we don't have any characters
	if(_rx_buffer->head == _rx_buffer->tail)
		return -1;
	else
	{
		unsigned char c = _rx_buffer->buffer[_rx_buffer->tail];
		_rx_buffer->tail = (unsigned int)(_rx_buffer->tail + 1) % UART_BUFFER_SIZE;
		return c;
	}
}

void Uart_write(int c)
{
	int i = (_tx_buffer->head + 1) % UART_BUFFER_SIZE;
	// If the output buffer is full, there's nothing for it other than to
	// wait for the interrupt handler to empty it a bit
	// ???: return 0 here instead?
	while (i == _tx_buffer->tail);
	_tx_buffer->buffer[_tx_buffer->head] = (uint8_t)c;
	_tx_buffer->head = i;
	__HAL_UART_ENABLE_IT(&_handle, UART_IT_TXE); // Enable UART transmission interrupt
}

int IsDataAvailable(void)
{
	return (uint16_t)(UART_BUFFER_SIZE + _rx_buffer->head - _rx_buffer->tail) % UART_BUFFER_SIZE;
}

uint16_t get_pos (char *string)
{
	static uint8_t so_far;
	uint16_t counter;
	int len = strlen (string);

	if (_rx_buffer->tail>_rx_buffer->head)
	{
		if (Uart_read() == string[so_far])
		{
			counter=UART_BUFFER_SIZE-1;
			so_far++;
		}
		else
			so_far=0;
	}

	unsigned int start = _rx_buffer->tail;
	unsigned int end = _rx_buffer->head;

	for (unsigned int i=start; i<end; i++)
	{
		if (Uart_read() == string[so_far])
		{
			counter=i;
			so_far++;
		}
		else so_far =0;
	}

	if (so_far == len)
	{
		so_far =0;
		return counter;
	}
	else return -1;
}

void Uart_sendArray (char arr[],int numBytes)
{
	for(int i=0;i<numBytes;i++)
	{
		Uart_write(arr[i]);	//send one byte at a time
	}
}

void Uart_sendstring (const char *s)
{
	while(*s) Uart_write(*s++);
}

void Uart_printbase (long n, uint8_t base)
{
	char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
	char *s = &buf[sizeof(buf) - 1];

	*s = '\0';
	// prevent crash if called with base == 1
	if (base < 2) base = 10;
	do
	{
		unsigned long m = n;
		n /= base;
		char c = m - base * n;
		*--s = c < 10 ? c + '0' : c + 'A' - 10;
	} while(n);

	while(*s) Uart_write(*s++);
}

void Get_string (char *buffer)
{
	int index=0;

	while (_rx_buffer->tail>_rx_buffer->head)
	{
		if ((_rx_buffer->buffer[_rx_buffer->head-1] == '\n')||((_rx_buffer->head == 0) && (_rx_buffer->buffer[UART_BUFFER_SIZE-1] == '\n')))
		{
			buffer[index] = Uart_read();
			index++;
		}
	}

	unsigned int start = _rx_buffer->tail;
	unsigned int end = (_rx_buffer->head);

	if ((_rx_buffer->buffer[end-1] == '\n'))
	{
		for (unsigned int i=start; i<end; i++)
		{
			buffer[index] = Uart_read();
			index++;
		}
	}
}

int wait_until (char *string, char*buffertostore)
{
	while (!(IsDataAvailable()));
	int index=0;

	while (_rx_buffer->tail>_rx_buffer->head)
	{
		if ((_rx_buffer->buffer[_rx_buffer->head-1] == '\n')||((_rx_buffer->head == 0) && (_rx_buffer->buffer[UART_BUFFER_SIZE-1] == '\n')))
		{
			buffertostore[index] = Uart_read();
			index++;
		}
	}

	unsigned int start = _rx_buffer->tail;
	unsigned int end = (_rx_buffer->head);

	if ((_rx_buffer->buffer[end-1] == '\n'))
	{
		for (unsigned int i=start; i<end; i++)
		{
			buffertostore[index] = Uart_read();
			index++;
		}
		return 1;
	}
	return 0;
}

void flush(void)
{
	_rx_buffer->tail = _rx_buffer->head;	//move the tail up to the head so all available chars 'disappear'
}

void Uart_isr (UART_HandleTypeDef *huart)
{
	uint32_t isrflags   = READ_REG(huart->Instance->ISR);
	uint32_t cr1its     = READ_REG(huart->Instance->CR1);

	/* if DR is not empty and the Rx Int is enabled */
	if (((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
	{
		/******************
		*  @note   PE (Parity error), FE (Framing error), NE (Noise error), ORE (Overrun
		*          error) and IDLE (Idle line detected) flags are cleared by software
		*          sequence: a read operation to USART_SR register followed by a read
		*          operation to USART_DR register.
		* @note   RXNE flag can be also cleared by a read to the USART_DR register.
		* @note   TC flag can be also cleared by software sequence: a read operation to
		*          USART_SR register followed by a write operation to USART_DR register.
		* @note   TXE flag is cleared only by a write to the USART_DR register.
		*********************/
		huart->Instance->ISR;                       /* Read status register */
		unsigned char c = huart->Instance->RDR;     /* Read data register */
		store_char (c, _rx_buffer);  // store data in buffer
		return;
	}

	/*If interrupt is caused due to Transmit Data Register Empty */
	if (((isrflags & USART_ISR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
	{
		if(tx_buffer.head == tx_buffer.tail)
		{
			// Buffer empty, so disable interrupts
			__HAL_UART_DISABLE_IT(huart, UART_IT_TXE);
		}
		else
		{
			// There is more data in the output buffer. Send the next byte
			unsigned char c = tx_buffer.buffer[tx_buffer.tail];
			tx_buffer.tail = (tx_buffer.tail + 1) % UART_BUFFER_SIZE;
			huart->Instance->ISR;
			huart->Instance->TDR = c;
		}
		return;
	}
}
