#include <stdio.h>
#include "pico/stdlib.h"

#include "defines.h"
#include "sim7028.h"

#include "pico/mutex.h"

#include "queue.h"
#include "logging.h"

void setup(){
    stdio_init_all(); //includes stdout to usb uart init @ 115200 baud

    uart_init(MODEM_UART, MODEM_UART_BAUD);
    gpio_set_function(MODEM_UART_RX_PIN, GPIO_FUNC_UART);
    gpio_set_function(MODEM_UART_TX_PIN, GPIO_FUNC_UART);
}

int main(){

    setup();

    while(1){
        int testInt = 42;
        char testString[] = "Herojam slava!";
        float testFloat = 3.14159;
        
        logmsg(debug, "Test integer is %d", testInt);
        logmsg(info, "Compared to test integer %d, test float %.3f is smaller.", testInt, testFloat);
        logmsg(warn, "This is a warning.");
        logmsg(error, "test string is %s", testString);

        busy_wait_ms(5000);

        if(usbLoggingLevel == debug){
            usbLoggingLevel = warn;
        }
        else{
            usbLoggingLevel = debug;
        }

    }




}
