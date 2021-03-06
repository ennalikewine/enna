/*
 * Enna Media Center
 * Copyright (C) 2005-2013 Enna Team. All rights reserved.
 *
 * This file is part of Enna.
 *
 * Enna is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Enna is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Enna; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "config.h"

#define _GNU_SOURCE
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include <Edje.h>
#include <Ecore.h>
#include <Ecore_File.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "utils.h"
#include "mainmenu.h"
#include "exit.h"
#include "module.h"
#include "content.h"
#include "logs.h"
#include "volumes.h"
#include "metadata.h"
#include "mediaplayer.h"
#include "input.h"
#include "gadgets.h"
#include "videoplayer_obj.h"

#ifdef HAVE_ECORE_X
#include <Ecore_X.h>
#endif

#ifndef ELM_LIB_QUICKLAUNCH

#define EDJE_GROUP_MAIN_LAYOUT "enna/main/layout"
#define EDJE_PART_MAINMENU_SWALLOW "enna.mainmenu.swallow"

/* seconds after which the mouse pointer disappears*/
#define ENNA_MOUSE_IDLE_TIMEOUT 10


/* Global Variable Enna *enna*/
Enna *enna;

static char *conffile = NULL;
static const char *app_theme = NULL;
static unsigned int app_w = 1280;
static unsigned int app_h = 720;
static unsigned int app_x_off = 0;
static unsigned int app_y_off = 0;
static int run_fullscreen = 0;

/* Functions */
static int _create_gui(void);

/* Callbacks */

static Eina_Bool
_reset_screensaver_cb(void *data EINA_UNUSED)
{
#ifdef HAVE_ECORE_X
    /* Disable screensaver */
    ecore_x_screensaver_set(0, 0, 0, 0);
    ecore_x_screensaver_custom_blanking_disable();
#endif 
    return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_idle_timer_cb(void *data EINA_UNUSED)
{
    if (enna_exit_visible())
    {
        enna_log(ENNA_MSG_INFO, NULL, "gracetime is over, quitting enna.");
        ecore_main_loop_quit();
    }
    else
    {
      if (enna_mediaplayer_state_get() == PLAYING)
      {
          enna_log(ENNA_MSG_INFO, NULL, "still playing, renewing idle timer");
          return ECORE_CALLBACK_RENEW;
      }
      else if (enna_activity_request_quit_all())
      {
          enna_log(ENNA_MSG_INFO, NULL,
                   "at least one activity's busy, renewing idle timer");
          return ECORE_CALLBACK_RENEW;
      }
      else
      {
        enna_log(ENNA_MSG_INFO, NULL,
                 "enna seems to be idle, sending quit msg and waiting 20s");
        enna_input_event_emit(ENNA_INPUT_QUIT);
        enna->idle_timer = ecore_timer_add(20, _idle_timer_cb, NULL);
      }
    }
    return ECORE_CALLBACK_CANCEL;
}

static void
_set_scale(int h)
{
    double scale = 1.0;
    if (h)
        scale = (h / 720.0);
    if (scale >= 1.0)
        scale = 1.0;
    //elm_scale_set(scale);

    enna_log(ENNA_MSG_INFO, NULL, "Scale: %3.3f", scale);
}


static void _window_delete_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
    ecore_main_loop_quit();
}

static void
_window_resize_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
    Evas_Coord w, h;

    evas_object_geometry_get(enna->win, NULL, NULL, &w, &h);
    evas_object_resize(enna->layout, w - 2 * app_x_off, h - 2 * app_y_off);
    evas_object_move(enna->layout, app_x_off, app_y_off);
    _set_scale(h);
}

static void
_button_back_clicked_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
    if (enna_mainmenu_visible(enna->o_menu))
        enna_input_event_emit(ENNA_INPUT_QUIT);
    else
        enna_input_event_emit(ENNA_INPUT_BACK);
}


static void _list_engines(void)
{
    Eina_List  *lst;
    Eina_List  *n;
    const char *engine;

    enna_log(ENNA_MSG_INFO, NULL, "Supported engines:");

    lst = ecore_evas_engines_get();

    EINA_LIST_FOREACH(lst, n, engine)
    {
        if (strcmp(engine, "buffer") != 0)
            enna_log(ENNA_MSG_INFO, NULL, "\t* %s", engine);
    }

    ecore_evas_engines_free(lst);
}

/* Functions */

