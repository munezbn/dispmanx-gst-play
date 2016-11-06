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

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <termio.h>
#include <unistd.h>
#include <assert.h>

#include "dispmanx-gst-play.h"

static dispmanx_display_t s_dispmanx;

static void dispmanx_win_create_background(GstPlay *player_instance, int32_t layer, uint32_t bg_color);
static void dispmanx_win_add_background_element(dispmanx_background_t *bg, DISPMANX_DISPLAY_HANDLE_T display, DISPMANX_UPDATE_HANDLE_T update);
    
static void dispmanx_win_create_background(GstPlay *player_instance, int32_t layer, uint32_t bg_color)
{
  uint32_t image_ptr;
  uint16_t color = (uint16_t)bg_color;
  VC_IMAGE_TYPE_T type = VC_IMAGE_RGBA16;
  VC_RECT_T dst_rect;

  player_instance->bg.resource = vc_dispmanx_resource_create(type, 1, 1, &image_ptr);
  if(player_instance->bg.resource == 0)
    return;

  vc_dispmanx_rect_set(&dst_rect, 0, 0, 1, 1);

  player_instance->bg.layer = layer;

  vc_dispmanx_resource_write_data(player_instance->bg.resource,
      type,
      sizeof(color),
      &color,
      &dst_rect);
  return;
}

static void dispmanx_win_add_background_element(dispmanx_background_t *bg, DISPMANX_DISPLAY_HANDLE_T display, DISPMANX_UPDATE_HANDLE_T update)
{
  VC_RECT_T src_rect;
  VC_RECT_T dst_rect;
  VC_DISPMANX_ALPHA_T alpha =
  {   
    DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 
    bg->opacity,
    0   
  };  

  vc_dispmanx_rect_set(&src_rect, 0, 0, 1, 1); 
  vc_dispmanx_rect_set(&dst_rect, 0, 0, 0, 0); 

  bg->element = vc_dispmanx_element_add(update,
      display,
      bg->layer,
      &dst_rect,
      bg->resource,
      &src_rect,
      DISPMANX_PROTECTION_NONE,
      &alpha,
      NULL,
      DISPMANX_NO_ROTATE);
  return;
}

void dispmanx_win_show_background_element(dispmanx_background_t *bg, gboolean show)
{
  DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
  VC_RECT_T src_rect;
  VC_RECT_T dst_rect;
  int8_t result = 0;

  if(show == TRUE)
    bg->opacity = 255;
  else /* Hide*/
    bg->opacity = 0;

  vc_dispmanx_rect_set(&src_rect, 0, 0, 1, 1); 
  vc_dispmanx_rect_set(&dst_rect, 0, 0, 0, 0); 

  result = vc_dispmanx_element_change_attributes(update,
      bg->element,
      ELEMENT_CHANGE_OPACITY,
      bg->layer,
      bg->opacity,
      &dst_rect,
      &src_rect,
      0,
      DISPMANX_NO_ROTATE);

  assert(result == 0);

  result = vc_dispmanx_update_submit_sync(update);
  assert(result == 0);

  return;
}

void dispmanx_win_destroy_background_element(dispmanx_background_t *bg)
{
  int8_t result = 0;

  DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
  assert(update != 0); 

  result = vc_dispmanx_element_remove(update, bg->element);
  assert(result == 0); 

  result = vc_dispmanx_update_submit_sync(update);
  assert(result == 0); 

  result = vc_dispmanx_resource_delete(bg->resource);
  assert(result == 0);

  return;	
}

