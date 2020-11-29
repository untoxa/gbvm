#include <gb/gb.h>
#include <stdio.h>

#include "vm.h"

extern const UBYTE BYTECODE[];                  // defined in bytecode.s
extern void __bank_BYTECODE;

void main() {
    ScriptRunnerInit();
    ExecuteScript((UBYTE)&__bank_BYTECODE, BYTECODE);
    printf("- START\n");
    while (ScriptRunnerUpdate());
    printf("- DONE\n");
}
