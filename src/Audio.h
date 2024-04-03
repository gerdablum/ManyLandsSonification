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

struct TickData {
    stk::Instrmnt *instrument;
    stk::StkFloat frequency;
    stk::StkFloat scaler;
    long counter;
    bool done;

    // Default constructor.
    TickData()
            : instrument(nullptr), frequency(440.0), scaler(1.0), counter(0), done( false ) {}
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
