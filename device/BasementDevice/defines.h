#include "hardware/uart.h"

#if !defined(DEFINES_H)
#define DEFINES_H

#define MODEM_UART uart0 //for communicating with the SIM modem
#define MODEM_UART_BAUD 115200
#define MODEM_UART_TX_PIN 0
#define MODEM_UART_RX_PIN 1

//The uart runs at 115200 and has a 32 byte deep input fifo. Let's check it after ~20 bytes had time to accumulate, to be on the safe side
#define IRQ_PERIOD_MODEM_UART_US 1736 // 1/115200 * 10 * 20



#endif // MACRO
