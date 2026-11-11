#include "bsp_fault.h"
#include "bsp_console.h"
#include "bsp_lcd.h"
#include "bsp_led.h"

#include "stm32f7xx.h"

#include "FreeRTOS.h"
#include "task.h"

/* Bewusst ohne printf/snprintf: Der Handler soll mit minimalem Stack und auch
 * bei beschaedigtem Heap/Libc funktionieren. */

typedef enum { FAULT_NMI, FAULT_HARD, FAULT_MEM, FAULT_BUS, FAULT_USAGE } fault_kind_t;

static const char *const kind_name[] = {
    "NMI", "HARD FAULT", "MEMMANAGE FAULT", "BUS FAULT", "USAGE FAULT"
};

static char *put_str(char *p, const char *s)
{
    while (*s != '\0') {
        *p++ = *s++;
    }
    return p;
}

static char *put_hex(char *p, uint32_t v)
{
    static const char digits[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        *p++ = digits[(v >> i) & 0xFU];
    }
    return p;
}

/* "LABEL xxxxxxxx  LABEL xxxxxxxx\n" */
static void print2(const char *l1, uint32_t v1, const char *l2, uint32_t v2)
{
    char buf[48];
    char *p = buf;
    p = put_str(p, l1);
    *p++ = ' ';
    p = put_hex(p, v1);
    if (l2 != NULL) {
        p = put_str(p, "  ");
        p = put_str(p, l2);
        *p++ = ' ';
        p = put_hex(p, v2);
    }
    *p++ = '\n';
    *p = '\0';
    bsp_console_write_unlocked(buf, BSP_COLOR_WHITE);
}

typedef struct { uint32_t mask; const char *text; } flag_t;

static const flag_t cfsr_flags[] = {
    { SCB_CFSR_IACCVIOL_Msk,    "MPU: Zugriff auf Code verboten" },
    { SCB_CFSR_DACCVIOL_Msk,    "MPU: Datenzugriff verboten" },
    { SCB_CFSR_MSTKERR_Msk,     "MPU: Fehler beim Stacken" },
    { SCB_CFSR_MUNSTKERR_Msk,   "MPU: Fehler beim Entstacken" },
    { SCB_CFSR_IBUSERR_Msk,     "Bus: Fehler beim Code-Holen" },
    { SCB_CFSR_PRECISERR_Msk,   "Bus: praeziser Datenfehler" },
    { SCB_CFSR_IMPRECISERR_Msk, "Bus: unpraeziser Datenfehler" },
    { SCB_CFSR_UNSTKERR_Msk,    "Bus: Fehler beim Entstacken" },
    { SCB_CFSR_STKERR_Msk,      "Bus: Fehler beim Stacken" },
    { SCB_CFSR_UNDEFINSTR_Msk,  "Usage: undefinierter Befehl" },
    { SCB_CFSR_INVSTATE_Msk,    "Usage: ungueltiger Zustand (Thumb)" },
    { SCB_CFSR_INVPC_Msk,       "Usage: ungueltiger EXC_RETURN" },
    { SCB_CFSR_NOCP_Msk,       "Usage: Coprozessor nicht da" },
    { SCB_CFSR_UNALIGNED_Msk,   "Usage: unausgerichteter Zugriff" },
    { SCB_CFSR_DIVBYZERO_Msk,   "Usage: Division durch 0" },
};

void bsp_fault_report(const uint32_t *frame, uint32_t exc_return, uint32_t kind_u)
{
    __disable_irq();

    const fault_kind_t kind = (fault_kind_t)kind_u;
    if (!bsp_console_ready()) {
        bsp_led_blink_forever(7);
    }

    const uint32_t cfsr = SCB->CFSR;
    const uint32_t hfsr = SCB->HFSR;

    bsp_console_clear_unlocked();
    bsp_console_write_unlocked("*** ", BSP_COLOR_RED);
    bsp_console_write_unlocked(kind_name[kind], BSP_COLOR_RED);
    bsp_console_write_unlocked(" ***\n", BSP_COLOR_RED);

    /* Task, in der der Fehler auftrat */
    {
        char buf[40];
        char *p = put_str(buf, "Task: ");
        const char *name = "-";
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED && (exc_return & 0x4U) != 0U) {
            TaskHandle_t t = xTaskGetCurrentTaskHandle();
            if (t != NULL) {
                name = pcTaskGetName(t);
            }
        } else {
            name = "(Start/ISR)";
        }
        for (int i = 0; name[i] != '\0' && i < 24; i++) {
            *p++ = name[i];
        }
        *p++ = '\n';
        *p = '\0';
        bsp_console_write_unlocked(buf, BSP_COLOR_YELLOW);
    }

    /* Exception-Frame: R0 R1 R2 R3 R12 LR PC xPSR */
    print2("PC ", frame[6], "LR ", frame[5]);
    print2("PSR", frame[7], "SP ", (uint32_t)(uintptr_t)(frame + 8));
    print2("R0 ", frame[0], "R1 ", frame[1]);
    print2("R2 ", frame[2], "R3 ", frame[3]);
    print2("R12", frame[4], NULL, 0);
    print2("CFSR", cfsr, "HFSR", hfsr);
    print2("MMFAR", (cfsr & SCB_CFSR_MMARVALID_Msk) ? SCB->MMFAR : 0U,
           "BFAR", (cfsr & SCB_CFSR_BFARVALID_Msk) ? SCB->BFAR : 0U);

    /* Dekodierte Ursachen */
    if (hfsr & SCB_HFSR_VECTTBL_Msk) {
        bsp_console_write_unlocked("Vektortabellen-Lesefehler\n", BSP_COLOR_CYAN);
    }
    if (hfsr & SCB_HFSR_FORCED_Msk) {
        bsp_console_write_unlocked("Eskaliert aus Konfigurationsfehler\n", BSP_COLOR_CYAN);
    }
    for (unsigned i = 0; i < sizeof cfsr_flags / sizeof cfsr_flags[0]; i++) {
        if (cfsr & cfsr_flags[i].mask) {
            bsp_console_write_unlocked(cfsr_flags[i].text, BSP_COLOR_CYAN);
            bsp_console_write_unlocked("\n", BSP_COLOR_CYAN);
        }
    }

    bsp_console_status_unlocked("Reset-Taster druecken / neu flashen");
    bsp_led_blink_forever(7);
}

/* Gemeinsamer Einstieg: ermittelt den aktiven Stack (MSP/PSP) aus EXC_RETURN Bit 2
 * und uebergibt Frame, EXC_RETURN und Fault-Art an bsp_fault_report(). */
#define FAULT_ENTRY(name, kind)                                \
    __attribute__((naked)) void name(void)                     \
    {                                                          \
        __asm volatile(                                        \
            "tst   lr, #4          \n"                         \
            "ite   eq              \n"                         \
            "mrseq r0, msp         \n"                         \
            "mrsne r0, psp         \n"                         \
            "mov   r1, lr          \n"                         \
            "movs  r2, %0          \n"                         \
            "b     bsp_fault_report\n"                         \
            : : "i"(kind));                                    \
    }

FAULT_ENTRY(NMI_Handler,         FAULT_NMI)
FAULT_ENTRY(HardFault_Handler,   FAULT_HARD)
FAULT_ENTRY(MemManage_Handler,   FAULT_MEM)
FAULT_ENTRY(BusFault_Handler,    FAULT_BUS)
FAULT_ENTRY(UsageFault_Handler,  FAULT_USAGE)
