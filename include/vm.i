; bytecode
; calling convention
;      args: big-endian
;      order: left-to-right (leftmost argument pushed first)

; exception ID's
EXCEPTION_RESET	= 1

; aliases
.ARG0 = -1
.ARG1 = -2
.ARG2 = -3
.ARG3 = -4
.ARG4 = -5
.ARG5 = -6
.ARG6 = -7
.ARG7 = -8
.ARG8 = -9
.ARG9 = -10
.ARG10 = -11
.ARG11 = -12
.ARG12 = -13
.ARG13 = -14
.ARG14 = -15
.ARG15 = -16
.ARG16 = -17

.PARAM0 = .ARG2
.PARAM1 = .ARG3
.PARAM2 = .ARG4
.PARAM3 = .ARG5
.PARAM4 = .ARG6
.PARAM5 = .ARG7
.PARAM6 = .ARG8
.PARAM7 = .ARG9
.PARAM8 = .ARG10
.PARAM9 = .ARG11
.PARAM10 = .ARG12
.PARAM11 = .ARG13
.PARAM12 = .ARG14
.PARAM13 = .ARG15
.PARAM14 = .ARG16
.PARAM15 = -18
.PARAM16 = -19

; stops execution of context
OP_VM_STOP         = 0x00
.macro VM_STOP
        .db OP_VM_STOP
.endm

; push immediate value onto VM stack
OP_VM_PUSH_CONST   = 0x01
.macro VM_PUSH_CONST ARG0
        .db OP_VM_PUSH_CONST, #>ARG0, #<ARG0
.endm

; removes ARG0 values from VM stack
OP_VM_POP          = 0x02
.macro VM_POP ARG0
        .db OP_VM_POP, #ARG0
.endm

; call by near address
OP_VM_CALL         = 0x04
.macro VM_CALL ARG0
        .db OP_VM_CALL, #>ARG0, #<ARG0
.endm

; return from near call
OP_VM_RET          = 0x05
.macro VM_RET
        .db OP_VM_RET, 0
.endm

; return from near call and clear n arguments on stack
.macro VM_RET_N ARG0
        .db OP_VM_RET, #<ARG0
.endm

OP_VM_GET_FAR      = 0x06
.GET_BYTE          = 0
.GET_WORD          = 1
;-- Get byte or word by the far pointer into variable
; @param IDX Target variable
; @param SIZE Size of the ojject to be acquired:
;   `.GET_BYTE`  - get 8-bit value
;   `.GET_WORD`  - get 16-bit value
; @param BANK Bank number of the object
; @param ADDR Address of the object
.macro VM_GET_FAR IDX, SIZE, BANK, ADDR
        .db OP_VM_GET_FAR, #>ADDR, #<ADDR, #<BANK, #<SIZE, #>IDX, #<IDX
.endm

; loop by near address, counter is on stack, counter is removed on exit
OP_VM_LOOP         = 0x07
.macro VM_LOOP IDX, LABEL, NPOP
        .db OP_VM_LOOP, #<NPOP, #>LABEL, #<LABEL, #>IDX, #<IDX
.endm

OP_VM_SWITCH       = 0x08
.macro .CASE VAL, LABEL
        .dw #VAL, #LABEL
.endm
;-- Compares variable with a set of values, and if equal jump to the specified label.
; values for testing may be defined with the .CASE macro, where VAL parameter is a value for testing and LABEL is a jump label
; @param IDX variable for compare
; @param SIZE amount of entries for test.
; @param N amount of values to de cleaned from stack on exit
.macro VM_SWITCH IDX, SIZE, N
        .db OP_VM_SWITCH, #<N, #<SIZE, #>IDX, #<IDX
.endm

; loop by near address
OP_VM_JUMP         = 0x09
.macro VM_JUMP ARG0
        .db OP_VM_JUMP, #>ARG0, #<ARG0
.endm

; call far (inter-bank call)
OP_VM_CALL_FAR     = 0x0A
.macro VM_CALL_FAR ARG0, ARG1
        .db OP_VM_CALL_FAR, #>ARG1, #<ARG1, #<ARG0
.endm

; rerurn from far call and clear n arguments on stack
OP_VM_RET_FAR      = 0x0B
.macro VM_RET_FAR
        .db OP_VM_RET_FAR, 0
.endm

; rerurn from far call and clear n arguments on stack
.macro VM_RET_FAR_N ARG0
        .db OP_VM_RET_FAR, #<ARG0
.endm

; returns game boy system time on VM stack
OP_VM_GET_SYSTIME  = 0x0C
.macro VM_GET_SYSTIME IDX
        .db OP_VM_GET_SYSTIME, #>IDX, #<IDX
.endm

; invokes <bank>:<address> C function until it returns true
OP_VM_INVOKE       = 0x0D
.macro VM_INVOKE ARG0, ARG1, ARG2, ARG3
        .db OP_VM_INVOKE, #>ARG3, #<ARG3, #<ARG2, #>ARG1, #<ARG1, #<ARG0
