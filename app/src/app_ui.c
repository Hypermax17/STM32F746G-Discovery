#include "app_ui.h"
#include "pages.h"

static const ui_page_def_t k_pages[] = {
    { "System",    LV_SYMBOL_SETTINGS, page_system_create, page_system_update },
    { "Messwerte", LV_SYMBOL_CHARGE,   page_values_create, page_values_update },
    { "Dashboard", LV_SYMBOL_EYE_OPEN, page_dashboard_create, page_dashboard_update },
    { "Log",       LV_SYMBOL_LIST,     page_log_create,    page_log_update },
};

void app_ui_init(void)
{
    ui_pages_init(k_pages, sizeof k_pages / sizeof k_pages[0], &ui_design);
    ui_pages_show(0);
}
