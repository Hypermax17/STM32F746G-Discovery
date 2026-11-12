#include "bsp_util.h"

#include <stdint.h>
#include <stdio.h>

int bsp_format_float(char *buf, size_t size, float value, unsigned decimals)
{
    if (value != value) {                      /* NaN */
        return snprintf(buf, size, "nan");
    }
    if (decimals > 6U) decimals = 6U;

    int64_t scale = 1;
    for (unsigned i = 0; i < decimals; i++) scale *= 10;

    /* ausserhalb des darstellbaren Bereichs begrenzen */
    const float limit = 2.0e9f / (float)scale;
    if (value > limit)  value = limit;
    if (value < -limit) value = -limit;

    int64_t scaled = (int64_t)(value * (float)scale + (value >= 0.0f ? 0.5f : -0.5f));
    const int neg = scaled < 0;
    if (neg) scaled = -scaled;

    const unsigned long ip = (unsigned long)(scaled / scale);
    const unsigned long fp = (unsigned long)(scaled % scale);
    if (decimals == 0U) {
        return snprintf(buf, size, "%s%lu", neg ? "-" : "", ip);
    }
    return snprintf(buf, size, "%s%lu.%0*lu", neg ? "-" : "", ip, (int)decimals, fp);
}
