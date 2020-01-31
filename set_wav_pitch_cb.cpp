//
// Created by mlste on 5/20/2019.
//

#include "set_wav_pitch_cb.h"
#include "circular_buffer.h"
#include "AudioFile.h"
#include <stdio.h>
#include <math.h>
#include <fstream>
#include <vector>

#define SAMPLE_RATE         (44100)
#define FRAMES_PER_BUFFER   (1024)

#define FREC_FUND_MIN       (100)
#define FREC_FUND_MAX       (700)

#define GET_FREQ(o,n)       (FUND_FREQ * pow(2, ((float)(n) + OCTAVE_SUBDIVISION*(float)(octave))/OCTAVE_SUBDIVISION))

#define OCTAVES             (5)
#define OCTAVE_SUBDIVISION  (12)
#define FUND_FREQ C_FREQ
#define C_FREQ  32.7
#define Cs_FREQ 34.65
#define D_FREQ  36.71
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

using namespace std;

typedef float SAMPLE;

typedef struct wav_pitch_user_data_t
{
    circular_buffer<SAMPLE> * samples_in_left;
    circular_buffer<SAMPLE> * samples_in_right;
    circular_buffer<SAMPLE> * samples_out_left;
    circular_buffer<SAMPLE> * samples_out_right;
    bool isFirstTime;

	freq_detector_t freq_detector;
	bool is_real_time;
	bool is_alvin;

	/******************/
	// FOR ALVIN
	float stretch;

	// FOR DUKI
	//todo: scale o puntero a funcion que devuelva target fundamental frequency
	
	/******************/
	// FOR NON REAL TIME
	ofstream * datafile;
	const char * wav_filename;
	const char * out_suffix;
	const char * bin_suffix;
	const char * freq_det_suffix;

	// FOR REAL TIME

	/******************/


} wav_pitch_user_data_t;

typedef float(*autocorrelator_t)(circular_buffer<SAMPLE>& samples, unsigned int n_samples, unsigned int tau);



float get_frequency_by_autocorrelation(circular_buffer<SAMPLE>& samples, unsigned int n_samples, autocorrelator_t autocorrelator);
void stretch(circular_buffer<SAMPLE> & samples_out, circular_buffer<SAMPLE> & samples_in, unsigned int n_samples, float originalFundamentalFrequency, float targetFundamentalFrequency);
float getTargetFundamentalFrequency(float originalFundamentalFrequency, wav_pitch_user_data_t * ud);
float autocorrelation_v1(circular_buffer<SAMPLE> &  samples, unsigned int n_samples, unsigned int tau);
float autocorrelation_v2(circular_buffer<SAMPLE> &  samples, unsigned int n_samples, unsigned int tau);
unsigned int getPitchMarkOffset(circular_buffer<SAMPLE> & samples, unsigned int tau);
unsigned int getOutputWindowCenter(int window_index, unsigned int offset, float window_length, float stretch);


bool scale[OCTAVE_SUBDIVISION] =
        {
            true,   // I
            false,  // IIb
            true,   // II
            false,  // IIIb
            true,   // III
            false,  // IV
            true,   // IV#/Vb
            false,  // V
            true,   // V#/VIb
            false,  // VI
            true,   // VIIb
            false   // VII
        };

float scale_fund_freq = C_FREQ;