static const struct {
    const char *name;
    enna_msg_level_t lvl;
} msg_level_mapping[] = {
    { "none",       ENNA_MSG_NONE     },
    { "event",      ENNA_MSG_EVENT    },
    { "info",       ENNA_MSG_INFO     },
    { "warning",    ENNA_MSG_WARNING  },
    { "error",      ENNA_MSG_ERROR    },
    { "critical",   ENNA_MSG_CRITICAL },
    { NULL,         0                 }
};

static void _elm_init (int argc, char **argv)
{
    setenv("ELM_ENGINE", enna_config->engine, 1);
    elm_init(argc, argv);
}

static int _enna_init(int argc, char **argv)
{
    int i;
    Eina_List *l;
    Enna_Class_Activity *a;

    enna->lvl = ENNA_MSG_INFO;

    /* register configuration parsers */
    enna_main_cfg_register();
    enna_mediaplayer_cfg_register();
    enna_videoplayer_obj_cfg_register();
    enna_metadata_cfg_register();

    enna_module_init();
    enna_config_set_default();
    enna_config_load();

    if (app_theme)
    {
        ENNA_FREE(enna_config->theme);
        enna_config->theme = strdup(app_theme);
    }
    enna_config_load_theme();

    enna_log(ENNA_MSG_INFO, NULL, "enna log file : %s\n",
             enna_config->log_file);
    enna_log_init(enna_config->log_file);

    if (enna_config->verbosity)
    {
        for (i = 0; msg_level_mapping[i].name; i++)
            if (!strcmp (enna_config->verbosity, msg_level_mapping[i].name))
            {
                enna->lvl = msg_level_mapping[i].lvl;
                break;
            }
    }

    enna->slideshow_delay = enna_config->slideshow_delay;


    /* Create ecore events (we should put here ALL the event_type_new) */
    ENNA_EVENT_BROWSER_CHANGED = ecore_event_type_new();
    /* try to init the requested video engine */
    if (!_create_gui())
    {
        /* try to init with failsafe settings (software_x11) */
        enna_log(ENNA_MSG_WARNING, NULL,
                 "Requested engine '%s' has failed to register, " \
                 "using software_x11 as a default.", enna_config->engine);
        ENNA_FREE(enna_config->engine);
#ifndef WIN32
        enna_config->engine = strdup("software_x11");
#else
        enna_config->engine = strdup("software_gdi");
#endif
        _elm_init(argc, argv);
        if (!_create_gui())
            return 0;
    }

    /* Init various stuff */
    enna_metadata_init ();

    if (!enna_mediaplayer_init())
        return 0;

    /* Init Gadgets */
    enna_gadgets_init();

    /* Load available modules */
    enna_module_load_all();
    enna_config_load();

    /* Dinamically init activities */
    EINA_LIST_FOREACH(enna_activities_get(), l, a)
        enna_activity_init(a->name);

    /* Show mainmenu */
    //~ enna_mainmenu_select_nth(0);

    if (enna_config->display_mouse == EINA_TRUE)
    {
        enna->idle_timer = NULL;
        enna_idle_timer_renew();
    }

#ifdef HAVE_ECORE_X
    /* Disable screensaver */
    _reset_screensaver_cb(NULL);
    ecore_timer_add(60.0, _reset_screensaver_cb, NULL);
#endif

    return 1;
}

