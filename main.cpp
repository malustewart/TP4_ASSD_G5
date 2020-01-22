#include <iostream>
#include <string>
#include "portaudio.h"
#include "set_wav_pitch_cb.h"

#include <allegro5\allegro.h>
#include <allegro5\allegro_primitives.h>
#include <allegro5\allegro_image.h>

#include "imgui.h"
#include "imgui_impl_allegro5.h"

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

	// Setup Allegro
	al_init();
	al_install_keyboard();
	al_install_mouse();
	al_init_primitives_addon();
	al_init_image_addon();
	al_set_new_display_flags(ALLEGRO_RESIZABLE);
	ALLEGRO_DISPLAY* display = al_create_display(1280, 720);
	al_set_window_title(display, "Dear ImGui Allegro 5 example");
	ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
	al_register_event_source(queue, al_get_display_event_source(display));
	al_register_event_source(queue, al_get_keyboard_event_source());
	al_register_event_source(queue, al_get_mouse_event_source());

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplAllegro5_Init(display);

	// Main loop
	bool running = true;



	err = set_wav_pitch_cb(stream, inputParameters, outputParameters, err);


	while (running)
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		ALLEGRO_EVENT ev;
		while (al_get_next_event(queue, &ev))
		{
			ImGui_ImplAllegro5_ProcessEvent(&ev);
			if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
				running = false;
			if (ev.type == ALLEGRO_EVENT_DISPLAY_RESIZE)
			{
				ImGui_ImplAllegro5_InvalidateDeviceObjects();
				al_acknowledge_resize(display);
				ImGui_ImplAllegro5_CreateDeviceObjects();
			}
		}

		// Start the Dear ImGui frame
		ImGui_ImplAllegro5_NewFrame();
		ImGui::NewFrame();

		ImGui::Text("HOLA");
		ImGui::ShowDemoWindow();

		// Rendering
		ImGui::Render();
		al_clear_to_color({ 0.0f, 0.0f, 0.0f, 0.0f });
		ImGui_ImplAllegro5_RenderDrawData(ImGui::GetDrawData());
		al_flip_display();
	}



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