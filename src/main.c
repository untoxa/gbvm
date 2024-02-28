#include <gbdk/platform.h>
#include <gbdk/font.h>
#include <stdio.h>

#include "vm.h"

extern const uint8_t BYTECODE[];                  // defined in bytecode.s
BANKREF_EXTERN(BYTECODE)

typedef struct actor_t {
    int16_t x, y;
    int16_t ID;
} actor_t;

const actor_t ACTORS[2] = {
    { 
        .ID = 1
    },{
        .ID = 2
    }
};

void process_VM(void) {
    while (TRUE) {
        switch (script_runner_update()) {
            case RUNNER_DONE: return;
            case RUNNER_IDLE: wait_vbl_done(); break;
//            case RUNNER_BUSY: ;
        }
    }
}

void main(void) {
    font_init();
    font_set(font_load(font_spect));
    
    script_runner_init(TRUE);
    script_execute(BANK(BYTECODE), BYTECODE, 0, 0);
    printf(">> VM START\n");
    process_VM();
    printf("<< VM DONE");
}
