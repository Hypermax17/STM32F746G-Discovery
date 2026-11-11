/* Linker-Script STM32F746NG (1 MB Flash, 320 KB RAM) + externes SDRAM
 *
 *  FLASH  0x08000000  1 MB    (AXIM-Interface)
 *  DTCM   0x20000000  64 KB   (Main-Stack / MSP)
 *  RAM    0x20010000  256 KB  (SRAM1 240 KB + SRAM2 16 KB, zusammenhaengend)
 *  SDRAM  0xC0000000  8 MB    (16-bit Bus an 128-Mbit-Chip -> 8 MB nutzbar)
 *         davon 0xC0000000..0xC007FFFF (512 KB) fuer den LCD-Framebuffer reserviert
 */
ENTRY(Reset_Handler)

_Min_Heap_Size  = 0x200;   /* newlib-Heap (FreeRTOS nutzt eigenen Heap) */
_Min_Stack_Size = 0x800;   /* Main-Stack (nur bis zum Start des Schedulers/ISRs) */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
  DTCM  (xrw) : ORIGIN = 0x20000000, LENGTH = 64K
  RAM   (xrw) : ORIGIN = 0x20010000, LENGTH = 256K
  SDRAM_FB (rw) : ORIGIN = 0xC0000000, LENGTH = 512K   /* LCD-Framebuffer (bsp_lcd) */
  SDRAM (xrw) : ORIGIN = 0xC0080000, LENGTH = 7680K
}

_estack = ORIGIN(DTCM) + LENGTH(DTCM);

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab : { . = ALIGN(4); *(.ARM.extab* .gnu.linkonce.armextab.*) . = ALIGN(4); } >FLASH
  .ARM : { . = ALIGN(4); __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; . = ALIGN(4); } >FLASH

  .preinit_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .init_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .fini_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
    . = ALIGN(4);
  } >FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = ALIGN(8);
  } >RAM

  /* Main-Stack im DTCM: Platz pruefen */
  .main_stack (NOLOAD) :
  {
    . = ALIGN(8);
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >DTCM

  /* Daten im externen SDRAM (nicht initialisiert, z. B. Framebuffer).
     Das SDRAM wird vom BSP vor dem ersten Zugriff initialisiert. */
  .sdram (NOLOAD) :
  {
    . = ALIGN(32);
    *(.sdram)
    *(.sdram*)
    . = ALIGN(32);
  } >SDRAM

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
