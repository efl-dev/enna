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

#include <string.h>

#include <Edje.h>
#include <Ecore_File.h>

#include "enna.h"
#include "enna_config.h"
#include "metadata.h"
#include "image.h"
#include "logs.h"
#include "mediaplayer.h"
#include "smart_player.h"

#define ENNA_MODULE_NAME "video"

#define SMART_NAME "smart_player"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *o_edje;
    Evas_Object *o_snapshot;
    Evas_Object *o_snapshot_old;
    Evas_Object *o_cover;
    Evas_Object *o_cover_old;
    Evas_Object *o_fs;
};

/* local subsystem functions */
static void _enna_mediaplayer_smart_reconfigure(Smart_Data * sd);
static void _enna_mediaplayer_smart_init(void);
static void _e_smart_add(Evas_Object * obj);
static void _e_smart_del(Evas_Object * obj);
static void _e_smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y);
static void _e_smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h);
static void _e_smart_show(Evas_Object * obj);
static void _e_smart_hide(Evas_Object * obj);
static void _e_smart_color_set(Evas_Object * obj, int r, int g, int b, int a);
static void _e_smart_clip_set(Evas_Object * obj, Evas_Object * clip);
static void _e_smart_clip_unset(Evas_Object * obj);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
Evas_Object *
enna_smart_player_add(Evas * evas)
{
    _enna_mediaplayer_smart_init();
    return evas_object_smart_add(evas, _e_smart);
}

void enna_smart_player_show_video(Evas_Object *obj)
{
    Evas_Object *o_video;

    API_ENTRY
    ;

    o_video = enna_mediaplayer_video_obj_get();
    if (o_video)
        edje_object_part_swallow(enna->o_edje, "enna.swallow.fullscreen",
                o_video);

}

void enna_smart_player_hide_video(Evas_Object *obj)
{
    Evas_Object *o_video;

    API_ENTRY
    ;

    o_video = enna_mediaplayer_video_obj_get();
    if (o_video)
    {
        edje_object_part_unswallow(enna->o_edje, o_video);
        evas_object_hide(o_video);
    }
}

void enna_smart_player_snapshot_set(Evas_Object *obj,
                                         Enna_Metadata *metadata)
{
    char *snap_file = NULL;

    API_ENTRY;

    if (!metadata)
        return;

    snap_file = metadata->backdrop ? metadata->backdrop : metadata->snapshot;
    if (snap_file)
    {
        Evas_Coord ow,oh;
        enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "snapshot filename : %s", snap_file);

        /* FIXME : add edje cb at end of snapshot transition to switch properly snapshots*/
        sd->o_snapshot_old = sd->o_snapshot;
        sd->o_snapshot = enna_image_add(evas_object_evas_get(sd->o_edje));
        evas_object_show(sd->o_snapshot);

        /* Stretch image to fit the parent container */
        enna_image_fill_inside_set(sd->o_snapshot, 0);

        enna_image_file_set(sd->o_snapshot, snap_file);
        /* Full definition for image loaded */
        enna_image_size_get(sd->o_snapshot, &ow, &oh);
        enna_image_load_size_set(sd->o_snapshot, ow, oh);

        edje_object_part_swallow(sd->o_edje,
                                 "enna.swallow.snapshot", sd->o_snapshot);
        edje_object_signal_emit(sd->o_edje, "snapshot,show", "enna");
        evas_object_del(sd->o_snapshot_old);
    }
    else
    {
        edje_object_signal_emit(sd->o_edje, "snapshot,hide", "enna");
        evas_object_del(sd->o_cover);
    }
}

void enna_smart_player_cover_set(Evas_Object *obj,
                                      Enna_Metadata *metadata)
{
    char *cover_file = NULL;

    API_ENTRY;

    if (!metadata)
        return;

    cover_file = metadata->cover;
    if (cover_file)
    {
        Evas_Coord ow,oh;

        enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "cover filename : %s",
                cover_file);
        /* FIXME : add edje cb at end of cover transition to switch properly covers*/
        sd->o_cover_old = sd->o_cover;

        sd->o_cover = enna_image_add(evas_object_evas_get(sd->o_edje));
        enna_image_file_set(sd->o_cover, cover_file);
        enna_image_size_get(sd->o_cover, &ow, &oh);
        enna_image_load_size_set(sd->o_cover, ow, oh);
        enna_image_fill_inside_set(sd->o_cover, 0);
        edje_object_part_swallow(sd->o_edje, "enna.swallow.cover", sd->o_cover);
        edje_object_signal_emit(sd->o_edje, "cover,show", "enna");
        evas_object_del(sd->o_cover_old);
    }
    else
    {
        edje_object_signal_emit(sd->o_edje, "cover,hide", "enna");
        evas_object_del(sd->o_cover);
    }
}

