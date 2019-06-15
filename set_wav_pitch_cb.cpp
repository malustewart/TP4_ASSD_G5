//
// Created by mlste on 5/20/2019.
//

#include "set_wav_pitch_cb.h"
#include "circular_buffer.h"
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <vector>

#define SAMPLE_RATE         (44100)
#define FRAMES_PER_BUFFER   (600)
#define PITCH_DETECT_SEGMENT_LENGTH       ((SAMPLE_RATE)/100) //TODO: borrar magic number

#define GET_FREQ(o,n)       (FUND_FREQ * pow(2, ((float)(n) + OCTAVE_SUBDIVISION*(float)(octave))/OCTAVE_SUBDIVISION))

#define OCTAVES             (5)
#define OCTAVE_SUBDIVISION  (12)
#define FUND_FREQ C_FREQ
#define C_FREQ  32.7
#define Cs_FREQ 34.65
#define D_FREQ  36.71
#define Ds_FREQ 38.89
#define E_FREQ  41.20
#define F_FREQ  43.65
#define Fs_FREQ 46.25
#define G_FREQ  49.00
#define Gs_FREQ 51.91
#define A_FREQ  55.00
#define As_FREQ 58.27
#define B_FREQ  61.74


#define Db_FREQ Cs_FREQ
#define Eb_FREQ Ds_FREQ
#define Gb_FREQ Fs_FREQ
#define Ab_FREQ Gs_FREQ
#define Bb_FREQ As_FREQ


#define HANN_FACTOR(length, position) (0.5*(1+cos(2*3.1415926*(position)/(length))))


typedef float SAMPLE;

using namespace std;

typedef  struct
{
    circular_buffer<SAMPLE>& samples_in_left;
    circular_buffer<SAMPLE>& samples_in_right;
    circular_buffer<SAMPLE>& samples_out_left;
    circular_buffer<SAMPLE>& samples_out_right;
    bool isFirstTime;
//    vector<double> samples_in_right;
//    vector<double> samples_in_left;
//    vector<double>::iterator next_sample_right;
//    vector<double>::iterator next_sample_left;
//    float targetFrequency;
} wav_pitch_user_data_t;



float getFundamentalFrequency(circular_buffer<SAMPLE> & samples, unsigned int n_samples);
void stretch(circular_buffer<SAMPLE> & samples_out, circular_buffer<SAMPLE> & samples_in, unsigned int n_samples, float originalFundamentalFrequency, float targetFundamentalFrequency);
float getTargetFundamentalFrequency(float originalFundamentalFrequency);
float autocorrelation_v1(circular_buffer<SAMPLE> &  samples, unsigned int n_samples, unsigned int tau);
float autocorrelation_v2(circular_buffer<SAMPLE> &  samples, unsigned int n_samples, unsigned int tau);
unsigned int getPitchMarkOffset(circular_buffer<SAMPLE> & samples, unsigned int tau);
unsigned int getOutputWindowCenter(int window_index, unsigned int offset, float window_length, float stretch);


/*
bool scale[OCTAVE_SUBDIVISION] =
        {
            true,   // I
            false,  // IIb
            true,   // II
            false,  // IIIb
            true,   // III
            true,   // IV
            false,  // IV#/Vb
            true,   // V
            false,  // V#/VIb
            true,   // VI
            false,  // VIIb
            true    // VII
        };
*/

float scale_fund_freq = C_FREQ;

