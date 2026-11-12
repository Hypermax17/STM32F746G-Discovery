#include "app_ui.h"
#include "pages.h"

/* Seitenliste. Startdesign je Seite: jede Seite zeigt bewusst ein anderes Design (A, B, C), damit sie
 * verglichen werden koennen. Mit der Schaltflaeche "Design X" in der Navigation laesst sich jede Seite
 * mit dem naechsten Design neu aufbauen. */
static const ui_page_def_t k_pages[] = {
    { "System",    LV_SYMBOL_SETTINGS, page_system_create, page_system_update },
    { "Messwerte", LV_SYMBOL_CHARGE,   page_values_create, page_values_update },
    { "Log",       LV_SYMBOL_LIST,     page_log_create,    page_log_update },
};

static const uint8_t k_start_design[] = { 0 /* A Dashboard */, 1 /* B Cockpit */, 2 /* C Glas */ };

void app_ui_init(void)
{
    ui_pages_init(k_pages, k_start_design, sizeof k_pages / sizeof k_pages[0]);
    ui_pages_show(0);
}
