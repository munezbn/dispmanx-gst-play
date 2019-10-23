/* MIT License

Copyright (c) 2016 Munez Bokkapatna Nayakwady <munezbn.dev@gmail.com>

<F10>Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wconversion"
#include <gst/video/videooverlay.h>
#pragma GCC diagnostic pop

#include <player.h>
#include <dispmanx_window.h>

/* static function*/

/* static variable*/
static GMainLoop *s_player_main_loop = NULL;
static GThread *s_player_context_thread = NULL;

/* ********** All Static Functions Defined Here ***********/

/* gmain loop thread*/
static gpointer s_player_loop(gpointer data)
{
    s_player_main_loop = g_main_loop_new (NULL, FALSE);

    I_LOG_DEBUG("Run Player Loop : %p\n", s_player_main_loop);
    g_main_loop_run (s_player_main_loop); /* Blocked until g_main_quit is called*/
    I_LOG_DEBUG("Exit Player Loop : %p\n", s_player_main_loop);
    g_main_loop_unref (s_player_main_loop);
    s_player_main_loop = NULL;
    return NULL;
}

/* reset for new file/stream */
static void play_reset (player_instance_t *player_instance)
{
	return;
}

static gchar *play_uri_get_display_name (player_instance_t *player_instance, const gchar *src_uri)
{
	gchar *uri_location = NULL;

	if (gst_uri_has_protocol (src_uri, "file"))
		uri_location = g_filename_from_uri (src_uri, NULL, NULL);
	else if (gst_uri_has_protocol (src_uri, "pushfile"))
		uri_location = g_filename_from_uri (src_uri + 4, NULL, NULL);
	else
		uri_location = g_strdup (src_uri);

  	/* Maybe additionally use glib's filename to display name function */

	return uri_location;
}

/* ********** All Local Functions[ Visible outside this file but used only inside this package] Defined Here ***********/

/* ********** All Global Functions Defined Here ***********/
int8_t player_play(player_instance_t *player_instance)
{
	gchar *uri_location = NULL;
    int8_t ret_status = -1;
	
	I_ARG_CHECK( (player_instance != NULL && player_instance->pipeline != NULL && player_instance->src_uri != NULL), /* Check this condition*/
                    ret_status, /* On error return this*/
                    -1); /* with this value*/

	uri_location = play_uri_get_display_name (player_instance, player_instance->src_uri);
	I_LOG_DEBUG("Now playing %s\n", uri_location);

	g_object_set (player_instance->player, "uri", player_instance->src_uri, NULL);
	gst_player_play (player_instance->player);

	ret_status = 0;

safe_exit:
	if(uri_location)
		g_free (uri_location);
    return ret_status;
}

int8_t player_toggle_pause(player_instance_t *player_instance)
{
	int8_t ret_status = -1;
    
	I_ARG_CHECK( (player_instance != NULL), /* Check this condition*/
                    ret_status, /* On error return this*/
                    -1); /* with this value*/
	
    if (player_instance->desired_state == GST_STATE_PLAYING)
    {
        player_instance->desired_state = GST_STATE_PAUSED;
        gst_player_pause (player_instance->player);
    }
    else
    {
        player_instance->desired_state = GST_STATE_PLAYING;
        gst_player_play (player_instance->player);
    }
        
    ret_status = 0;

safe_exit:
    return ret_status;
}

int8_t player_stop(player_instance_t *player_instance)
{
	int8_t ret_status = -1;

    I_ARG_CHECK( (player_instance != NULL), /* Check this condition*/
                    ret_status, /* On error return this*/
                    -1); /* with this value*/

    gst_player_stop (player_instance->player);
    ret_status = 0;
    
safe_exit:
	return ret_status;
}

void player_relative_seek (player_instance_t *player_instance, gdouble percent)
{
	gint64 dur = -1, pos = -1;

	g_return_if_fail (percent >= -1.0 && percent <= 1.0);

	g_object_get (player_instance->player, "position", &pos, "duration", &dur, NULL);

	if (dur <= 0)
	{
		I_LOG_WARNING("!!!!!!!!!! Could Not Seek !!!!!!!!!!\n");
		return;
	}

	pos = pos + (gint64)((gdouble)dur * percent);
	if (pos < 0)
		pos = 0;
	gst_player_seek (player_instance->player, (GstClockTime)pos);
	return;
}


