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

#ifndef VIEW_COVER_H
#define VIEW_COVER_H

#include "enna.h"
#include "vfs.h"

Evas_Object * enna_view_cover_add(Evas * evas, int horizontal);
void enna_view_cover_file_append(Evas_Object *obj, Enna_Vfs_File *file,
     void (*func_activated) (void *data), void (*func_selected) (void *data), void *data);
Eina_List* enna_view_cover_files_get(Evas_Object* obj);
void enna_view_cover_select_nth(Evas_Object *obj, int nth);
Eina_Bool enna_view_cover_input_feed(Evas_Object *obj, enna_input event);
void *enna_view_cover_selected_data_get(Evas_Object *obj);
int enna_view_cover_jump_label(Evas_Object *obj, const char *label);
void enna_view_cover_jump_ascii(Evas_Object *obj, char k);
void enna_view_cover_clear(Evas_Object *obj);

#endif /* VIEW_COVER_H */
