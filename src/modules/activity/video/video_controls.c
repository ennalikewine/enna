/*
 * Copyright (C) 2005-2009 The Enna Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software and its Copyright notices. In addition publicly
 * documented acknowledgment must be given that this software has been used if
 * no source code of this software is made available publicly. This includes
 * acknowledgments in either Copyright notices, Manuals, Publicity and
 * Marketing documents or any documentation provided with any product
 * containing this software. This License does not apply to any software that
 * links to the libraries provided by this software (statically or
 * dynamically), but only to the software provided.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <Edje.h>

#include "enna.h"
#include "enna_config.h"
#include "video.h"
#include "video_controls.h"

#define EDJE_GROUP "activity/video/controls"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Object *o_edje;
    Evas_Object *o_back;
};

static void
video_controls_del(void *data, Evas *a, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    ENNA_OBJECT_DEL(sd->o_back);
    ENNA_FREE(sd);
}

static void
cb_back_clicked(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    media_controls_display(0);
}

/****************************************************************************/
/*                               Public API                                 */
/****************************************************************************/

Evas_Object *
enna_video_controls_add(Evas * evas)
{
    Smart_Data *sd;

    sd = calloc(1, sizeof(Smart_Data));

    sd->o_edje = edje_object_add(evas);
    edje_object_file_set(sd->o_edje, enna_config_theme_get(), EDJE_GROUP);
    evas_object_show(sd->o_edje);
    evas_object_data_set(sd->o_edje, "sd", sd);
    evas_object_event_callback_add(sd->o_edje, EVAS_CALLBACK_DEL,
                                   video_controls_del, sd);

    sd->o_back = edje_object_add(evas_object_evas_get(sd->o_edje));
    edje_object_file_set(sd->o_back,
                         enna_config_theme_get(), "ctrl/hibernate");
    edje_object_part_swallow(sd->o_edje,
                             "controls.back.btn", sd->o_back);
    evas_object_event_callback_add (sd->o_back, EVAS_CALLBACK_MOUSE_DOWN,
                                    cb_back_clicked, sd);

    return sd->o_edje;
}

void
enna_video_controls_set_title(Evas_Object *obj, const char *title)
{
    Smart_Data *sd;

    sd = evas_object_data_get(obj, "sd");
    if (!sd)
        return;

    edje_object_part_text_set(sd->o_edje, "controls.title.str",
                              title ? title : "");
}