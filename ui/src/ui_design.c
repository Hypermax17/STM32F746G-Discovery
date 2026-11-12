#include "ui_design.h"

const ui_design_t ui_design = {
    .name = "Glas violett",
    .bg = 0x2A1458, .bg2 = 0x081A3A, .bg_gradient = 1,
    .panel = 0xFFFFFF, .panel_opa = 30, .border = 0xFFFFFF, .border_opa = 70, .shadow_w = 14,
    .text = 0xF5F3FF, .text_dim = 0xB4A9D8,
    .accent = 0xB794F6, .accent_text = 0x180A33,
    .ok = 0x6EE7B7, .warn = 0xF9A8D4, .bad = 0xFF6B6B,
    .nav_bg = 0x120A2B,
    .radius = 16, .border_w = 1,
    .font_small = &lv_font_montserrat_12, .font = &lv_font_montserrat_14,
    .font_big = &lv_font_montserrat_28, .font_title = &lv_font_montserrat_20,
};
