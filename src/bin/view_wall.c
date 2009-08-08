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

#include <Ecore.h>
#include <Ecore_File.h>
#include <Ecore_Data.h>
#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "view_wall.h"
#include "image.h"
#include "logs.h"
#include "event_key.h"
#include "thumb.h"
#include "vfs.h"

#ifdef BUILD_LIBEXIF
#include <libexif/exif-data.h>
#endif

#define SMART_NAME "enna_wall"

typedef struct _Smart_Data Smart_Data;
typedef struct _Picture_Item Picture_Item;

struct _Picture_Item
{
    Evas_Object *o_edje;
    int row;
    Evas_Object *o_pict; // Enna image object
    Smart_Data *sd;
    unsigned char selected : 1;
    Enna_Vfs_File *file;
    void *data;
    void (*func) (void *data);
};

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *obj;
    Evas_Object *o_scroll;
    Evas_Object *o_table;
    Eina_List *pictures;
    Evas_Object *o_cont;
    Evas_Object *o_box[3];
    Eina_List *items[3];
    int nb;
    int row_sel;
    int col_sel;
};

struct _Preload_Data
{
    Smart_Data *sd;
    Evas_Object *item;
};

/* local subsystem functions */
static void _wall_left_select(Evas_Object *obj);
static void _wall_right_select(Evas_Object *obj);
static void _wall_up_select(Evas_Object *obj);
static void _wall_down_select(Evas_Object *obj);
static Picture_Item *_smart_selected_item_get(Smart_Data *sd, int *row,
        int *col);
static void _smart_item_unselect(Smart_Data *sd, Picture_Item *pi);
static void _smart_item_select(Smart_Data *sd, Picture_Item *pi);
static void _smart_event_mouse_down(void *data, Evas *evas, Evas_Object *obj,
        void *event_info);
static void _smart_reconfigure(Smart_Data * sd);

/* local subsystem globals */
static Evas_Smart *_smart = NULL;


static
void _wall_image_preload_cb (void *data, Evas_Object *obj, void *event_info)
{
    Picture_Item *pi = data;
    Evas_Coord w, h, ow, oh;
    int row, w0, w1, w2, h0, h1, h2;
    double f = 1.0;
    int orientation = 0;
#ifdef BUILD_LIBEXIF
    ExifData  *exif;
    ExifEntry *entry = NULL;
    ExifByteOrder bo;
#endif

    if (!pi) return;

#ifdef BUILD_LIBEXIF
    exif = exif_data_new_from_file(enna_thumb_icon_file_get(pi->o_pict));
    if (exif)
    {
	entry = exif_data_get_entry(exif, EXIF_TAG_ORIENTATION);
	if (entry)
	{
	    bo = exif_data_get_byte_order(exif);
	    orientation = exif_get_short(entry->data, bo);
	}
	exif_data_free(exif);
    }
#endif

    if (orientation > 1 && orientation < 9)
    {
	Enna_Image_Orient t1 = ENNA_IMAGE_ORIENT_NONE;

	switch (orientation)
	{
	case 2:		/* Horizontal flip */
	    t1 = ENNA_IMAGE_FLIP_HORIZONTAL;
	    break;
	case 3:		/* Rotate 180 clockwise */
	    t1 = ENNA_IMAGE_ROTATE_180_CW;
	    break;
	case 4:		/* Vertical flip */
	    t1 = ENNA_IMAGE_FLIP_VERTICAL;
	    break;
	case 5:		/* Transpose */
	    t1 = ENNA_IMAGE_FLIP_TRANSPOSE;
	    break;
	case 6:		/* Rotate 90 clockwise */
	    t1 = ENNA_IMAGE_ROTATE_90_CW;
	    break;
	case 7:		/* Transverse */
	    t1 = ENNA_IMAGE_FLIP_TRANSVERSE;
	    break;
	case 8:		/* Rotate 90 counter-clockwise */
	    t1 = ENNA_IMAGE_ROTATE_90_CCW;
	    break;
	}
	if (t1)
	    enna_thumb_icon_orient_set(pi->o_pict, t1);
    }

    enna_thumb_icon_size_get(pi->o_pict, &w, &h);
    if (h)
	f = (float)w/(float)h;

    edje_object_part_swallow(pi->o_edje, "enna.swallow.content", pi->o_pict);

    evas_object_size_hint_min_get(pi->sd->o_box[0], &w0, &h0);
    evas_object_size_hint_min_get(pi->sd->o_box[1], &w1, &h1);
    evas_object_size_hint_min_get(pi->sd->o_box[2], &w2, &h2);

    if (w0 <= w1 && w0 <= w2)
    {
	row = 0;
	oh = h0;
    }
    else if (w1 <= w2)
    {
        row = 1;
	oh = h1;
    }
    else
    {
        row = 2;
	oh = h2;
    }
    oh -= 8;
    ow = oh * f;
    pi->row = row;
    pi->selected = 0;
    pi->sd->items[row] = eina_list_append(pi->sd->items[row], pi);
    evas_object_size_hint_min_set(pi->o_edje, ow, oh);
    evas_object_size_hint_align_set(pi->o_edje, 0.5, 0.5);
    elm_box_pack_end(pi->sd->o_box[pi->row], pi->o_edje);
    evas_object_show(pi->o_edje);
    edje_object_signal_emit(pi->o_edje, "thumb,show", "enna");
}