static int wav_pitch_Callback( const void *inputBuffer, void *outputBuffer,
                               unsigned long framesPerBuffer,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void * userData )
{
    SAMPLE *out = (SAMPLE*)outputBuffer;
    const SAMPLE *in = (const SAMPLE*)inputBuffer;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    wav_pitch_user_data_t * ud = (wav_pitch_user_data_t *) userData;


    if( inputBuffer == NULL )
    {
        for( unsigned int i=0; i<framesPerBuffer; i++ )
        {
            *out++ = 0;  /* left - silent */
            *out++ = 0;  /* right - silent */
        }
//        gNumNoInputs += 1;
    }
    else
    {
//        for (unsigned int i = 0; i < framesPerBuffer; i++)
//        {
//            *out++ = *in++;  /* left */
//            *out++ = *in++;  /* right */
//        }
        for (int i = 0; i < framesPerBuffer; ++i)
        {
            // Only push samples between -1 and 1
            (int)(*in) ? ud->samples_in_left.push_back(0.0) : ud->samples_in_left.push_back(*in);
            in++;
            (int)(*in) ? ud->samples_in_right.push_back(0.0) : ud->samples_in_right.push_back(*in);
            in++;
        }
        if (ud->isFirstTime)
        {
            for (int i = 0; i < framesPerBuffer; ++i)
            {
                *out++ = 0;
                *out++ = 0;
            }
            ud->isFirstTime = false;
        }
        else
        {
            if(ud->samples_out_left.size() >= framesPerBuffer) // If stored, dump previous callback output
            {
                for (int i = 0; i < framesPerBuffer; ++i)
                {
                    ud->samples_out_left.pop_front();
                    ud->samples_out_left.push_back(0.0);
                    ud->samples_out_right.pop_front();
                    ud->samples_out_right.push_back(0.0);

                }
            }

            //TODO: menos distorsion con frecuencias mas chicas porque implica ventanas mas grandes
            float originalFundF = getFundamentalFrequency(ud->samples_in_left, framesPerBuffer);
            float targetFundF = 0.0f;
            if (originalFundF != NAN)
            {
                targetFundF = getTargetFundamentalFrequency(originalFundF);
                stretch(ud->samples_out_left , ud->samples_in_left , framesPerBuffer, originalFundF, targetFundF);
                stretch(ud->samples_out_right, ud->samples_in_right, framesPerBuffer, originalFundF, targetFundF);
            }

            for (int i = 0; i < framesPerBuffer; ++i)
            {
                float sample_out = ud->samples_out_left[i];
                float sample_in = ud->samples_in_left[i];
                float dis = sample_in - sample_out;
                *out++ = ud->samples_in_left[i];
                *out++ = ud->samples_out_right[i];
            }

            //Dump this callback's input
            for (int i = 0; i < framesPerBuffer; ++i)
            {
                ud->samples_in_left.pop_front();
                ud->samples_in_right.pop_front();
            }
        }
    }
    return paContinue;
}


PaError set_wav_pitch_cb(PaStream*& stream, PaStreamParameters& inputParameters, PaStreamParameters& outputParameters, PaError& err)
{
    circular_buffer<SAMPLE> samples_in_left;
    circular_buffer<SAMPLE> samples_in_right;
    circular_buffer<SAMPLE> samples_out_left;
    circular_buffer<SAMPLE> samples_out_right;


    samples_in_left.reserve(2*FRAMES_PER_BUFFER);
    samples_in_right.reserve(2*FRAMES_PER_BUFFER);
    samples_out_left.reserve(2*FRAMES_PER_BUFFER);
    samples_out_right.reserve(2*FRAMES_PER_BUFFER);

    for (int i = 0; i < 2*FRAMES_PER_BUFFER; ++i) {
        samples_out_left.push_back(0.0);
        samples_out_right.push_back(0.0);
    }

    wav_pitch_user_data_t userdata = {samples_in_left, samples_in_right,samples_out_left, samples_out_right, true};


    err = Pa_OpenStream(
            &stream,
            &inputParameters,
            &outputParameters,
            SAMPLE_RATE,
            FRAMES_PER_BUFFER,
            0, /* paClipOff, */  /* we won't output out of range samples so don't bother clipping them */
            wav_pitch_Callback,
            (void *) &userdata );
    if( err == paNoError )
    {
        err = Pa_StartStream( stream );
        if(err == paNoError)
        {
            printf("Hit ENTER to stop program.\n");
            getchar();
        }
    }
    return err;
}

//TODO: si queda muy bajas las frecuencias, limitar el tau maximo
float getFundamentalFrequency(circular_buffer<SAMPLE>& samples, unsigned int n_samples)
{
    float max_autocorrelation = 0.0;
    float freq = NAN;
    float autocorrelation = NAN;
    //tau_min = fs/f_fund_max
    for (int tau = 20; tau < n_samples - 300; ++tau)    //TODO: sacar magic number. si aumenta, no distingo frecuencias mas bajas pero es mas rapido y no se traba
    {
        autocorrelation = autocorrelation_v1(samples, n_samples, tau);
        if(autocorrelation > max_autocorrelation)
        {
            max_autocorrelation = autocorrelation;
            freq = ((float)SAMPLE_RATE)/((float)tau);
        }
    }
    return freq;
}

