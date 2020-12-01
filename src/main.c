#include <gb/gb.h>
#include <gb/font.h>
#include <stdio.h>

#include "vm.h"

extern const UBYTE BYTECODE[];                  // defined in bytecode.s
extern void __bank_BYTECODE;

void main() {
    font_init();
    font_set(font_load(font_spect));
    
    ScriptRunnerInit();
    ExecuteScript((UBYTE)&__bank_BYTECODE, BYTECODE);
    printf(">> VM START\n");
    while (ScriptRunnerUpdate());
    printf("<< VM DONE\n");
}