.endm

; spawns a thread in a separate context
OP_VM_BEGINTHREAD  = 0x0E
.macro VM_BEGINTHREAD BANK, THREADPROC, HTHREAD, NARGS
        .db OP_VM_BEGINTHREAD, #<NARGS, #>HTHREAD, #<HTHREAD, #>THREADPROC, #<THREADPROC, #<BANK
.endm

; condition
OP_VM_IF           = 0x0F
.EQ                = 1
.LT                = 2
.LTE               = 3
.GT                = 4
.GTE               = 5
.NE                = 6
.AND               = 7
.OR                = 8
.NOT               = 9
.macro VM_IF CONDITION, IDXA, IDXB, LABEL, N
        .db OP_VM_IF, #<N, #>LABEL, #<LABEL, #>IDXB, #<IDXB, #>IDXA, #<IDXA, #<CONDITION
.endm

OP_VM_PUSH_VALUE_IND = 0x10
;-- Pushes a value on VM stack or a global indirectly from an index in the variable
; @param IDX variable that contains the index of the variable to be pushed on stack
.macro VM_PUSH_VALUE_IND IDX
        .db OP_VM_PUSH_VALUE_IND, #>IDX, #<IDX
.endm

; pushes a value on VM stack or a global onto VM stack
OP_VM_PUSHVALUE    = 0x11
.macro VM_PUSHVALUE ARG0
        .db OP_VM_PUSHVALUE, #>ARG0, #<ARG0
.endm

; similar to pop
OP_VM_RESERVE    = 0x12
.macro VM_RESERVE ARG0
        .db OP_VM_RESERVE, #<ARG0
.endm

; assignes a value on VM stack or a global to a value on VM stack ar a global
OP_VM_SET        = 0x13
.macro VM_SET IDXA, IDXB
        .db OP_VM_SET, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

; assignes a value on VM stack or a global to immediate
OP_VM_SET_CONST  = 0x14
.macro VM_SET_CONST IDX, VAL
        .db OP_VM_SET_CONST, #>VAL, #<VAL, #>IDX, #<IDX
.endm

; rpn calculator, returns result on VM stack
OP_VM_RPN        = 0x15
.ADD               = '+'
.SUB               = '-'
.MUL               = '*'
.DIV               = '/'
.MOD               = '%'
.B_AND             = '&'
.B_OR              = '|'
.B_XOR             = '^'
.B_NOT             = '~'
.ABS               = '@'
.MIN               = 'm'
.MAX               = 'M'
;.EQ                = 1
;.LT                = 2
;.LTE               = 3
;.GT                = 4
;.GTE               = 5
;.NE                = 6
;.AND               = 7
;.OR                = 8
;.NOT               = 9

.macro VM_RPN
        .db OP_VM_RPN
.endm
.macro .R_INT8 ARG0
        .db -1, #<ARG0
.endm
.macro .R_INT16 ARG0
        .db -2
        .dw #ARG0
.endm
.macro .R_REF ARG0
        .db -3
        .dw #ARG0
.endm
.macro .R_OPERATOR ARG0
        .db ARG0
.endm
.macro .R_STOP
        .db 0
.endm

; joins a thread
OP_VM_JOIN       = 0x16
.macro VM_JOIN IDX
        .db OP_VM_JOIN, #>IDX, #<IDX
.endm

; kills a thread
OP_VM_TERMINATE  = 0x17
.macro VM_TERMINATE IDX
        .db OP_VM_TERMINATE, #>IDX, #<IDX
.endm

; signals runner that context in a waitable state
OP_VM_IDLE      = 0x18
.macro VM_IDLE
        .db OP_VM_IDLE
.endm

; gets thread local variable. non-negative index of second argument points to
; a thread local variable (parameters, passed into thread)
OP_VM_GET_TLOCAL= 0x19
.macro VM_GET_TLOCAL IDXA, IDXB
        .db OP_VM_GET_TLOCAL, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

; compares variable or value on stack with a constant
OP_VM_IF_CONST  = 0x1A
.macro VM_IF_CONST CONDITION, IDXA, B, LABEL, N
        .db OP_VM_IF_CONST, #<N, #>LABEL, #<LABEL, #>B, #<B, #>IDXA, #<IDXA, #<CONDITION
.endm

; gets unsigned int8 from VM RAM. second argument is a VM RAM address of unsigned int8
OP_VM_GET_UINT8 = 0x1B
.macro VM_GET_UINT8 IDXA, ADDR
        .db OP_VM_GET_UINT8, #>ADDR, #<ADDR, #>IDXA, #<IDXA
.endm

; gets int8 from VM RAM. second argument is a VM RAM address of int8
OP_VM_GET_INT8  = 0x1C
.macro VM_GET_INT8 IDXA, ADDR
        .db OP_VM_GET_INT8, #>ADDR, #<ADDR, #>IDXA, #<IDXA
