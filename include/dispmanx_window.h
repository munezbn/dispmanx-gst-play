/*-------------------------------------------------------------------------
 flags for vc_dispmanx_element_change_attributes

 can be found in interface/vmcs_host/vc_vchi_dispmanx.h
 but you can't include that file as
 interface/peer/vc_vchi_dispmanx_common.h is missing.

-------------------------------------------------------------------------*/

#ifndef __DISPMANX_WINDOW_H
#define __DISPMANX_WINDOW_H

#include "player.h"
#define WIN_MOVE_STEPS 20

typedef struct
{
    DISPMANX_DISPLAY_HANDLE_T display;
    DISPMANX_MODEINFO_T info;
}dispmanx_display_t;


/*dispmanx_window.c*/
gboolean dispmanx_initialize_window_system(void);
gboolean dispmanx_create_video_window(player_instance_t *player_instance);
void dispmanx_shutdown_window_system(void);
void dispmanx_win_show_background_element(dispmanx_background_t *bg, gboolean show);
void dispmanx_win_destroy_background_element(dispmanx_background_t *bg);
void dispmanx_win_set_fullscreen(dispmanx_window_t *vid_win, gboolean fullscreen);
void dispmanx_win_set_aspect_ratio(player_instance_t *player_instance, dispmanx_player_aspect_ratio_e ar);
void dispmanx_win_move(dispmanx_window_t *vid_win, gint x, gint y); 

#endif /* __DISPMANX_WINDOW_H*/
