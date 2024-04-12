//
// Created by Alina on 01.04.2024.
//

#include "Audio.h"

#include "RtAudio.h"
#include "SineWave.h"

int tick( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
          double streamTime, RtAudioStreamStatus status, void *dataPointer )
{
    TickData *data = (TickData *) dataPointer;
    stk::SineWave *sine = data->sine;
    sine->setFrequency(data->frequency* 200);
    std::cout << data->frequency;
    stk::StkFloat *samples = (stk::StkFloat *) outputBuffer;
    for ( unsigned int i=0; i<nBufferFrames; i++ ) {
        *samples++ = sine->tick();
    }

    return 0;
}

int Audio::initStream(TickData* userData) {
    stk::Stk::setSampleRate( 44100.0 );
    // Figure out how many bytes in an StkFloat and setup the RtAudio stream.
    RtAudio::StreamParameters parameters;
    parameters.deviceId = dac_->getDefaultOutputDevice();
    parameters.nChannels = 1;
    RtAudioFormat format = ( sizeof(stk::StkFloat) == 8 ) ? RTAUDIO_FLOAT64 : RTAUDIO_FLOAT32;
    unsigned int bufferFrames = stk::RT_BUFFER_SIZE;
    this->userData_ = userData;
    if ( dac_->openStream( &parameters,
                         NULL,
                         format,
                         (unsigned int)stk::Stk::sampleRate(),
                         &bufferFrames,
                         &tick,
                         userData ) ) {
        std::cout << dac_->getErrorText() << std::endl;
        return 0;
    }
    return 0;
}

void Audio::startStream() {
    if (!isPlayingSound) {
        if ( dac_->startStream() ) {
            std::cout << dac_->getErrorText() << std::endl;
        }
        isPlayingSound = true;
    }

}

void Audio::stopStream() {
    if (isPlayingSound) {
        dac_->stopStream();
        isPlayingSound = false;
    }

}

void Audio::closeStream() {
    dac_->closeStream();
    isPlayingSound = false;
}


