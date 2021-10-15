// Simple program to read a button press
#include <unistd.h> 

#include <iostream>

#include <pigpio.h>

using std::cout;
using std::endl;

const int IN_PIN_NUMBER = 6;
const int OUT_PIN_NUMBER = 16;

/*

void aFunction(int gpio, int level, uint32_t tick)
{
   printf("GPIO %d became %d at %d", gpio, level, tick);
}

// call aFunction whenever GPIO 4 changes state
gpioSetAlertFunc(4, aFunction);

*/

/*

void bFunction(void)
{
   printf("two seconds have elapsed");
}

// call bFunction every 2000 milliseconds
gpioSetTimerFunc(0, 2000, bFunction);

 */

// http://abyz.me.uk/rpi/pigpio/cif.html#gpioPulse_t
gpioPulse_t pulse[] = {
    {0, 0, 0},
    {0, 0, 0}
};

// Reference
// http://abyz.me.uk/rpi/pigpio/cif.html#gpioWaveChain
// 166 / 333 microseconds
// 333 microseconds = 0.00033333333 seconds = 3000 htz?

void makeSound() {
    pulse[0].gpioOn = (1<<OUT_PIN_NUMBER);
    pulse[0].gpioOff = 0;
    pulse[0].usDelay = 166;

    pulse[1].gpioOn = 0;
    pulse[1].gpioOff = (1<<OUT_PIN_NUMBER);
    pulse[1].usDelay = 166;

    gpioWaveAddNew();
    gpioWaveAddGeneric(2, pulse);

    int waveID = gpioWaveCreate();

    if (waveID < 0) {
        cout << "Could not create wave" << endl;
        return;
    }

    gpioWaveTxSend(waveID, PI_WAVE_MODE_REPEAT);

    // gpioWaveTxStop();
}

int main(int argc, char** argv) {
    if (gpioInitialise() < 0) {
        cout << "Failed to start GPIO" << endl;
        return -1;
    }
    
    cout << "Started up GPIO" << endl;

    gpioSetMode(IN_PIN_NUMBER, PI_INPUT);
    gpioSetMode(OUT_PIN_NUMBER, PI_OUTPUT);
    
    int counter = 200;
    bool outputActive = false;
    
    while (counter > 0) {
        int val = gpioRead(IN_PIN_NUMBER);

        if (val == PI_BAD_GPIO) {
            cout << "Got bad GPIO read output" << endl;
            return -1;
        }

        cout << "GPIO value = " << val << endl;

        if (val == 1 && outputActive == false) {
            makeSound();
            outputActive = true;
        }

        if (val == 0 && outputActive == true) {
            gpioWaveTxStop();
            outputActive = false;
        }

        usleep(1000000/4);
        //sleep(1);

        --counter;
    }

    cout << "Shutting down GPIO" << endl;
    
    gpioTerminate();
    return 0;
}
