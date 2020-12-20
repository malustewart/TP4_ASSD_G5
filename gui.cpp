
#include "gui.h"


allegro_settings_t gui_init(void)
{
	// Setup Allegro
	al_init();
	al_install_keyboard();
	al_install_mouse();
	al_init_primitives_addon();
	al_init_image_addon();
	al_set_new_display_flags(ALLEGRO_RESIZABLE);
	allegro_settings_t all_settings;
	all_settings.display = al_create_display(2100, 1000);
	al_set_window_title(all_settings.display, "TP4 - ASSD - MALULA Y VALULA");
	all_settings.queue = al_create_event_queue();
	al_register_event_source(all_settings.queue, al_get_display_event_source(all_settings.display));
	al_register_event_source(all_settings.queue, al_get_keyboard_event_source());
	al_register_event_source(all_settings.queue, al_get_mouse_event_source());



	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplAllegro5_Init(all_settings.display);

	return all_settings;

}


bool create_window(allegro_settings_t all_settings, int size)
{
	static bool running = true;
	ALLEGRO_EVENT ev;
	while (al_get_next_event(all_settings.queue, &ev))
	{
		ImGui_ImplAllegro5_ProcessEvent(&ev);
		if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
			running = false;
		if (ev.type == ALLEGRO_EVENT_DISPLAY_RESIZE)
		{
			ImGui_ImplAllegro5_InvalidateDeviceObjects();
			al_acknowledge_resize(all_settings.display);
			ImGui_ImplAllegro5_CreateDeviceObjects();
		}
	}

	ImGui_ImplAllegro5_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowSize(ImVec2(2000,800),ImGuiCond_Once);
	ImGui::Begin("TP 4 ASSD MALULA Y VALULA", &running);
	ImGui::SetWindowFontScale(size);

	return running;
}


const char* select_mode(void)
{
	const char* mode_items[] = { "REAL TIME", "AUDIO RECORDING" };
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 30);
	ImGui::Text("SELECT A MODE:  ");
	static const char* mode = NULL;
	ImGui::SetNextItemWidth(650);
	ImGui::SameLine();
	if (ImGui::BeginCombo("##mode", mode)) // The second parameter is the label previewed before opening the combo.
	{
		for (int n = 0; n < IM_ARRAYSIZE(mode_items); n++)
		{
			bool is_selected = (mode == mode_items[n]); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(mode_items[n], is_selected))
				mode = mode_items[n];
			if (is_selected)
				ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
		}
		ImGui::EndCombo();
	}
	return mode;
}


char* insert_wav(void)
{
	static char wav[1000] = "Insert here";
	ImGui::SameLine();
	ImGui::SetNextItemWidth(400);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 100);
	ImGui::InputText(".wav   ", wav, 1000, ImGuiInputTextFlags_CharsNoBlank);
	ImGui::Spacing();
	return wav;
}



const char* select_characteristic(void)
{
	const char* char_items[] = { "CALVIN", "DUKI" };
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 30);
	ImGui::Text("CHARACTERISTICS:  ");
	static const char* change = NULL;
	ImGui::SetNextItemWidth(300);
	ImGui::SameLine();
	if (ImGui::BeginCombo("##change", change)) // The second parameter is the label previewed before opening the combo.
	{
		for (int n = 0; n < IM_ARRAYSIZE(char_items); n++)
		{
			bool is_selected = (change == char_items[n]); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(char_items[n], is_selected))
				change = char_items[n];
			if (is_selected)
				ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
		}
		ImGui::EndCombo();
	}
	ImGui::Spacing();

	return change;
}


int select_factor(void)
{
	ImGui::Separator();
	static int int_value = 0;
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 2) - 150);
	ImGui::VSliderInt("##int", ImVec2(300, 270), &int_value, -12, 12);
	ImGui::Separator();
	return int_value;
}


