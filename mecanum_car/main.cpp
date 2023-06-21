
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
void my_handler(int s){
    cout << "Caught signal " << s << endl;
    running = false;
}

int main(int argc, char** argv) {
    if (gpioInitialise() < 0) {
        cout << "Failed to start GPIO" << endl;
        return -1;
    }

    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
    
    cout << "Started up GPIO" << endl;

    gpioSetMode(WHEEL_PIN_A, PI_OUTPUT);
    gpioSetMode(WHEEL_PIN_B, PI_OUTPUT);

    gpioPWM(WHEEL_PIN_A, 255);
    gpioPWM(WHEEL_PIN_B, 0);

    while (true) {
        cout << gpioTick() << endl;

        usleep(1000000/4);

        if (!running) {
            gpioPWM(WHEEL_PIN_A, 0);
            gpioPWM(WHEEL_PIN_B, 0);

            break;
        }
    }

    cout << "Shutting down GPIO" << endl;
    
    gpioTerminate();
    return 0;
}
