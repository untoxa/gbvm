#pragma bank 2

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vm.h"

#ifdef __SDCC
// define addressmod for HOME
void ___vm_dummy_fn(void) NONBANKED PRESERVES_REGS(a, b, c, d, e, h, l) {}
__addressmod ___vm_dummy_fn const HOME;
#else
#define HOME
#endif

// here we define all VM instructions: their handlers and parameter lengths in bytes
// this array must be nonbanked as well as STEP_VM()
HOME const SCRIPT_CMD script_cmds[] = {
    {&vm_push,         2}, // 0x01
    {&vm_pop,          1}, // 0x02
    {&vm_call_rel,     1}, // 0x03
    {&vm_call,         2}, // 0x04
    {&vm_ret,          1}, // 0x05
    {&vm_loop_rel,     4}, // 0x06
    {&vm_loop,         5}, // 0x07
    {&vm_jump_rel,     1}, // 0x08
    {&vm_jump,         2}, // 0x09
    {&vm_call_far,     3}, // 0x0A
    {&vm_ret_far,      1}, // 0x0B
    {&vm_systime,      2}, // 0x0C
    {&vm_invoke,       6}, // 0x0D
    {&vm_beginthread,  6}, // 0x0E
    {&vm_if,           8}, // 0x0F
    {&vm_debug,        1}, // 0x10
    {&vm_pushvalue,    2}, // 0x11
    {&vm_reserve,      1}, // 0x12
    {&vm_set,          4}, // 0x13
    {&vm_set_const,    4}, // 0x14
    {&vm_rpn,          0}, // 0x15
    {&vm_join,         2}, // 0x16
    {&vm_terminate,    2}, // 0x17
    {&vm_idle,         0}, // 0x18
    {vm_get_tlocal,    4}, // 0x19
    {&vm_if_const,     8}, // 0x1A
    {vm_get_uint8,     4}, // 0x1B
    {vm_get_int8,      4}, // 0x1C
    {vm_get_int16,     4}, // 0x1D
};


// contexts for executing scripts 
// ScriptRunnerInit(), ExecuteScript(), ScriptRunnerUpdate() manipulate these contexts
SCRIPT_CTX CTXS[VM_MAX_CONTEXTS];
SCRIPT_CTX * first_ctx, * free_ctxs;

// lock state 
uint8_t vm_lock_state;
// loaded state
uint8_t vm_loaded_state;
// exception flsg
uint8_t vm_exception_code;
uint8_t vm_exception_params_length;
uint8_t vm_exception_params_bank;
const void * vm_exception_params_offset;

// we need BANKED functions here to have two extra words before arguments
// we will put VM stuff there
// plus we get an ability to call them from wherever we want in native code
// you can manipulate context (THIS) within VM functions
// if VM function has no parameters and does not manipulate context
// then you may declare it without params at all bacause caller clears stack - that is safe

// this is a call instruction, it pushes return address onto VM stack
void vm_call_rel(SCRIPT_CTX * THIS, int8_t ofs) OLDCALL BANKED {
    // push current VM PC onto VM stack
    *(THIS->stack_ptr++) = (uint16_t)THIS->PC;
    // modify VM PC (goto PC + ofs)
    // pc is a pointer, you may point to any other script wherever you want
    // you may also pass absolute pointer instead of ofs, if you want
    THIS->PC += ofs;    
}
// call absolute instruction
void vm_call(SCRIPT_CTX * THIS, uint8_t * pc) OLDCALL BANKED {
    *(THIS->stack_ptr++) = (uint16_t)THIS->PC;
    THIS->PC = pc;    
}
// return instruction returns to a point where call was invoked
void vm_ret(SCRIPT_CTX * THIS, uint8_t n) OLDCALL BANKED {
    // pop VM PC from VM stack
    THIS->stack_ptr--;
    THIS->PC = (const uint8_t *)*(THIS->stack_ptr);
    if (n) THIS->stack_ptr -= n;
}

