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
    sine->setFrequency(data->frequency);
    stk::StkFloat *samples = (stk::StkFloat *) outputBuffer;
    for ( unsigned int i=0; i<nBufferFrames; i++ ) {
        *samples++ = sine->tick();
    }

    return 0;
}

void Audio::initStream(TickData* userData) {
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
    }
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

void TickData::setFrequencyFromSpeed(float speed, float min, float max) {
    //float percentage = (speed - min) / (max - min);

    float percentage = (speed - min) / (max - min);
    frequency = exp(percentage * (log(MAX_FREQ) - log(MIN_FREQ)) + log(MIN_FREQ));


//    float midiNote = percentage * (MAX_MIDI - MIN_MIDI) + MIN_MIDI;
//    float newFrequency = pow(2.0f, (midiNote -69.0f) / 12.0f) * 440.0f;
//    frequency = newFrequency;

}


