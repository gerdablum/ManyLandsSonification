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
#include "FreeVerb.h"


const static float MIN_FREQ_RANGE = 100.0f;
const static float MAX_FREQ_RANGE = 900.0f;

class TickData {
public:
    std::vector<stk::SineWave*> sines;
    std::vector<float> overtoneSteps;
    std::vector<float> overToneLoudness;
    stk::StkFloat fundamentalFrequency;
    stk::StkFloat scaler;
    stk::ADSR envelope;

    void setFundamentalFrequencyFromSpeed(float speed, float min, float max);
    float calcMidiFromFrequency(float freq);
    float calcFrequencyFromMidi(float midi);
    void initSines(int noOfSines);

    // Default constructor.
    TickData()
            : sines(), fundamentalFrequency(440.0), scaler(1.0), envelope() {
        minFreq = 200;
        maxFreq = 800;
        minMidi = calcMidiFromFrequency(minFreq);
        maxMidi = calcMidiFromFrequency(maxFreq);
    }

    void updateMinMaxFrequency(float minFrequency, float maxFrequency);

private:
    float minFreq;
    float maxFreq;

    float minMidi;
    float maxMidi;
};

class Audio {

public:
    Audio(RtAudio* dac): dac_(dac), userData_() {}
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
