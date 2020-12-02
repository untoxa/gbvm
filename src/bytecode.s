.include "vm.inc"
        
.globl b_wait_frames, _wait_frames

.area _CODE_3

___bank_BYTECODE = 3
.globl ___bank_BYTECODE

_BYTECODE::
        VM_SET_CONST    0, 2            ; set global[0] to 2
        
        VM_RPN
        .db .TYP_B, 5, .TYP_B, 3, "-", .TYP_REF, 0,0, "+", .TYP_B, -2, "+", .RPN_STOP   ; push(5 - 3 + global[0] + -2)  result is 2
                                        ;        ^^^ this is INT16 offset, index for global[] array
        VM_SET          1, .ARG0        ; set global[1] to *(SP-1)

        VM_DEBUG        3               ; printf("0:%d 1:%d -1:%d\n", global[0], global[1], *(SP-1));
        .dw 0, 1, .ARG0
        .asciz "[0]=%d [1]=%d [-1]=%d"  ; debug string may be embedded into the code

        VM_IF .EQ       0, 1, 1$, 0     ; compare global[0] with global[1]; jump to 1$ if EQUAL; don't cleanup stack
        VM_DEBUG        0
         .asciz "!ERROR!"
        VM_STOP
1$:    
        VM_CALL_REL     2$
        VM_LOOP_REL     1$              ; test loop 

        VM_PUSH         0               ; placeholder for thread handle
        VM_BEGINTHREAD  ___bank_THREAD1, _THREAD1, .ARG0

        VM_DEBUG        1               ; print handle of created thread
        .dw .ARG0
        .asciz "hThread: %d"

        VM_PUSH         3               ; value A to compare
        VM_PUSH         3               ; value B to compare

        VM_PUSHVALUE    .ARG1           ; test pushvalue, value == 3 must be pushed
5$:    
        VM_LOOP_REL     5$

        VM_IF .EQ       .ARG0, .ARG1, 3$, 2     ; compare *(SP-1) with *(SP-2); jump to 3$ if EQUAL; cleanup 2 arguments from stack
        VM_DEBUG        0
        .asciz "Not equal"
        VM_JUMP_REL     4$
3$:     
        VM_DEBUG        0
        .asciz "Equal"
4$:
        VM_PUSH         50
        VM_CALL_FAR     ___bank_libfuncs, _LIB01

;        VM_TERMINATE    .ARG0           ; kill thread (rest of threadfunc won't execute) 

        VM_JOIN         .ARG0           ; wait for thread exit (or kill): high byte of thread handle becomes non-zero
        VM_POP          1               ; deallocate thread handle

        VM_DEBUG        0
        .asciz "Main terminated"
        VM_STOP
2$:
        VM_PUSH         ___bank_BYTECODE
        VM_DEBUG        1
        .dw .ARG0
        .asciz "Hello! bank: %d"
        VM_SET_CONST    .ARG0, 60       ; reuse value on the top of stack, set to 60
        VM_INVOKE       b_wait_frames, _wait_frames, 1  ; call wait_frames(), dispose 1 parameter on stack after
        VM_RET

___bank_THREAD1 = 3
_THREAD1::
        VM_DEBUG        0
        .asciz "Thread started"
        VM_PUSH         90              ; 60 frames == 1s
        VM_INVOKE       b_wait_frames, _wait_frames, 1  ; call wait_frames(), dispose 1 parameter on stack after
        VM_PUSH         200
        VM_CALL_FAR     ___bank_libfuncs, _LIB01
        VM_DEBUG        0
        .asciz "Thread terminated"
        VM_STOP

.area _CODE_1

___bank_libfuncs = 1
.globl ___bank_libfuncs

_LIB01::
        VM_PUSH         ___bank_libfuncs
        VM_DEBUG        2
        .dw .ARG3, .ARG0                ; stack contains (reverse order): local variable (-1), ret bank(-2), ret address(-3), ARG0(-4) 
        .asciz "LIB01(%d) bank: %d"
        VM_POP          1               ; dispose bank number on stack before far return
        VM_RET_FAR_N    1               ; return from far call and dispose 1 argument on stack
