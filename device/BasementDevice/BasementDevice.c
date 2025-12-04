#include <stdio.h>
#include "pico/stdlib.h"

#include "defines.h"
#include "sim7028.h"

#include "pico/time.h"
#include "hardware/regs/uart.h"
#include "hardware/regs/intctrl.h"

#include "queue.h"
#include "logging.h"
#include "incomingBuffers.h"

struct repeating_timer timer;

void setup(){
    stdio_init_all(); //includes stdout to usb uart init @ 115200 baud

    if(initCommandBuffers()){
        while(1){
            logmsg(error, "fail!");
            busy_wait_ms(2000);
        }
    }

    uart_init(MODEM_UART, MODEM_UART_BAUD);
    gpio_set_function(MODEM_UART_RX_PIN, GPIO_FUNC_UART);
    gpio_set_function(MODEM_UART_TX_PIN, GPIO_FUNC_UART);

    // set up interrupt for modem uart incoming chars
    irq_set_exclusive_handler(UART0_IRQ, isrMoveCharsToIncomingQueue);
    * (volatile uint32_t *)(UART0_BASE + UART_UARTIMSC_OFFSET) |= UART_UARTIMSC_RXIM_BITS;  // enable interrupt on hw fifo level reached
    * (volatile uint32_t *)(UART0_BASE + UART_UARTIMSC_OFFSET) |= UART_UARTIMSC_RTIM_BITS;  // enable timeout interrupt (hf fifo not empty & no incoming for some time)
    * (volatile uint32_t *)(UART0_BASE + UART_UARTIFLS_OFFSET) = (0b011 << 3);              // 0b011 on bits 5:3 sets fifo interrupt level to 3/4 = 24 bytes
    irq_set_enabled(UART0_IRQ, true);


}

int main(){

    // wait 1 s before init to avoid startup messages by sim module (verify image sha256 whatever)
    busy_wait_ms(1000);

    setup();

    int counter = 0;

    logmsg(info, "wait 5s");
    busy_wait_ms(5000);

    logmsg(info, "wait 3s");
    busy_wait_ms(3000);

    logmsg(info, "loop begin");

    while(1){

        processIncomingCharsIntoBuffers(100);

        runFsm();

        sleep_ms(500);

    }


}