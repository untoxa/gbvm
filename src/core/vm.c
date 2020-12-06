#pragma bank 2

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "vm.h"

// define addressmod for HOME
void ___vm_dummy_fn(void) NONBANKED __preserves_regs(a, b, c, d, e, h, l);
__addressmod ___vm_dummy_fn const HOME;

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
SCRIPT_CTX CTXS[SCRIPT_MAX_CONTEXTS];
SCRIPT_CTX * first_ctx, * free_ctxs;

// we need __banked functions here to have two extra words before arguments
// we will put VM stuff there
// plus we get an ability to call them from wherever we want in native code
// you can manipulate context (THIS) within VM functions
// if VM function has no parameters and does not manipulate context
// then you may declare it without params at all bacause caller clears stack - that is safe

// this is a call instruction, it pushes return address onto VM stack
void vm_call_rel(SCRIPT_CTX * THIS, INT8 ofs) __banked {
    // push current VM PC onto VM stack
    *(THIS->stack_ptr++) = (UWORD)THIS->PC;
    // modify VM PC (goto PC + ofs)
    // pc is a pointer, you may point to any other script wherever you want
    // you may also pass absolute pointer instead of ofs, if you want
    THIS->PC += ofs;    
}
// call absolute instruction
void vm_call(SCRIPT_CTX * THIS, UBYTE * pc) __banked {
    *(THIS->stack_ptr++) = (UWORD)THIS->PC;
    THIS->PC = pc;    
}
// return instruction returns to a point where call was invoked
void vm_ret(SCRIPT_CTX * THIS, UBYTE n) __banked {
    // pop VM PC from VM stack
    THIS->stack_ptr--;
    THIS->PC = (const UBYTE *)*(THIS->stack_ptr);
    if (n) THIS->stack_ptr -= n;
}

// far call to another bank
void vm_call_far(SCRIPT_CTX * THIS, UBYTE bank, UBYTE * pc) __banked {
    *(THIS->stack_ptr++) = (UWORD)THIS->PC;
    *(THIS->stack_ptr++) = THIS->bank;
    THIS->PC = pc;
    THIS->bank = bank;
}
// ret from far call
void vm_ret_far(SCRIPT_CTX * THIS, UBYTE n) __banked {
    THIS->stack_ptr--;
    THIS->bank = (UBYTE)(*(THIS->stack_ptr));
    THIS->stack_ptr--;
    THIS->PC = (const UBYTE *)*(THIS->stack_ptr);
    if (n) THIS->stack_ptr -= n;
}

// you can also invent calling convention and pass parameters to scripts on VM stack,
// make a library of scripts and so on
// pushes word onto VM stack
void vm_push(SCRIPT_CTX * THIS, UWORD value) __banked {
    *(THIS->stack_ptr++) = value;
}
// cleans up to n words from stack and returns last one 
 UWORD vm_pop(SCRIPT_CTX * THIS, UBYTE n) __banked {
    if (n) THIS->stack_ptr -= n;
    return *(THIS->stack_ptr);
}

// do..while loop, callee cleanups stack
void vm_loop_rel(SCRIPT_CTX * THIS, INT16 idx, INT8 ofs, UBYTE n) __banked {
    UWORD * counter;
    if (idx < 0) counter = THIS->stack_ptr + idx; else counter = script_memory + idx;
    if (*counter) {
        THIS->PC += ofs, (*counter)--; 
    } else {
        if (n) THIS->stack_ptr -= n;
    }
}
// loop absolute, callee cleanups stack
void vm_loop(SCRIPT_CTX * THIS, INT16 idx, UINT8 * pc, UBYTE n) __banked {
    UWORD * counter;
    if (idx < 0) counter = THIS->stack_ptr + idx; else counter = script_memory + idx;
    if (*counter) {
        THIS->PC = pc, (*counter)--; 
    } else {
        if (n) THIS->stack_ptr -= n;
    }
}

// jump relative
void vm_jump_rel(SCRIPT_CTX * THIS, INT8 ofs) __banked {
    THIS->PC += ofs;    
}
// jump absolute
void vm_jump(SCRIPT_CTX * THIS, UBYTE * pc) __banked {
    THIS->PC = pc;    
}

// returns systime 
void vm_systime(SCRIPT_CTX * THIS, INT16 idx) __banked {
    UWORD * A;
    if (idx < 0) A = THIS->stack_ptr + idx; else A = script_memory + idx;
    *A = sys_time;
} 

