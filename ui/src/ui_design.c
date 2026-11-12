#include "ui_design.h"

const ui_design_t ui_design_dark = {
    .name = "Dashboard dunkel", .letter = 'A',
    .bg = 0x0B1220, .panel = 0x151F33, .border = 0x263654,
    .text = 0xE6EDF7, .text_dim = 0x8EA0BC,
    .accent = 0x22D3EE, .accent_text = 0x04202A,
    .ok = 0x34D399, .warn = 0xFBBF24, .bad = 0xF87171,
    .nav_bg = 0x0F1930,
    .radius = 10, .border_w = 1, .pad = 8,
    .font_small = &lv_font_montserrat_12, .font = &lv_font_montserrat_14, .font_big = &lv_font_montserrat_28,
    .font_title = &lv_font_montserrat_20,
    .nav = UI_NAV_TOP,
};

const ui_design_t ui_design_light = {
    .name = "Flach hell", .letter = 'B',
    .bg = 0xEEF2F7, .panel = 0xFFFFFF, .border = 0xD5DCE6,
    .text = 0x1F2937, .text_dim = 0x6B7280,
    .accent = 0x2563EB, .accent_text = 0xFFFFFF,
    .ok = 0x16A34A, .warn = 0xEA580C, .bad = 0xDC2626,
    .nav_bg = 0xFFFFFF,
    .radius = 6, .border_w = 1, .pad = 8,
    .font_small = &lv_font_montserrat_12, .font = &lv_font_montserrat_14, .font_big = &lv_font_montserrat_28,
    .font_title = &lv_font_montserrat_20,
    .nav = UI_NAV_BOTTOM,
};

const ui_design_t ui_design_terminal = {
    .name = "Terminal", .letter = 'C',
    .bg = 0x000000, .panel = 0x000000, .border = 0x1F7A33,
    .text = 0x33FF66, .text_dim = 0x1F9940,
    .accent = 0x33FF66, .accent_text = 0x000000,
    .ok = 0x33FF66, .warn = 0xFFD33F, .bad = 0xFF4D4D,
    .nav_bg = 0x000000,
    .radius = 0, .border_w = 1, .pad = 6,
    .font_small = &lv_font_unscii_8, .font = &lv_font_unscii_8, .font_big = &lv_font_unscii_16,
    .font_title = &lv_font_unscii_16,
    .nav = UI_NAV_TEXT,
};

const ui_design_t *const ui_designs[UI_DESIGN_COUNT] = {
    &ui_design_dark, &ui_design_light, &ui_design_terminal
};
