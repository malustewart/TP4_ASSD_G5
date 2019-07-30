#include <iostream>
#include <string>
#include "portaudio.h"
#include "set_wav_pitch_cb.h"


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

    err = set_wav_pitch_cb(stream,inputParameters, outputParameters,err);

    if(err == paNoError)
    {
        err = Pa_CloseStream( stream );
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

    return 0;

}