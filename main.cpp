#include <iostream>
#include <string>
#include "portaudio.h"
#include "set_wav_pitch_cb.h"

#include "gui.h"

#include <fstream>



using namespace std;


void run_duki_debug(const char * note, int note_number, bool * scale, const char * wav_filename, const char * out_suffix) {
	scale_t s;
	s.note = note;
	s.note_number = note_number;
	for (size_t i = 0; i < 12; i++)
	{
		s.scale_octave[i] = scale[i];
	}

	pitch_user_data_t * userdata = create_user_data(get_frequency_by_yin);
	set_duki_user_data(userdata, s);
	set_wav_user_data(userdata, wav_filename, "_bin", out_suffix, "_det", "_obj");
	process_wav(userdata);
}


int main() {

//	//DEBUG
//	const char* n[] = { "LA", "MI", "RE", "MI", "As", "" };
//	int note_number[] = { LA, MI, RE, MI, LAs };
//	const char * f[] = { "440_A4", "658_E5", "king_love", "168_E3", "233_Bb3" };
//	bool sc[] = {false, false, true, false, false, false, true, false, true, false, 
//				false, true};
//
//
//	//for (size_t i = 0; strcmp(n[i], ""); i++)
//	//{
//	//	run_duki_debug(n[i], note_number[i], sc, f[i], "_out_duki_+0");
//	//}
//
//	//sc[0] = false;
//	//sc[9] = true;
////	for (size_t i = 0; strcmp(n[i], ""); i++)
//	{
//		run_duki_debug(n[2], note_number[2], sc, f[2], "_out_duki");
//	}
//
//
//	return 0;
//	//END DEBUG



//******DEAR IMGUI CONFIG***********//
     //inicializamos la parte grafica
    allegro_settings_t all_settings = gui_init();
    // main loop
    bool running = true;
	bool processing_audio = false;



	pitch_user_data_t * userdata = create_user_data(get_frequency_by_autocorrelation_v2);




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

            if (mode_selected == "AUDIO RECORDING" && !processing_audio)
            {
 //               set_graphics("foto.PNG");
                set_wav_user_data(userdata, wav, "_freq", "", "_det", "_obj");
				process_wav(userdata);
                running = false;
            }
            else if (mode_selected == "REAL TIME" && !processing_audio)
            {
				set_real_time_user_data(userdata);
				run_real_time(userdata);
				processing_audio = true;
			}
			if (stop_playback())
			{
				stop_real_time(userdata);
			}
        }

        end_gui();
	}




    return 0;
}