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

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <Edje.h>

#include "enna.h"
#include "enna_config.h"
#include "buffer.h"
#include "configuration_credits.h"

/* local globals */
static Evas_Object *o_edje = NULL;

Evas_Object *credits_panel_show(void *data EINA_UNUSED)
{
    Enna_Buffer *b;

    o_edje = edje_object_add(enna->evas);
    edje_object_file_set(o_edje, enna_config_theme_get (),
                         "activity/configuration/credits");

    b = enna_buffer_new();
    enna_buffer_append(b, _("Enna is a GeeXboX-team MediaCenter"));
    enna_buffer_append(b, ", ");
    enna_buffer_append(b, _("based on Enlightenment Foundation Librairies (EFL)."));
    enna_buffer_append(b, "<br><br>");
    enna_buffer_append(b, _("Credits go to:"));
    enna_buffer_append(b, "<br>");
    enna_buffer_append(b, "Nicolas Aguirre, Fabien Brisset, Davide Cavalca, ");
    enna_buffer_append(b, "Matthias H?lzer, Guillaume Lecerf, Mathieu Schroeter ");
    enna_buffer_append(b, _("and"));
    enna_buffer_append(b, " Benjamin Zores.");
    edje_object_part_text_set(o_edje, "credits.text", b->buf);
    enna_buffer_free(b);

    edje_object_signal_emit(o_edje, "credits,show", "enna");

    return o_edje;
}

void credits_panel_hide(void *data EINA_UNUSED)
{
    ENNA_OBJECT_DEL(o_edje);
}