// far call to another bank
void vm_call_far(SCRIPT_CTX * THIS, uint8_t bank, uint8_t * pc) OLDCALL BANKED {
    *(THIS->stack_ptr++) = (uint16_t)THIS->PC;
    *(THIS->stack_ptr++) = THIS->bank;
    THIS->PC = pc;
    THIS->bank = bank;
}
// ret from far call
void vm_ret_far(SCRIPT_CTX * THIS, uint8_t n) OLDCALL BANKED {
    THIS->stack_ptr--;
    THIS->bank = (uint8_t)(*(THIS->stack_ptr));
    THIS->stack_ptr--;
    THIS->PC = (const uint8_t *)*(THIS->stack_ptr);
    if (n) THIS->stack_ptr -= n;
}

// you can also invent calling convention and pass parameters to scripts on VM stack,
// make a library of scripts and so on
// pushes word onto VM stack
void vm_push(SCRIPT_CTX * THIS, uint16_t value) OLDCALL BANKED {
    *(THIS->stack_ptr++) = value;
}
// cleans up to n words from stack and returns last one 
 uint16_t vm_pop(SCRIPT_CTX * THIS, uint8_t n) OLDCALL BANKED {
    if (n) THIS->stack_ptr -= n;
    return *(THIS->stack_ptr);
}

// do..while loop, callee cleanups stack
void vm_loop_rel(SCRIPT_CTX * THIS, int16_t idx, int8_t ofs, uint8_t n) OLDCALL BANKED {
    uint16_t * counter;
    if (idx < 0) counter = THIS->stack_ptr + idx; else counter = script_memory + idx;
    if (*counter) {
        THIS->PC += ofs, (*counter)--; 
    } else {
        if (n) THIS->stack_ptr -= n;
    }
}
// loop absolute, callee cleanups stack
void vm_loop(SCRIPT_CTX * THIS, int16_t idx, uint8_t * pc, uint8_t n) OLDCALL BANKED {
    uint16_t * counter;
    if (idx < 0) counter = THIS->stack_ptr + idx; else counter = script_memory + idx;
    if (*counter) {
        THIS->PC = pc, (*counter)--; 
    } else {
        if (n) THIS->stack_ptr -= n;
    }
}

// jump relative
void vm_jump_rel(SCRIPT_CTX * THIS, int8_t ofs) OLDCALL BANKED {
    THIS->PC += ofs;    
}
// jump absolute
void vm_jump(SCRIPT_CTX * THIS, uint8_t * pc) OLDCALL BANKED {
    THIS->PC = pc;    
}

// returns systime 
void vm_systime(SCRIPT_CTX * THIS, int16_t idx) OLDCALL BANKED {
    uint16_t * A;
    if (idx < 0) A = THIS->stack_ptr + idx; else A = script_memory + idx;
    *A = sys_time;
} 

uint8_t wait_frames(void * THIS, uint8_t start, uint16_t * stack_frame) OLDCALL BANKED {
    // we allocate one local variable (just write ahead of VM stack pointer, we have no interrupts, our local variables won't get spoiled)
    if (start) stack_frame[1] = sys_time;
    // check wait condition
    return ((sys_time - stack_frame[1]) < stack_frame[0]) ? ((SCRIPT_CTX *)THIS)->waitable = 1, 0 : 1;
}
// calls C handler until it returns true; callee cleanups stack
void vm_invoke(SCRIPT_CTX * THIS, uint8_t bank, uint8_t * fn, uint8_t nparams, int16_t idx) OLDCALL BANKED {
    uint16_t * stack_frame = (idx < 0) ? THIS->stack_ptr + idx : script_memory + idx;
    // update function pointer
    uint8_t start = ((THIS->update_fn != fn) || (THIS->update_fn_bank != bank)) ? THIS->update_fn = fn, THIS->update_fn_bank = bank, 1 : 0;
    // call handler
    if (FAR_CALL_EX(fn, bank, SCRIPT_UPDATE_FN, THIS, start, stack_frame)) {
        if (nparams) THIS->stack_ptr -= nparams;
        THIS->update_fn = 0, THIS->update_fn_bank = 0;
        return;
    }
    // call handler again, wait condition is not met
    THIS->PC -= (INSTRUCTION_SIZE + sizeof(bank) + sizeof(fn) + sizeof(nparams) + sizeof(idx));
} 