void dispmanx_win_move(player_video_window_t *vid_win, gint x, gint y)
{
  DISPMANX_UPDATE_HANDLE_T update = 0;
  VC_RECT_T result_dest;
  gint new_x = 0;
  gint new_y = 0;
  int8_t result = 0;
  static int8_t first = 1;
    
  if(vid_win->in_fullscreen == TRUE)
  {
    I_LOG_WARNING("!!!!!!!!!! Cannot Move Video Window in Fullscreen !!!!!!!!!!\n");
    return;
  }
  
  memcpy(&result_dest, &(vid_win->dst_rect), sizeof(VC_RECT_T));

  if(x != 0)
  {
    new_x = result_dest.x + x;
    if( new_x < 0)
      result_dest.x = 0;
    else if( (new_x + result_dest.width) > s_dispmanx.info.width)
      result_dest.x = (s_dispmanx.info.width - result_dest.width);
    else
      result_dest.x = new_x;

    I_LOG_DEBUG(" New X : %d [%d]\n", result_dest.x, new_x);
  } 
  
  if(y != 0)
  {
    new_y = result_dest.y + y;
    if( new_y < 0)
      result_dest.y = 0;
    else if( (new_y + result_dest.height) > s_dispmanx.info.height)
      result_dest.y = (s_dispmanx.info.height - result_dest.height);
    else
      result_dest.y = new_y;
    I_LOG_DEBUG(" New Y : %d [%d]\n", result_dest.y, new_y);
  }
  
  if( first || memcmp(&(vid_win->dst_rect), &result_dest, sizeof(VC_RECT_T)) ) /* Dont have to update if values have not changed*/
  {
    memcpy(&(vid_win->dst_rect), &result_dest, sizeof(VC_RECT_T));
  
    vid_win->src_rect.x = 0;
    vid_win->src_rect.y = 0;
    vid_win->src_rect.width = vid_win->vid_width << 16; 
    vid_win->src_rect.height = vid_win->vid_height << 16;


    I_LOG_INFO("Moving from %d %d %d %d ==> %d %d %d %d\n",
        vid_win->src_rect.x, vid_win->src_rect.y , vid_win->src_rect.width >> 16, vid_win->src_rect.height >> 16,
        vid_win->dst_rect.x, vid_win->dst_rect.y , vid_win->dst_rect.width, vid_win->dst_rect.height);
  
    update = vc_dispmanx_update_start(0);
    result = vc_dispmanx_element_change_attributes(update,
        vid_win->vid_window.element,
        ELEMENT_CHANGE_DEST_RECT,
        vid_win->vid_layer,
        255,
        &(vid_win->dst_rect),
        &(vid_win->src_rect),
        0,
        DISPMANX_NO_ROTATE);
    assert(result == 0);
    vc_dispmanx_update_submit_sync(update);
    first = 0;
  }
  else
  {
    I_LOG_WARNING("!!!!!!!!!! Window position has not changed !!!!!!!!!!\n");
  }

  return;
}

void dispmanx_win_set_fullscreen(player_video_window_t *vid_win, gboolean fullscreen)
{
  DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
  int result = -1;

  VC_RECT_T result_dest;

  if(fullscreen == TRUE)
  {
    if(vid_win->ar == PLAYER_AR_STRETCH)
    {
      result_dest.x = 0;
      result_dest.y = 0;
      result_dest.width = s_dispmanx.info.width;
      result_dest.height = s_dispmanx.info.height;
    }
    else /* use default aspect ratio from source*/
    {
      gdouble src_ratio, dst_ratio;
      src_ratio = (gdouble) vid_win->vid_width / vid_win->vid_height;
      dst_ratio = (gdouble) s_dispmanx.info.width / s_dispmanx.info.height;
      if (src_ratio > dst_ratio)
      {
        result_dest.width = s_dispmanx.info.width;
        result_dest.height = s_dispmanx.info.width / src_ratio;
        result_dest.x = 0;
        result_dest.y = (s_dispmanx.info.height - result_dest.height) / 2;
      }
      else if (src_ratio < dst_ratio)
      {
        result_dest.width = s_dispmanx.info.height * src_ratio;
        result_dest.height = s_dispmanx.info.height;
        result_dest.x = (s_dispmanx.info.width - result_dest.width) / 2;
        result_dest.y = 0;
      }
      else
      {
        result_dest.x = 0;
        result_dest.y = 0;
        result_dest.width = s_dispmanx.info.width;
        result_dest.height = s_dispmanx.info.height;
      } 
    }
    memcpy(&(vid_win->dst_rect), &result_dest, sizeof(VC_RECT_T));

    vid_win->in_fullscreen = TRUE;
  }
  else /* Exit fullscreen , display with actual resolution at the center of the screen*/
  {
    vid_win->dst_rect.x = (s_dispmanx.info.width - vid_win->vid_width) / 2 ;
    vid_win->dst_rect.y = (s_dispmanx.info.height - vid_win->vid_height) / 2;
    vid_win->dst_rect.width = vid_win->vid_width;
    vid_win->dst_rect.height = vid_win->vid_height;

    vid_win->in_fullscreen = FALSE;
  }

  vid_win->src_rect.x = 0;
  vid_win->src_rect.y = 0;
  vid_win->src_rect.width = vid_win->vid_width << 16; 
  vid_win->src_rect.height = vid_win->vid_height << 16;
  

  I_LOG_INFO("Scaling from %d %d %d %d ==> %d %d %d %d\n",
      vid_win->src_rect.x >> 16, vid_win->src_rect.y >> 16, vid_win->src_rect.width >> 16, vid_win->src_rect.height >> 16,
      vid_win->dst_rect.x, vid_win->dst_rect.y , vid_win->dst_rect.width, vid_win->dst_rect.height);

  result = vc_dispmanx_element_change_attributes(update,
      vid_win->vid_window.element,
      ELEMENT_CHANGE_DEST_RECT,
      vid_win->vid_layer,
      255,
      &(vid_win->dst_rect),
      &(vid_win->src_rect),
      0,
      DISPMANX_NO_ROTATE);
  assert(result == 0);
  vc_dispmanx_update_submit_sync(update);

  return;
}

