; bytecode
; calling convention 
;      args: big-endian
;      order: left-to-right (leftmost argument pushed first)

OP_VM_STOP         = 0x00
.macro VM_STOP
        .db OP_VM_STOP 
.endm

OP_VM_PUSH         = 0x01
.macro VM_PUSH ARG0
        .db OP_VM_PUSH, #>ARG0, #<ARG0
.endm

OP_VM_POP          = 0x02
.macro VM_POP ARG0
        .db OP_VM_POP, #ARG0
.endm

OP_VM_CALL_REL     = 0x03
.macro VM_CALL_REL ARG0
        .db OP_VM_CALL_REL, #(ARG0 - . - 1)
.endm

OP_VM_CALL         = 0x04
.macro VM_CALL ARG0
        .db OP_VM_CALL, #>ARG0, #<ARG0
.endm

OP_VM_RET          = 0x05
.macro VM_RET
        .db OP_VM_RET 
.endm

OP_VM_LOOP_REL     = 0x06
.macro VM_LOOP_REL ARG0
        .db OP_VM_LOOP_REL, #(ARG0 - . - 1)
.endm

OP_VM_LOOP         = 0x07
.macro VM_LOOP ARG0
        .db OP_VM_LOOP, #>ARG0, #<ARG0
.endm

OP_VM_JUMP_REL     = 0x08
.macro VM_JUMP_REL ARG0
        .db OP_VM_JUMP_REL, #(ARG0 - . - 1)
.endm

OP_VM_JUMP         = 0x09
.macro VM_JUMP ARG0
        .db OP_VM_JUMP, #>ARG0, #<ARG0
.endm

OP_VM_CALL_FAR     = 0x0A
.macro VM_CALL_FAR ARG0, ARG1
        .db OP_VM_CALL_FAR, #>ARG1, #<ARG1, #<ARG0
.endm

OP_VM_RET_FAR      = 0x0B
.macro VM_RET_FAR
        .db OP_VM_RET_FAR 
.endm

OP_VM_GET_SYSTIME  = 0x0C
.macro VM_GET_SYSTIME
        .db OP_VM_GET_SYSTIME
.endm

OP_VM_INVOKE       = 0x0D
.macro VM_INVOKE ARG0, ARG1, ARG2
        .db OP_VM_INVOKE, #<ARG2, #>ARG1, #<ARG1, #<ARG0
.endm

OP_VM_BEGINTHREAD  = 0x0E
.macro VM_BEGINTHREAD ARG0, ARG1
        .db OP_VM_BEGINTHREAD, #>ARG1, #<ARG1, #<ARG0
.endm

OP_VM_IF           = 0x0F
EQUAL              = 0
LESS               = 1
GREATER            = 2
LE_EQ              = 3
GR_EQ              = 4
.macro VM_IF CONDITION, LABEL
        .db OP_VM_IF, #>LABEL, #<LABEL, #<CONDITION
.endm

OP_VM_DEBUG        = 0x10
.macro VM_DEBUG ARG0
        .db OP_VM_DEBUG, #>ARG0, #<ARG0
.endm
