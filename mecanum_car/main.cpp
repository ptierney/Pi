
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

const int WHEEL_PIN_A = 23;
const int WHEEL_PIN_B = 24;

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

    gpioSetMode(WHEEL_PIN_A, PI_OUTPUT);
    gpioSetMode(WHEEL_PIN_B, PI_OUTPUT);

    gpioPWM(WHEEL_PIN_A, 100);
    gpioPWM(WHEEL_PIN_B, 0);

    while (running) {
        cout << gpioTick() << endl;
        usleep(1000000/4);
    }

    // Reset pins
    gpioPWM(WHEEL_PIN_A, 0);
    gpioPWM(WHEEL_PIN_B, 0);

    cout << "Shutting down GPIO" << endl;
    
    gpioTerminate();
    return 0;
}