// runs script in a new thread
void vm_beginthread(DUMMY0_t dummy0, DUMMY1_t dummy1, SCRIPT_CTX * THIS, uint8_t bank, uint8_t * pc, int16_t idx, uint8_t nargs) OLDCALL NONBANKED {
    dummy0; dummy1;
    uint16_t * A;
    if (idx < 0) A = THIS->stack_ptr + idx; else A = script_memory + idx;
    SCRIPT_CTX * ctx = script_execute(bank, pc, A, 0);
    // initialize thread locals if any
    if (!(nargs)) return;
    if (ctx) {
        uint8_t _save = _current_bank;        // we must preserve current bank, 
        SWITCH_ROM(THIS->bank);        // then switch to bytecode bank
        for (uint8_t i = 0; i < nargs; i++) {
            uint16_t * A;
            if (idx < 0) A = THIS->stack_ptr + idx; else A = script_memory + idx;
            *(ctx->stack_ptr++) = *A;
            THIS->PC += 2;
        }
        SWITCH_ROM(_save);
    }
}
// 
void vm_join(SCRIPT_CTX * THIS, int16_t idx) OLDCALL BANKED {
    uint16_t * A;
    if (idx < 0) A = THIS->stack_ptr + idx; else A = script_memory + idx;
    if (!(*A >> 8)) THIS->PC -= (INSTRUCTION_SIZE + sizeof(idx)), THIS->waitable = 1;
}
// 
void vm_terminate(SCRIPT_CTX * THIS, int16_t idx) OLDCALL BANKED {
    uint16_t * A;
    if (idx < 0) A = THIS->stack_ptr + idx; else A = script_memory + idx;
    script_terminate((uint8_t)(*A));
}

