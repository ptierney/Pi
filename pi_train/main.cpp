
#include <unistd.h> 

#include <iostream>
#include <ostream>
#include <vector>
#include <map>

#include <pigpio.h>

using std::cout;
using std::endl;
using std::vector;
using std::map;

// Base source from: https://ben.akrin.com/?p=9768

// IN is the label on the stepper motor driver
const int IN1 = 17;
const int IN2 = 18;
const int IN3 = 27;
const int IN4 = 22;

vector<int> motor_pins = {IN1, IN2, IN3, IN4};

vector<vector<int>> step_sequence = { { 1, 0, 0, 1 },
                                      { 1, 0, 0, 0 },
                                      { 1, 1, 0, 0 },
                                      { 0, 1, 0, 0 },
                                      { 0, 1, 1, 0 },
                                      { 0, 0, 1, 0 },
                                      { 0, 0, 1, 1 },
                                      { 0, 0, 0, 1 } };

double step_sleep = 0.002;

int step_count = 4096;

bool rotate_clockwise = true;

void setPinModes() {
    gpioSetMode(IN1, PI_OUTPUT);
    gpioSetMode(IN2, PI_OUTPUT);
    gpioSetMode(IN3, PI_OUTPUT);
    gpioSetMode(IN4, PI_OUTPUT);
}

int main(int argc, char** argv) {
    if (gpioInitialise() < 0) {
        cout << "Failed to start GPIO" << endl;
        return -1;
    }
    
    cout << "Started up GPIO" << endl;

    setPinModes();

    int motor_step_counter = 0;
    
    while (true) {
        //cout << gpioTick() << endl;

        for (int i = 0; i < motor_pins.size(); ++i) {
            int pin = motor_pins[i];
            gpioWrite(pin, step_sequence[motor_step_counter][i]);
        }

        motor_step_counter = (motor_step_counter + 1) % 8;

        usleep(1000000 * step_sleep);
    }

    cout << "Shutting down GPIO" << endl;
    
    gpioTerminate();
    return 0;
}


