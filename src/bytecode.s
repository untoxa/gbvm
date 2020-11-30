.include "vm.inc"
        
.globl b_wait_frames, _wait_frames

.area _CODE_3

___bank_BYTECODE = 3
.globl ___bank_BYTECODE

_BYTECODE::
        VM_RESERVE      2               ; reserve 2 words for globals        
        VM_PUSH         2               ; push do..while loop count
1$:    
        VM_CALL_REL     2$
        VM_LOOP_REL     1$              ; test loop 

        VM_BEGINTHREAD  ___bank_THREAD1, _THREAD1

        VM_PUSH         3
        VM_PUSH         3 

        VM_PUSHVALUE    -2              ; test pushvalue, value == 3 must be pushed
5$:    
        VM_CALL_REL     2$
        VM_LOOP_REL     5$

        VM_IF EQUAL     -1, -2, 3$, 2   ; compare (SP-1) with (SP-2); jump to 3$ if EQUAL; cleanup 2 arguments from stack
        VM_DEBUG        s_not_equal
        VM_JUMP_REL     4$
3$:     
        VM_DEBUG        s_equal
4$:
        VM_CALL_FAR     ___bank_libfuncs, _LIBFUNC01
        VM_DEBUG        s_main_trmt
        VM_STOP
2$:
        VM_DEBUG        s_dummy
        VM_PUSH         60              ; 90 frames == 1,5s
        VM_INVOKE       b_wait_frames, _wait_frames, 1
        VM_RET

___bank_THREAD1 = 3
_THREAD1::
        VM_DEBUG        s_thread_strt
        VM_PUSH         90              ; 60 frames == 1s
        VM_INVOKE       b_wait_frames, _wait_frames, 1
        VM_CALL_FAR     ___bank_libfuncs, _LIBFUNC01
        VM_DEBUG        s_thread_trmt
        VM_STOP

.area _CODE_1

___bank_libfuncs = 1
.globl ___bank_libfuncs

_LIBFUNC01::
        VM_DEBUG        s_libfunc
        VM_RET_FAR

.area _CODE

s_dummy:        .asciz "Hello, world!"
s_main_trmt:    .asciz "Main terminated"

s_libfunc:      .asciz "LIBFUNC01()"

s_thread_strt:  .asciz "Thread started"
s_thread_trmt:  .asciz "Thread terminated"

s_equal:        .asciz "Equal"
s_not_equal:    .asciz "Not equal"