void dispmanx_shutdown_window_system()
{
  I_LOG_WARNING("---------- UN-IMPLEMENTED ----------\n");
  /* TODO remove background and video layer elements*/
  vc_dispmanx_display_close( s_dispmanx.display );
  bcm_host_deinit();

  return;
}

gboolean dispmanx_initialize_window_system()
{
  gboolean status = FALSE;

  uint32_t ret = -1, screen = 0;

  if( s_dispmanx.display > 0)
  {
    I_LOG_WARNING("!!!!!!!!!! Display is already Initialized !!!!!!!!!! Display %d\n", s_dispmanx.display);
    goto safe_exit;
  }

  bcm_host_init();

  I_LOG_DEBUG("Opening display... %u\n", screen);
  s_dispmanx.display = vc_dispmanx_display_open(screen);
  I_ASSERT(s_dispmanx.display != 0);

  ret = vc_dispmanx_display_get_info(s_dispmanx.display, &s_dispmanx.info);
  I_ASSERT(ret == 0);

  I_LOG_DEBUG("Opened Display [%d x %d]\n", s_dispmanx.info.width, s_dispmanx.info.height);

  status = TRUE;

safe_exit:
  return status;
}

void dispmanx_win_set_aspect_ratio(GstPlay *player_instance, dispmanx_player_aspect_ratio_e ar)
{
  I_LOG_TRACE("Key r Pressed Window is Ful%d\n", player_instance->vid_win.in_fullscreen);

  if(player_instance->vid_win.ar != ar)
    player_instance->vid_win.ar = ar;
  else
  {
    I_LOG_WARNING("!!!!!!!!!! Aspect Ratio %d Is Set Already !!!!!!!!!!\n", ar);
    goto safe_exit;
  }

  if(player_instance->vid_win.in_fullscreen)
    dispmanx_win_set_fullscreen(&player_instance->vid_win, true); /* call fullscreen to reflect new Aspect ratio*/
  else
  {
    I_LOG_DEBUG("Aspect Ratio is adjusted only in fullscreen.. saving the value for now\n");
  }

safe_exit:
  return;
}

gboolean dispmanx_create_video_window(GstPlay *player_instance)
{
  DISPMANX_UPDATE_HANDLE_T dispman_update;
  gboolean ret = FALSE;

  int32_t screen = 0;
  VC_DISPMANX_ALPHA_T alpha = { DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 
    255, /*alpha 0->255*/
    0 };

  if(player_instance == NULL)
  {
    I_LOG_ERROR("xxxxxxxxxx Invalid Argument xxxxxxxxxx\n");
    goto safe_exit;
  }

  /* create background to add at layer -1, i.e before video layer*/
  dispmanx_win_create_background(player_instance, -1, 0x000F);

  player_instance->vid_win.dst_rect.x = 0;
  player_instance->vid_win.dst_rect.y = 0;
  player_instance->vid_win.dst_rect.width = s_dispmanx.info.width;
  player_instance->vid_win.dst_rect.height = s_dispmanx.info.height;

  player_instance->vid_win.src_rect.x = 0;
  player_instance->vid_win.src_rect.y = 0;
  player_instance->vid_win.src_rect.width = s_dispmanx.info.width << 16; 
  player_instance->vid_win.src_rect.height = s_dispmanx.info.height << 16; 

  dispman_update = vc_dispmanx_update_start(screen);

  /* Add background element*/
  dispmanx_win_add_background_element(&player_instance->bg, s_dispmanx.display, dispman_update);

  /* Add video element at layer 0*/
  player_instance->vid_win.vid_layer = 0;

  /* Form EGL_DISPMANX_WINDOW_T using the Dispmanx window*/
  player_instance->vid_win.vid_window.element =  vc_dispmanx_element_add(dispman_update, s_dispmanx.display,
      player_instance->vid_win.vid_layer/*layer*/, &player_instance->vid_win.dst_rect, 0/*src*/,
      &player_instance->vid_win.src_rect, DISPMANX_PROTECTION_NONE, 
      &alpha/*alpha*/, 0/*clamp*/, DISPMANX_SNAPSHOT_FILL/*0*//*transform*/);
  player_instance->vid_win.vid_window.width = s_dispmanx.info.width;
  player_instance->vid_win.vid_window.height = s_dispmanx.info.height;

  /* save the window handle*/
  player_instance->video_window_handle = (gpointer)(&(player_instance->vid_win.vid_window));

  vc_dispmanx_update_submit_sync(dispman_update);
  ret = TRUE;

safe_exit:
  return ret;
}
