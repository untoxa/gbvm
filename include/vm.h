#ifndef _VM_H_INCLUDE
#define _VM_H_INCLUDE

#include <gbdk/platform.h>
#include <gbdk/far_ptr.h>

#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>

#if defined(NINTENDO)
#define STEP_FUNC_ATTR OLDCALL PRESERVES_REGS(b, c) 
typedef uint16_t DUMMY0_t;
typedef uint16_t DUMMY1_t;
#elif defined(SEGA)
#define STEP_FUNC_ATTR Z88DK_FASTCALL
typedef uint8_t DUMMY0_t;
typedef uint16_t DUMMY1_t;
#endif

typedef void * SCRIPT_CMD_FN;

typedef struct _SCRIPT_CMD {
  SCRIPT_CMD_FN fn;  
  uint8_t args_len;
} SCRIPT_CMD;

#define FAR_CALL_EX(addr, seg, typ, ...) (__call_banked_addr=(addr),__call_banked_bank=(seg),((typ)(&__call__banked))(__VA_ARGS__))
typedef uint8_t (*SCRIPT_UPDATE_FN)(void * THIS, uint8_t start, uint16_t * stack_frame) OLDCALL BANKED;

typedef struct SCRIPT_CTX {
    const uint8_t * PC;
    uint8_t bank;
    // linked list of contexts for cooperative multitasking
    struct SCRIPT_CTX * next;
    // update function
    void * update_fn;
    uint8_t update_fn_bank;
    // VM stack pointer
    uint16_t * stack_ptr;
    uint16_t * base_addr;
    // thread control
    uint8_t ID;
    uint16_t * hthread;
    uint8_t terminated;
    // waitable state
    uint8_t waitable;
    uint8_t lock_count;
    uint8_t flags;
} SCRIPT_CTX;

#define INSTRUCTION_SIZE 1

// maximum number of concurrent running VM threads
#define VM_MAX_CONTEXTS 16
// stack size of each VM thread
#define VM_CONTEXT_STACK_SIZE 64
// number of shared variables
#define VM_HEAP_SIZE 1024
// quant size
#define INSTRUCTIONS_PER_QUANT 0x10
// termination flag
#define SCRIPT_TERMINATED 0x8000

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
extern uint16_t script_memory[VM_HEAP_SIZE + (VM_MAX_CONTEXTS * VM_CONTEXT_STACK_SIZE)];  // maximum stack depth is 16 words

// contexts for executing scripts 
// ScriptRunnerInit(), ExecuteScript(), ScriptRunnerUpdate() manipulate these contexts
extern SCRIPT_CTX CTXS[VM_MAX_CONTEXTS];
extern SCRIPT_CTX * first_ctx, * free_ctxs;

// lock state 
extern uint8_t vm_lock_state;
// loaded state
extern uint8_t vm_loaded_state;
// exception flag and parameters
extern uint8_t vm_exception_code;
extern uint8_t vm_exception_params_length;
extern uint8_t vm_exception_params_bank;
extern const void * vm_exception_params_offset;

// script core functions
void vm_push(SCRIPT_CTX * THIS, uint16_t value) OLDCALL BANKED;
uint16_t vm_pop(SCRIPT_CTX * THIS, uint8_t n) OLDCALL BANKED;
void vm_call_rel(SCRIPT_CTX * THIS, int8_t ofs) OLDCALL BANKED;
void vm_call(SCRIPT_CTX * THIS, uint8_t * pc) OLDCALL BANKED;
void vm_ret(SCRIPT_CTX * THIS, uint8_t n) OLDCALL BANKED;
void vm_call_far(SCRIPT_CTX * THIS, uint8_t bank, uint8_t * pc) OLDCALL BANKED;
void vm_ret_far(SCRIPT_CTX * THIS, uint8_t n) OLDCALL BANKED;
void vm_loop_rel(SCRIPT_CTX * THIS, int16_t idx, int8_t ofs, uint8_t n) OLDCALL BANKED;
void vm_loop(SCRIPT_CTX * THIS, int16_t idx, uint8_t * pc, uint8_t n) OLDCALL BANKED;
void vm_jump_rel(SCRIPT_CTX * THIS, int8_t ofs) OLDCALL BANKED;
void vm_jump(SCRIPT_CTX * THIS, uint8_t * pc) OLDCALL BANKED;
void vm_systime(SCRIPT_CTX * THIS, int16_t idx) OLDCALL BANKED;
void vm_invoke(SCRIPT_CTX * THIS, uint8_t bank, uint8_t * fn, uint8_t nparams, int16_t idx) OLDCALL BANKED;
void vm_beginthread(DUMMY0_t dummy0, DUMMY1_t dummy1, SCRIPT_CTX * THIS, uint8_t bank, uint8_t * pc, int16_t idx, uint8_t nargs) OLDCALL NONBANKED;
void vm_if(SCRIPT_CTX * THIS, uint8_t condition, int16_t idxA, int16_t idxB, uint8_t * pc, uint8_t n) OLDCALL BANKED;
void vm_if_const(SCRIPT_CTX * THIS, uint8_t condition, int16_t idxA, int16_t B, uint8_t * pc, uint8_t n) OLDCALL BANKED;
void vm_debug(DUMMY0_t dummy0, DUMMY1_t dummy1, SCRIPT_CTX * THIS, uint8_t nargs) OLDCALL NONBANKED;
void vm_pushvalue(SCRIPT_CTX * THIS, int16_t idx) OLDCALL BANKED;
void vm_reserve(SCRIPT_CTX * THIS, int8_t ofs) OLDCALL BANKED;
void vm_set(SCRIPT_CTX * THIS, int16_t idxA, int16_t idxB) OLDCALL BANKED;
void vm_set_const(SCRIPT_CTX * THIS, int16_t idx, uint16_t value) OLDCALL BANKED;
void vm_rpn(DUMMY0_t dummy0, DUMMY1_t dummy1, SCRIPT_CTX * THIS) OLDCALL NONBANKED;
void vm_join(SCRIPT_CTX * THIS, int16_t idx) OLDCALL BANKED;
void vm_terminate(SCRIPT_CTX * THIS, int16_t idx) OLDCALL BANKED;
void vm_idle(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_get_tlocal(SCRIPT_CTX * THIS, int16_t idxA, int16_t idxB) OLDCALL BANKED;
void vm_get_uint8(SCRIPT_CTX * THIS, int16_t idxA, uint8_t * addr) OLDCALL BANKED;
void vm_get_int8(SCRIPT_CTX * THIS, int16_t idxA, int8_t * addr) OLDCALL BANKED;
void vm_get_int16(SCRIPT_CTX * THIS, int16_t idxA, int16_t * addr) OLDCALL BANKED;

// return zero if script end
// bank with VM code must be active
uint8_t VM_STEP(SCRIPT_CTX * CTX) NONBANKED STEP_FUNC_ATTR;

// initialize script runner contexts
void script_runner_init(uint8_t reset) BANKED;
// execute a script in the new allocated context
SCRIPT_CTX * script_execute(uint8_t bank, uint8_t * pc, uint16_t * handle, uint8_t nargs, ...) BANKED;
// terminate script by ID; returns non zero if no such thread is running
uint8_t script_terminate(uint8_t ID) BANKED; 

#define RUNNER_DONE 0
#define RUNNER_IDLE 1
#define RUNNER_BUSY 2
#define RUNNER_EXCEPTION 3

#define EXCEPTION_CODE_NONE 0

// process all contexts
uint8_t script_runner_update() NONBANKED;

#endif