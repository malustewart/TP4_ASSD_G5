//
// Created by mlste on 5/20/2019.
//

#ifndef ASSD_TP_4_SET_WAV_PITCH_CB_H
#define ASSD_TP_4_SET_WAV_PITCH_CB_H

#include <portaudio.h>
#include <iostream>
#include "circular_buffer.h"
#include "gui.h"

//#define USE_WAV
#define WAV_FILE "C:\Users\User\Documents\Visual Studio 2019\Code Snippets\Visual C++\My Code Snippets\TP_FINAL_ASSD\TP_FINAL_ASSD\king_love"  //Path relativo del archivo .wav SIN EXTENSION
#define WAV_EXTENSION ".wav"

extern int gNumNoInputs;

typedef float SAMPLE;
typedef struct pitch_user_data_t pitch_user_data_t;
typedef float(*freq_detector_t)(circular_buffer<float>& samples, unsigned int n_samples, int& prev_tau);

// Frequency detectors
float get_frequency_by_autocorrelation_v1(circular_buffer<SAMPLE>& samples, unsigned int n_samples, int& prev_tau);
float get_frequency_by_autocorrelation_v2(circular_buffer<SAMPLE>& samples, unsigned int n_samples, int& prev_tau);
float get_frequency_by_yin               (circular_buffer<SAMPLE>& samples, unsigned int n_samples, int& prev_tau);


// Managing configurations:
pitch_user_data_t* create_user_data(freq_detector_t freq_detector = get_frequency_by_autocorrelation_v2);
pitch_user_data_t* set_wav_user_data(pitch_user_data_t * ud, const char * filename, const char * bin_suffix, const char * out_suffix, const char * freq_det_suffix, const char * freq_obj_suffix);
pitch_user_data_t* set_real_time_user_data(pitch_user_data_t * ud);
pitch_user_data_t* set_alvin_user_data(pitch_user_data_t * ud, float stretch);
pitch_user_data_t* set_duki_user_data(pitch_user_data_t* ud, scale_t scale);
float SelectedFundFrec(int note);
void delete_user_data(pitch_user_data_t * ud);

// 
void process_wav(pitch_user_data_t * userdata);	//Takes ownership and deletes userdata
int run_real_time(pitch_user_data_t * userdata);
void stop_real_time(pitch_user_data_t * userdata);
#endif //ASSD_TP_4_SET_WAV_PITCH_CB_H
