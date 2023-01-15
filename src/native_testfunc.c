#pragma bank 255

#include <gbdk/platform.h>
#include <stdio.h>
#include "vm.h"

void my_native_function(SCRIPT_CTX * THIS) OLDCALL BANKED {
    printf("native: %d\n", *(int16_t*)VM_REF_TO_PTR(FN_ARG0)); 
}