scale_t select_scale(void)
{
	static scale_t scale = { "DO",DO, true, false, true, false, true, true, false, true, false, true, false, true};

	ImGui::Separator();
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 2) - 250);
	ImGui::Text("CHANGE SCALE:  ");
	const char* note_items[] = { "DO", "DOs", "RE", "REs", "MI", "FA","FAs", "SOL","SOLs", "LA","LAs", "SI" };
	ImGui::SetNextItemWidth(250);
	ImGui::SameLine();
	if (ImGui::BeginCombo("##note", scale.note)) // The second parameter is the label previewed before opening the combo.
	{
		for (int n = 0; n < IM_ARRAYSIZE(note_items); n++)
		{
			bool is_selected = (scale.note == note_items[n]); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(note_items[n], is_selected))
			{
				scale.note = note_items[n];
				scale.note_number = n;
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
		}
		ImGui::EndCombo();
	}

	ImGui::Columns(4);//, "mixed");
	ImGui::Separator();

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));
	ImGui::Checkbox("I", &scale.scale_octave[0]); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));
	ImGui::Checkbox("IIb", &scale.scale_octave[1]); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));
	ImGui::Checkbox("II", &scale.scale_octave[2]); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));
	ImGui::NextColumn();

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));


	ImGui::Checkbox("IIIb", &scale.scale_octave[3]); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));
	ImGui::Checkbox("III", &scale.scale_octave[4]); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));
	ImGui::Checkbox("IV", &scale.scale_octave[5]); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));
	ImGui::NextColumn();

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));

	ImGui::Checkbox("Vb", &scale.scale_octave[6]); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));
	ImGui::Checkbox("V", &scale.scale_octave[7]); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));
	ImGui::Checkbox("VIb", &scale.scale_octave[8]); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));
	ImGui::NextColumn();

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));

	ImGui::Checkbox("VI", &scale.scale_octave[9]); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));
	ImGui::Checkbox("VIIb", &scale.scale_octave[10]); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));
	ImGui::Checkbox("VII", &scale.scale_octave[11]); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 10));
	ImGui::NextColumn();

	ImGui::Columns(1);
	ImGui::Separator();
	ImGui::Separator();
	return scale;
}


bool confirm_data(bool data_enter)
{
	static bool config_done = false;
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 2) - 100);
	if (data_enter)
	{
		if (ImGui::ButtonEx("Select", ImVec2(210, 60), ImGuiButtonFlags_None))
		{
			config_done = true;
		}
	}
	else
	{
		ImGui::ButtonEx("Select", ImVec2(210, 60), ImGuiButtonFlags_Disabled);
	}
	return config_done;
}


void set_graphics(const char* foto)
{
	if (ImGui::CollapsingHeader("GRAPHICS"))
	{
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 5));
		ImGui::Text("SPECTROGRAM");
		ALLEGRO_BITMAP* image = NULL;
		image = al_load_bitmap(foto);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 5) - 300);
		ImGui::Image(image, ImVec2(1000, 1000));


		ImGui::NextColumn();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 5));
		ImGui::Text("HISTOGRAM");
		static float arr[] = { 0.1f,0.1f, 0.7f, 0.05f, 0.05f };
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 300);
		ImGui::PlotHistogram("##Histogram", arr, IM_ARRAYSIZE(arr), 0, "", 0.0f, 1.0f, ImVec2(1000, 1000));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 350);
		ImGui::Text("-2"); ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 150);
		ImGui::Text("-1"); ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 150);
		ImGui::Text("0"); ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 150);
		ImGui::Text("+1"); ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 150);
		ImGui::Text("+2"); 

		ImGui::NextColumn();
		ImGui::Columns(1);
		ImGui::Separator();


	}
}

void end_gui(void)
{
	//ImGui::ShowDemoWindow();
	ImGui::End();
	ImGui::Render();
	al_clear_to_color({ 0.0f, 0.0f, 0.0f, 0.0f });
	ImGui_ImplAllegro5_RenderDrawData(ImGui::GetDrawData());
	al_flip_display();
}