// if condition; compares two arguments on VM stack
// idxA, idxB point to arguments to compare
// negative indexes are parameters on the top of VM stack, positive - absolute indexes in stack[] array
void vm_if(SCRIPT_CTX * THIS, uint8_t condition, int16_t idxA, int16_t idxB, uint8_t * pc, uint8_t n) OLDCALL BANKED {
    int16_t A, B;
    if (idxA < 0) A = *(THIS->stack_ptr + idxA); else A = script_memory[idxA];
    if (idxB < 0) B = *(THIS->stack_ptr + idxB); else B = script_memory[idxB];
    uint8_t res = 0;
    switch (condition) {
        case VM_OP_EQ: res = (A == B); break;
        case VM_OP_LT: res = (A <  B); break;
        case VM_OP_LE: res = (A <= B); break;
        case VM_OP_GT: res = (A >  B); break;
        case VM_OP_GE: res = (A >= B); break;
        case VM_OP_NE: res = (A != B); break;
    }
    if (res) THIS->PC = pc;
    if (n) THIS->stack_ptr -= n;
}
// if condition; compares argument on VM stack with an immediate value
// idxA point to arguments to compare, B is a value
// negative indexes are parameters on the top of VM stack, positive - absolute indexes in stack[] array
void vm_if_const(SCRIPT_CTX * THIS, uint8_t condition, int16_t idxA, int16_t B, uint8_t * pc, uint8_t n) OLDCALL BANKED {
    int16_t A;
    if (idxA < 0) A = *(THIS->stack_ptr + idxA); else A = script_memory[idxA];
    uint8_t res = 0;
    switch (condition) {
        case VM_OP_EQ: res = (A == B); break;
        case VM_OP_LT: res = (A <  B); break;
        case VM_OP_LE: res = (A <= B); break;
        case VM_OP_GT: res = (A >  B); break;
        case VM_OP_GE: res = (A >= B); break;
        case VM_OP_NE: res = (A != B); break;
    }
    if (res) THIS->PC = pc;
    if (n) THIS->stack_ptr -= n;
}
// pushes value from VM stack onto VM stack
// if idx >= 0 then idx is absolute, else idx is relative to VM stack pointer
void vm_pushvalue(SCRIPT_CTX * THIS, int16_t idx) OLDCALL BANKED {
    if (idx < 0) *(THIS->stack_ptr) = *(THIS->stack_ptr + idx); else *(THIS->stack_ptr) = script_memory[idx];
    THIS->stack_ptr++;
}
// manipulates VM stack pointer
void vm_reserve(SCRIPT_CTX * THIS, int8_t ofs) OLDCALL BANKED {
    THIS->stack_ptr += ofs;
}
// sets value on stack indexed by idxA to value on stack indexed by idxB 
void vm_set(SCRIPT_CTX * THIS, int16_t idxA, int16_t idxB) OLDCALL BANKED {
    int16_t * A, * B;
    if (idxA < 0) A = THIS->stack_ptr + idxA; else A = script_memory + idxA;
    if (idxB < 0) B = THIS->stack_ptr + idxB; else B = script_memory + idxB;
    *A = *B;
}
// sets value on stack indexed by idx to value
void vm_set_const(SCRIPT_CTX * THIS, int16_t idx, uint16_t value) OLDCALL BANKED {
    uint16_t * A;
    if (idx < 0) A = THIS->stack_ptr + idx; else A = script_memory + idx;
    *A = value;
}
// sets value on stack indexed by idxA to value on stack indexed by idxB 
void vm_get_tlocal(SCRIPT_CTX * THIS, int16_t idxA, int16_t idxB) OLDCALL BANKED {
    int16_t * A, * B;
    if (idxA < 0) A = THIS->stack_ptr + idxA; else A = script_memory + idxA;
    if (idxB < 0) B = THIS->stack_ptr + idxB; else B = THIS->base_addr + idxB;
    *A = *B;
}
// rpn calculator; must be NONBANKED because we access VM bytecode
// dummy parameters are needed to make nonbanked function to be compatible with banked call
void vm_rpn(DUMMY0_t dummy0, DUMMY1_t dummy1, SCRIPT_CTX * THIS) OLDCALL NONBANKED {
    dummy0; dummy1; // suppress warnings
    int16_t * A, * B, * ARGS;
    int16_t idx;

    uint8_t _save = _current_bank;        // we must preserve current bank, 
    SWITCH_ROM(THIS->bank);        // then switch to bytecode bank

    ARGS = THIS->stack_ptr;
    while (1) {
        int8_t op = *(THIS->PC++);
        if (op < 0) {
            switch (op) {
                // reference
                case -3:
                    idx = *((int16_t *)(THIS->PC)); 
                    if (idx < 0) A = ARGS + idx; else A = script_memory + idx;
                    *(THIS->stack_ptr) = *A;
                    THIS->PC += 2;
                    break;
                // int16
                case -2: 
                    *(THIS->stack_ptr) = *((uint16_t *)(THIS->PC));
                    THIS->PC += 2;
                    break;
                // int8
                case -1:
                    op = *(THIS->PC++); 
                    *(THIS->stack_ptr) = op;
                    break;
                default:
                    SWITCH_ROM(_save);         // restore bank
                    return;
            }
            THIS->stack_ptr++;
        } else {
            A = THIS->stack_ptr - 2; B = A + 1;
            switch (op) {
                // arithmetics
                case '+': *A = *A  +  *B; break;
                case '-': *A = *A  -  *B; break;
                case '*': *A = *A  *  *B; break;
                case '/': *A = *A  /  *B; break;
                case '%': *A = *A  %  *B; break;
                // logical
                case VM_OP_EQ:  *A = (*A  ==  *B); break;
                case VM_OP_LT:  *A = (*A  <   *B); break;
                case VM_OP_LE:  *A = (*A  <=  *B); break;
                case VM_OP_GT:  *A = (*A  >   *B); break;
                case VM_OP_GE:  *A = (*A  >=  *B); break;
                case VM_OP_NE:  *A = (*A  !=  *B); break;
                case VM_OP_AND: *A = ((bool)(*A)  &&  (bool)(*B)); break;
                case VM_OP_OR:  *A = ((bool)(*A)  ||  (bool)(*B)); break;
                // bit
                case '&': *A = *A  &  *B; break;
                case '|': *A = *A  |  *B; break;
                case '^': *A = *A  ^  *B; break;
                // unary
                case '@': *B = abs(*B); continue;
                // terminator
                default:
                    SWITCH_ROM(_save);         // restore bank
                    return;
            }
            THIS->stack_ptr--;
        }
    }
}

