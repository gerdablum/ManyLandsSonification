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
#include "ADSR.h"


class TickData {
public:
    stk::Instrmnt* instrument;
    stk::SineWave* sines[2];
    stk::StkFloat frequency;
    stk::StkFloat scaler;
    stk::ADSR envelope;
    long counter;
    bool done;

    void setFrequencyFromSpeed(float speed, float min, float max);
    float calcMidiFromFrequency(float freq);
    float calcFrequencyFromMidi(float midi);

    // Default constructor.
    TickData()
            : instrument(nullptr), sines(), frequency(440.0), scaler(1.0), envelope(), counter(0), done(false ) {
        minFreq = 200;
        maxFreq = 1000;

        minMidi = calcMidiFromFrequency(minFreq);
        maxMidi = calcMidiFromFrequency(maxFreq);
    }
private:
    float minFreq;
    float maxFreq;

    float minMidi;
    float maxMidi;

};

class Audio {

public:
    Audio(RtAudio* dac): dac_(dac) {}
    bool isPlayingSound = false;

    void initStream(TickData* userData);
    void startPlayingAudio();
    void stopPlayingAudio();
    void closeStream();
    void setTickData(TickData* tickData) {
        this->userData_ = tickData;
    }
private:
    RtAudio* dac_;
    TickData* userData_;
    bool isStreamOpen = false;
};


#endif //MANYLANDS_AUDIO_H
