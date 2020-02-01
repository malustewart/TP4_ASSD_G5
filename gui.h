#ifndef ASSD_TP_4_GUI_H
#define ASSD_TP_4_GUI_H

#include "imgui.h"
#include "imgui_impl_allegro5.h"
#include "imgui_internal.h"
#include <allegro5\allegro.h>
#include <allegro5\allegro_primitives.h>
#include <allegro5\allegro_image.h>

typedef struct allegro_settings_t
{
	ALLEGRO_DISPLAY* display;
	ALLEGRO_EVENT_QUEUE* queue;

} allegro_settings_t;

typedef struct scale_t
{
    const char* note;
    bool I;
    bool IIb;
    bool II;
    bool IIIb;
    bool III;
    bool IV;
    bool Vb;
    bool V;
    bool VIb;
    bool VI;
    bool VIIb;
    bool VII;
}scale_t;

allegro_settings_t gui_init(void);

bool create_window(allegro_settings_t all_settings, int size);
const char* select_mode(void);
char* insert_wav(void);
const char* select_characteristic(void);
int select_factor(void);
scale_t select_scale(void);
bool confirm_data(bool data_enter);

void set_graphics(const char* foto);
void end_gui(void);


#endif //ASSD_TP_4_GUI_H
