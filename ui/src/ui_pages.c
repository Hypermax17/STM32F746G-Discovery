#include "ui_pages.h"

#define MAX_PAGES 8U
#define UPDATE_PERIOD_MS 500U

static const ui_page_def_t *s_defs;
static unsigned s_count;
static unsigned s_current;
static uint8_t  s_design[MAX_PAGES];
static lv_obj_t *s_screen[MAX_PAGES];
static lv_timer_t *s_timer;

static void update_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (s_current < s_count && s_defs[s_current].update != NULL && s_screen[s_current] != NULL) {
        s_defs[s_current].update();
    }
}

static lv_obj_t *build(unsigned idx)
{
    return s_defs[idx].create(ui_designs[s_design[idx]], idx);
}

void ui_pages_init(const ui_page_def_t *defs, const uint8_t *design_idx, unsigned count)
{
    s_defs = defs;
    s_count = (count > MAX_PAGES) ? MAX_PAGES : count;
    for (unsigned i = 0; i < s_count; i++) {
        s_design[i] = (design_idx != NULL) ? (uint8_t)(design_idx[i] % UI_DESIGN_COUNT) : 0U;
        s_screen[i] = NULL;
    }
    s_current = 0;
    if (s_timer == NULL) {
        s_timer = lv_timer_create(update_timer_cb, UPDATE_PERIOD_MS, NULL);
    }
}

unsigned ui_pages_count(void)   { return s_count; }
unsigned ui_pages_current(void) { return s_current; }

const char *ui_pages_name(unsigned idx)
{
    return (idx < s_count) ? s_defs[idx].name : "";
}

const ui_design_t *ui_pages_design(unsigned idx)
{
    return ui_designs[s_design[idx % MAX_PAGES]];
}

void ui_pages_show(unsigned idx)
{
    if (idx >= s_count) {
        return;
    }
    const int first = (s_screen[idx] == NULL);
    if (first) {
        s_screen[idx] = build(idx);
    }
    const unsigned prev = s_current;
    s_current = idx;
    if (idx != prev || first) {
        /* Screens bleiben erhalten (Zustand, schneller Wechsel) */
        lv_screen_load_anim(s_screen[idx], LV_SCREEN_LOAD_ANIM_FADE_IN, 120, 0, false);
    }
    if (s_defs[idx].update != NULL) {
        s_defs[idx].update();
    }
}

void ui_pages_next(void)
{
    if (s_count > 0U) {
        ui_pages_show((s_current + 1U) % s_count);
    }
}

/* Das Design wird nicht direkt im Button-Ereignis gewechselt: der alte Bildschirm enthaelt den Button,
 * der das Ereignis ausloest, und darf dort nicht geloescht werden. */
static void design_switch_async(void *arg)
{
    const unsigned idx = (unsigned)(uintptr_t)arg;
    if (idx >= s_count) {
        return;
    }
    s_design[idx] = (uint8_t)((s_design[idx] + 1U) % UI_DESIGN_COUNT);

    lv_obj_t *old = s_screen[idx];
    s_screen[idx] = build(idx);
    if (idx == s_current) {
        lv_screen_load(s_screen[idx]);
    }
    if (old != NULL) {
        lv_obj_delete(old);
    }
    if (s_defs[idx].update != NULL && idx == s_current) {
        s_defs[idx].update();
    }
}

void ui_pages_next_design(unsigned idx)
{
    lv_async_call(design_switch_async, (void *)(uintptr_t)idx);
}
