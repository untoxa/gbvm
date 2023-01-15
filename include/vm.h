#ifndef _VM_H_INCLUDE
#define _VM_H_INCLUDE

#include <gbdk/platform.h>
#include <gbdk/far_ptr.h>

#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>

BANKREF_EXTERN(VM_MAIN)

#define FN_ARG0 -1
#define FN_ARG1 -2
#define FN_ARG2 -3
#define FN_ARG3 -4
#define FN_ARG4 -5
#define FN_ARG5 -6
#define FN_ARG6 -7
#define FN_ARG7 -8

#if defined(NINTENDO)
#define STEP_FUNC_ATTR
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
    uint8_t fn_bank;
    uint8_t args_len;
} SCRIPT_CMD;

#define FAR_CALL_EX(addr, seg, typ, ...) (__call_banked_addr=(addr),__call_banked_bank=(seg),((typ)(&__call__banked))(__VA_ARGS__))
typedef uint8_t (*SCRIPT_UPDATE_FN)(void * THIS, uint8_t start, uint16_t * stack_frame) OLDCALL BANKED;

#define VM_REF_TO_PTR(idx) (void *)(((idx) < 0) ? THIS->stack_ptr + (idx) : script_memory + (idx))

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
#define VM_OP_NOT 9

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

// return zero if script end
// bank with VM code must be active
uint8_t VM_STEP(SCRIPT_CTX * CTX) NONBANKED STEP_FUNC_ATTR;

// initialize script runner contexts
void script_runner_init(uint8_t reset) BANKED;
// execute a script in the new allocated context
SCRIPT_CTX * script_execute(uint8_t bank, uint8_t * pc, uint16_t * handle, uint8_t nargs, ...) BANKED;
// terminate script by ID; returns non zero if no such thread is running
uint8_t script_terminate(uint8_t ID) BANKED;
// detach script from the monitoring handle
uint8_t script_detach_hthread(uint8_t ID) BANKED;

#define RUNNER_DONE 0
#define RUNNER_IDLE 1
#define RUNNER_BUSY 2
#define RUNNER_EXCEPTION 3

#define EXCEPTION_CODE_NONE 0

// process all contexts
uint8_t script_runner_update() NONBANKED;

#endif