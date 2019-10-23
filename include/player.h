#ifndef __PLAYER_H
#define __PLAYER_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wconversion"
#include <glib.h>
#include <gst/gst.h>
#include <gst/gsturi.h>
#include <gst/gstpluginfeature.h>
#include <gst/pbutils/pbutils.h>
#include <gst/player/player.h>
#pragma GCC diagnostic pop

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "bcm_host.h"
#include "interface/vmcs_host/vc_dispmanx_types.h"
#include "interface/vmcs_host/vc_vchi_dispmanx.h"

#define P_MAX_BUFFER_SIZE 128
#define PLAYER_SRC_ELEM_NAME "Player_Src_Elem"

#define GST_ELEM_NAME(p_element)    (gst_element_get_name(p_element))
#define IS_STATIC_PAD(pad) (GST_PAD_TEMPLATE_PRESENCE(gst_pad_get_pad_template(pad)) == GST_PAD_ALWAYS)

#define VOLUME_STEPS 20

#define I_LOG_FATAL(msg, args...) \
    printf("\e[0;31m%-9s : %s -> %s(%d) : " msg "\e[0m", "[FATAL]", __FILE__, __func__, __LINE__, ## args);
#define I_LOG_ERROR(msg, args...) \
    printf("\e[0;31m%-9s : %s -> %s(%d) : " msg "\e[0m", "[ERROR]", __FILE__, __func__, __LINE__, ## args);
#define I_LOG_WARNING(msg, args...) \
    printf("\e[0;33m%-9s : %s -> %s(%d) : " msg "\e[0m", "[WARN]", __FILE__, __func__, __LINE__, ## args);
#define I_LOG_INFO(msg, args...) \
    printf("\e[0;32m%-9s :\e[0m %s -> %s(%d) : " msg, "[INFO]", __FILE__, __func__, __LINE__, ## args);
#define I_LOG_DEBUG(msg, args...) \
    printf("\e[0;36m%-9s :\e[0m %s -> %s(%d) : " msg, "[DEBUG]", __FILE__, __func__, __LINE__, ## args);
#define I_LOG_TRACE(msg, args...) \
    printf("\e[0;34m%-9s :\e[0m %s -> %s(%d) : " msg, "[TRACE]", __FILE__, __func__, __LINE__, ## args);

#define I_ASSERT assert
#define I_ZEROMEM(ptr, length)  (memset(ptr, 0, length))

#define I_ARG_CHECK(cond, result, ret_val) \
    if( !(cond) ) \
    {\
        I_LOG_ERROR("xxxxxxxxxx Invalid Argument xxxxxxxxxx\n");\
        (result) = (ret_val); \
        goto safe_exit; \
    }

typedef enum
{
    PLAYER_AR_ORIGINAL = 1,
    PLAYER_AR_STRETCH
}dispmanx_player_aspect_ratio_e;

typedef struct
{
    int32_t vid_layer;
	EGL_DISPMANX_WINDOW_T vid_window;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;
    VC_RECT_T render_rect;
    guint vid_width;
    guint vid_height;
    gboolean in_fullscreen;
    dispmanx_player_aspect_ratio_e ar;
}dispmanx_window_t;

typedef struct
{
    GCallback position_updated_cb;
    GCallback state_changed_cb;
    GCallback buffering_cb;
    GCallback end_of_stream_cb;
    GCallback error_cb;
    GCallback media_info_cb;
}i_player_signal_handlers_t;

typedef struct
{
    int32_t layer;
	uint32_t color;
	uint8_t opacity;
    DISPMANX_RESOURCE_HANDLE_T resource;
    DISPMANX_ELEMENT_HANDLE_T element;
} dispmanx_background_t;


/* Player Structure*/
typedef struct
{
    uint32_t player_handler;
    char player_name[P_MAX_BUFFER_SIZE];
    gchar *src_uri;
    gchar *dest_uri;

    GstPlayer *player;
    GstElement *pipeline;
    GstPlayerVideoRenderer *renderer;

    /*Streaming*/
    gboolean is_live;
    gboolean local_playback;

    /* State*/
    GstState desired_state;

    /*Trick & Timing*/
    gint64 duration;
    gboolean seek_enabled;
    gdouble rate;

    gdouble volume;

    /* For handling messages on pipeline bus and sending signals on dbus*/
    GstBus *bus;
    void *player_object;
    
    /* Video Window & background*/
    gpointer video_window_handle;
    dispmanx_window_t vid_win;
	dispmanx_background_t bg;
}player_instance_t;

/* player_interface.c*/
int8_t player_play(player_instance_t *player_instance);
int8_t player_get_handler(const char *src_uri, const char *dest_uri, i_player_signal_handlers_t *sig_handlers, player_instance_t **player_instance);
int8_t player_stop(player_instance_t *player_instance);
int8_t player_toggle_pause(player_instance_t *player_instance);
int8_t player_toggle_fullscreen(player_instance_t *player_instance);
int8_t player_toggle_aspect_ratio(player_instance_t *player_instance);
int8_t player_toggle_background(player_instance_t *player_instance);
void player_relative_seek (player_instance_t *player_instance, gdouble percent);
void player_release(player_instance_t *player_instance);
void play_set_relative_volume (player_instance_t *player_instance, gdouble volume_step);
void player_init(void);
void player_shutdown(void);

#endif /*__PLAYER_H*/