static int process_window( const void *inputBuffer, void *outputBuffer,
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
        for( unsigned int i=0; i < framesPerBuffer; i++ )
        {
            *out++ = 0;  /* left - silent */
            *out++ = 0;  /* right - silent */
        }
    }
    else
    {

//Push samples to buffer. If real time, use portaudio buffer for source. Else, use user defined buffer.
// Only push samples between -1 and 1
        for (int i = 0; i < framesPerBuffer; ++i)
        {
            (int)(*in) ? ud->samples_in_left->push_back(0.0) : ud->samples_in_left->push_back(*in);
            in++;
            (int)(*in) ? ud->samples_in_right->push_back(0.0) : ud->samples_in_right->push_back(*in);
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

            if(ud->samples_out_left->size() >= framesPerBuffer) // If stored, dump previous callback output
            {
                for (int i = 0; i < framesPerBuffer; ++i)
                {
                    ud->samples_out_left->pop_front();
                    ud->samples_out_left->push_back(0.0);
                    ud->samples_out_right->pop_front();
                    ud->samples_out_right->push_back(0.0);

                }
            }

            float originalFundF = get_frequency_by_autocorrelation_v1(*(ud->samples_in_left), framesPerBuffer);
            int originalFundF_int = 0;
            float targetFundF = 0.0f;
            if (!isnan(originalFundF))
            {
                originalFundF_int = (int)originalFundF;
                targetFundF = getTargetFundamentalFrequency(originalFundF, ud);
                stretch(*(ud->samples_out_left) , *(ud->samples_in_left) , framesPerBuffer, originalFundF, targetFundF);
                stretch(*(ud->samples_out_right), *(ud->samples_in_right), framesPerBuffer, originalFundF, targetFundF);
            }
            ud->datafile->write((char*)&originalFundF_int, sizeof(int));


            for (int i = 0; i < framesPerBuffer; ++i)
            {
                *out++ = ((*ud->samples_out_left)[i] + (*ud->samples_out_left)[i+1])/2;
                *out++ = (*ud->samples_out_right)[i];
            }

            //Dump this callback's input
            for (int i = 0; i < framesPerBuffer; ++i)
            {
                ud->samples_in_left->pop_front();
                ud->samples_in_right->pop_front();
            }
        }
    }
    return paContinue;
}




PaError set_wav_pitch_cb(PaStream*& stream,
	PaStreamParameters& inputParameters,
	PaStreamParameters& outputParameters,
	PaError& err)
{

	wav_pitch_user_data_t * userdata = create_user_data();
	set_wav_user_data(userdata, WAV_FILE, "_freq", "_out", "_v1");
#ifdef USE_WAV
	AudioFile<float> wav_manager;
	std::string file_name(WAV_FILE);
	file_name += WAV_EXTENSION;

	wav_manager.load(file_name.c_str());
	wav_manager.printSummary();

	int n_samples = wav_manager.getNumSamplesPerChannel();


	float * output_samples = new float[2 * n_samples + FRAMES_PER_BUFFER];
	float * input_samples = new float[2 * n_samples + FRAMES_PER_BUFFER];
	float * input_samples_aux = input_samples;
	float * output_samples_aux = output_samples;



	//Load input samples in callback-friendly buffer format
	for (int i = 0; i < n_samples; i++)
	{
		*input_samples_aux++ = wav_manager.samples[0][i];
		*input_samples_aux++ = wav_manager.samples[1][i];
	}

	PaStreamCallbackFlags statusFlags = 0;

	// Process wav
	for (int i = 0; i < n_samples / (FRAMES_PER_BUFFER / 2); i++)
	{
		process_window((const void *)(input_samples + i * FRAMES_PER_BUFFER),
			output_samples + i * FRAMES_PER_BUFFER,
			FRAMES_PER_BUFFER,
			nullptr,
			statusFlags,
			(void*)(userdata));
	}

	// Store new wav
	for (int i = 0; i < n_samples; i++)
	{
		wav_manager.samples[0][i] = *output_samples_aux++;
		wav_manager.samples[1][i] = *output_samples_aux++;
	}

	file_name = std::string(userdata->wav_filename);
	file_name += userdata->out_suffix;
	file_name += userdata->freq_det_suffix;
	file_name += WAV_EXTENSION;

	wav_manager.save(file_name.c_str());
	delete output_samples;
	delete input_samples;
#else

	char control = 0;

	err = Pa_OpenStream(
		&stream,
		&inputParameters,
		&outputParameters,
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		0, /* paClipOff, */  /* we won't output out of range samples so don't bother clipping them */
		process_window,
		(void *)userdata);
	if (err == paNoError)
	{
		err = Pa_StartStream(stream);
		//       getchar();
	}
#endif

	return err;
}