void play_set_relative_volume (player_instance_t *player_instance, gdouble volume_step)
{
	gdouble volume;

	g_object_get (player_instance->player, "volume", &volume, NULL);
	volume = round ((volume + volume_step) * VOLUME_STEPS) / VOLUME_STEPS;
	volume = CLAMP (volume, 0.0, 10.0);

	g_object_set (player_instance->player, "volume", volume, NULL);

	I_LOG_DEBUG("Volume: %.0f%%                  \n", volume * 100);

	return;
}

int8_t player_toggle_fullscreen(player_instance_t *player_instance)
{
	int8_t ret_status = -1;
    
    I_ARG_CHECK( (player_instance != NULL), /* Check this condition*/
                    ret_status, /* On error return this*/
                    -1); /* with this value*/
    
    /* Toggle between fullscreen mode*/
    I_LOG_DEBUG("Key f Pressed Window is FullSCreen %d\n", player_instance->vid_win.in_fullscreen);
    if (player_instance->vid_win.in_fullscreen)
    {
        I_LOG_TRACE("Un-FullScreen\n");
        dispmanx_win_set_fullscreen(&player_instance->vid_win, FALSE);
    }
    else
    {
        I_LOG_TRACE("FullScreen\n");
        dispmanx_win_set_fullscreen(&player_instance->vid_win, TRUE);
    }
    ret_status = 0;
    
safe_exit:
	return ret_status;
}

int8_t player_toggle_background(player_instance_t *player_instance)
{
	int8_t ret_status = -1;
    
    I_ARG_CHECK( (player_instance != NULL), /* Check this condition*/
                    ret_status, /* On error return this*/
                    -1); /* with this value*/
    
    /* Toggle background */
    I_LOG_TRACE("Key b Pressed , toggle background (%d)\n", player_instance->bg.opacity);
    if(player_instance->bg.opacity != 0)
        dispmanx_win_show_background_element(&player_instance->bg, FALSE);
    else
        dispmanx_win_show_background_element(&player_instance->bg, TRUE);
    ret_status = 0;
    
safe_exit:
	return ret_status;
}

int8_t player_toggle_aspect_ratio(player_instance_t *player_instance)
{
	int8_t ret_status = -1;
    
    I_ARG_CHECK( (player_instance != NULL), /* Check this condition*/
                    ret_status, /* On error return this*/
                    -1); /* with this value*/
                    
    /* Toggle between PLAYER_AR_ORIGINAL & PLAYER_AR_STRETCH*/
    I_LOG_TRACE("Key r Pressed Window is FullSCreen %d\n", player_instance->vid_win.in_fullscreen);
    if(player_instance->vid_win.ar == PLAYER_AR_STRETCH)
        dispmanx_win_set_aspect_ratio(player_instance, PLAYER_AR_ORIGINAL);
    else
        dispmanx_win_set_aspect_ratio(player_instance, PLAYER_AR_STRETCH);

    ret_status = 0;
    
safe_exit:
	return ret_status;
}

