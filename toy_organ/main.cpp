
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

const int speaker_pin = 16;

vector<int> key_pins = {
    17,
    27,
    22,
    5,
    6,
    25,
    24,
    23
};

// Units = htz
vector<int> key_tones = {
    262, // Middle C
    294, // D
    330, // E
    349, // F
    392, // G
    440, // A 440
    494, // B
    523  // C
};

// Given the GPIO pin number, get the index, ie the
// number in the array for it and the tone
map<int, int> pin_note_index_map;
vector<bool> key_states(8, false);

/*
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
void makeToneMicroseconds(int uSeconds) {
    pulse[0].gpioOn = (1<<speaker_pin);
    pulse[0].gpioOff = 0;
    pulse[0].usDelay = uSeconds / 2;

    pulse[1].gpioOn = 0;
    pulse[1].gpioOff = (1<<speaker_pin);
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

// This logic is incorrect:
// https://music.stackexchange.com/questions/42694/what-is-a-chord-in-terms-of-frequencies
// The frequencies don't combine by a simple sum
int currentToneHertz() {
    int tone_sum = 0;
    
    for (int i = 0; i < key_tones.size(); ++i) {
        if (key_states[i] == false) {
            continue;
        }

        tone_sum += key_tones[i];
    }

    return tone_sum;
}

void buttonCallbackFunction(int gpio, int level, uint32_t tick) {
    if (level == 2) {
        cout << "No level change" << endl;
        return;
    }

    cout << "Pin: " << gpio << " -- Level: " << level << endl;
    
    int note_index = pin_note_index_map[gpio];

    key_states[note_index] = level == 1 ? true : false;

    int current_tone = currentToneHertz();

    if (current_tone == 0) {
        stopSound();
        return;
    }

    makeToneHertz(current_tone);    
}

void setPinModes() {
    gpioSetMode(speaker_pin, PI_OUTPUT);

    for (auto p : key_pins) {
        gpioSetMode(p, PI_INPUT);
    }
}

void setupCallbacks() {
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
    setupCallbacks();
    
    while (true) {
        cout << gpioTick() << endl;

        usleep(1000000/4);
    }

    cout << "Shutting down GPIO" << endl;
    
    gpioTerminate();
    return 0;
}
