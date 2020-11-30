#ifndef _VM_H_INCLUDE
#define _VM_H_INCLUDE

#include <gb/gb.h>
#include <gb/far_ptr.h>

#include <stdio.h>

typedef void * SCRIPT_CMD_FN;

typedef struct _SCRIPT_CMD {
  SCRIPT_CMD_FN fn;  
  UBYTE args_len;
} SCRIPT_CMD;

typedef UBYTE (*SCRIPT_UPDATE_FN)(void * THIS, UBYTE start, UBYTE nparams, UWORD * stack_frame) __banked;

typedef struct SCRIPT_CTX {
  const UBYTE * PC;
  UBYTE bank;
  // linked list of contexts for cooperative multitasking
  struct SCRIPT_CTX * next;
  // update function
  FAR_PTR update_fn;
  // VM stack 
  UWORD * stack_ptr;        // stack pointer
  UWORD stack[16];          // maximum stack depth is 16 words
} SCRIPT_CTX;

#define INSTRUCTION_SIZE 1

// maximum number of concurrent running threads
#define SCRIPT_MAX_CONTEXTS 10

// script core functions
void vm_push(SCRIPT_CTX * THIS, UWORD value) __banked;
UWORD vm_pop(SCRIPT_CTX * THIS, UBYTE n) __banked;
void vm_call_rel(SCRIPT_CTX * THIS, INT8 ofs) __banked;
void vm_call(SCRIPT_CTX * THIS, UBYTE * pc) __banked;
void vm_ret(SCRIPT_CTX * THIS) __banked;
void vm_call_far(SCRIPT_CTX * THIS, UBYTE bank, UBYTE * pc) __banked;
void vm_ret_far(SCRIPT_CTX * THIS) __banked;
void vm_loop_rel(SCRIPT_CTX * THIS, INT8 ofs) __banked;
void vm_loop(SCRIPT_CTX * THIS, UINT8 * pc) __banked;
void vm_jump_rel(SCRIPT_CTX * THIS, INT8 ofs) __banked;
void vm_jump(SCRIPT_CTX * THIS, UBYTE * pc) __banked;
void vm_systime(SCRIPT_CTX * THIS) __banked;
void vm_invoke(SCRIPT_CTX * THIS, UBYTE bank, UBYTE * fn, UBYTE nparams) __banked;
void vm_beginthread(SCRIPT_CTX * THIS, UBYTE bank, UBYTE * pc) __banked;
void vm_ifcond(SCRIPT_CTX * THIS, UBYTE condition, INT8 idxA, INT8 idxB, UBYTE * pc, UBYTE n) __banked;
void vm_debug(SCRIPT_CTX * THIS, char * str) __banked;
void vm_pushvalue(SCRIPT_CTX * THIS, INT8 idx) __banked;
void vm_reserve(SCRIPT_CTX * THIS, INT8 ofs) __banked;
void vm_set(SCRIPT_CTX * THIS, INT8 idxA, INT8 idxB) __banked;
void vm_set_const(SCRIPT_CTX * THIS, INT8 idx, UWORD value) __banked;
void vm_rpn(UWORD dummy0, UWORD dummy1, SCRIPT_CTX * THIS) __nonbanked;

// return zero if script end
// bank with VM code must be active
UBYTE STEP_VM(SCRIPT_CTX * CTX) __naked __nonbanked __preserves_regs(b, c);

// initialize script runner contexts
void ScriptRunnerInit() __banked;
// execute a script in the new allocated context
UBYTE ExecuteScript(UBYTE bank, UBYTE * pc) __banked;
// process all contexts
UBYTE ScriptRunnerUpdate() __nonbanked;

#endif