// prints debug string into the text buffer then outputs to screen
void vm_debug(DUMMY0_t dummy0, DUMMY1_t dummy1, SCRIPT_CTX * THIS, uint8_t nargs) OLDCALL NONBANKED {
    dummy0; dummy1; // suppress warnings

    static unsigned char display_text[80];

    uint8_t _save = _current_bank;
    SWITCH_ROM(THIS->bank);
    
    const uint8_t * args = THIS->PC;
    unsigned char * d = display_text; 
    const unsigned char * s = args + (nargs << 1);
    int16_t idx;

    while (*s) {
        if (*s == '%') {
            s++;
            switch (*s) {
                case 'd':
                    idx = *((int16_t *)args);
                    if (idx < 0) idx = *(THIS->stack_ptr + idx); else idx = script_memory[idx];
                    d += strlen(itoa(idx, d, 10));
                    s++;
                    args += 2;
                    continue;
                case '%':
                    break;
                default:
                    s--;
            }
        }
        *d++ = *s++;
    }
    *d = 0, s++;

    puts(display_text);

    SWITCH_ROM(_save);
    THIS->PC = s;
}

// puts context into a waitable state
void vm_idle(SCRIPT_CTX * THIS) OLDCALL BANKED {
    THIS->waitable = 1;
}