void enna_smart_player_metadata_set(Evas_Object *obj,
        Enna_Metadata *metadata)
{
    char buf[4096];
    int h, mn;
    float len;

    API_ENTRY return;

    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "metadata set");

    if (!metadata)
        return;
    if (metadata->title)
        edje_object_part_text_set(sd->o_edje, "enna.text.title",
                ecore_file_file_get(metadata->title));
    else if (metadata->uri)
        edje_object_part_text_set(sd->o_edje, "enna.text.title",
                ecore_file_file_get(metadata->uri));

    len = metadata->length / 1000.0;
    h = (int) (len / 3600);
    mn = (int) ((len - (h * 3600)) / 60);

    snprintf(buf, sizeof(buf),
        "<hilight>Size<tab><tab><tab></hilight>:<tab>%.2f MB<br>"
        "<hilight>Length<tab><tab></hilight>:<tab>%.2d h %.2d mn<br>"
        "<hilight>Video Codec<tab></hilight>:<tab>%s<br>"
        "<hilight>Size<tab><tab><tab></hilight>:<tab>%dx%d<br>"
        "<hilight>Framerate<tab></hilight>:<tab>%.2f fps<br>"
        "<hilight>Audio Codec<tab></hilight>:<tab>%s<br>"
        "<hilight>Bitrate<tab><tab></hilight>:<tab>%i kbps<br>"
        "<hilight>Samplerate</hilight><tab>:<tab>%i Hz<br>",
        metadata->size / 1024.0 / 1024.0,
        h, mn,
        metadata->video->codec,
        metadata->video->width, metadata->video->height,
        metadata->video->framerate,
        metadata->music->codec,
        metadata->music->bitrate / 1000,
        metadata->music->samplerate);

    edje_object_part_text_set(sd->o_edje, "enna.text", buf);
}

/* local subsystem globals */
static void _enna_mediaplayer_smart_reconfigure(Smart_Data * sd)
{
    Evas_Coord x, y, w, h;

    x = sd->x;
    y = sd->y;
    w = sd->w;
    h = sd->h;

    evas_object_move(sd->o_edje, x, y);
    evas_object_resize(sd->o_edje, w, h);

}

static void _enna_mediaplayer_smart_init(void)
{
    if (_e_smart)
        return;
    static const Evas_Smart_Class sc =
    {
        SMART_NAME,
        EVAS_SMART_CLASS_VERSION,
        _e_smart_add,
        _e_smart_del,
        _e_smart_move,
        _e_smart_resize,
        _e_smart_show,
        _e_smart_hide,
        _e_smart_color_set,
        _e_smart_clip_set,
        _e_smart_clip_unset,
        NULL
    };
    _e_smart = evas_smart_class_new(&sc);
}

static void _e_smart_add(Evas_Object * obj)
{
    Smart_Data *sd;

    sd = calloc(1, sizeof(Smart_Data));
    if (!sd)
        return;
    sd->o_edje = edje_object_add(evas_object_evas_get(obj));
    edje_object_file_set(sd->o_edje, enna_config_theme_get(), "video_info");
    sd->x = 0;
    sd->y = 0;
    sd->w = 0;

    sd->h = 0;
    evas_object_smart_member_add(sd->o_edje, obj);
    evas_object_smart_data_set(obj, sd);
}

static void _e_smart_del(Evas_Object * obj)
{
    Smart_Data *sd;
    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;
    ENNA_OBJECT_DEL(sd->o_snapshot);
    ENNA_OBJECT_DEL(sd->o_snapshot_old);
    ENNA_OBJECT_DEL(sd->o_cover);
    ENNA_OBJECT_DEL(sd->o_cover_old);
    ENNA_OBJECT_DEL(sd->o_edje);
    free(sd);
}

static void _e_smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;
    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _enna_mediaplayer_smart_reconfigure(sd);
}

static void _e_smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;
    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;
    _enna_mediaplayer_smart_reconfigure(sd);
    enna_mediaplayer_video_resize(0, 0, 0, 0);
}

static void _e_smart_show(Evas_Object * obj)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;
    evas_object_show(sd->o_edje);
}

static void _e_smart_hide(Evas_Object * obj)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;
    evas_object_hide(sd->o_edje);
}

static void _e_smart_color_set(Evas_Object * obj, int r, int g, int b, int a)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;
    evas_object_color_set(sd->o_edje, r, g, b, a);
}

static void _e_smart_clip_set(Evas_Object * obj, Evas_Object * clip)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;
    evas_object_clip_set(sd->o_edje, clip);
}

static void _e_smart_clip_unset(Evas_Object * obj)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;
    evas_object_clip_unset(sd->o_edje);
}
