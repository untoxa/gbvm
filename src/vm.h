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
  const UBYTE ** stack_ptr;       // stack pointer
  const UBYTE * stack[16];        // maximum stack depth is 16 words
} SCRIPT_CTX;

#define INSTRUCTION_SIZE 1

// maximum number of concurrent running threads
#define SCRIPT_MAX_CONTEXTS 10

// script core functions
void push(SCRIPT_CTX * THIS, UWORD value) __banked;
const UBYTE * pop(SCRIPT_CTX * THIS, UBYTE n) __banked;
void call_rel(SCRIPT_CTX * THIS, INT8 ofs) __banked;
void call(SCRIPT_CTX * THIS, UBYTE * pc) __banked;
void ret(SCRIPT_CTX * THIS) __banked;
void call_far(SCRIPT_CTX * THIS, UBYTE bank, UBYTE * pc) __banked;
void ret_far(SCRIPT_CTX * THIS) __banked;
void loop_rel(SCRIPT_CTX * THIS, INT8 ofs) __banked;
void loop(SCRIPT_CTX * THIS, UINT8 * pc) __banked;
void jump_rel(SCRIPT_CTX * THIS, INT8 ofs) __banked;
void jump(SCRIPT_CTX * THIS, UBYTE * pc) __banked;
void systime(SCRIPT_CTX * THIS) __banked;
void invoke(SCRIPT_CTX * THIS, UBYTE bank, UBYTE * fn, UBYTE nparams) __banked;
void beginthread(SCRIPT_CTX * THIS, UBYTE bank, UBYTE * pc) __banked;
void nop() __banked;
void debug(SCRIPT_CTX * THIS, char * str) __banked;

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