void process_wav(wav_pitch_user_data_t * userdata)
{
	AudioFile<float> wav_manager;
	std::string file_name(userdata->wav_filename);
	file_name += WAV_EXTENSION;

	wav_manager.load(file_name.c_str());
	wav_manager.printSummary();

	int n_samples = wav_manager.getNumSamplesPerChannel();

	float * output_samples = new float[2 * n_samples + FRAMES_PER_BUFFER];
	float * input_samples = new float[2 * n_samples + FRAMES_PER_BUFFER];
	float * input_samples_aux = input_samples;
	float * output_samples_aux = output_samples;

	//Load input samples
	for (int i = 0; i < n_samples; i++)
	{
		*input_samples_aux++ = wav_manager.samples[0][i];
		*input_samples_aux++ = wav_manager.samples[1][i];
	}

	// Process wav
	PaStreamCallbackFlags statusFlags = 0;
	for (int i = 0; i < n_samples / (FRAMES_PER_BUFFER / 2); i++)
	{
		process_window((const void *)(input_samples + i * FRAMES_PER_BUFFER),
			output_samples + i * FRAMES_PER_BUFFER,
			FRAMES_PER_BUFFER,
			nullptr,
			statusFlags,
			(void*)(userdata));
	}

	// Store new wav
	for (int i = 0; i < n_samples; i++)
	{
		wav_manager.samples[0][i] = *output_samples_aux++;
		wav_manager.samples[1][i] = *output_samples_aux++;
	}

	file_name = std::string(userdata->wav_filename);
	file_name += userdata->out_suffix;
	file_name += userdata->freq_det_suffix;
	file_name += WAV_EXTENSION;

	wav_manager.save(file_name.c_str());
	delete output_samples;
	delete input_samples;
	delete_user_data(userdata);
}

//todo: hacer con SAMPLE * en vez de circular_buffer<SAMPLE>&
float get_frequency_by_autocorrelation_v2(circular_buffer<SAMPLE>& samples, unsigned int n_samples)
{
	return get_frequency_by_autocorrelation(samples, n_samples, autocorrelation_v2);
}

float get_frequency_by_autocorrelation_v1(circular_buffer<SAMPLE>& samples, unsigned int n_samples)
{
	return get_frequency_by_autocorrelation(samples, n_samples, autocorrelation_v1);
}

wav_pitch_user_data_t * create_user_data(freq_detector_t freq_detector)
{
	circular_buffer<SAMPLE> * samples_in_left = new circular_buffer<SAMPLE>;
	circular_buffer<SAMPLE> * samples_in_right = new circular_buffer<SAMPLE>;
	circular_buffer<SAMPLE> * samples_out_left = new circular_buffer<SAMPLE>;
	circular_buffer<SAMPLE> * samples_out_right = new circular_buffer<SAMPLE>;

	samples_in_left->reserve(2 * FRAMES_PER_BUFFER);
	samples_in_right->reserve(2 * FRAMES_PER_BUFFER);
	samples_out_left->reserve(2 * FRAMES_PER_BUFFER);
	samples_out_right->reserve(2 * FRAMES_PER_BUFFER);

	for (int i = 0; i < 2 * FRAMES_PER_BUFFER; ++i)
	{
		samples_out_left->push_back(0.0);
		samples_out_right->push_back(0.0);
	}

	wav_pitch_user_data_t * userdata = new wav_pitch_user_data_t;

	userdata->samples_in_left = samples_in_left;
	userdata->samples_in_right = samples_in_right;
	userdata->samples_out_left = samples_out_left;
	userdata->samples_out_right = samples_out_right;

	userdata->datafile = nullptr;
	userdata->wav_filename = nullptr;
	userdata->bin_suffix = nullptr;
	userdata->out_suffix = nullptr;

	userdata->freq_detector = freq_detector;
	userdata->isFirstTime = true;

	userdata->is_alvin = true;	//por default
	userdata->stretch = 1.0;	//por default
	
	userdata->is_real_time = true;

	return userdata;
}

wav_pitch_user_data_t * set_wav_user_data(wav_pitch_user_data_t * ud, const char * filename, const char * bin_suffix, const char * out_suffix, const char * freq_det_suffix)
{
	ud->bin_suffix = bin_suffix;
	ud->out_suffix = out_suffix;
	ud->wav_filename = filename;
	ud->freq_det_suffix = freq_det_suffix;

	std::string file_name(filename);
	file_name += bin_suffix;
	file_name += freq_det_suffix;
	file_name += ".bin";

	ud->datafile = new ofstream(file_name.c_str(), ios::binary | ios::out);

	return ud;
}

