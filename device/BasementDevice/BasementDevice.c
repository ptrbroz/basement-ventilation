#include <stdio.h>
#include "pico/stdlib.h"
#include "defines.h"
#include "sim7028.h"

#include "pico/mutex.h"

#include "queue.h"

void setup(){
    stdio_init_all(); //includes stdout to usb uart init @ 115200 baud

    uart_init(MODEM_UART, MODEM_UART_BAUD);
    gpio_set_function(MODEM_UART_RX_PIN, GPIO_FUNC_UART);
    gpio_set_function(MODEM_UART_TX_PIN, GPIO_FUNC_UART);

}

int main(){



    setup();

    Queue q;
    initQueue(&q, 5);

    busy_wait_ms(3000);

    for(int i = 0; i < 8; i++){
        int retVal = pushToQueue(&q, i);
        printf("Push %d, retval %d\n\r", i, retVal);
    }

    for(int i = 0; i < 3; i++){
        int pop = -100;
        int retVal = popFromQueue(&q, &pop);
        printf("POP val %d, retval %d\n\r", pop, retVal);
    }

    for(int i = 10; i < 15; i++){
        int retVal = pushToQueue(&q, i);
        printf("Push %d, retval %d\n\r", i, retVal);
    }

    for(int i = 0; i < 10; i++){
        int pop = -100;
        int retVal = popFromQueue(&q, &pop);
        printf("POP val %d, retval %d\n\r", pop, retVal);
    }


    while(1){
        busy_wait_ms(5000);
        printf("testing at\n\r");
    }




}