void enna_wall_file_append(Evas_Object *obj, Enna_Vfs_File *file,
    void (*func) (void *data), void *data )
{
    Evas_Object *o, *o_pict;

    Picture_Item *pi;

    API_ENTRY return;
    if (!file) return;

    pi = calloc(1, sizeof(Picture_Item));

    sd->nb++;
    pi->file = file;
    o = edje_object_add(evas_object_evas_get(sd->o_scroll));
    edje_object_file_set(o, enna_config_theme_get(), "enna/picture/item");
    edje_object_part_text_set(o, "enna.text.label", file->label);

    o_pict = enna_thumb_icon_add(evas_object_evas_get(sd->o_scroll));
    enna_thumb_icon_file_set(o_pict, file->uri+7, "enna/thumbnails");
    enna_thumb_icon_size_set(o_pict, 400, 400);
    evas_object_show(o_pict);

    evas_object_smart_callback_add(o_pict, "enna_thumb_gen", _wall_image_preload_cb, pi);

    pi->o_pict = o_pict;
    pi->o_edje = o;
    pi->data = data;
    pi->func = func;
    pi->sd = sd;


    evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
            _smart_event_mouse_down, pi);

    enna_thumb_icon_begin(pi->o_pict);
}

void enna_wall_event_feed(Evas_Object *obj, void *event_info)
{
    Evas_Event_Key_Down *ev = event_info;
    enna_key_t key = enna_get_key(ev);

    API_ENTRY return;
   printf("browser event feed\n");

     enna_log(ENNA_MSG_EVENT, SMART_NAME, "Key pressed : %s", ev->key);
    switch (key)
    {
    case ENNA_KEY_LEFT:
        _wall_left_select(obj);
        break;
    case ENNA_KEY_RIGHT:
        _wall_right_select(obj);
        break;
    case ENNA_KEY_UP:
        _wall_up_select(obj);
        break;
    case ENNA_KEY_DOWN:
        _wall_down_select(obj);
        break;
    default:
        break;
    }


}

void enna_wall_select_nth(Evas_Object *obj, int col, int row)
{
    Picture_Item *pi;

    API_ENTRY;

    pi = eina_list_nth(sd->items[row], col);
    if (!pi) return;

    _smart_item_unselect(sd, _smart_selected_item_get(sd, NULL, NULL));
    _smart_item_select(sd, pi);


}

void *enna_wall_selected_data_get(Evas_Object *obj)
{
    Picture_Item *si;
    API_ENTRY return NULL;

    si = _smart_selected_item_get(sd, NULL, NULL);
    return si ? si->data : NULL;
}

void enna_wall_selected_geometry_get(Evas_Object *obj, int *x, int *y, int *w, int *h)
{
    Eina_List *l;
    API_ENTRY;

    for (l = sd->items[sd->row_sel]; l; l = l->next)
    {
        Picture_Item *pi = l->data;
        if (pi->selected)
        {
            evas_object_geometry_get(pi->o_pict, x, y, w, h);
            return;
        }
    }
}

const char * enna_wall_selected_filename_get(Evas_Object *obj)
{
    API_ENTRY;
    Eina_List *l;
    for (l = sd->items[sd->row_sel]; l; l = l->next)
    {
        Picture_Item *pi = l->data;
        if (pi->selected)
        {
            return enna_thumb_icon_file_get(pi->o_pict);
        }
    }
    return NULL;
}

Eina_List* enna_wall_get_filenames(Evas_Object* obj)
{
    API_ENTRY return NULL;

    Eina_List *l, *files = NULL;
    const  char *fname = NULL;
    Picture_Item *pi;
    int row;

    for (row=0; row<3; row++)
    {
        for (l = sd->items[row]; l; l = l->next)
        {
            pi = l->data;
            fname = enna_thumb_icon_file_get(pi->o_pict);
            files = eina_list_append(files, fname);
        }
    }

    return files;
}

