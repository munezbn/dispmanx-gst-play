/* GStreamer command line playback testing utility for raspberrypi
 *
 * Copyright (C) 2016 Munez BN <munezbn.dev@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __DISPMANX_GST_PLAY_H
#define __DISPMANX_GST_PLAY_H

#include <gst/gst.h>
#include <gst/player/player.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "bcm_host.h"
#include "interface/vmcs_host/vc_dispmanx_types.h"
/*#include "interface/vmcs_host/vc_vchi_dispmanx.h"*/

/*-------------------------------------------------------------------------
 * flags for vc_dispmanx_element_change_attributes
 *
 * can be found in interface/vmcs_host/vc_vchi_dispmanx.h
 * but you can't include that file as interface/peer/vc_vchi_dispmanx_common.h is missing.
 *-------------------------------------------------------------------------*/

#ifndef ELEMENT_CHANGE_LAYER
#define ELEMENT_CHANGE_LAYER (1<<0)
#endif

#ifndef ELEMENT_CHANGE_OPACITY
#define ELEMENT_CHANGE_OPACITY (1<<1)
#endif

#ifndef ELEMENT_CHANGE_DEST_RECT
#define ELEMENT_CHANGE_DEST_RECT (1<<2)
#endif

#ifndef ELEMENT_CHANGE_SRC_RECT
#define ELEMENT_CHANGE_SRC_RECT (1<<3)
#endif

#ifndef ELEMENT_CHANGE_MASK_RESOURCE
#define ELEMENT_CHANGE_MASK_RESOURCE (1<<4)
#endif

#ifndef ELEMENT_CHANGE_TRANSFORM
#define ELEMENT_CHANGE_TRANSFORM (1<<5)
#endif

#define ENABLE_EXTRA_LOGS
#define ENABLE_BASIC_LOGS

#ifdef ENABLE_BASIC_LOGS
  #define I_LOG_FATAL(msg, args...) \
    printf("\e[0;31m%-9s : %s -> %s(%d) : \e[0m" msg, "[FATAL]", __FILE__, __func__, __LINE__, ## args);
  #define I_LOG_ERROR(msg, args...) \
    printf("\e[0;31m%-9s : %s -> %s(%d) : \e[0m" msg, "[ERROR]", __FILE__, __func__, __LINE__, ## args);
  #define I_LOG_WARNING(msg, args...) \
    printf("\e[0;33m%-9s : %s -> %s(%d) : \e[0m" msg, "[WARNING]", __FILE__, __func__, __LINE__, ## args);
#else
  #define I_LOG_FATAL(msg,args...) 
  #define I_LOG_ERROR(msg,args...) 
  #define I_LOG_WARNING(msg,args...) 
#endif

#ifdef ENABLE_EXTRA_LOGS
  #define I_LOG_INFO(msg, args...) \
    printf("\e[0;32m%-9s : %s -> %s(%d) : \e[0m" msg, "[INFO]", __FILE__, __func__, __LINE__, ## args);
  #define I_LOG_DEBUG(msg, args...) \
    printf("\e[0;36m%-9s : %s -> %s(%d) : \e[0m" msg, "[DEBUG]", __FILE__, __func__, __LINE__, ## args);
  #define I_LOG_TRACE(msg, args...) \
    printf("\e[0;34m%-9s : %s -> %s(%d) : \e[0m" msg, "[TRACE]", __FILE__, __func__, __LINE__, ## args);
  #define I_LOG_USER(msg, args...) \
    printf(msg, ## args);
#else
  #define I_LOG_INFO(msg,args...) 
  #define I_LOG_TRACE(msg,args...) 
  #define I_LOG_USER(msg,args...) 
#endif

#define I_ASSERT assert

typedef struct
{
    DISPMANX_DISPLAY_HANDLE_T display;
    DISPMANX_MODEINFO_T info;
}dispmanx_display_t;

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
}player_video_window_t;

typedef struct
{
  int32_t layer;
  uint32_t color;
  uint8_t opacity;
  DISPMANX_RESOURCE_HANDLE_T resource;
  DISPMANX_ELEMENT_HANDLE_T element;
} dispmanx_background_t;

typedef struct
{
  gchar **uris;
  guint num_uris;
  gint cur_idx;

  GstPlayer *player;
  GstElement *pipeline;
  GstPlayerVideoRenderer *renderer;
  GstState desired_state;

  gboolean repeat;

  GMainLoop *loop;

  /* Video Window & background*/
  gpointer video_window_handle;
  player_video_window_t vid_win;
  dispmanx_background_t bg;
} GstPlay;

/*dispmanx_window.c*/
gboolean dispmanx_initialize_window_system();
gboolean dispmanx_create_video_window(GstPlay *player_instance);
void dispmanx_shutdown_window_system(void);
void dispmanx_win_show_background_element(dispmanx_background_t *bg, gboolean show);
void dispmanx_win_destroy_background_element(dispmanx_background_t *bg);
void dispmanx_win_set_fullscreen(player_video_window_t *vid_win, gboolean fullscreen);
void dispmanx_win_set_aspect_ratio(GstPlay *player_instance, dispmanx_player_aspect_ratio_e ar);
void dispmanx_win_move(player_video_window_t *vid_win, gint x, gint y);

#endif /* __DISPMANX_GST_PLAY_H */
