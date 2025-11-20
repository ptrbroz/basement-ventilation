#if !defined(SIM0728)
#define SIM0728

// Sends an AT command to the sim modem via uart.
// Specifically, given input <str>, this sends <cr><lf>AT<str><cr><lf>.
void sendAT(char *str);

// Execute one cycle of modem-handling fsm
void runFsm();

#endif // SIM0728