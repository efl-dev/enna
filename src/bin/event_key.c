#include <ctype.h>

#include "enna.h"
#include "event_key.h"

typedef struct _Input_Module_Item Input_Module_Item;
struct _Input_Module_Item
{
    Enna_Module *module;
    Enna_Class_Input *class;
};

static Eina_List *_input_modules;

static const struct
{
    const char *keyname;
    enna_key_t keycode;
} enna_keymap[] = {
    { "Left",                       ENNA_KEY_LEFT          },
    { "Right",                      ENNA_KEY_RIGHT         },
    { "Up",                         ENNA_KEY_UP            },
    { "KP_Up",                      ENNA_KEY_UP            },
    { "Down",                       ENNA_KEY_DOWN          },
    { "KP_Down",                    ENNA_KEY_DOWN          },
    { "Home",                       ENNA_KEY_HOME          },
    { "KP_Home",                    ENNA_KEY_HOME          },
    { "End",                        ENNA_KEY_END           },
    { "KP_End",                     ENNA_KEY_END           },
    { "Prior",                      ENNA_KEY_PAGE_UP       },
    { "Next",                       ENNA_KEY_PAGE_DOWN     },
    { "Return",                     ENNA_KEY_OK            },
    { "KP_Enter",                   ENNA_KEY_OK            },
    { "BackSpace",                  ENNA_KEY_CANCEL        },
    { "space",                      ENNA_KEY_SPACE         },
    { "Escape",                     ENNA_KEY_QUIT          },
    { "Super_L",                    ENNA_KEY_MENU          },
    { "0",                          ENNA_KEY_0             },
    { "KP_0",                       ENNA_KEY_0             },
    { "1",                          ENNA_KEY_1             },
    { "KP_1",                       ENNA_KEY_1             },
    { "2",                          ENNA_KEY_2             },
    { "KP_2",                       ENNA_KEY_2             },
    { "3",                          ENNA_KEY_3             },
    { "KP_3",                       ENNA_KEY_3             },
    { "4",                          ENNA_KEY_4             },
    { "KP_4",                       ENNA_KEY_4             },
    { "5",                          ENNA_KEY_5             },
    { "KP_5",                       ENNA_KEY_5             },
    { "6",                          ENNA_KEY_6             },
    { "KP_6",                       ENNA_KEY_6             },
    { "7",                          ENNA_KEY_7             },
    { "KP_7",                       ENNA_KEY_7             },
    { "8",                          ENNA_KEY_8             },
    { "KP_8",                       ENNA_KEY_8             },
    { "9",                          ENNA_KEY_9             },
    { "KP_9",                       ENNA_KEY_9             },
    { "a",                          ENNA_KEY_A             },
    { "b",                          ENNA_KEY_B             },
    { "c",                          ENNA_KEY_C             },
    { "d",                          ENNA_KEY_D             },
    { "e",                          ENNA_KEY_E             },
    { "f",                          ENNA_KEY_F             },
    { "g",                          ENNA_KEY_G             },
    { "h",                          ENNA_KEY_H             },
    { "i",                          ENNA_KEY_I             },
    { "j",                          ENNA_KEY_J             },
    { "k",                          ENNA_KEY_K             },
    { "l",                          ENNA_KEY_L             },
    { "m",                          ENNA_KEY_M             },
    { "n",                          ENNA_KEY_N             },
    { "o",                          ENNA_KEY_O             },
    { "p",                          ENNA_KEY_P             },
    { "q",                          ENNA_KEY_Q             },
    { "r",                          ENNA_KEY_R             },
    { "s",                          ENNA_KEY_S             },
    { "t",                          ENNA_KEY_T             },
    { "u",                          ENNA_KEY_U             },
    { "v",                          ENNA_KEY_V             },
    { "w",                          ENNA_KEY_W             },
    { "x",                          ENNA_KEY_X             },
    { "y",                          ENNA_KEY_Y             },
    { "z",                          ENNA_KEY_Z             },
    { NULL,                         ENNA_KEY_UNKNOWN       }
};

/* Static functions */

static void _event_cb(void *data, char *event)
{

    if (!event)
        return;

    evas_event_feed_key_down(enna->evas, event, event, event, NULL, ecore_time_get(), data);
    enna_log(ENNA_MSG_EVENT, NULL, "LIRC event : %s", event);


}

/* Public Functions */

enna_key_t
enna_get_key (void *event)
{
    int i;
    Evas_Event_Key_Down *ev;

    ev = event;

    if (!ev)
    return ENNA_KEY_UNKNOWN;

    enna_log (ENNA_MSG_EVENT, NULL, "Key pressed : %s", ev->key);

    for (i = 0; enna_keymap[i].keyname; i++)
    if (!strcmp (enna_keymap[i].keyname, ev->key))
    return enna_keymap[i].keycode;

    return ENNA_KEY_UNKNOWN;
}

int enna_key_is_alpha(enna_key_t key)
{
    return (key >= ENNA_KEY_A && key <= ENNA_KEY_Z);
}

char enna_key_get_alpha(enna_key_t key)
{
    int i;

    for (i = 0; enna_keymap[i].keyname; i++)
        if (enna_keymap[i].keycode == key)
            return enna_keymap[i].keyname[0];

    return ' ';
}

void enna_input_init()
{
#ifdef BUILD_INPUT_LIRC
    Enna_Module *em;
    Input_Module_Item *item;
#endif

    /* Create Input event "Key Down" */
    ENNA_EVENT_INPUT_KEY_DOWN = ecore_event_type_new();

    _input_modules = NULL;

#ifdef BUILD_INPUT_LIRC
    em = enna_module_open("input_lirc", ENNA_MODULE_INPUT, enna->evas);
    item = calloc(1, sizeof(Input_Module_Item));
    item->module = em;
    _input_modules = eina_list_append(_input_modules, item);
    enna_module_enable(em);
#endif

}

void enna_input_shutdown()
{
    Eina_List *l = NULL;

    for (l = _input_modules; l; l = l->next)
    {
        Input_Module_Item *item = l->data;
        item->class->func.class_shutdown(0);
        enna_module_disable(item->module);
    }
    eina_list_free(_input_modules);
}

int enna_input_class_register(Enna_Module *module, Enna_Class_Input *class)
{
    Eina_List *l = NULL;

    for (l = _input_modules; l; l = l->next)
    {
        Input_Module_Item *item = l->data;
        if (module == item->module)
        {
            item->class = class;
            class->func.class_init(0);
            class->func.class_event_cb_set(_event_cb, item);

            return 0;
        }
    }
    return -1;
}
