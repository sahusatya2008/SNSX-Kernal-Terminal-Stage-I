ENTRY(snsx_app_main)

SECTIONS
{
  . = 0x00400000;

  .text ALIGN(16) :
  {
    *(.text*)
  }

  .rodata ALIGN(16) :
  {
    *(.rodata*)
  }

  .data ALIGN(16) :
  {
    *(.data*)
  }

  .bss ALIGN(16) :
  {
    *(.bss*)
    *(COMMON)
  }
}