UBYTE wait_frames(void * THIS, UBYTE start, UWORD * stack_frame) __banked {
    // we allocate one local variable (just write ahead of VM stack pointer, we have no interrupts, our local variables won't get spoiled)
    if (start) stack_frame[1] = sys_time;
    // check wait condition
    return ((sys_time - stack_frame[1]) < stack_frame[0]) ? ((SCRIPT_CTX *)THIS)->waitable = 1, 0 : 1;
}
// calls C handler until it returns true; callee cleanups stack
void vm_invoke(SCRIPT_CTX * THIS, UBYTE bank, UBYTE * fn, UBYTE nparams, INT16 idx) __banked {
    UWORD * stack_frame = (idx < 0) ? THIS->stack_ptr + idx : script_memory + idx;
    // update function pointer
    UBYTE start = ((THIS->update_fn != fn) || (THIS->update_fn_bank != bank)) ? THIS->update_fn = fn, THIS->update_fn_bank = bank, 1 : 0;
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
void vm_beginthread(UWORD dummy0, UWORD dummy1, SCRIPT_CTX * THIS, UBYTE bank, UBYTE * pc, INT16 idx, UBYTE nargs) __nonbanked {
    dummy0; dummy1;
    UWORD * A;
    if (idx < 0) A = THIS->stack_ptr + idx; else A = script_memory + idx;
    SCRIPT_CTX * ctx = ExecuteScript(bank, pc, A, 0);
    // initialize thread locals if any
    if (!(nargs)) return;
    if (ctx) {
        UBYTE _save = _current_bank;        // we must preserve current bank, 
        SWITCH_ROM_MBC1(THIS->bank);        // then switch to bytecode bank
        for (UBYTE i = 0; i < nargs; i++) {
            UWORD * A;
            if (idx < 0) A = THIS->stack_ptr + idx; else A = script_memory + idx;
            *(ctx->stack_ptr++) = *A;
            THIS->PC += 2;
        }
        SWITCH_ROM_MBC1(_save);
    }
}
// 
void vm_join(SCRIPT_CTX * THIS, INT16 idx) __banked {
    UWORD * A;
    if (idx < 0) A = THIS->stack_ptr + idx; else A = script_memory + idx;
    if (!(*A >> 8)) THIS->PC -= (INSTRUCTION_SIZE + sizeof(idx)), THIS->waitable = 1;
}
// 
void vm_terminate(SCRIPT_CTX * THIS, INT16 idx) __banked {
    UWORD * A;
    if (idx < 0) A = THIS->stack_ptr + idx; else A = script_memory + idx;
    TerminateScript((UBYTE)(*A));
}

// if condition; compares two arguments on VM stack
// idxA, idxB point to arguments to compare
// negative indexes are parameters on the top of VM stack, positive - absolute indexes in stack[] array
void vm_if(SCRIPT_CTX * THIS, UBYTE condition, INT16 idxA, INT16 idxB, UBYTE * pc, UBYTE n) __banked {
    INT16 A, B;
    if (idxA < 0) A = *(THIS->stack_ptr + idxA); else A = script_memory[idxA];
    if (idxB < 0) B = *(THIS->stack_ptr + idxB); else B = script_memory[idxB];
    UBYTE res = 0;
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
void vm_if_const(SCRIPT_CTX * THIS, UBYTE condition, INT16 idxA, INT16 B, UBYTE * pc, UBYTE n) __banked {
    INT16 A;
    if (idxA < 0) A = *(THIS->stack_ptr + idxA); else A = script_memory[idxA];
    UBYTE res = 0;
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
void vm_pushvalue(SCRIPT_CTX * THIS, INT16 idx) __banked {
    if (idx < 0) *(THIS->stack_ptr) = *(THIS->stack_ptr + idx); else *(THIS->stack_ptr) = script_memory[idx];
    THIS->stack_ptr++;
}
// manipulates VM stack pointer
void vm_reserve(SCRIPT_CTX * THIS, INT8 ofs) __banked {
    THIS->stack_ptr += ofs;
}
// sets value on stack indexed by idxA to value on stack indexed by idxB 
void vm_set(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) __banked {
    INT16 * A, * B;
    if (idxA < 0) A = THIS->stack_ptr + idxA; else A = script_memory + idxA;
    if (idxB < 0) B = THIS->stack_ptr + idxB; else B = script_memory + idxB;
    *A = *B;
}
// sets value on stack indexed by idx to value
void vm_set_const(SCRIPT_CTX * THIS, INT16 idx, UWORD value) __banked {
    UWORD * A;
    if (idx < 0) A = THIS->stack_ptr + idx; else A = script_memory + idx;
    *A = value;
}
// sets value on stack indexed by idxA to value on stack indexed by idxB 
void vm_get_tlocal(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) __banked {
    INT16 * A, * B;
    if (idxA < 0) A = THIS->stack_ptr + idxA; else A = script_memory + idxA;
    if (idxB < 0) B = THIS->stack_ptr + idxB; else B = THIS->base_addr + idxB;
    *A = *B;
}
// rpn calculator; must be __nonbanked because we access VM bytecode
// dummy parameters are needed to make nonbanked function to be compatible with banked call
void vm_rpn(UWORD dummy0, UWORD dummy1, SCRIPT_CTX * THIS) __nonbanked {
    dummy0; dummy1; // suppress warnings
    INT16 * A, * B, * ARGS;
    INT16 idx;

    UBYTE _save = _current_bank;        // we must preserve current bank, 
    SWITCH_ROM_MBC1(THIS->bank);        // then switch to bytecode bank

    ARGS = THIS->stack_ptr;
    while (1) {
        INT8 op = *(THIS->PC++);
        if (op < 0) {
            switch (op) {
                // reference
                case -3:
                    idx = *((INT16 *)(THIS->PC)); 
                    if (idx < 0) A = ARGS + idx; else A = script_memory + idx;
                    *(THIS->stack_ptr) = *A;
                    THIS->PC += 2;
                    break;
                // int16
                case -2: 
                    *(THIS->stack_ptr) = *((UWORD *)(THIS->PC));
                    THIS->PC += 2;
                    break;
                // int8
                case -1:
                    op = *(THIS->PC++); 
                    *(THIS->stack_ptr) = op;
                    break;
                default:
                    SWITCH_ROM_MBC1(_save);         // restore bank
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
                    SWITCH_ROM_MBC1(_save);         // restore bank
                    return;
            }
            THIS->stack_ptr--;
        }
    }
}

// text buffer
unsigned char display_text[80];

// prints debug string into the text buffer then outputs to screen
void vm_debug(UWORD dummy0, UWORD dummy1, SCRIPT_CTX * THIS, UBYTE nargs) __nonbanked {
    dummy0; dummy1; // suppress warnings

    UBYTE _save = _current_bank;
    SWITCH_ROM_MBC1(THIS->bank);
    
    const UBYTE * args = THIS->PC;
    unsigned char * d = display_text; 
    const unsigned char * s = args + (nargs << 1);
    INT16 idx;

    while (*s) {
        if (*s == '%') {
            s++;
            switch (*s) {
                case 'd':
                    idx = *((INT16 *)args);
                    if (idx < 0) idx = *(THIS->stack_ptr + idx); else idx = script_memory[idx];
                    d += strlen(itoa(idx, d));
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

    SWITCH_ROM_MBC1(_save);
    THIS->PC = s;
}

// puts context into a waitable state
void vm_idle(SCRIPT_CTX * THIS) __banked {
    THIS->waitable = 1;
}

// gets unsigned int8 from RAM by address
void vm_get_uint8(SCRIPT_CTX * THIS, INT16 idxA, UINT8 * addr) __banked {
    INT16 * A;
    if (idxA < 0) A = THIS->stack_ptr + idxA; else A = script_memory + idxA;
    *A = *addr;
}
// gets int8 from RAM by address
void vm_get_int8(SCRIPT_CTX * THIS, INT16 idxA, INT8 * addr) __banked {
    INT16 * A;
    if (idxA < 0) A = THIS->stack_ptr + idxA; else A = script_memory + idxA;
    *A = *addr;
}
// gets int16 from RAM by address
void vm_get_int16(SCRIPT_CTX * THIS, INT16 idxA, INT16 * addr) __banked {
    INT16 * A;
    if (idxA < 0) A = THIS->stack_ptr + idxA; else A = script_memory + idxA;
    *A = *addr;
}
// executes one step in the passed context
// return zero if script end
// bank with VM code must be active
UBYTE STEP_VM(SCRIPT_CTX * CTX) __naked __nonbanked __preserves_regs(b, c) {
    CTX;
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
        
        ld a, (HL+)             ; load current command and return if terminator
        ld e, a
        or a
        jr z, 3$

        push bc                 ; store bc
        push hl

        add a
        add e                   ; a = a * sizeof(SCRIPT_CMD)

        ld hl, #_script_cmds
        add l
        ld l, a
        adc h
        sub l
        ld h, a                 ; hl = &script_cmds[command+1]
        dec hl                  ; hl = &script_cmds[command].args_len

        ld a, (hl-)
        ld e, a                 ; e = args_len
        ld b, (hl)
        dec hl
        ld c, (hl)              ; bc = fn

        pop hl                  ; hl points to the next VM instruction or a first byte of the args
        ld d, e                 ; d = param count
        srl d
        jr nc, 4$
        ld a, (hl+)
        push af
        inc sp
4$:
        jr z, 1$
2$:                             ; copy args onto stack
        ld a, (hl+)
        push af
        inc sp
        ld a, (hl+)
        push af
        inc sp
        dec d
        jr nz, 2$
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
        push de                 ; d: fn_bank, e: args_len

        ld a, #b_vm_call        ; a = script_bank (all script functions in one bank: take any complimantary symbol)
        ldh (__current_bank), a
        ld (0x2000), a          ; switch bank with functions

        rst 0x20                ; call hl

        pop hl                  ; h: _current_bank, l: args_len

        ld  h, #0
        ld  a, #4
        add l
        ld l, a
        add hl, sp              ; deallocate dummy word, this and args_len bytes from the stack
        ld sp, hl

        pop bc                  ; restore bc

        ld e, #1                ; command executed
3$:     
        pop af
        ldh (__current_bank), a
        ld (0x2000), a          ; restore bank

        ret
__endasm;
}
// dummy function for __addressmod attribure, it never gets called
void ___vm_dummy_fn(void) __nonbanked __preserves_regs(a, b, c, d, e, h, l) __naked { __asm__("ret"); }

// global shared script memory
UWORD script_memory[MAX_GLOBAL_VARS + (SCRIPT_MAX_CONTEXTS * CONTEXT_STACK_SIZE)];

// initialize script runner contexts
// resets whole VM engine
void ScriptRunnerInit() __banked {
    UWORD * base_addr = &script_memory[MAX_GLOBAL_VARS];
    free_ctxs = CTXS, first_ctx = 0;
    memset(script_memory, 0, sizeof(script_memory));
    memset(CTXS, 0, sizeof(CTXS));

    SCRIPT_CTX * nxt = 0;
    SCRIPT_CTX * tmp = CTXS + (SCRIPT_MAX_CONTEXTS - 1);
    for (UBYTE i = 0; i < SCRIPT_MAX_CONTEXTS; i++) {
        tmp->next = nxt;
        tmp->base_addr = base_addr;
        tmp->ID = i + 1;
        base_addr += CONTEXT_STACK_SIZE;
        nxt = tmp--;
    }
}

// execute a script in the new allocated context
// actually, it initializes free context with bytecode and moves it into the active context chain
SCRIPT_CTX * ExecuteScript(UBYTE bank, UBYTE * pc, UWORD * handle, INT8 nargs, ...) __banked {
    if (free_ctxs) {
        SCRIPT_CTX * tmp = free_ctxs;
        // remove context from free list
        free_ctxs = free_ctxs->next;
        // initialize context
        tmp->PC = pc, tmp->bank = bank, tmp->stack_ptr = tmp->base_addr;
        // set thread handle by reference
        tmp->hthread = handle;
        if (handle) *handle = tmp->ID;
        // clear termination flag
        tmp->terminated = 0;
        // add context to active list
        tmp->next = first_ctx, first_ctx = tmp;
        // push threadlocals
        if (nargs) {
            va_list va;
            va_start(va, nargs);
            for (INT8 i = 0; i < nargs; i++) {
                *(tmp->stack_ptr++) = va_arg(va, INT16);
            }
        }
        // return thread ID
        return tmp;
    }
    return 0;
}

// terminate script by ID
UBYTE TerminateScript(UBYTE ID) __banked {
    static SCRIPT_CTX * ctx;
    ctx = first_ctx; 
    while (ctx) {
        if (ctx->ID == ID) {
            ctx->terminated = 1;
            return 1;
        } else ctx = ctx->next;
    }
    return 0;
}

// process all contexts
// executes one command in each active context
UBYTE ScriptRunnerUpdate() __nonbanked {
    static SCRIPT_CTX * old_ctx, * ctx;
    static UBYTE waitable;
    static UBYTE counter;
    old_ctx = 0, ctx = first_ctx;
    waitable = 1;
    counter = INSTRUCTIONS_PER_QUANT;
    while (ctx) {
        ctx->waitable = 0;
        if ((ctx->terminated) || (!STEP_VM(ctx))) {
            // update handle if present
            if (ctx->hthread) *(ctx->hthread) |= 0x8000;
            // script is finished, remove from linked list
            if (old_ctx) old_ctx->next = ctx->next; else first_ctx = ctx->next;
            // add terminated context to free contexts list
            ctx->next = free_ctxs, free_ctxs = ctx;
            // next context
            if (old_ctx) ctx = old_ctx->next; else ctx = first_ctx;
        } else {
            if (!(ctx->waitable) && (counter--)) continue;
            waitable &= ctx->waitable; 
            old_ctx = ctx, ctx = ctx->next;
            counter = INSTRUCTIONS_PER_QUANT;
        }
    }
    // return 0 if all threads are finished
    if (first_ctx == 0) return 0;
    // return 1 if all threads in waitable state else return 2
    if (waitable) return 1; else return 2;
}