// gets unsigned int8 from RAM by address
void vm_get_uint8(SCRIPT_CTX * THIS, int16_t idxA, uint8_t * addr) OLDCALL BANKED {
    int16_t * A;
    if (idxA < 0) A = THIS->stack_ptr + idxA; else A = script_memory + idxA;
    *A = *addr;
}
// gets int8 from RAM by address
void vm_get_int8(SCRIPT_CTX * THIS, int16_t idxA, int8_t * addr) OLDCALL BANKED {
    int16_t * A;
    if (idxA < 0) A = THIS->stack_ptr + idxA; else A = script_memory + idxA;
    *A = *addr;
}
// gets int16 from RAM by address
void vm_get_int16(SCRIPT_CTX * THIS, int16_t idxA, int16_t * addr) OLDCALL BANKED {
    int16_t * A;
    if (idxA < 0) A = THIS->stack_ptr + idxA; else A = script_memory + idxA;
    *A = *addr;
}
// executes one step in the passed context
// return zero if script end
// bank with VM code must be active
uint8_t VM_STEP(SCRIPT_CTX * CTX) NAKED NONBANKED STEP_FUNC_ATTR {
    CTX;
#ifdef __SDCC
#if defined(NINTENDO)
__asm
        lda hl, 2(sp)
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        
        inc hl
        inc hl

        ld a, (hl-)
        ld e, a
        ld a, (hl-)
        ld l, (hl)
        ld h, a

        ldh a, (__current_bank)
        push af

        ld a, e
        ldh (__current_bank), a
        ld (0x2000), a          ; switch bank with vm code
        
        ld a, (hl+)             ; load current command and return if terminator
        ld e, a
        or a
        jr z, 3$

        push bc                 ; store bc
        push hl

        ld d, #0
        ld h, d
        ld l, e
        add hl, hl
        add hl, de              ; hl = de * sizeof(SCRIPT_CMD)
        dec hl
        ld de, #_script_cmds
        add hl, de              ; hl = &script_cmds[command].args_len

        ld a, (hl-)
        ld e, a                 ; e = args_len
        ld a, (hl-)
        ld b, a
        ld c, (hl)              ; bc = fn

        pop hl                  ; hl points to the next VM instruction or a first byte of the args
        ld d, e                 ; d = arg count
        srl d
        jr nc, 4$               ; d is even?
        ld a, (hl+)             ; copy one arg onto stack
        push af
        inc sp
4$:
        jr z, 1$                ; only one arg?
2$:                             
        ld a, (hl+)
        push af
        inc sp
        ld a, (hl+)
        push af
        inc sp
        dec d
        jr nz, 2$               ; loop through remaining args, copy 2 bytes at a time
1$:
        push bc                 ; save function pointer

        ld b, h
        ld c, l                 ; bc points to the next VM instruction

        lda hl, 8(sp)
        add hl, de              ; add correction
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        ld (hl), c
        ld c, l
        ld a, h
        inc hl
        ld (hl), b              ; PC = PC + sizeof(instruction) + args_len
        ld b, a                 ; bc = THIS

        pop hl                  ; restore function pointer
        push bc                 ; pushing THIS

        push de                 ; not used
        push de                 ; de: args_len

        ld a, #<b_vm_call       ; a = script_bank (all script functions in one bank: take any complimantary symbol)
        ldh (__current_bank), a
        ld (0x2000), a          ; switch bank with functions

        rst 0x20                ; call hl

        pop hl                  ; hl: args_len
        add hl, sp
        ld sp, hl               ; deallocate args_len bytes from the stack
        add sp, #4              ; deallocate dummy word and THIS

        pop bc                  ; restore bc

        ld e, #1                ; command executed
3$:     
        pop af
        ldh (__current_bank), a
        ld (0x2000), a          ; restore bank

        ret
__endasm;
#elif defined(SEGA)
__asm    
        .ez80

        ex de, hl
        ld iyh, d
        ld iyl, e

        ld l, 0 (iy)
        ld h, 1 (iy)

        ld a, (_MAP_FRAME1)
        push af

        ld a, 2 (iy)
        ld (_MAP_FRAME1), a
        
        ld a, (hl)              ; load current command and return if terminator
        inc hl
        ld e, a
        or a
        jr z, 3$

        ld d, #0
        dec e

        ld b, h
        ld c, l                 ; save hl to bc

        ld h, d
        ld l, e

        add hl, hl      
        add hl, de              ; hl = de * sizeof(SCRIPT_CMD)

        ld de, #_script_cmds
        add hl, de              ; hl = &script_cmds[command]

        ld e, (hl)
        inc hl
        ld d, (hl)              ; de = fn
        inc hl
        ld a, (hl)

        ld h, b
        ld l, c                 ; restore hl from bc
        ld c, a

        ld b, c                 ; b = c = args_len
        srl b
        jr nc, 4$               ; d is even?
        ld a, (hl)              ; copy one arg onto stack
        inc hl
        push af
        inc sp
4$:
        jr z, 1$                ; only one parameter?
2$:                             
        ld a, (hl)
        inc hl
        push af
        inc sp
        ld a, (hl)
        inc hl
        push af
        inc sp
        dec b
        jr nz, 2$               ; loop through remaining parameters, copy 2 bytes at a time
1$:

        ld 0 (iy), l
        ld 1 (iy), h            ; PC = PC + sizeof(instruction) + args_len

        push iy                 ; pushing THIS

        push bc                 ; bc: args_len
        dec sp                  ; not used

        ld a, #<b_vm_call       ; a = script_bank (all script functions in one bank: take any complimantary symbol)
        ld (_MAP_FRAME1), a     ; switch bank with functions

        ex de, hl
        rst  0x30

        inc sp

        pop hl                  ; hl: args_len
        pop de                  ; deallocate THIS
        add hl, sp
        ld sp, hl

        ld e, #1                ; command executed
3$:     
        pop af
        ld (_MAP_FRAME1), a     ; restore bank

        ld l, e                 ; __z88dk_fastcall function must return result in l

        ret
__endasm;
#endif
#endif
}

// global shared script memory
uint16_t script_memory[VM_HEAP_SIZE + (VM_MAX_CONTEXTS * VM_CONTEXT_STACK_SIZE)];

