//
// Created by Alina on 01.04.2024.
//

#ifndef MANYLANDS_AUDIO_H
#define MANYLANDS_AUDIO_H

#include "FileLoop.h"
#include "FileWvOut.h"
#include "RtAudio.h"
#include "SineWave.h"
#include "RtWvOut.h"
#include "Instrmnt.h"
const float MIN_FREQ = 200;
const float MAX_FREQ = 1000;
class TickData {
public:
    stk::SineWave* sine;
    stk::StkFloat frequency;
    stk::StkFloat scaler;
    long counter;
    bool done;

    void setFrequencyFromSpeed(float speed, float min, float max) {
        float percentage = (speed - min) / (max - min);
        frequency = percentage * (MAX_FREQ - MIN_FREQ) + MIN_FREQ;
    }

    // Default constructor.
    TickData()
            : sine(), frequency(440.0), scaler(1.0), counter(0), done( false ) {}
};

class Audio {

public:
    Audio(RtAudio* dac): dac_(dac) {}
    bool isPlayingSound = false;

    int initStream(TickData* userData);
    void startStream();
    void stopStream();
    void closeStream();
    void setTickData(TickData* tickData) {
        this->userData_ = tickData;
    }
private:
    RtAudio* dac_;
    TickData* userData_;
};


#endif //MANYLANDS_AUDIO_H
