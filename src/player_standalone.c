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
#include <ctype.h>
#include <termio.h>

#include <player.h>
#include <dispmanx_window.h>

static int s_stdin_fd = -1; 
static struct termios s_original;
pthread_t s_kbd_event_thread;
gboolean s_event_run = TRUE;
 
static GMainLoop *s_player_main_loop = NULL;

void * s_on_key_pressed(void *user_data);
static void s_init_keyboard_input(player_instance_t *player_instance);
static gboolean s_key_is_pressed(int32_t *character);

static void buffering_cb (GstPlayer * player, gint percent, player_instance_t *player_instance);
static void state_changed_cb (GstPlayer * player, GstPlayerState state, player_instance_t *player_instance);
static void position_updated_cb (GstPlayer * player, GstClockTime pos, player_instance_t *player_instance);
static void error_cb (GstPlayer * player, GError * err, player_instance_t *player_instance);
static void end_of_stream_cb (GstPlayer * player, player_instance_t *player_instance);
static void media_info_cb (GstPlayer * player, GstPlayerMediaInfo * info, player_instance_t *player_instance);

i_player_signal_handlers_t s_sig_handlers = 
{
    G_CALLBACK (position_updated_cb),
    G_CALLBACK (state_changed_cb),
    G_CALLBACK (buffering_cb),
    G_CALLBACK (end_of_stream_cb),
    G_CALLBACK (error_cb),
    G_CALLBACK (media_info_cb)
};

/* static function*/

static void end_of_stream_cb (GstPlayer * player, player_instance_t *player_instance)
{
    I_LOG_INFO("========== Reached end of stream ==========\n");
	return;
}

static void error_cb (GstPlayer * player, GError * err, player_instance_t *player_instance)
{
    I_LOG_ERROR("xxxxxxxxxx Playback Error xxxxxxxxxx  %s for %s\n", err->message, player_instance->src_uri)
    return;
}

static void position_updated_cb (GstPlayer * player, GstClockTime pos, player_instance_t *player_instance)
{
    GstClockTime dur = (GstClockTime)-1; 
    gchar status[64] = { 0, };

    g_object_get (player_instance->player, "duration", &dur, NULL);

    memset (status, ' ', sizeof (status) - 1); 

    if (pos != -1 && dur > 0 && dur != -1)
    {
        gchar dstr[32], pstr[32];

        /* FIXME: pretty print in nicer format */
        g_snprintf (pstr, 32, "%" GST_TIME_FORMAT, GST_TIME_ARGS (pos));
        pstr[9] = '\0';
        g_snprintf (dstr, 32, "%" GST_TIME_FORMAT, GST_TIME_ARGS (dur));
        dstr[9] = '\0';
        /*I_LOG_USER("%s / %s %s\r", pstr, dstr, status);*/
        g_print ("%s / %s %s\r", pstr, dstr, status);
    }
    return;
}

static void state_changed_cb (GstPlayer * player, GstPlayerState state, player_instance_t *player_instance)
{
    I_LOG_INFO ("========== State changed ========== %s\n", gst_player_state_get_name (state));
    return;
}

static void buffering_cb (GstPlayer * player, gint percent, player_instance_t *player_instance)
{
    I_LOG_INFO("========== Buffering ========== %d%% \r", percent);
    return;
}

static void print_video_info (GstPlayerVideoInfo * info)
{
  gint fps_n, fps_d;
  guint par_n, par_d;

  if (info == NULL)
    return;

  g_print ("  width : %d\n", gst_player_video_info_get_width (info));
  g_print ("  height : %d\n", gst_player_video_info_get_height (info));
  g_print ("  max_bitrate : %d\n",
      gst_player_video_info_get_max_bitrate (info));
  g_print ("  bitrate : %d\n", gst_player_video_info_get_bitrate (info));
  gst_player_video_info_get_framerate (info, &fps_n, &fps_d);
  g_print ("  framerate : %.2f\n", (gdouble) fps_n / fps_d);
  gst_player_video_info_get_pixel_aspect_ratio (info, &par_n, &par_d);
  g_print ("  pixel-aspect-ratio  %u:%u\n", par_n, par_d);
}