/* local subsystem globals */
static void _wall_h_select(Evas_Object *obj, int pos)
{
    Picture_Item *pi, *ppi;
    int row, col;

    API_ENTRY return;

    ppi = _smart_selected_item_get(sd, &row, &col);
    if (!ppi)
        col = 0;
    else
    {
        if (pos)
            col++;
        else
            col--;
    }

    pi = eina_list_nth(sd->items[sd->row_sel], col);
    if (pi)
    {
        Evas_Coord x, xedje, wedje, xbox;

        evas_object_geometry_get(pi->o_edje, &xedje, NULL, &wedje, NULL);
        evas_object_geometry_get(sd->o_box[sd->row_sel], &xbox, NULL, NULL, NULL);
        if (pos)
            x = (xedje + wedje / 2 - xbox + sd->w / 2 );
        else
            x = (xedje + wedje / 2 - xbox - sd->w / 2 );
        elm_scroller_region_show(sd->o_scroll, x, 0, 0, 0);

        _smart_item_select(sd, pi);
        if (ppi) _smart_item_unselect(sd, ppi);
    }
}

static void _wall_v_select(Evas_Object *obj, int pos)
{
    Picture_Item *pi, *ppi;
    Eina_List *l;
    Evas_Coord sx, x;
    int i;
    int row, col;

    API_ENTRY
    return;

    ppi = NULL;
    pi = NULL;
    sx = 0;
    x = 0;

    ppi = _smart_selected_item_get(sd, &row, &col);
    if (ppi)
    {
        evas_object_geometry_get(ppi->o_edje, &sx, NULL, NULL, NULL);
        _smart_item_unselect(sd, ppi);
    }

    if (pos)
    {
        row++;
        row %= 3;
    }
    else
    {
        row--;
        if (row < 0)
            row = 2;
    }
    sd->row_sel = row;

    for (l = sd->items[sd->row_sel], i = 0; l; l = l->next, ++i)
    {
        pi = l->data;
        evas_object_geometry_get(pi->o_edje, &x, NULL, NULL, NULL);
        if (x >= sx)
        {
            _smart_item_select(sd, pi);
            return;
        }
    }

    pi = eina_list_data_get(eina_list_last(sd->items[sd->row_sel]));
    if (pi)
        _smart_item_select(sd, pi);
}

static void _wall_left_select(Evas_Object *obj)
{
    _wall_h_select (obj, 0);
}

static void _wall_right_select(Evas_Object *obj)
{
    _wall_h_select (obj, 1);
}

static void _wall_up_select(Evas_Object *obj)
{
    _wall_v_select (obj, 0);
}

static void _wall_down_select(Evas_Object *obj)
{
    _wall_v_select (obj, 1);
}

static Picture_Item *_smart_selected_item_get(Smart_Data *sd, int *row,
        int *col)
{
    Eina_List *l;
    int i, j;

    for (i = 0; i < 3; ++i)
    {
        for (l = sd->items[i], j = 0; l; l = l->next, j++)
        {
            Picture_Item *pi = l->data;
            if (pi->selected)
            {
                if (row)
                    *row = i;
                if (col)
                    *col = j;
                return pi;
            }
        }
    }
    if (row)
        *row = -1;
    if (col)
        *col = -1;

    return NULL;
}

static void _smart_item_unselect(Smart_Data *sd, Picture_Item *pi)
{
    if (!pi || !sd)
        return;
    pi->selected = 0;
    edje_object_signal_emit(pi->o_edje, "unselect", "enna");

}

static void _smart_item_select(Smart_Data *sd, Picture_Item *pi)
{
    if (!pi || !sd)
        return;

    pi->selected = 1;
    evas_object_raise(pi->o_edje);
    evas_object_raise(pi->sd->o_box[pi->row]);
    switch (pi->row)
    {
    case 0:
        edje_object_signal_emit(pi->o_edje, "select0", "enna");
        break;
    case 1:
        edje_object_signal_emit(pi->o_edje, "select1", "enna");
        break;
    case 2:
        edje_object_signal_emit(pi->o_edje, "select2", "enna");
        break;
    default:
        break;
    }
    sd->row_sel = pi->row;
}

static void _smart_event_mouse_down(void *data, Evas *evas, Evas_Object *obj,
        void *event_info)
{
    Picture_Item *pi, *ppi;
    int col, row;
    Evas_Event_Mouse_Down *ev = event_info;

    pi = data;

    ppi = _smart_selected_item_get(pi->sd, &row, &col);
    if (ppi && ppi != pi)
    {
        ppi->selected = 0;
        edje_object_signal_emit(ppi->o_edje, "unselect", "enna");
    }
    else if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
    {
	evas_object_smart_callback_call(pi->sd->obj, "selected", NULL);
	return;
    }
    else if (ppi == pi)
    {
        return;
    }

    evas_object_raise(pi->sd->o_box[pi->row]);
    evas_object_raise(pi->o_edje);
    pi->selected = 1;
    pi->sd->row_sel = pi->row;
    switch (pi->row)
    {
    case 0:
        edje_object_signal_emit(pi->o_edje, "select0", "enna");
        break;
    case 1:
        edje_object_signal_emit(pi->o_edje, "select1", "enna");
        break;
    case 2:
        edje_object_signal_emit(pi->o_edje, "select2", "enna");
        break;
    default:
        break;
    }

}

