#include "ui_pages.h"

#define MAX_PAGES 8U
#define UPDATE_PERIOD_MS 500U

static const ui_page_def_t *s_defs;
static unsigned s_count;
static unsigned s_current;
static const ui_design_t *s_design;
static lv_obj_t *s_screen[MAX_PAGES];
static lv_timer_t *s_timer;

static void update_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (s_current < s_count && s_defs[s_current].update != NULL && s_screen[s_current] != NULL) {
        s_defs[s_current].update();
    }
}

void ui_pages_init(const ui_page_def_t *defs, unsigned count, const ui_design_t *design)
{
    s_defs = defs;
    s_count = (count > MAX_PAGES) ? MAX_PAGES : count;
    s_design = design;
    for (unsigned i = 0; i < s_count; i++) {
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

const char *ui_pages_icon(unsigned idx)
{
    return (idx < s_count && s_defs[idx].icon != NULL) ? s_defs[idx].icon : "";
}

const ui_design_t *ui_pages_design(void)
{
    return s_design;
}

void ui_pages_show(unsigned idx)
{
    if (idx >= s_count) {
        return;
    }
    const int first = (s_screen[idx] == NULL);
    if (first) {
        s_screen[idx] = s_defs[idx].create(s_design, idx);
    }
    const unsigned prev = s_current;
    s_current = idx;
    if (idx != prev || first) {
        /* Die Bildschirme bleiben erhalten (schneller Wechsel) */
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