.endm

; gets int8 from VM RAM. second argument is a VM RAM address of int8
OP_VM_GET_INT16  = 0x1D
.macro VM_GET_INT16 IDXA, ADDR
        .db OP_VM_GET_INT16, #>ADDR, #<ADDR, #>IDXA, #<IDXA
.endm

OP_VM_SET_UINT8 = 0x1E
;-- Sets unsigned int8 in WRAM from variable
; @param ADDR Address of the unsigned 8-bit value in WRAM
; @param IDXA Source variable
.macro VM_SET_UINT8 ADDR, IDXA
        .db OP_VM_SET_UINT8, #>IDXA, #<IDXA, #>ADDR, #<ADDR
.endm

OP_VM_SET_INT8  = 0x1F
;-- Sets signed int8 in WRAM from variable
; @param ADDR Address of the signed 8-bit value in WRAM
; @param IDXA Source variable
.macro VM_SET_INT8 ADDR, IDXA
        .db OP_VM_SET_INT8, #>IDXA, #<IDXA, #>ADDR, #<ADDR
.endm

OP_VM_SET_INT16  = 0x20
;-- Sets signed int16 in WRAM from variable
; @param ADDR Address of the signed 16-bit value in WRAM
; @param IDXA Source variable
.macro VM_SET_INT16 ADDR, IDXA
        .db OP_VM_SET_INT16, #>IDXA, #<IDXA, #>ADDR, #<ADDR
.endm

OP_VM_SET_CONST_INT8 = 0x21
;-- Sets signed int8 in WRAM to the immediate value
; @param ADDR Address of the signed 8-bit value in WRAM
; @param V Immediate value
.macro VM_SET_CONST_INT8 ADDR, V
        .db OP_VM_SET_CONST_INT8, #<V, #>ADDR, #<ADDR
.endm

;-- Sets unsigned int8 in WRAM to the immediate value
; @param ADDR Address of the unsigned 8-bit value in WRAM
; @param V Immediate value
.macro VM_SET_CONST_UINT8 ADDR, V
        .db OP_VM_SET_CONST_INT8, #<V, #>ADDR, #<ADDR
.endm

OP_VM_SET_CONST_INT16 = 0x22
;-- Sets signed int16 in WRAM to the immediate value
; @param ADDR Address of the signed 16-bit value in WRAM
; @param V Immediate value
.macro VM_SET_CONST_INT16 ADDR, V
        .db OP_VM_SET_CONST_INT16, #>V, #<V, #>ADDR, #<ADDR
.endm

OP_VM_LOCK            = 0x25
;-- Disable switching of VM threads
.macro VM_LOCK
        .db OP_VM_LOCK
.endm

OP_VM_UNLOCK          = 0x26
;-- Enable switching of VM threads
.macro VM_UNLOCK
        .db OP_VM_UNLOCK
.endm

;-- Raises an exception
; @param CODE Exception code:
;   `EXCEPTION_RESET`        - Resets the device.
;   `EXCEPTION_CHANGE_SCENE` - Changes to a new scene.
;   `EXCEPTION_SAVE`         - Saves the state of the game.
;   `EXCEPTION_LOAD`         - Loads the saved state of the game.
; @param SIZE Length of the parameters to be passed into the exception handler
OP_VM_RAISE           = 0x27
.macro VM_RAISE CODE, SIZE
        .db OP_VM_RAISE, #<SIZE, #<CODE
.endm

OP_VM_SET_INDIRECT    = 0x28
;-- Assigns variable that is addressed indirectly to the other variable
; @param IDXA Variable that contains the index of the target variable
; @param IDXB Source variable that contains the value to be assigned
.macro VM_SET_INDIRECT IDXA, IDXB
        .db OP_VM_SET_INDIRECT, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

OP_VM_GET_INDIRECT    = 0x29
;-- Assigns a variable to the value of variable that is addressed indirectly
; @param IDXA Target variable
; @param IDXB Variable that contains the index of the source variable
.macro VM_GET_INDIRECT IDXA, IDXB
        .db OP_VM_GET_INDIRECT, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

OP_VM_PUSH_REFERENCE  = 0x2C
;-- Translates IDX into absolute index and pushes result to VM stack
; @param IDX index of the variable
.macro VM_PUSH_REFERENCE IDX
        .db OP_VM_PUSH_REFERENCE, #>IDX, #<IDX
.endm

OP_VM_CALL_NATIVE     = 0x2D
;-- Calls native code by the far pointer
; @param BANK Bank number of the native routine
; @param PTR Address of the native routine
.macro VM_CALL_NATIVE BANK, PTR
        .db OP_VM_CALL_NATIVE, #>PTR, #<PTR, #<BANK
.endm

; printf()
OP_VM_DEBUG           = 0x2E
.macro VM_DEBUG ARG0
        .db OP_VM_DEBUG, #<ARG0
.endm