int8_t player_get_handler(const char *src_uri, const char *dest_uri, i_player_signal_handlers_t *sig_handlers, player_instance_t **player_instance)
{
    player_instance_t *new_player_instance = NULL;

    int8_t ret_status = -1;
	
    if(src_uri == NULL || player_instance == NULL)
    {
        I_LOG_ERROR("xxxxxxxxxx Invalid Argument xxxxxxxxxx\n");
        goto safe_exit;
    }

    /* Alloate memory for instance*/
    new_player_instance = g_new0 (player_instance_t, 1);
    if(new_player_instance == NULL)
    {
        I_LOG_FATAL("xxxxxxxxxx Malloc Failed xxxxxxxxxx\n");
        goto safe_exit;
    }
  
	/* create video renderer */
  	if (dispmanx_create_video_window(new_player_instance) == FALSE)
	{   
		I_LOG_ERROR("xxxxxxxxxx Couldnt Create Player Window xxxxxxxxxx\n");
	}
	else
	{
		I_LOG_DEBUG("Rcived  window handle : %p\n", new_player_instance->video_window_handle);
	}

    new_player_instance->renderer = gst_player_video_overlay_video_renderer_new (new_player_instance->video_window_handle);

    /* Create gst player */
	new_player_instance->player = gst_player_new (new_player_instance->renderer, gst_player_g_main_context_signal_dispatcher_new(NULL));
	new_player_instance->pipeline = gst_player_get_pipeline (new_player_instance->player);
	new_player_instance->bus = gst_element_get_bus (new_player_instance->pipeline);
    if (!(new_player_instance->pipeline))
    {
        I_LOG_FATAL("xxxxxxxxxx Couldnt Create Player Pipeline xxxxxxxxxx\n");
        goto safe_exit;
    }

    /* Initialize with default values*/

	if (gst_uri_is_valid (src_uri))
		new_player_instance->src_uri = g_strdup(src_uri);
	else
    {
        I_LOG_WARNING("!!!!!!!!!! Invalid URI... Fixing It !!!!!!!!!! %s\n", src_uri);
		new_player_instance->src_uri = gst_filename_to_uri (src_uri, NULL);
    }

	I_LOG_INFO("============ %s\n", new_player_instance->src_uri); 

    new_player_instance->dest_uri = (dest_uri) ? g_strdup(dest_uri) : NULL; /* TODO For future use*/
	new_player_instance->desired_state = GST_STATE_PLAYING;
    new_player_instance->volume = 1.0;

   	play_set_relative_volume (new_player_instance, new_player_instance->volume - 1.0);

    /* register signal handlers*/
    if(sig_handlers)
    {
        if(sig_handlers->position_updated_cb)
		  	g_signal_connect (new_player_instance->player, "position-updated", G_CALLBACK (sig_handlers->position_updated_cb), new_player_instance);
        if(sig_handlers->state_changed_cb)    
            g_signal_connect (new_player_instance->player, "state-changed", G_CALLBACK (sig_handlers->state_changed_cb), new_player_instance); 
        if(sig_handlers->buffering_cb)
            g_signal_connect (new_player_instance->player, "buffering", G_CALLBACK (sig_handlers->buffering_cb), new_player_instance);
        if(sig_handlers->end_of_stream_cb)
            g_signal_connect (new_player_instance->player, "end-of-stream", G_CALLBACK (sig_handlers->end_of_stream_cb), new_player_instance);
        if(sig_handlers->error_cb)
            g_signal_connect (new_player_instance->player, "error", G_CALLBACK (sig_handlers->error_cb), new_player_instance);
        if(sig_handlers->media_info_cb)
            g_signal_connect (new_player_instance->player, "media-info-updated", G_CALLBACK (sig_handlers->media_info_cb), new_player_instance);
    }

    ret_status = 0;
    *player_instance = new_player_instance;

safe_exit:
    if(ret_status != 0)
    {
        player_release(*player_instance);
        *player_instance = NULL;
    }
    return ret_status;
}

void player_release(player_instance_t *player_instance)
{
    if(player_instance)
    {
        I_LOG_INFO("~~~~~~~~~~ Freeing Player [%s] ~~~~~~~~~~\n", player_instance->player_name ? player_instance->player_name: "Unknown Player");
     
		if (player_instance->player)
		{
            play_reset(player_instance);
			gst_player_stop (player_instance->player);
			gst_object_unref (player_instance->player);
		}
   
        dispmanx_win_show_background_element(&player_instance->bg, FALSE);
        dispmanx_win_destroy_background_element(&player_instance->bg);

        if(player_instance->bus)
            gst_object_unref (player_instance->bus);
        if(player_instance->src_uri)
            g_free(player_instance->src_uri);
        if(player_instance->dest_uri)
            g_free(player_instance->dest_uri);
    
        I_ZEROMEM(player_instance, (sizeof(player_instance_t)));

        free(player_instance);
    }
    return;
}

void player_init()
{

    s_player_context_thread = g_thread_try_new ("PlayerLoopThread",
                                s_player_loop,
                                NULL,
							   	NULL);
    I_LOG_DEBUG("Player Loop Thread Created : %p\n", s_player_context_thread);
	return;
}

void player_shutdown()
{
	if(s_player_main_loop)
		g_main_quit(s_player_main_loop);
	if(s_player_context_thread)
    {
		g_thread_join(s_player_context_thread);
		s_player_context_thread = NULL;
    }
    return;
}
