//
// Created by mlste on 5/20/2019.
//

#ifndef ASSD_TP_4_SET_WAV_PITCH_CB_H
#define ASSD_TP_4_SET_WAV_PITCH_CB_H

#include "portaudio.h"
#include <iostream>
extern int gNumNoInputs;
PaError set_wav_pitch_cb(PaStream*& stream, PaStreamParameters& inputParameters, PaStreamParameters& outputParameters, PaError& err, std::ofstream * datafile);

#endif //ASSD_TP_4_SET_WAV_PITCH_CB_H