float autocorrelation_v1(circular_buffer<SAMPLE>& samples, unsigned int n_samples, unsigned int tau)
{
    float autocorrelation = 0.0;
    for (int i = 1; i < n_samples; ++i)
    {
        float a = samples[i];
        float b = samples[i+tau];
        autocorrelation += a*b;
    }
    return autocorrelation;
}

float autocorrelation_v2(circular_buffer<SAMPLE>& samples, unsigned int n_samples, unsigned int tau)
{
    float autocorrelation = 0.0;
    for (int i = 1; i < n_samples - tau; ++i)
    {
        autocorrelation += samples[i]*samples[i+tau];
    }
    return autocorrelation;
}

float getTargetFundamentalFrequency(float originalFundamentalFrequency)
{
    float aux = (log2f(originalFundamentalFrequency / FUND_FREQ));
    int octave = (int)aux;
    int note = (int)roundf(OCTAVE_SUBDIVISION*(aux-octave));  // note in octave (from 0 (fundamental) to OCTAVE_SUBDIVISION - 1)
    return GET_FREQ(octave, 0);
}
/*

void stretch(circular_buffer<SAMPLE> & samples_out, circular_buffer<SAMPLE> & samples_in, unsigned int n_samples, float originalFundamentalFrequency, float targetFundamentalFrequency)
{
    float window_length =  SAMPLE_RATE/originalFundamentalFrequency*2;
    unsigned int offset = getPitchMarkOffset(samples_in, (unsigned int)window_length);
    int window_center_in = offset;
    float stretch = 1.5;//targetFundamentalFrequency/originalFundamentalFrequency;
    int window_center_out = getOutputWindowCenter(0, offset, window_length, stretch);


    for(int i = 0, counter=0;   // i: samples_in. counter: samples_out.
//        window_center_out <= n_samples;
        window_center_out < n_samples + window_length;
        ++i, ++counter)
    {
        window_center_in = offset + i * window_length / 2;
        window_center_out = getOutputWindowCenter(counter, offset, window_length, stretch);


        for (int j = -window_length/2;
                 j <= window_length/2;
                 j++)
        {
            if((window_center_in + j) >= 0 && (window_center_out + j) >=0)
            {
                float hann = HANN_FACTOR(window_length, j);
                unsigned int index = window_center_out + j;
                float value = samples_in[index];
                float stored = samples_out[index];
                samples_out[index] += value*hann/stretch;
            }
        }
        if(counter % 3 == 0)
        {
            i--;
        }
    }
}

*/

void stretch(circular_buffer<SAMPLE> & samples_out, circular_buffer<SAMPLE> & samples_in, unsigned int n_samples, float originalFundamentalFrequency, float targetFundamentalFrequency)
{
    float window_length =  SAMPLE_RATE/originalFundamentalFrequency;
    unsigned int offset = getPitchMarkOffset(samples_in, (unsigned int)window_length);
    int window_center_in = offset;
    float stretch = 0.9;//targetFundamentalFrequency/originalFundamentalFrequency;
    int window_center_out = getOutputWindowCenter(0, offset, window_length, stretch);


    for(int i = 0, counter=0;   // i: samples_in. counter: samples_out.
//        window_center_out <= n_samples;
        window_center_out < n_samples + window_length;
        ++counter)
    {
        window_center_in = offset + i * window_length / 2;
        window_center_out = getOutputWindowCenter(counter, offset, window_length, stretch);


        for (int j = -window_length/2;
             j <= window_length/2;
             j++)
        {
            if((window_center_in + j) >= 0 && (window_center_out + j) >=0)
            {
                float hann = HANN_FACTOR(window_length, j);
                unsigned int index = window_center_out + j;
                float value = samples_in[index];
                float stored = samples_out[index];
                samples_out[index / stretch] += value*hann/stretch;
            }
        }
//        if(counter % 2 == 0)
        {
            i = (int)((float)counter / stretch);
        }
    }
}


unsigned int getPitchMarkOffset(circular_buffer<SAMPLE> & samples, unsigned int tau)
{
    unsigned int offset = 0;    //por default
    float max_sample = 0.0;
    for (int i = 0; i < tau; ++i)
    {
        if(fabs(samples[i]) > max_sample)
        {
            offset = i;
            max_sample = samples[i];
        }
    }
    return offset;
}

unsigned int getOutputWindowCenter(int window_index, unsigned int offset, float window_length, float stretch)
{
    return (unsigned int)((offset + window_index * window_length / 2)/stretch);
}