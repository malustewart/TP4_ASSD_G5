//
// Created by mlste on 5/20/2019.
//

#include "set_wav_pitch_cb.h"
#include "circular_buffer.h"
#include "AudioFile.h"
#include "aubio\aubio.h"


#include <stdio.h>
#include <math.h>
#include <fstream>
#include <vector>

#define FRAMES_PER_BUFFER   (1024)

#define FREC_FUND_MIN       (100)
#define FREC_FUND_MAX       (700)

#define GET_FREQ(f,o,n)       (f * pow(2, ((float)(n) + OCTAVE_SUBDIVISION*(float)(octave))/OCTAVE_SUBDIVISION))

#define OCTAVES             (5)
#define OCTAVE_SUBDIVISION  (12)
//#define FUND_FREQ C_FREQ

#define MIN_FREQ C_FREQ
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


int SAMPLE_RATE = 44100;

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

	int prev_tau;

	/******************/
	// FOR ALVIN
	float stretch;

	// FOR DUKI
	float scale_fund_freq; 
	bool scale[OCTAVE_SUBDIVISION];
	
	/******************/
	// FOR NON REAL TIME
	ofstream * freq_det_bin;
	ofstream * freq_obj_bin;
	const char * wav_filename;
	const char * out_suffix;
	const char * bin_suffix;
	const char * freq_det_suffix;
	const char * freq_obj_suffix;

	// FOR REAL TIME

	/******************/


} wav_pitch_user_data_t;

typedef float(*autocorrelator_t)(circular_buffer<SAMPLE>& samples, unsigned int n_samples, unsigned int tau);




float get_frequency_by_autocorrelation(circular_buffer<SAMPLE>& samples, unsigned int n_samples, autocorrelator_t autocorrelator, int& prev_tau);
void stretch(circular_buffer<SAMPLE> & samples_out, circular_buffer<SAMPLE> & samples_in, unsigned int n_samples, float originalFundamentalFrequency, float targetFundamentalFrequency);
float getTargetFundamentalFrequency(float originalFundamentalFrequency, wav_pitch_user_data_t * ud);
float autocorrelation_v1(circular_buffer<SAMPLE> &  samples, unsigned int n_samples, unsigned int tau);
float autocorrelation_v2(circular_buffer<SAMPLE> &  samples, unsigned int n_samples, unsigned int tau);
unsigned int getPitchMarkOffset(circular_buffer<SAMPLE> & samples, unsigned int tau);
int getOutputWindowCenter(int window_index, unsigned int offset, float window_length, float stretch);




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

            float originalFundF = ud->freq_detector(*(ud->samples_in_left), framesPerBuffer, ud->prev_tau);
			int originalFundF_int = 0, targetFundF_int = 0;
            float targetFundF = 0.0f;
            if (!isnan(originalFundF))
            {
				targetFundF = getTargetFundamentalFrequency(originalFundF, ud);
                stretch(*(ud->samples_out_left) , *(ud->samples_in_left) , framesPerBuffer, originalFundF, targetFundF);
                stretch(*(ud->samples_out_right), *(ud->samples_in_right), framesPerBuffer, originalFundF, targetFundF);
			
				originalFundF_int = (int)originalFundF; targetFundF_int = (int)targetFundF;
			}

			ud->freq_det_bin->write((char*)&originalFundF_int, sizeof(int));
			ud->freq_obj_bin->write((char*)&targetFundF_int, sizeof(int));


            for (int i = 0; i < framesPerBuffer; ++i)
            {
                *out++ = (*ud->samples_out_left) [i];
                *out++ = (*ud->samples_out_right)[i];
            }

            //Dump previous callback's input
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
	set_wav_user_data(userdata, WAV_FILE, "_freq", "_out", "_det","_obj" );
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

	int n_samples = wav_manager.getNumSamplesPerChannel();

	if (n_samples == 0) {
		cout << file_name << " not found." << endl;
		return;
	}

	SAMPLE_RATE = wav_manager.getSampleRate();

	cout << "\n" << file_name << endl;

	wav_manager.printSummary();



	float * output_samples = new float[2 * n_samples + FRAMES_PER_BUFFER];
	float * input_samples = new float[2 * n_samples + FRAMES_PER_BUFFER];
	float * input_samples_aux = input_samples;
	float * output_samples_aux = output_samples;



	// El output siempre es stereo pero el input puede ser mono
	// Si el input es mono, utilizar el unico canal de entrada para los dos canales de salida.
	// Si el input es stereo, usar el primer canal de entrada para el primer canal de salida, 
	// y el segundo canal de enrada para el segundo de salida
	int output_channel_1_source = 0;
	int output_channel_2_source = wav_manager.getNumChannels() - 1;


	//Load input samples
	for (int i = 0; i < n_samples; i++)
	{
		*input_samples_aux++ = wav_manager.samples[output_channel_1_source][i];
		*input_samples_aux++ = wav_manager.samples[output_channel_2_source][i];
	}

	// Process wav
	PaStreamCallbackFlags statusFlags = 0;
	for (int i = 0; i < n_samples / (FRAMES_PER_BUFFER); i++)
	{
		process_window((const void *)(input_samples + i * FRAMES_PER_BUFFER * 2),
			output_samples + i * FRAMES_PER_BUFFER * 2,
			FRAMES_PER_BUFFER,
			nullptr,
			statusFlags,
			(void*)(userdata));
	}

	// Store new wav
	for (int i = 0; i < n_samples; i++)
	{
		wav_manager.samples[output_channel_1_source][i] = *output_samples_aux++;
		wav_manager.samples[output_channel_2_source][i] = *output_samples_aux++;
	}

	file_name = std::string(userdata->wav_filename);
	file_name += userdata->out_suffix;
	file_name += WAV_EXTENSION;

	wav_manager.save(file_name.c_str());
	delete output_samples;
	delete input_samples;
	delete_user_data(userdata);
}

