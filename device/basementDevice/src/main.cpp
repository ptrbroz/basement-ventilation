//#include <Arduino.h>

#define TINY_GSM_MODEM_SIM7028
#define MODE_NB_IOT
#include <TinyGsmClient.h> // !! has to come after modem def above
#include <PubSubClient.h>

#define SerialMon Serial //USB uart
#define SerialAT Serial1 //HW uart 0

// set GSM PIN, if any
#define GSM_PIN ""
// Your GPRS credentials, if any
const char apn[]      = "lpwa.vodafone.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

// MQTT details
const char* broker = "broker.hivemq.com";
const char* topic       = "soarburn";

const int fakeResetPin = 28; //not actually used, but constructor call requires it

#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger, fakeResetPin);
//TinyGsm        modem(SerialAT, fakeResetPin);

TinyGsmClient client(modem);
PubSubClient  mqtt(client);

void setup() {

  SerialMon.begin(9600); //usb
  delay(500);
  SerialMon.println("Started.");

  SerialAT.begin(115200); 
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_set_function(1, GPIO_FUNC_UART);
  delay(1000);

  modem.init();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);


  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork(60000*30)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) { SerialMon.println("Network connected"); }


}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
}
