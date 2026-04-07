ENTRY(kernel_entry)

SECTIONS
{
  . = 0x00020000;
  __kernel_start = .;

  .text ALIGN(16) :
  {
    *(.text.entry)
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
    __bss_start = .;
    *(.bss*)
    *(COMMON)
    __bss_end = .;
  }

  __kernel_end = .;
}
