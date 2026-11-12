#ifndef PAGES_H
#define PAGES_H

#include "ui_pages.h"

/* Seiten der Applikation. Jede Seite baut sich ueber ein ui_design_t auf (siehe ui/include/ui_widgets.h). */

lv_obj_t *page_system_create(const ui_design_t *d, unsigned idx);
void      page_system_update(void);

lv_obj_t *page_values_create(const ui_design_t *d, unsigned idx);
void      page_values_update(void);

lv_obj_t *page_log_create(const ui_design_t *d, unsigned idx);
void      page_log_update(void);

#endif
