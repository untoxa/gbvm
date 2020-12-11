.include "vm.i"
        
.globl b_wait_frames, _wait_frames

; for testing direct memory access
.globl _ACTORS

; these definitions should be put into some common include where game types are defined 
actor_t.x       = 0
actor_t.y       = 2
actor_t.ID      = 4
sizeof_actor_t  = 6

.area _CODE_3

___bank_BYTECODE = 3
.globl ___bank_BYTECODE

_BYTECODE::
        VM_GET_SYSTIME  2               ; sys_time(&global[2])
        VM_GET_INT16    0, ^/(_ACTORS + (sizeof_actor_t * 1) + actor_t.ID)/     ; global[0] = ACTORS[1].ID

        VM_RPN                          ; push(abs(5 - 3 + global[0] + -6))  result is 2
            .R_INT8     5
            .R_INT8     3
            .R_OPERATOR .SUB
            .R_REF      0
            .R_OPERATOR .ADD
            .R_INT16    -6
            .R_OPERATOR .ADD
            .R_OPERATOR .ABS
            .R_STOP

        VM_SET          1, .ARG0        ; global[1] = *(SP-1)

        VM_DEBUG        3               ; printf("0:%d 1:%d -1:%d\n", global[0], global[1], *(SP-1));
        .dw 0, 1, .ARG0
        .asciz "[0]=%d [1]=%d [-1]=%d"  ; debug string may be embedded into the code

        VM_IF .EQ       0, 1, 1$, 0     ; compare global[0] with global[1]; jump to 1$ if EQUAL; don't cleanup stack
        VM_DEBUG        0
         .asciz "!ERROR!"
        VM_STOP
1$:
        VM_CALL         2$              ; 2$ is too far for relative call, use near call
        VM_LOOP_REL     .ARG0, 1$, 1    ; loop to 1$; use *(SP-1) as counter; cleanup counter from stack after

        VM_PUSH         0               ; placeholder for thread handle
        VM_BEGINTHREAD  ___bank_THREAD1, _THREAD1, .ARG0, 1     ; start a thread, pass copy of .ARG0 as a parameter into thread locals
        .dw .ARG0

        VM_DEBUG        1               ; print handle of created thread
        .dw .ARG0
        .asciz "hThread: %d"

        VM_PUSH         3               ; value A to compare
        VM_PUSH         3               ; value B to compare

        VM_PUSHVALUE    .ARG1           ; test pushvalue, value == 3 must be pushed
5$:
        VM_IDLE                         ; execution may be delayed here
        VM_LOOP_REL     .ARG0, 5$, 1    ; loop to 5$; cleanup counter from stack after

        VM_IF .EQ       .ARG0, .ARG1, 3$, 2     ; if (*(SP-1) == *(SP-2)) goto 3$; also cleanup 2 arguments from stack
        VM_DEBUG        0
        .asciz "err: ARG0 != ARG1"
        VM_JUMP_REL     4$
3$:
        VM_DEBUG        0
        .asciz "ok: ARG0 == ARG1"
4$:
        VM_PUSH         50
        VM_CALL_FAR     ___bank_libfuncs, _LIB01

;        VM_TERMINATE    .ARG0           ; kill thread (rest of threadfunc won't execute) 

        VM_DEBUG        0
        .asciz "wait for join()"
        VM_JOIN         .ARG0           ; wait for thread exit (or kill): high byte of thread handle becomes non-zero
        VM_POP          1               ; dispose thread handle
        VM_DEBUG        0
        .asciz "thread joined"

        VM_PUSH         0               ; allocate tmp on stack (alias: .ARG0)
        VM_GET_SYSTIME  .ARG0           ; sys_time(&tmp)
        
        VM_RPN                          ; push(tmp - global[2])
            .R_REF      .ARG0
            .R_REF      3
            .R_OPERATOR .SUB
            .R_STOP

        VM_DEBUG        1               ; printf("Main terminated\nexec time: %d", tmp)
        .dw -1
        .asciz "Main terminated\nexec time: %d"

        VM_POP          2               ; dispose 2 values on stack: result of VM_RPN and tmp
        VM_STOP                         ; stop main
2$:
        VM_PUSH         ___bank_BYTECODE
        VM_DEBUG        1
        .dw .ARG0
        .asciz "wait(1s) bank: %d"
        VM_SET_CONST    .ARG0, 60       ; reuse value on the top of stack, set to 60
        VM_INVOKE       b_wait_frames, _wait_frames, 1, .ARG0  ; call wait_frames(), dispose 1 parameter on stack after
        VM_RET

___bank_THREAD1 = 3
_THREAD1::
        VM_PUSH         0               ; reserve place on VM stack for the value
        VM_GET_TLOCAL   .ARG0, 0        ; we have a thread local that is initialized by caller, get value of that local 
        VM_DEBUG        1               ; print that thread local variable
        .dw .ARG0
        .asciz "Trd started, ID:%d"
                                                ; switch(.ARG0)
        VM_IF_CONST .EQ .ARG0, 2, 1$, 0         ;     case 2: goto $1         
        VM_IF_CONST .EQ .ARG0, 4, 2$, 0         ;     case 4: goto $2
        VM_IF_CONST .EQ .ARG0, 7, 3$, 0         ;     case 7: goto $3
                                                ;     default: 
        VM_DEBUG        0
        .asciz "ID not in [2,4,7]"
        VM_JUMP_REL     4$
1$:     
        VM_DEBUG        0
        .asciz "case ID == 2"
        VM_JUMP_REL     4$
2$:
        VM_DEBUG        0
        .asciz "case ID == 4"
        VM_JUMP_REL     4$
3$:
        VM_RPN                          ; complex condition test (note: full boolean evaluation)
            .R_INT8     1
            .R_INT8     0
            .R_OPERATOR .AND
            .R_INT16    3
            .R_OPERATOR .OR
            .R_STOP
        VM_IF_CONST .NE .ARG0, 1, 4$, 1         ; if !((bool)1 && (bool)0 || (bool)3) goto 4$; (dispose RPN result)

        VM_DEBUG        0
        .asciz "case ID == 7"
4$:
        VM_POP          1               ; dispose value

        VM_DEBUG        0
        .asciz "wait(1.5s) thread"
        VM_PUSH         90              ; 60 frames == 1s
        VM_INVOKE       b_wait_frames, _wait_frames, 1, .ARG0  ; call wait_frames(); dispose 1 parameter on stack after
        VM_PUSH         75
        VM_CALL_FAR     ___bank_libfuncs, _LIB01
        VM_DEBUG        0
        .asciz "Trd terminated"
        VM_STOP                         ; stop thread

.area _CODE_1

___bank_libfuncs = 1
.globl ___bank_libfuncs

_LIB01::
        VM_PUSH         ___bank_libfuncs
        VM_DEBUG        2
        .dw .ARG3, .ARG0                ; stack contains (reverse order): local variable (-1), ret bank(-2), ret address(-3), PARAM0(-4) 
        .asciz "LIB01(%d) bank: %d"
        VM_POP          1               ; dispose bank number on stack before far return
        VM_RET_FAR_N    1               ; return from far call and dispose 1 argument on stack
