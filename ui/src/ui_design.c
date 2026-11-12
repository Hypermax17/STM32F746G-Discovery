#include "ui_design.h"

/* A: dunkles Dashboard (unveraendert) */
const ui_design_t ui_design_dark = {
    .name = "Dashboard dunkel", .letter = 'A',
    .bg = 0x0B1220, .bg2 = 0x0B1220, .bg_gradient = 0,
    .panel = 0x151F33, .panel_opa = 255, .border = 0x263654, .border_opa = 255, .shadow_w = 0,
    .text = 0xE6EDF7, .text_dim = 0x8EA0BC,
    .accent = 0x22D3EE, .accent_text = 0x04202A,
    .ok = 0x34D399, .warn = 0xFBBF24, .bad = 0xF87171,
    .nav_bg = 0x0F1930,
    .radius = 10, .border_w = 1,
    .font_small = &lv_font_montserrat_12, .font = &lv_font_montserrat_14, .font_big = &lv_font_montserrat_28,
    .font_title = &lv_font_montserrat_20, .font_hero = &lv_font_montserrat_48,
    .nav = UI_NAV_TOP, .layout = UI_LAYOUT_CARDS,
};

/* B: Cockpit - fast schwarzer Graphit mit leichtem Gruenstich, leuchtend gruener Akzent, bernsteinfarbene
 * zweite Kurve, kaum Rahmen, grosse Zahlen und runde Anzeigen */
const ui_design_t ui_design_cockpit = {
    .name = "Cockpit gruen", .letter = 'B',
    .bg = 0x090D0A, .bg2 = 0x090D0A, .bg_gradient = 0,
    .panel = 0x111813, .panel_opa = 255, .border = 0x24332A, .border_opa = 255, .shadow_w = 0,
    .text = 0xE9F5EC, .text_dim = 0x7E9A87,
    .accent = 0x4ADE80, .accent_text = 0x05220F,
    .ok = 0x4ADE80, .warn = 0xFACC15, .bad = 0xF87171,
    .nav_bg = 0x0D120E,
    .radius = 6, .border_w = 1,
    .font_small = &lv_font_montserrat_12, .font = &lv_font_montserrat_14, .font_big = &lv_font_montserrat_28,
    .font_title = &lv_font_montserrat_20, .font_hero = &lv_font_montserrat_48,
    .nav = UI_NAV_SIDE, .layout = UI_LAYOUT_COCKPIT,
};

/* C: Glas - Verlauf von tiefem Violett nach Nachtblau, durchscheinende Karten mit hellem Rand und weichem
 * Schatten, grosse Rundungen, Akzent Violett, zweite Kurve Rosa */
const ui_design_t ui_design_glass = {
    .name = "Glas violett", .letter = 'C',
    .bg = 0x2A1458, .bg2 = 0x081A3A, .bg_gradient = 1,
    .panel = 0xFFFFFF, .panel_opa = 30, .border = 0xFFFFFF, .border_opa = 70, .shadow_w = 14,
    .text = 0xF5F3FF, .text_dim = 0xB4A9D8,
    .accent = 0xB794F6, .accent_text = 0x180A33,
    .ok = 0x6EE7B7, .warn = 0xF9A8D4, .bad = 0xFF6B6B,
    .nav_bg = 0x120A2B,
    .radius = 16, .border_w = 1,
    .font_small = &lv_font_montserrat_12, .font = &lv_font_montserrat_14, .font_big = &lv_font_montserrat_28,
    .font_title = &lv_font_montserrat_20, .font_hero = &lv_font_montserrat_48,
    .nav = UI_NAV_SIDE, .layout = UI_LAYOUT_CARDS,
};

const ui_design_t *const ui_designs[UI_DESIGN_COUNT] = {
    &ui_design_dark, &ui_design_cockpit, &ui_design_glass
};