//todo: hacer con SAMPLE * en vez de circular_buffer<SAMPLE>&
float get_frequency_by_autocorrelation_v2(circular_buffer<SAMPLE>& samples, unsigned int n_samples, int& prev_tau)
{
	return get_frequency_by_autocorrelation(samples, n_samples, autocorrelation_v2, prev_tau);
}

float get_frequency_by_autocorrelation_v1(circular_buffer<SAMPLE>& samples, unsigned int n_samples, int& prev_tau)
{
	return get_frequency_by_autocorrelation(samples, n_samples, autocorrelation_v1, prev_tau);
}

float get_frequency_by_yin(circular_buffer<SAMPLE>& samples, unsigned int n_samples, int& prev_tau)
{
	float freq;
	// 1. allocate some memory
	uint_t n = 0; // frame counter
	uint_t win_s = n_samples * 4; // window size
	uint_t hop_s = n_samples; // hop size

							   // create some vectors
	fvec_t *input = new_fvec(hop_s); // input buffer
	fvec_t *out = new_fvec(1); // output candidates
							   // create pitch object
	aubio_pitch_t *o = new_aubio_pitch("default", win_s, hop_s, SAMPLE_RATE);
	 
	for (size_t i = 0; i < hop_s; i++)
	{
		input->data[i] = samples[i];
	}

	// 2. do something with it
	// get `hop_s` new samples into `input`
	// ...
	// exectute pitch
	aubio_pitch_do(o, input, out);


	freq = out->data[0];
	
	// 3. clean up memory
	del_aubio_pitch(o);
	del_fvec(out);
	del_fvec(input);
	aubio_cleanup();

	return freq;
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

	userdata->freq_det_bin = nullptr;
	userdata->freq_obj_bin = nullptr;
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

wav_pitch_user_data_t * set_wav_user_data(wav_pitch_user_data_t * ud, const char * filename, const char * bin_suffix, const char * out_suffix, const char * freq_det_suffix, const char * freq_obj_suffix)
{
	ud->prev_tau = -1;

	ud->bin_suffix = bin_suffix;
	ud->out_suffix = out_suffix;
	ud->wav_filename = filename;
	ud->freq_det_suffix = freq_det_suffix;
	ud->freq_obj_suffix = freq_obj_suffix;

	std::string freq_det_file_name(filename);
	freq_det_file_name += out_suffix;
	freq_det_file_name += bin_suffix;
	freq_det_file_name += freq_det_suffix;
	freq_det_file_name += ".bin";
	ud->freq_det_bin = new ofstream(freq_det_file_name.c_str(), ios::binary | ios::out);

	std::string freq_obj_file_name(filename);
	freq_obj_file_name += out_suffix;
	freq_obj_file_name += bin_suffix;
	freq_obj_file_name += freq_obj_suffix;
	freq_obj_file_name += ".bin";
	ud->freq_obj_bin = new ofstream(freq_obj_file_name.c_str(), ios::binary | ios::out);
	
	return ud;
}

void set_alvin_user_data(wav_pitch_user_data_t * ud, float stretch)
{
	ud->is_alvin = true;
	ud->stretch = stretch;
}

void set_duki_user_data(wav_pitch_user_data_t* ud, scale_t scale_)
{
	ud->is_alvin = false;
	for (int i = 0; i < OCTAVE_SUBDIVISION; i++)
	{
		ud->scale[i] = scale_.scale_octave[i];
	}
	ud->scale_fund_freq = SelectedFundFrec(scale_.note_number);
}

float SelectedFundFrec(int note)
{
	float fundFreq;
	switch (note)
	{
	case DO:
		fundFreq = C_FREQ;
		break;
	case DOs:
		fundFreq = Cs_FREQ;
		break;
	case RE:
		fundFreq = D_FREQ;
		break;
	case REs:
		fundFreq = Ds_FREQ;
		break;
	case MI:
		fundFreq = E_FREQ;
		break;
	case FA:
		fundFreq = F_FREQ;
		break;
	case FAs:
		fundFreq = Fs_FREQ;
		break;
	case SOL:
		fundFreq = G_FREQ;
		break;
	case SOLs:
		fundFreq = Gs_FREQ;
		break;
	case LA:
		fundFreq = A_FREQ;
		break;
	case LAs:
		fundFreq = As_FREQ;
		break;
	case SI:
		fundFreq = B_FREQ;
		break;
	default:
		break;
	}
	return fundFreq;
}

void delete_user_data(wav_pitch_user_data_t * ud)
{
	if (ud->freq_det_bin) 
	{ 
		ud->freq_det_bin->close();
		delete ud->freq_det_bin;
	}
	if (ud->freq_obj_bin)
	{
		ud->freq_obj_bin->close();
		delete ud->freq_obj_bin;
	}
	delete ud->samples_in_left;
	delete ud->samples_in_right;
	delete ud->samples_out_left;
	delete ud->samples_out_right;

	delete ud;

	return;
}

float get_frequency_by_autocorrelation(circular_buffer<SAMPLE>& samples, unsigned int n_samples, autocorrelator_t autocorrelator, int& prev_tau)
{
	float max_autocorrelation = 0.0;
	float freq = NAN;
	float autocorrelation = NAN;
	int tau_min = (double)SAMPLE_RATE / (double)FREC_FUND_MAX;
	int tau_max = (double)SAMPLE_RATE / (double)FREC_FUND_MIN;
	int current_best_tau = tau_min;
	for (int tau = tau_min; tau < tau_max; tau++)
	{
		float window_factor = 1; //prev_tau == -1 ? 1 : 0.5 + (0.5*HANN_FACTOR(0.4 * prev_tau, tau));
		autocorrelation = autocorrelator(samples, n_samples, tau) * window_factor;
		if (autocorrelation > max_autocorrelation)
		{
			max_autocorrelation = autocorrelation;
			current_best_tau = tau;
		}
	}
	
	prev_tau = current_best_tau;
	return ((float)SAMPLE_RATE) / ((float)current_best_tau);
	//debug:
	//return 440;
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
	if (originalFundamentalFrequency < MIN_FREQ)
	{
		return originalFundamentalFrequency;
	}

    float aux = (log2f(originalFundamentalFrequency / ud->scale_fund_freq));
    int octave = (int)aux;
    int note = (int)roundf(OCTAVE_SUBDIVISION*(aux-octave));  // note in octave (from 0 (fundamental) to OCTAVE_SUBDIVISION - 1)
	if (note == OCTAVE_SUBDIVISION)
	{
		note = 0;
		octave++;
	}



	//si la nota esta permitida
	if (ud->scale[note])
	{
		//cout << note << " " << note << endl;
		return GET_FREQ(ud->scale_fund_freq, octave, note);
	}

	//si tengo que buscar otra:
	int min_dist = INT_MAX;
	int target_note = note;
	int octave_offset = 0;

	for (int i = -1; -OCTAVE_SUBDIVISION / 2 + 1 <= i; i--)
	{
		// tengo cuidado con que el modulo me puede dar negativo
		int current_note = ((note + i % OCTAVE_SUBDIVISION) + OCTAVE_SUBDIVISION) % OCTAVE_SUBDIVISION;
		if (ud->scale[current_note])
		{
			min_dist = -i;	// guardo el valor en positivo
			target_note = current_note;
			if (note + i < 0)
				octave_offset = -1;
			break;
		}
	}
	for (int i = 1; i <= OCTAVE_SUBDIVISION / 2 + 1 ; i++)
	{
		// el modulo siempre va a dar positivo, no tengo que poner las operaciones extras
		int current_note = (note + i) % OCTAVE_SUBDIVISION;
		if (ud->scale[current_note])
		{
			if (i < min_dist)
			{
				min_dist = i;
				target_note = current_note;
			}
			break;
			if (note + i >= OCTAVE_SUBDIVISION)
				octave_offset = +1;
		}
	}
	//cout << note << " " << target_note << endl;
	return GET_FREQ(ud->scale_fund_freq, octave + octave_offset, target_note);

															  
/*    if(!ud->scale[note])	//todo: scale sacarlo de ud
    {
        float dif = OCTAVE_SUBDIVISION*(aux-octave) - roundf(OCTAVE_SUBDIVISION*(aux-octave));
        if( dif>0)  //to next allowed frequency
        {
            while (!ud->scale[note])
            {
                note = ++note % OCTAVE_SUBDIVISION;
                if(!note){octave++;};   //if note=0, octave has been increased
            }
        } else {    //to previous allowed frequency
            while (!ud->scale[note])
            {
                if(note) { note--; }  // if note is not first in octave
                else{ note = OCTAVE_SUBDIVISION - 1; if(octave) {octave--;}};   //
            }
        }
    }
    return GET_FREQ(ud->scale_fund_freq,octave, note);*/

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
    unsigned int offset = getPitchMarkOffset(samples_in, (unsigned int)(SAMPLE_RATE/originalFundamentalFrequency));
    
	float stretch = targetFundamentalFrequency/originalFundamentalFrequency;

	float window_length_in = 2 * SAMPLE_RATE / originalFundamentalFrequency;
	float window_length_out = window_length_in / stretch;

    int window_center_out = 0;	//solo se le da valor para poder entrar al for (ver la condicion de salida)
    int window_center_in = offset;
	
	int window_index_in, window_index_out;

	int buff_fade_len = n_samples / 16;

    for(window_index_out =  - (buff_fade_len / (window_length_out / 2) + 2);   // window_index_in: samples_in. window_index_out: samples_out.
        window_center_out - window_length_out / 2.0 < n_samples;
        ++window_index_out)
    {
        window_index_in = round((float)(window_index_out + 0.1) / stretch); //todo: por que round y no casteo a int?

        window_center_in = offset + window_index_in * window_length_in / 2;
        window_center_out = getOutputWindowCenter(window_index_out, offset, window_length_in, stretch);

		
        for (int j = -window_length_out/2.0;
             j <= window_length_out/2.0;
             j++)
        {     
			int sample_pos_out = window_center_out + j;
			if ( -buff_fade_len <= sample_pos_out && sample_pos_out < (int)n_samples)
			{
				int sample_pos_in = window_center_in + j * stretch;
				float sample = samples_in[n_samples + sample_pos_in];
				sample *= HANN_FACTOR(window_length_in, j*stretch);
				
				// buffer fade:
				if (sample_pos_out < 0)
					sample *= HANN_FACTOR(buff_fade_len * 2, sample_pos_out);
				else if (sample_pos_out > (int) n_samples - buff_fade_len)
					sample *= HANN_FACTOR(buff_fade_len * 2, sample_pos_out - (n_samples - buff_fade_len));

				samples_out[n_samples + window_center_out + j] += sample;
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
		float sample = samples[i];
        if(samples[i] > max_sample)
        {
            offset = i;
            max_sample = samples[i];
        }
    }
    return offset;
	//debug:
	//return 0;
}

int getOutputWindowCenter(int window_index, unsigned int offset, float window_length, float stretch)
{
    return (int)(((int)offset + window_index * window_length / 2)/stretch);
}