static int _create_gui(void)
{
    Evas_Object *ic;
    Evas_Object *rect;
    Evas_Coord minw, minh;

    // set custom elementary theme
    //    elm_theme_extension_add(NULL, enna_config_theme_get());
    elm_theme_overlay_add(NULL, enna_config_theme_get());


    // show supported engines
    _list_engines();
    enna_log(ENNA_MSG_INFO, NULL, "Using engine: %s", enna_config->engine); //FIXME
    //~ ENNA_FREE(enna_config->engine);
    //~ enna_config->engine=strdup(ecore_evas_engine_name_get(enna->ee));

    // main window
    enna->win = elm_win_add(NULL, "enna", ELM_WIN_BASIC);
    if (!enna->win)
      return 0;
    elm_win_title_set(enna->win, "Enna MediaCenter");
    enna->run_fullscreen = enna_config->fullscreen | run_fullscreen;
    elm_win_fullscreen_set(enna->win, enna->run_fullscreen);
    evas_object_smart_callback_add(enna->win, "delete,request",
                                   _window_delete_cb, NULL);
    evas_object_event_callback_add(enna->win, EVAS_CALLBACK_RESIZE,
                                   _window_resize_cb, NULL);
    //~ ecore_evas_shaped_set(enna->ee, 1);  //TODO why this ???
    enna->evas = evas_object_evas_get(enna->win);
    
    /* Enable evas cache (~4 backgrounds in the cache at a time) : 1 background =  1280x720*4 = 3,7MB */
    /* ==> Set cache to 16MB */
    evas_image_cache_set(enna->evas, 4 * 4 * 1024 * 1024);
    
    /* Black rect */
    rect = evas_object_rectangle_add(enna->evas);
    evas_object_color_set(rect, 0, 0, 0, 255);
    evas_object_show(rect);
    //    elm_win_resize_object_add(enna->win, rect);

    // main layout widget
    enna->layout = elm_layout_add(enna->win);
    if (!elm_layout_file_set(enna->layout, enna_config_theme_get(), EDJE_GROUP_MAIN_LAYOUT))
    {
        CRIT("Unable to find group \"%s\" in theme %s", EDJE_GROUP_MAIN_LAYOUT, enna_config_theme_get());
        return 0;
    }
    evas_object_size_hint_weight_set(enna->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(enna->layout);

    // mainmenu
    enna->o_menu = enna_mainmenu_add(enna->win);
    elm_layout_content_set(enna->layout, EDJE_PART_MAINMENU_SWALLOW, enna->o_menu);
    enna_mainmenu_show(enna->o_menu);

    // exit dialog
    enna_exit_init();

    // Back button
    enna->o_button_back = elm_button_add(enna->layout);
    elm_object_style_set(enna->o_button_back, "mediaplayer");
    ic = elm_icon_add(enna->o_button_back);
    elm_image_file_set(ic, enna_config_theme_get(), "ctrl/shutdown");
    elm_object_content_set(enna->o_button_back, ic);

    elm_layout_content_set(enna->layout, "back.swallow", enna->o_button_back);
    evas_object_smart_callback_add(enna->o_button_back, "clicked", _button_back_clicked_cb, NULL);

    // show all
    if (!app_w || !app_h)
      {
         edje_object_size_min_get(elm_layout_edje_get(enna->layout), &minw, &minh);
         if (!minw || !minh)
           {
              app_w = 1280;
              app_h = 720;
           }
         else
           {
              app_w = minw;
              app_h = minh;
           }
      }
    printf("resize %d %d\n", app_w, app_h);
    evas_object_resize(enna->win, app_w, app_h);
    evas_object_resize(enna->layout, app_w - 2 * app_x_off, app_h - 2 * app_y_off);
    evas_object_move(enna->layout, app_x_off, app_y_off);

    //_set_scale(app_h);
    evas_object_show(enna->win);

    return 1;
}

static void _enna_shutdown(void)
{
    ENNA_TIMER_DEL(enna->idle_timer);

    enna_activity_del_all();
    enna_config_shutdown();
    enna_module_shutdown();
    enna_metadata_shutdown();
    enna_mediaplayer_shutdown();

    evas_object_del(enna->o_background);
    evas_object_del(enna->o_menu);
    evas_object_del(enna->o_content);

    enna_exit_shutdown();
    elm_shutdown();
    enna_util_shutdown();
    enna_log(ENNA_MSG_INFO, NULL, "Bye Bye !");
    enna_log_shutdown();
    ENNA_FREE(enna);
}

static void _opt_geometry_parse(const char *optargs,
                                unsigned int *pw, unsigned int *ph, unsigned int *px, unsigned int *py)
{
    int w = 0, h = 0;
    int x = 0, y = 0;
    int ret;

    ret = sscanf(optargs, "%dx%d:%d:%d", &w, &h, &x, &y);

    if ( ret != 2 && ret != 4 )
        return;

    if (pw) *pw = w;
    if (ph) *ph = h;
    if (px) *px = x;
    if (py) *py = y;
}

static const struct {
    const char *name;
    const char *theme;
    int width;
    int height;
} profile_resolution_mapping[] = {
    { "480p",       "default",   640,  480 },
    { "576p",       "default",   720,  576 },
    { "720p",       "default",  1280,  720 },
    { "1080p",      "default",  1920, 1080 },
    { "vga",        "default",   640,  480 },
    { "ntsc",       "default",   720,  480 },
    { "pal",        "default",   768,  576 },
    { "wvga",       "default",   800,  480 },
    { "svga",       "default",   800,  600 },
    { "touchbook","touchbook",  1024,  600 },
    { "netbook",    "default",  1024,  600 },
    { "hdready",    "default",  1366,  768 },
    { "fullhd",     "default",  1920, 1080 },
    { NULL,              NULL,     0,    0 }
};

static void
_opt_profile_parse (const char *optargs, const char **pt,
                    unsigned int *pw, unsigned int *ph)
{
    int i;

    if (!optargs)
        return;

    for (i = 0; profile_resolution_mapping[i].name; i++)
        if (!strcasecmp(optargs, profile_resolution_mapping[i].name))
        {
            *pt = profile_resolution_mapping[i].theme;
            *pw = profile_resolution_mapping[i].width;
            *ph = profile_resolution_mapping[i].height;
            break;
        }
}

void enna_idle_timer_renew(void)
{
    if (enna_config->idle_timeout)
    {
        if (enna->idle_timer)
        {
            ENNA_TIMER_DEL(enna->idle_timer)
        }
        else
        {
            enna_log(ENNA_MSG_INFO, NULL,
                     "setting up idle timer to %i minutes",
                     enna_config->idle_timeout);
        }
        if (!(enna->idle_timer = ecore_timer_add(enna_config->idle_timeout*60,
                                                 _idle_timer_cb, NULL)))
        {
            enna_log(ENNA_MSG_CRITICAL, NULL, "adding timer failed!");
        }
    }
}

static Eina_Bool
exit_signal(void *data EINA_UNUSED, int type EINA_UNUSED, void *e)
{
    Ecore_Event_Signal_Exit *event = e;

    fprintf(stderr,
            "Enna got exit signal [interrupt=%u, quit=%u, terminate=%u]\n",
            event->interrupt, event->quit, event->terminate);

    ecore_main_loop_quit();
    return 1;
}


static void usage(char *binname)
{
    int i;

    printf(_("Enna MediaCenter\n"));
    printf(_(" Usage: %s [options ...]\n"), binname);
    printf(_(" Available options:\n"));
    printf(_("  -c, (--config):  Specify configuration file to be used.\n"));
    printf(_("  -f, (--fs):      Force fullscreen mode.\n"));
    printf(_("  -h, (--help):    Display this help.\n"));
    printf(_("  -t, (--theme):   Specify theme name to be used.\n"));
    printf(_("  -g, (--geometry):Specify window geometry. (geometry=1280x720)\n"));
    printf(_("  -g, (--geometry):Specify window geometry and offset. (geometry=1280x720:10:20)\n"));
    printf(_("  -p, (--profile): Specify display profile\n"));
    printf(_("    Supported: "));
    for (i = 0; profile_resolution_mapping[i].name; i++)
        printf("%s ", profile_resolution_mapping[i].name);
    printf("\n");
    printf(_("  -V, (--version): Display Enna version number.\n"));
    exit(EXIT_SUCCESS);
}

static void version(void)
{
    printf(PACKAGE_STRING"\n");
    exit(EXIT_SUCCESS);
}

static int parse_command_line(int argc, char **argv)
{
    int c, idx;
    char short_options[] = "Vhfc:t:b:g:p:";
    struct option long_options [] =
        {
            { "help",          no_argument,       0, 'h' },
            { "version",       no_argument,       0, 'V' },
            { "fs",            no_argument,       0, 'f' },
            { "config",        required_argument, 0, 'c' },
            { "theme",         required_argument, 0, 't' },
            { "geometry",      required_argument, 0, 'g' },
            { "profile",       required_argument, 0, 'p' },
            { 0,               0,                 0,  0  }
        };

    /* command line argument processing */
    while (1)
    {
        c = getopt_long(argc, argv, short_options, long_options, &idx);

        if (c == EOF)
            break;

        switch (c)
        {
        case 0:
            /* opt = long_options[idx].name; */
            break;

        case '?':
        case 'h':
            usage(argv[0]);
        return -1;

        case 'V':
            version();
            break;

        case 'f':
            run_fullscreen = 1;
            break;

        case 'c':
            conffile = strdup(optarg);
            break;

        case 't':
            app_theme = strdup(optarg);
            break;

        case 'g':
            _opt_geometry_parse(optarg, &app_w, &app_h, &app_x_off, &app_y_off);
            break;
        case 'p':
            _opt_profile_parse(optarg, &app_theme, &app_w, &app_h);
            break;

        default:
            usage(argv[0]);
            return -1;
        }
    }

    return 0;
}

EAPI_MAIN int
elm_main(int argc, char **argv)
{
    int res = EXIT_FAILURE;

    init_locale();

    if (parse_command_line(argc, argv) < 0)
        return EXIT_SUCCESS;

    enna_util_init();

    /* Must be called first */
    enna_config_init(conffile);
    ENNA_FREE(conffile);

    enna = calloc(1, sizeof(Enna));

    if (!_enna_init(argc, argv))
        goto out;

    ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, exit_signal, enna);
    ecore_main_loop_begin();

    _enna_shutdown();
    res = EXIT_SUCCESS;

 out:
    return res;
}

#endif
ELM_MAIN()
