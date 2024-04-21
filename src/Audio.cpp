//
// Created by Alina on 01.04.2024.
//

#include "Audio.h"

#include "RtAudio.h"
#include "SineWave.h"


int tick( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
          double streamTime, RtAudioStreamStatus status, void *dataPointer )
{
    auto *data = (TickData *) dataPointer;
    stk::SineWave *sine1 = data->sines[0];
    stk::SineWave *sine2 = data->sines[1];
    sine1->setFrequency(data->frequency);
    float midi = data->calcMidiFromFrequency(data->frequency);
    float midiSine2 = midi + 7;
    sine2->setFrequency(data->calcFrequencyFromMidi(midiSine2));
    stk::StkFloat *samples = (stk::StkFloat *) outputBuffer;
    for ( unsigned int i=0; i<nBufferFrames; i++ ) {
        *samples++ = (sine1->tick() * 0.5 + sine2->tick() * 0.5) * data->envelope.tick();
    }

    return 0;
}

void Audio::initStream(TickData* userData) {
    if (this->isStreamOpen) {
        dac_->closeStream();
    }
    stk::Stk::setSampleRate( 44100.0 );
    RtAudio::StreamParameters outputParameters;
    outputParameters.deviceId = dac_->getDefaultOutputDevice();
    outputParameters.nChannels = 1;
    RtAudio::StreamParameters inputParameters;
    inputParameters.deviceId = dac_->getDefaultInputDevice();
    inputParameters.nChannels = 1;
    RtAudioFormat format = RTAUDIO_FLOAT64;
    unsigned int bufferFrames = stk::RT_BUFFER_SIZE;
    this->userData_ = userData;
    if ( dac_->openStream( &outputParameters,
                         &inputParameters,
                         format,
                         (unsigned int)stk::Stk::sampleRate(),
                         &bufferFrames,
                         &tick,
                         userData ) ) {
        std::cout << dac_->getErrorText() << std::endl;
    } else {
        this->isStreamOpen = true;
        if ( dac_->startStream() ) {
            std::cout << dac_->getErrorText() << std::endl;
        }

    }

}

void Audio::startPlayingAudio() {

    if (!isPlayingSound) {
        userData_->envelope.keyOn();
        isPlayingSound = true;
    }

}

void Audio::stopPlayingAudio() {
    if (isPlayingSound) {
        userData_->envelope.keyOff();
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
    frequency = exp(percentage * (log(this->maxFreq) - log(this->minFreq)) + log(this->minFreq));


//    float midiNote = percentage * (MAX_MIDI - MIN_MIDI) + MIN_MIDI;
//    float newFrequency = pow(2.0f, (midiNote -69.0f) / 12.0f) * 440.0f;
//    frequency = newFrequency;

}

float TickData::calcMidiFromFrequency(float freq) {
    return 12.0f * (log(freq/440.0f) / log(2.0f)) + 69.0f;
}

float TickData::calcFrequencyFromMidi(float midi) {
    return pow(2.0f, (midi -69.0f) / 12.0f) * 440.0f;
}