static void print_audio_info (GstPlayerAudioInfo * info)
{
  if (info == NULL)
    return;

  g_print ("  sample rate : %d\n",
      gst_player_audio_info_get_sample_rate (info));
  g_print ("  channels : %d\n", gst_player_audio_info_get_channels (info));
  g_print ("  max_bitrate : %d\n",
      gst_player_audio_info_get_max_bitrate (info));
  g_print ("  bitrate : %d\n", gst_player_audio_info_get_bitrate (info));
  g_print ("  language : %s\n", gst_player_audio_info_get_language (info));
}

static void print_subtitle_info (GstPlayerSubtitleInfo * info)
{
  if (info == NULL)
    return;

  g_print ("  language : %s\n", gst_player_subtitle_info_get_language (info));
}


static void print_current_tracks (player_instance_t *player_instance)
{
   	GstPlayerAudioInfo *audio = NULL;
	GstPlayerVideoInfo *video = NULL;
	GstPlayerSubtitleInfo *subtitle = NULL;

	g_print ("Current video track: \n");
	video = gst_player_get_current_video_track (player_instance->player);
	print_video_info (video);

	g_print ("Current audio track: \n");
	audio = gst_player_get_current_audio_track (player_instance->player);
	print_audio_info (audio);

	g_print ("Current subtitle track: \n");
	subtitle = gst_player_get_current_subtitle_track (player_instance->player);
	print_subtitle_info (subtitle);

	if (audio)
		g_object_unref (audio);

	if (video)
		g_object_unref (video);

	if (subtitle)
		g_object_unref (subtitle);

	return;
}


static void media_info_cb (GstPlayer * player, GstPlayerMediaInfo * info, player_instance_t *player_instance)
{
	GstPlayerVideoInfo *video = NULL;

	video = gst_player_get_current_video_track (player_instance->player);
	if(video)
	{
		gint width = 0, height = 0;

		width = gst_player_video_info_get_width (video);
		height = gst_player_video_info_get_height (video);

		if(player_instance->vid_win.vid_width != width || player_instance->vid_win.vid_height != height)
		{
			I_LOG_WARNING("?????????? Video Resolution Changed ?????????? %d %d ==> %d %d\n", player_instance->vid_win.vid_width , player_instance->vid_win.vid_height, width, height);
			player_instance->vid_win.vid_width = (guint)width;
			player_instance->vid_win.vid_height = (guint)height;
			/*once we have video dimensions we can call fullscreen*/
			dispmanx_win_set_fullscreen(&player_instance->vid_win, TRUE);
		}
  	}
	return;
}


void * s_on_key_pressed(void *user_data)
{
    player_instance_t *player_instance = (player_instance_t *) user_data;

    while (s_event_run)
    {   
        int32_t c = 0;
        if (s_key_is_pressed(&c))
        {   
            c = tolower(c);
			switch (c)
			{
				case ' ':
				{
					I_LOG_DEBUG("Key ' ' Pressed\n");
                    player_toggle_pause(player_instance);
					break;
				}
                case 27:
                case 'q':
                {
                    I_LOG_DEBUG("Key q/Esc Pressed\n");
                    s_event_run = FALSE;
                    player_release(player_instance);
                    g_main_quit(s_player_main_loop);
                    break;
                }
                case 'b':
                {
                    /* Toggle background */
                    I_LOG_TRACE("Key b Pressed , toggle background (%d)\n", player_instance->bg.opacity);
                    if(player_instance->bg.opacity != 0)
                        dispmanx_win_show_background_element(&player_instance->bg, FALSE);
                    else
                        dispmanx_win_show_background_element(&player_instance->bg, TRUE);
                    break;
                }
                case 'r':
                {
                    /* Toggle between PLAYER_AR_ORIGINAL & PLAYER_AR_STRETCH*/
                    I_LOG_TRACE("Key r Pressed Window is FullSCreen %d\n", player_instance->vid_win.in_fullscreen);
                    if(player_instance->vid_win.ar == PLAYER_AR_STRETCH)
                        dispmanx_win_set_aspect_ratio(player_instance, PLAYER_AR_ORIGINAL);
                    else
                        dispmanx_win_set_aspect_ratio(player_instance, PLAYER_AR_STRETCH);

                    break;
                }
                case 'f':
                {
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
                    break;
                }
				case 'w':
                    dispmanx_win_move(&player_instance->vid_win, 0, -(WIN_MOVE_STEPS));
                    break;
				case 's':
                    dispmanx_win_move(&player_instance->vid_win, 0, WIN_MOVE_STEPS);
                    break;
				case 'a':
                    dispmanx_win_move(&player_instance->vid_win, -(WIN_MOVE_STEPS), 0);
                    break;
				case 'd':
                    dispmanx_win_move(&player_instance->vid_win, WIN_MOVE_STEPS, 0);
                    break;
                case 'i':
                    print_current_tracks(player_instance);
                    break;
                case '1':
                    if(player_instance->src_uri)
                    {
                        g_free(player_instance->src_uri);
                        player_instance->src_uri = g_strdup("file:/mnt/big_buck_bunny_720p_h264.mov");
                        player_play(player_instance);
                    }
                    break;
                case '2':
                    if(player_instance->src_uri)
                    {
                        g_free(player_instance->src_uri);
                        player_instance->src_uri = g_strdup("file:/mnt/8_Min_Abs_Workout_how_to_have_six_pack_HD_Version.mp4");
                        player_play(player_instance);
                    }
                    break;
                case '3':
                    if(player_instance->src_uri)
                    {
                        g_free(player_instance->src_uri);
                        player_instance->src_uri = g_strdup("file:/mnt/the_wolverine.mkv");
                        player_play(player_instance);
                    }
                    break;
                default:
                    /*player_relative_seek(player_instance, +0.08);*/
                    break;
            } /* switch*/
        }/*ke_pressed*/
    }/*while*/

    return NULL;
}

