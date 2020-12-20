#include <iostream>
#include <string>
#include "portaudio.h"
#include "set_wav_pitch_cb.h"

#include "gui.h"

#include <fstream>
#include "main.h"

#define PA_SAMPLE_TYPE  paFloat32

using namespace std;

int main() {

//******PORTAUDIO CONFIG***********//
    PaStreamParameters inputParameters, outputParameters;
    PaStream * stream;
    PaError err = Pa_Initialize();


    if( err == paNoError ) //INPUT STREAM CONFIG
    {
        inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
        if (inputParameters.device == paNoDevice)
        {
            fprintf(stderr,"Error: No default input device.\n");
            err++;  //Indicar que hubo error sin indicar un error en especifico.
        }
        else
        {
            inputParameters.channelCount = 2;       /* stereo input */
            inputParameters.sampleFormat = PA_SAMPLE_TYPE;
            inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
            inputParameters.hostApiSpecificStreamInfo = nullptr;
        }
    }
    if( err == paNoError ) //OUTPUT STREAM CONFIG
    {
        outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
        if (outputParameters.device == paNoDevice)
        {
            fprintf(stderr,"Error: No default output device.\n");
            err++;  //Indicar que hubo error sin indicar un error en especifico.
        }
        else
        {
            outputParameters.channelCount = 2;       /* stereo output */
            outputParameters.sampleFormat = PA_SAMPLE_TYPE;
            outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
            outputParameters.hostApiSpecificStreamInfo = nullptr;
        }
    }

//******DEAR IMGUI CONFIG***********//
     //inicializamos la parte grafica
    allegro_settings_t all_settings = gui_init();
    // main loop
    bool running = true;



	wav_pitch_user_data_t * userdata = create_user_data(get_frequency_by_autocorrelation_v2);


	//	err = set_wav_pitch_cb(stream, inputParameters, outputParameters, err);


	while (running)
	{
        running = create_window(all_settings, 5); //el segundo parametro es el tamano de la letra 
        const char* mode_selected = select_mode(); // AUDIO RECORDING & REAL TIME
        static char* wav = NULL;
        scale_t scale;
        if (mode_selected == "AUDIO RECORDING")
        {
            wav = insert_wav();
        }
        const char* characteristic_selected = select_characteristic(); //CALVIN & DUKI

        int selected_factor;
        if (characteristic_selected == "CALVIN")
        {
            selected_factor = select_factor(); //devuelve int del -12 al 12
        }
        else if (characteristic_selected == "DUKI")
        {
            scale = select_scale();
        }

        bool data_enter = mode_selected != NULL && characteristic_selected != NULL;
        bool config_done = confirm_data(data_enter);

        if (config_done)
        {
            if (characteristic_selected == "CALVIN")
            {
                set_alvin_user_data(userdata, pow(2,selected_factor/12.0));
            } 
            else if (characteristic_selected == "DUKI")
            {
                set_duki_user_data(userdata, scale);
            }

            if (mode_selected == "AUDIO RECORDING")
            {
 //               set_graphics("foto.PNG");
                set_wav_user_data(userdata, wav, "_freq", "_out", "_autocor_v2");
                process_wav(userdata);
                running = false;
            }
            else if (mode_selected == "REAL TIME")
            {
                // todo: start stream
            }
        }

        end_gui();
	}

#ifndef USE_WAV


    if(err == paNoError)
    {
        err = Pa_CloseStream( stream ); //todo: no llamar a esto si nunca se abrio el stream, por ejemplo en modo wav
    }
    if( err == paNoError )
    {
        printf("Finished.");
        Pa_Terminate();
    }
    else
    {
        Pa_Terminate();
        fprintf( stderr, "An error occured while using the portaudio stream\n" );
        fprintf( stderr, "Error number: %d\n", err );
        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    }
    
#endif // USE_WAV



    return 0;
}