static void _smart_reconfigure(Smart_Data * sd)
{
    Evas_Coord x, y, w, h;
    Evas_Coord ow = 0;

    int i = 0;

    x = sd->x;
    y = sd->y;
    w = sd->w;
    h = sd->h;

    for (i = 0; i < 3; i++)
    {
        evas_object_size_hint_min_get(sd->o_box[i], &w, &h);
        evas_object_resize(sd->o_box[i], w, sd->h / 3);
        if (w > ow)
            ow = w;
        evas_object_size_hint_min_set(sd->o_box[i], w, sd->h / 3);
        evas_object_size_hint_align_set(sd->o_box[i], 0, 0);
    }

    evas_object_size_hint_min_get(sd->o_cont, &w, &h);
    evas_object_resize(sd->o_cont, w, h);

    evas_object_move(sd->o_scroll, sd->x, sd->y);
    evas_object_resize(sd->o_scroll, sd->w, sd->h);
}

static void _smart_add(Evas_Object * obj)
{
    Smart_Data *sd;
    int i;
    sd = calloc(1, sizeof(Smart_Data));
    if (!sd)
        return;
    sd->obj = obj;
    sd->nb = -1;
    sd->o_scroll = elm_scroller_add(obj);
    evas_object_show(sd->o_scroll);

    sd->o_cont = elm_box_add(sd->o_scroll);
    elm_box_homogenous_set(sd->o_cont, 0);
    elm_box_horizontal_set(sd->o_cont, 0);
    evas_object_show(sd->o_cont);
    elm_scroller_content_set(sd->o_scroll, sd->o_cont);

    for (i = 0; i < 3; i++)
    {
        sd->o_box[i] = elm_box_add(sd->o_cont);
        elm_box_homogenous_set(sd->o_box[i], 0);
        elm_box_horizontal_set(sd->o_box[i], 1);
        elm_box_pack_end(sd->o_cont, sd->o_box[i]);
        evas_object_show(sd->o_box[i]);
    }
    enna_thumb_init();
    evas_object_smart_member_add(sd->o_scroll, obj);
    evas_object_smart_data_set(obj, sd);
}

static void _smart_del(Evas_Object * obj)
{
    int i;
    INTERNAL_ENTRY;

    evas_object_del(sd->o_scroll);
    evas_object_del(sd->o_cont);
    for (i = 0; i < 3; i++)
    {
        evas_object_del(sd->o_box[i]);
        while (sd->items[i])
        {
            Picture_Item *pi = sd->items[i]->data;
            sd->items[i] = eina_list_remove_list(sd->items[i], sd->items[i]);
            evas_object_del(pi->o_edje);
            evas_object_smart_callback_del(pi->o_pict, "enna_thumb_gen", _wall_image_preload_cb);
	    enna_thumb_icon_end(pi->o_pict);
            evas_object_del(pi->o_pict);
            free(pi);
        }
        free(sd->items[i]);
    }
    enna_thumb_shutdown();
    evas_object_del(sd->o_scroll);
    free(sd);
}

static void _smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
    INTERNAL_ENTRY;

    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _smart_reconfigure(sd);
}

static void _smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
    INTERNAL_ENTRY;

    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;
    _smart_reconfigure(sd);
}

static void _smart_show(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_show(sd->o_scroll);
}

static void _smart_hide(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_hide(sd->o_scroll);
}

static void _smart_color_set(Evas_Object * obj, int r, int g, int b, int a)
{
    INTERNAL_ENTRY;
    evas_object_color_set(sd->o_scroll, r, g, b, a);
}

static void _smart_clip_set(Evas_Object * obj, Evas_Object * clip)
{
    INTERNAL_ENTRY;
    evas_object_clip_set(sd->o_scroll, clip);
}

static void _smart_clip_unset(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_clip_unset(sd->o_scroll);
}

static void _smart_init(void)
{
    static const Evas_Smart_Class sc = {
        SMART_NAME,
        EVAS_SMART_CLASS_VERSION,
        _smart_add,
        _smart_del,
        _smart_move,
        _smart_resize,
        _smart_show,
        _smart_hide,
        _smart_color_set,
        _smart_clip_set,
        _smart_clip_unset,
        NULL,
        NULL
    };

    if (!_smart)
        _smart = evas_smart_class_new(&sc);
}

/* externally accessible functions */
Evas_Object * enna_wall_add(Evas * evas)
{
    _smart_init();
    return evas_object_smart_add(evas, _smart);
}