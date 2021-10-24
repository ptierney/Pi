// Simple program to read a button press
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

const int speaker_pin_number = 16;

// Starts at A
vector<int> key_pins = {
    5,
    6,
    17,
    22,
    23,
    24,
    25,
    27
};

// Starts at A
// Units = htz
vector<int> key_notes = {
    262, // Middle C
    294, // D
    330, // E
    349, // F
    392, // G
    440, // A 440
    494, // B
    523  // C
};

// 
map<int, int> pin_note_index_map;
vector<int> tones;
vector<bool> key_states(8, false);

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


void makeToneMicroseconds(int uSeconds) {
    pulse[0].gpioOn = (1<<speaker_pin_number);
    pulse[0].gpioOff = 0;
    pulse[0].usDelay = uSeconds / 2;

    pulse[1].gpioOn = 0;
    pulse[1].gpioOff = (1<<speaker_pin_number);
    pulse[1].usDelay = uSeconds / 2;

    gpioWaveAddNew();
    gpioWaveAddGeneric(2, pulse);

    int waveID = gpioWaveCreate();

    if (waveID < 0) {
        cout << "Could not create wave" << endl;
        return;
    }

    gpioWaveTxSend(waveID, PI_WAVE_MODE_REPEAT);
}

void makeToneHertz(int hertz) {
    makeToneMicroseconds(1000000 / hertz);
}

void stopSound() {
    gpioWaveTxStop();
}

int currentToneHertz() {
    int tone_sum = 0;
    
    for (int i = 0; i < key_notes.size(); ++i) {
        if (key_states[i] == false) {
            continue;
        }

        tone_sum += key_notes[i];
    }

    return tone_sum;
}

bool buttonsArePressed() {
    for (bool state : key_states) {
        if (state == true) {
            return true;
        }
    }

    return false;
}

void buttonCallbackFunction(int gpio, int level, uint32_t tick) {
    if (level == 2) {
        cout << "No level change" << endl;
        return;
    }

    int note_index = pin_note_index_map[gpio];

    key_states[note_index] = level == 1 ? true : false;

    if (buttonsArePressed() == false) {
        stopSound();
        return;
    }

    int current_tone = currentToneHertz();

    makeToneHertz(current_tone);    
}

void setPinModes() {
    gpioSetMode(speaker_pin_number, PI_OUTPUT);

    for (auto p : key_pins) {
        gpioSetMode(p, PI_INPUT);
    }
}

void setupCallbacks() {
    // PI 4 has 40 pins
    pin_note_index_map = map<int, int>();
    
    for (int i = 0; i < key_pins.size(); ++i) {
        gpioSetAlertFunc(key_pins[i], buttonCallbackFunction);
        pin_note_index_map[key_pins[i]] = i;
    }
}


int main(int argc, char** argv) {
    if (gpioInitialise() < 0) {
        cout << "Failed to start GPIO" << endl;
        return -1;
    }
    
    cout << "Started up GPIO" << endl;

    setPinModes();
    
    while (true) {
        cout << ".";

        usleep(1000000/4);
    }

    cout << "Shutting down GPIO" << endl;
    
    gpioTerminate();
    return 0;
}
