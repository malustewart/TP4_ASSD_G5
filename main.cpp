#include <iostream>
#include <string>
#include "portaudio.h"
#include "set_wav_pitch_cb.h"

#include "gui.h"

#include <fstream>
#include <vector>


#define	BATCHMODE

#ifdef BATCHMODE

typedef scale_t duki_data_t;

typedef int alvin_note_t;

#endif

using namespace std;




int main() {

#ifdef BATCHMODE
	vector<alvin_note_t> alvin_notes = {0,1,-1, 4, -4, 12, -12};
	vector<duki_data_t> dukis;

	dukis.push_back({ "MI", MI, {true,false,true,false,true,false,true,false,true,false,true,false} });

	vector<string> wavfiles = {"king_love", "sirena"};	//sin extension!!!!

	for (auto wavfile : wavfiles)
	{
		for (auto& alvin_note : alvin_notes)
		{
			pitch_user_data_t * u = create_user_data(get_frequency_by_yin);
			set_alvin_user_data(u, pow(2, alvin_note / 12.0));
			string out_suffix = "_alvin_";
			out_suffix += to_string(alvin_note);
			set_wav_user_data(u, wavfile.c_str(),"_bin", out_suffix.c_str(), "_det", "_obj");
			process_wav(u);
			//delete_user_data(u);
		}


		for (auto& duki : dukis)
		{
			pitch_user_data_t * u = create_user_data(get_frequency_by_yin);
			set_duki_user_data(u,duki);
			string out_suffix = "_duki_";
			out_suffix += duki.note;
			for (size_t i = 0; i < OCTAVE_SUBDIVISION; i++) {
				out_suffix += duki.scale_octave[i] ? '1' : '0';
			}
			out_suffix += "_";
			set_wav_user_data(u, wavfile.c_str(), "_bin", out_suffix.c_str(), "_det", "_obj");
			process_wav(u);
			//delete_user_data(u);
		}
	}



	return 0;
#endif


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
                set_wav_user_data(userdata, wav, "_freq", "_out", "_det", "_obj");
				process_wav(userdata);
                running = false;
            }
			else if (mode_selected == "REAL TIME" && !processing_audio)
			{
				set_real_time_user_data(userdata);
				run_real_time(userdata);
				char c = 0;
				cin >> c;
				if (c != 'c') {
					stop_real_time(userdata);
					running = false;
				}
			}
        }

        end_gui();
	}




    return 0;
}