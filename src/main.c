#include <gb/gb.h>
#include <gb/font.h>
#include <stdio.h>

#include "vm.h"

extern const UBYTE BYTECODE[];                  // defined in bytecode.s
extern void __bank_BYTECODE;

const INT16 some_const = 2;

void process_VM() {
    while (1) {
        switch (ScriptRunnerUpdate()) {
            case RUNNER_DONE: return;
            case RUNNER_IDLE: wait_vbl_done(); break;
//            case RUNNER_BUSY: ;
        }
    }
}

void main() {
    font_init();
    font_set(font_load(font_spect));
    
    ScriptRunnerInit();
    ExecuteScript((UBYTE)&__bank_BYTECODE, BYTECODE, 0, 0);
    printf(">> VM START\n");
    process_VM();
    printf("<< VM DONE\n");
}
