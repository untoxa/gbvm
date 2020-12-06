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
  // VM stack pointer
  UWORD * stack_ptr;
  UWORD * base_addr;
  // thread control
  UBYTE ID;
  UWORD * hthread;
  UBYTE terminated;
  // waitable state
  UBYTE waitable;
} SCRIPT_CTX;

#define INSTRUCTION_SIZE 1

// maximum number of concurrent running VM threads
#define SCRIPT_MAX_CONTEXTS 8
// stack size of each VM thread
#define CONTEXT_STACK_SIZE 16
// number of shared variables
#define MAX_GLOBAL_VARS 32
// quant size
#define INSTRUCTIONS_PER_QUANT 0x10

// logical operators
#define VM_OP_EQ  1
#define VM_OP_LT  2
#define VM_OP_LE  3
#define VM_OP_GT  4
#define VM_OP_GE  5
#define VM_OP_NE  6
#define VM_OP_AND 7
#define VM_OP_OR  8

// shared context memory
extern UWORD script_memory[MAX_GLOBAL_VARS + (SCRIPT_MAX_CONTEXTS * CONTEXT_STACK_SIZE)];  // maximum stack depth is 16 words

// script core functions
void vm_push(SCRIPT_CTX * THIS, UWORD value) __banked;
UWORD vm_pop(SCRIPT_CTX * THIS, UBYTE n) __banked;
void vm_call_rel(SCRIPT_CTX * THIS, INT8 ofs) __banked;
void vm_call(SCRIPT_CTX * THIS, UBYTE * pc) __banked;
void vm_ret(SCRIPT_CTX * THIS, UBYTE n) __banked;
void vm_call_far(SCRIPT_CTX * THIS, UBYTE bank, UBYTE * pc) __banked;
void vm_ret_far(SCRIPT_CTX * THIS, UBYTE n) __banked;
void vm_loop_rel(SCRIPT_CTX * THIS, INT16 idx, INT8 ofs, UBYTE n) __banked;
void vm_loop(SCRIPT_CTX * THIS, INT16 idx, UINT8 * pc, UBYTE n) __banked;
void vm_jump_rel(SCRIPT_CTX * THIS, INT8 ofs) __banked;
void vm_jump(SCRIPT_CTX * THIS, UBYTE * pc) __banked;
void vm_systime(SCRIPT_CTX * THIS, INT16 idx) __banked;
void vm_invoke(SCRIPT_CTX * THIS, UBYTE bank, UBYTE * fn, UBYTE nparams, INT16 idx) __banked;
void vm_beginthread(UWORD dummy0, UWORD dummy1, SCRIPT_CTX * THIS, UBYTE bank, UBYTE * pc, INT16 idx, UBYTE nargs) __nonbanked;
void vm_if(SCRIPT_CTX * THIS, UBYTE condition, INT16 idxA, INT16 idxB, UBYTE * pc, UBYTE n) __banked;
void vm_if_const(SCRIPT_CTX * THIS, UBYTE condition, INT16 idxA, INT16 B, UBYTE * pc, UBYTE n) __banked;
void vm_debug(UWORD dummy0, UWORD dummy1, SCRIPT_CTX * THIS, UBYTE nargs) __nonbanked;
void vm_pushvalue(SCRIPT_CTX * THIS, INT16 idx) __banked;
void vm_reserve(SCRIPT_CTX * THIS, INT8 ofs) __banked;
void vm_set(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) __banked;
void vm_set_const(SCRIPT_CTX * THIS, INT16 idx, UWORD value) __banked;
void vm_rpn(UWORD dummy0, UWORD dummy1, SCRIPT_CTX * THIS) __nonbanked;
void vm_join(SCRIPT_CTX * THIS, INT16 idx) __banked;
void vm_terminate(SCRIPT_CTX * THIS, INT16 idx) __banked;
void vm_idle(SCRIPT_CTX * THIS) __banked;
void vm_get_tlocal(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) __banked;
void vm_get_uint8(SCRIPT_CTX * THIS, INT16 idxA, UINT8 * addr) __banked;
void vm_get_int8(SCRIPT_CTX * THIS, INT16 idxA, INT8 * addr) __banked;
void vm_get_int16(SCRIPT_CTX * THIS, INT16 idxA, INT16 * addr) __banked;

// return zero if script end
// bank with VM code must be active
UBYTE STEP_VM(SCRIPT_CTX * CTX) __naked __nonbanked __preserves_regs(b, c);

// initialize script runner contexts
void ScriptRunnerInit() __banked;
// execute a script in the new allocated context
SCRIPT_CTX * ExecuteScript(UBYTE bank, UBYTE * pc, UWORD * handle, INT8 nargs, ...) __banked;
// terminate script by ID; returns non zero if no such thread is running
UBYTE TerminateScript(UBYTE ID) __banked; 

#define RUNNER_DONE 0
#define RUNNER_IDLE 1
#define RUNNER_BUSY 2

// process all contexts
UBYTE ScriptRunnerUpdate() __nonbanked;

#endif