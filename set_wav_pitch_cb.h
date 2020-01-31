//
// Created by mlste on 5/20/2019.
//

#ifndef ASSD_TP_4_SET_WAV_PITCH_CB_H
#define ASSD_TP_4_SET_WAV_PITCH_CB_H

#include <portaudio.h>
#include <iostream>
#include "circular_buffer.h"

#define USE_WAV
#define WAV_FILE "C:\\Users\\mlste\\source\\repos\\TP_FINAL_ASSD\\TP_FINAL_ASSD\\bin\\you-call-that-fun"  //Path relativo del archivo .wav SIN EXTENSION
#define WAV_EXTENSION ".wav"

extern int gNumNoInputs;

typedef float SAMPLE;
typedef struct wav_pitch_user_data_t wav_pitch_user_data_t;
typedef float(*freq_detector_t)(circular_buffer<float>& samples, unsigned int n_samples);

// Frequency detectors
float get_frequency_by_autocorrelation_v1(circular_buffer<SAMPLE>& samples, unsigned int n_samples);
float get_frequency_by_autocorrelation_v2(circular_buffer<SAMPLE>& samples, unsigned int n_samples);

// Managing configurations:
wav_pitch_user_data_t * create_user_data(freq_detector_t freq_detector = get_frequency_by_autocorrelation_v2);
wav_pitch_user_data_t * set_wav_user_data(wav_pitch_user_data_t * ud, const char * filename, const char * bin_suffix, const char * out_suffix, const char * freq_det_suffix);
wav_pitch_user_data_t * set_alvin_user_data(wav_pitch_user_data_t * ud, float stretch);
void delete_user_data(wav_pitch_user_data_t * ud);

// 
void process_wav(wav_pitch_user_data_t * userdata);	//Takes ownership and deletes userdata
PaError set_wav_pitch_cb(PaStream*& stream, PaStreamParameters& inputParameters, PaStreamParameters& outputParameters, PaError& err);

#endif //ASSD_TP_4_SET_WAV_PITCH_CB_H