// initialize script runner contexts
// resets whole VM engine
void script_runner_init(uint8_t reset) BANKED {
    if (reset) {
        memset(script_memory, 0, sizeof(script_memory));
        memset(CTXS, 0, sizeof(CTXS));
    }
    uint16_t * base_addr = &script_memory[VM_HEAP_SIZE];
    free_ctxs = CTXS, first_ctx = 0;
    SCRIPT_CTX * nxt = 0;
    SCRIPT_CTX * tmp = CTXS + (VM_MAX_CONTEXTS - 1);
    for (uint8_t i = VM_MAX_CONTEXTS; i != 0; i--) {
        tmp->next = nxt;
        tmp->base_addr = base_addr;
        tmp->ID = i;
        base_addr += VM_CONTEXT_STACK_SIZE;
        nxt = tmp--;
    }
    vm_lock_state = 0;
    vm_loaded_state = FALSE;
}

// execute a script in the new allocated context
// actually, it initializes free context with bytecode and moves it into the active context chain
SCRIPT_CTX * script_execute(uint8_t bank, uint8_t * pc, uint16_t * handle, uint8_t nargs, ...) BANKED {
    if (free_ctxs == 0) return NULL;
#ifdef SAFE_SCRIPT_EXECUTE
    if (pc == NULL) return NULL;
#endif

    SCRIPT_CTX * tmp = free_ctxs;
    // remove context from free list
    free_ctxs = free_ctxs->next;
    // initialize context
    tmp->PC = pc, tmp->bank = bank, tmp->stack_ptr = tmp->base_addr;
    // set thread handle by reference
    tmp->hthread = handle;
    if (handle) *handle = tmp->ID;
    // clear termination flag
    tmp->terminated = FALSE;
    // clear lock count
    tmp->lock_count = 0;
    // clear flags
    tmp->flags = 0;
    // Clear update fn
    tmp->update_fn_bank = 0;
    // add context to active list
    tmp->next = first_ctx, first_ctx = tmp;
    // push threadlocals
    if (nargs) {
        va_list va;
        va_start(va, nargs);
        for (uint8_t i = nargs; i != 0; i--) {
            *(tmp->stack_ptr++) = va_arg(va, INT16);
        }
    }
    // return thread ID
    return tmp;
}

// terminate script by ID
uint8_t script_terminate(uint8_t ID) BANKED {
    static SCRIPT_CTX * ctx;
    ctx = first_ctx; 
    while (ctx) {
        if (ctx->ID == ID) {
            if (ctx->hthread) {
                *(ctx->hthread) |= SCRIPT_TERMINATED;
                ctx->hthread = 0; 
            } 
            return ctx->terminated = TRUE;
        } else ctx = ctx->next;
    }
    return FALSE;
}

// process all contexts
// executes one command in each active context
uint8_t script_runner_update() NONBANKED {
    static SCRIPT_CTX * old_ctx, * ctx;
    static uint8_t waitable;
    static uint8_t counter;
    old_ctx = 0, ctx = first_ctx;
    waitable = TRUE;
    counter = INSTRUCTIONS_PER_QUANT;
    while (ctx) {
        vm_exception_code = EXCEPTION_CODE_NONE;
        ctx->waitable = FALSE;
        if ((ctx->terminated != FALSE) || (!VM_STEP(ctx))) {
            // update lock state
            vm_lock_state -= ctx->lock_count;
            // update handle if present
            if (ctx->hthread) *(ctx->hthread) |= SCRIPT_TERMINATED;
            // script is finished, remove from linked list
            if (old_ctx) old_ctx->next = ctx->next; else first_ctx = ctx->next;
            // add terminated context to free contexts list
            ctx->next = free_ctxs, free_ctxs = ctx;
            // next context
            if (old_ctx) ctx = old_ctx->next; else ctx = first_ctx;
        } else {
            // check exception
            if (vm_exception_code) return RUNNER_EXCEPTION;
            // loop until waitable state or quant is expired 
            if (!(ctx->waitable) && (counter--)) continue;
            // switch to the next context
            waitable &= ctx->waitable; 
            old_ctx = ctx, ctx = ctx->next;
            counter = INSTRUCTIONS_PER_QUANT;
        }
    }
    // return 0 if all threads are finished
    if (first_ctx == 0) return RUNNER_DONE;
    // return 1 if all threads in waitable state else return 2
    if (waitable) return RUNNER_IDLE; else return RUNNER_BUSY;
}