static void s_init_keyboard_input(player_instance_t *player_instance)
{
	struct termios term;

    if (s_stdin_fd == -1) 
    {   
        s_stdin_fd = fileno(stdin);

        /*Get the terminal (termios) attritubets for stdin*/ 
        tcgetattr(s_stdin_fd, &s_original);

        /*Copy the termios attributes so we can modify them.*/
        memcpy(&term, &s_original, sizeof(term));

        /*Unset ICANON and ECHO for stdin. */
        term.c_lflag &= (tcflag_t)~(ICANON|ECHO);
        tcsetattr(s_stdin_fd, TCSANOW, &term);

        /*Turn off buffering for stdin.*/
        setbuf(stdin, NULL);

        pthread_create(&s_kbd_event_thread, NULL, s_on_key_pressed, player_instance);
    }
    return;
}

static void s_reset_keyboard_input()
{
    if(s_stdin_fd != -1)
    {
        tcsetattr(s_stdin_fd, TCSANOW, &s_original);
        s_stdin_fd = -1;
    }
    pthread_join(s_kbd_event_thread, NULL);
    return;
}


static gboolean s_key_is_pressed(int32_t *character)
{
    int characters_buffered = 0;
	gboolean pressed = FALSE; 

	if(character == NULL)
	{
		I_LOG_WARNING("Invalid Argument\n");
		return pressed;
	}

	/*Get the number of characters that are waiting to be read.*/
    ioctl(s_stdin_fd, FIONREAD, &characters_buffered);
	pressed = (characters_buffered != 0);

    if (characters_buffered == 1)
		*character = fgetc(stdin);
	else if(characters_buffered > 0)
		fflush(stdin);

	return pressed;
}


/* ********** All Local Functions[ Visible outside this file but used only inside this package] Defined Here ***********/


/* ********** All Global Functions Defined Here ***********/


/* ********** Main Goes Here ***********/

int32_t main(int32_t argc,char *argv[])
{
    player_instance_t *player_instance = NULL;

    if(argv[1] == NULL)
    {
        I_LOG_FATAL("%s <url to play>\n", argv[0]);
        return -1;
    }

    /* Main Loop Init*/
    s_player_main_loop = g_main_loop_new (NULL, FALSE);

    /* Initialize Dispmanx windowsystem*/
    dispmanx_initialize_window_system();

    gst_init (&argc, &argv);
    
    /* Player Init*/
    player_init();

    player_get_handler(argv[1], NULL, &s_sig_handlers, &player_instance);
    s_init_keyboard_input(player_instance);
    player_play(player_instance);

    g_main_loop_run (s_player_main_loop); /* Blocked until g_main_quit is called*/
    g_main_loop_unref (s_player_main_loop);

    /*Do cleanups here*/
    s_reset_keyboard_input();

    player_shutdown();
    
    /* De-Initialize Dispmanx windowsystem*/
    dispmanx_shutdown_window_system();

    return 0;
}