wav_pitch_user_data_t * set_alvin_user_data(wav_pitch_user_data_t * ud, float stretch)
{
	ud->is_alvin = true;
	ud->stretch = stretch;
	return nullptr;
}

void delete_user_data(wav_pitch_user_data_t * ud)
{
	if (ud->datafile) 
	{ 
		ud->datafile->close();
		delete ud->datafile;
	}
	delete ud->samples_in_left;
	delete ud->samples_in_right;
	delete ud->samples_out_left;
	delete ud->samples_out_right;

	delete ud;

	return;
}

float get_frequency_by_autocorrelation(circular_buffer<SAMPLE>& samples, unsigned int n_samples, autocorrelator_t autocorrelator)
{
	float max_autocorrelation = 0.0;
	float freq = NAN;
	float autocorrelation = NAN;
	int tau_min = (double)SAMPLE_RATE / (double)FREC_FUND_MAX;
	int tau_max = (double)SAMPLE_RATE / (double)FREC_FUND_MIN;
	int current_best_tau = tau_min;
	for (int tau = tau_min; tau < tau_max; tau++)
	{
		autocorrelation = autocorrelator(samples, n_samples, tau);
		if (autocorrelation > max_autocorrelation)
		{
			max_autocorrelation = autocorrelation;
			current_best_tau = tau;
		}
	}
	return ((float)SAMPLE_RATE) / ((float)current_best_tau);
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

float getTargetFundamentalFrequency(float originalFundamentalFrequency, wav_pitch_user_data_t * ud)
{
    if(isnan(originalFundamentalFrequency))
    {
        return originalFundamentalFrequency;
    }
	if (ud->is_alvin)
	{
		return originalFundamentalFrequency * ud->stretch;
	}
    float aux = (log2f(originalFundamentalFrequency / FUND_FREQ));
    int octave = (int)aux;
    int note = (int)roundf(OCTAVE_SUBDIVISION*(aux-octave));  // note in octave (from 0 (fundamental) to OCTAVE_SUBDIVISION - 1)
    if(!scale[note])	//todo: scale sacarlo de ud
    {
        float dif = OCTAVE_SUBDIVISION*(aux-octave) - roundf(OCTAVE_SUBDIVISION*(aux-octave));
        if( dif>0)  //to next allowed frequency
        {
            while (!scale[note])
            {
                note = ++note % OCTAVE_SUBDIVISION;
                if(!note){octave++;};   //if note=0, octave has been increased
            }
        } else {    //to previous allowed frequency
            while (!scale[note])
            {
                if(note) { note--; }  // if note is not first in octave
                else{ note = OCTAVE_SUBDIVISION - 1; if(octave) {octave--;}};   //
            }
        }
    }
    return GET_FREQ(octave, note);
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

//todo: check for nan
void stretch(circular_buffer<SAMPLE> & samples_out, circular_buffer<SAMPLE> & samples_in, unsigned int n_samples, float originalFundamentalFrequency, float targetFundamentalFrequency)
{
    float window_length =  SAMPLE_RATE/originalFundamentalFrequency;
    unsigned int offset = getPitchMarkOffset(samples_in, (unsigned int)window_length);
    float stretch = targetFundamentalFrequency/originalFundamentalFrequency;
    int window_center_out = getOutputWindowCenter(0, offset, window_length, stretch);
    int window_center_in = offset;


    //todo: por que arranca en cero y no -1? es porque ya lo cubre el extra del callback pasado?
    for(int window_in_index = 0, window_out_index=0;   // window_in_index: samples_in. window_out_index: samples_out.
//        window_center_out <= n_samples;
        window_center_out < n_samples + window_length;
        ++window_out_index)
    {
        window_in_index = round((float)(window_out_index + 0.1) / stretch); //todo: por que round y no casteo a int?

        window_center_in = offset + window_in_index * window_length / 2;
        window_center_out = getOutputWindowCenter(window_out_index, offset, window_length, stretch);

        for (int j = -window_length/2.0/stretch;
             j <= window_length/2.0/stretch;
             j++)
        {
            if((window_center_in + j*stretch) >= 0 && (window_center_out + j) >=0)
            {
                samples_out[window_center_out + j] += samples_in[window_center_in + j*stretch]*HANN_FACTOR(window_length, j*stretch);
            }
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
