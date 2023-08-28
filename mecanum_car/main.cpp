
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include <iostream>
#include <ostream>
#include <vector>
#include <map>

#include <pigpio.h>

using std::cout;
using std::endl;
using std::vector;
using std::map;

// Assume where wires protrude is the front of the car

// Naming Guide:
// LF = Left Front
// RF = Right Front
// LB = Left Back
// RB = Right Back
// F = Forward
// R = Reverse

const int WHEEL_PIN_LF_F = 23;
const int WHEEL_PIN_LF_R = 24;

const int WHEEL_PIN_RF_F = 20;
const int WHEEL_PIN_RF_R = 21;

const int WHEEL_PIN_LB_F = 5;
const int WHEEL_PIN_LB_R = 6;

const int WHEEL_PIN_RB_F = 17;
const int WHEEL_PIN_RB_R = 18;


bool running = true;

// sigint code from
// https://stackoverflow.com/questions/1641182/how-can-i-catch-a-ctrl-c-event
void interruptHandler(int s){
    cout << "Caught signal " << s << endl;
    running = false;
}

void setupSigInt() {
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = interruptHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
}

int main(int argc, char** argv) {
    if (gpioInitialise() < 0) {
        cout << "Failed to start GPIO" << endl;
        return -1;
    }

    setupSigInt();
    
    cout << "Started up GPIO" << endl;

    gpioSetMode(WHEEL_PIN_LF_F, PI_OUTPUT);
    gpioSetMode(WHEEL_PIN_LF_R, PI_OUTPUT);
    
    gpioSetMode(WHEEL_PIN_RF_F, PI_OUTPUT);
    gpioSetMode(WHEEL_PIN_RF_R, PI_OUTPUT);

    gpioSetMode(WHEEL_PIN_LB_F, PI_OUTPUT);
    gpioSetMode(WHEEL_PIN_LB_R, PI_OUTPUT);
    
    gpioSetMode(WHEEL_PIN_RB_F, PI_OUTPUT);
    gpioSetMode(WHEEL_PIN_RB_R, PI_OUTPUT);

    gpioPWM(WHEEL_PIN_LF_F, 100);
    gpioPWM(WHEEL_PIN_RF_F, 100);
    gpioPWM(WHEEL_PIN_LB_F, 100);
    gpioPWM(WHEEL_PIN_RB_F, 100);
    
    gpioPWM(WHEEL_PIN_LF_R, 0);
    gpioPWM(WHEEL_PIN_RF_R, 0);
    gpioPWM(WHEEL_PIN_LB_R, 0);
    gpioPWM(WHEEL_PIN_RB_R, 0);

    while (running) {
        cout << gpioTick() << endl;
        usleep(1000000/4);
    }

    // Reset pins
    gpioPWM(WHEEL_PIN_LF_F, 0);
    gpioPWM(WHEEL_PIN_LF_R, 0);
    gpioPWM(WHEEL_PIN_RF_F, 0);
    gpioPWM(WHEEL_PIN_RF_R, 0);
    gpioPWM(WHEEL_PIN_LB_F, 0);
    gpioPWM(WHEEL_PIN_LB_R, 0);
    gpioPWM(WHEEL_PIN_RB_F, 0);
    gpioPWM(WHEEL_PIN_RB_R, 0);

    cout << "Shutting down GPIO" << endl;
    
    gpioTerminate();
    return 0;
}
