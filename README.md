# STM32Ringbuffer
A ringbuffer based on the code available from controllerstech.com

How to use this?
1. Move the source and header files to your project.
2. include the header file in your project.
3. replace the IRQ service handler with the one called Uart_isr - available in the header file. You might need to include that header file in your IT file as well.
4. remember to call Ringbuf_init before writing to the buffer.
5. ???
6. profit


For more information, have a look at the instruction video here: 
https://www.youtube.com/watch?v=6a_VNVWbYvU
