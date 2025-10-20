#include <stdio.h>
#include "pico/stdlib.h"

#include "defines.h"
#include "sim7028.h"

#include "pico/time.h"

#include "queue.h"
#include "logging.h"
#include "incomingBuffers.h"

struct repeating_timer timer;

void setup(){
    stdio_init_all(); //includes stdout to usb uart init @ 115200 baud

    uart_init(MODEM_UART, MODEM_UART_BAUD);
    gpio_set_function(MODEM_UART_RX_PIN, GPIO_FUNC_UART);
    gpio_set_function(MODEM_UART_TX_PIN, GPIO_FUNC_UART);

    gpio_init(5);
    gpio_set_dir(5, GPIO_OUT);
    gpio_put(5, 0);

    if(initCommandBuffers()){
        while(1){
            logmsg(error, "fail!");
            busy_wait_ms(2000);
        }
    }

    if(!add_repeating_timer_us(IRQ_PERIOD_MODEM_UART_US, isrModemUartRx, NULL, &timer)){
        while(1){
            logmsg(error, "fail2!");
            busy_wait_ms(2000);
        }
    }

}

int main(){

    setup();

    int counter = 0;


    while(1){

        sendAT("");

        sleep_ms(3000);

    }




}
