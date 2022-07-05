#if !defined(METAVM_TYPES_HPP)
#define METAVM_TYPES_HPP

enum VMOperandType {
    VMOPTYPE_REGISTER,
    VMOPTYPE_IMMEDIATE,
    VMOPTYPE_POINTER,

    // NOTE: cannot be used on float registers
    VMOPTYPE_INDIRECT, 
    VMOPTYPE_DISPLACEMENT,
};

enum VMOperandSize : u8 {
    VMOPSIZE_BYTE  = sizeof(u8),
    VMOPSIZE_WORD  = sizeof(u16),
    VMOPSIZE_DWORD = sizeof(u32),
    VMOPSIZE_QWORD = sizeof(u64),
};

union VMWord {
    u8   ubytes[8];
    s8   sbytes[8];

    u16  uwords[4];
    s16  swords[4];

    u32 udwords[2];
    s32 sdwords[2];

    f32 fsingles[2];

    u64          u;
    s64          s;
    f64          f;

    operator u64() const {
        return u;
    }

    operator s64() const {
        return s;
    }

    operator f64() const {
        return f;
    }

    VMWord() {
        u = 0;
    }

    VMWord(u64 value) {
        u = value;
    }

    VMWord(u32 value) {
        u = value;
    }

    VMWord(u16 value) {
        u = value;
    }

    VMWord(u8 value) {
        u = value;
    }

    VMWord(s64 value) {
        s = value;
    }

    VMWord(s32 value) {
        s = value;
    }
    
    VMWord(s16 value) {
        s = value;
    }
    
    VMWord(s8 value) {
        s = value;
    }
    
    VMWord(f64 value) {
        f = value;
    }

    VMWord(f32 value) {
        fsingles[0] = value;
    }
};

struct VMOperand {
    VMOperandType          type;
    VMOperandSize          size;
    u8            registerIndex;
    VMWord                value;
};

enum VMOPCode {
    // core and reserved
    VMOPCODE_HLT = 0,
    VMOPCODE_NOP,
    
    // move
    VMOPCODE_MOV = 0x20,
    
    // stack
    VMOPCODE_PUSH,      VMOPCODE_POP,

    // arithmetics
    VMOPCODE_ADD,       VMOPCODE_SUB,      VMOPCODE_MUL,      VMOPCODE_DIV,    VMOPCODE_DIVR,
    VMOPCODE_ADDS,      VMOPCODE_SUBS,     VMOPCODE_MULS,     VMOPCODE_DIVS,   VMOPCODE_DIVSR,
    VMOPCODE_ADDF,      VMOPCODE_SUBF,     VMOPCODE_MULF,     VMOPCODE_DIVF,
    VMOPCODE_ADDFS,     VMOPCODE_SUBFS,    VMOPCODE_MULFS,    VMOPCODE_DIVFS,
    VMOPCODE_NEG,

    // bits 
    VMOPCODE_AND,       VMOPCODE_OR,       VMOPCODE_XOR,      VMOPCODE_NOT,

    // other
    VMOPCODE_JMP,       VMOPCODE_JEQ,      VMOPCODE_JNE,      VMOPCODE_JGT,      VMOPCODE_JLT,    VMOPCODE_JGE,    VMOPCODE_JLE,
};

struct VMInstruction {
    VMOPCode    opcode;
    VMOperand operand1;
    VMOperand operand2;
    VMOperand operand3;
};

// 32 registers is more than enough for most operations
constexpr u64 REGISTER_COUNT = 32;

enum VMOPFlags {
    VMOPFLAGS_CARRY      = 0x01,
    VMOPFLAGS_ZERO       = 0x02,
    VMOPFLAGS_SIGN       = 0x04,
    VMOPFLAGS_OVERFLOW   = 0x08,
    VMOPFLAGS_UNDERFLOW  = 0x10,
};

struct VMRegisters {
    u64 instructionPointer;
    u64 stackPointer;       
    u64 framePointer;       
    u64 linkPointer;       
    u8  flags;

    // general purpose registers
    // can also be used as floating point registers
    VMWord data[REGISTER_COUNT] {};
};

enum VMException {
    VMEXCEPT_INVALID_OPERANDS,
    VMEXCEPT_STACK_OVERFLOW,
    VMEXCEPT_STACK_UNDERFLOW,
    VMEXCEPT_INTEGER_OVERFLOW,
    VMEXCEPT_FLOAT_OVERFLOW,
};

inline const char *getExceptionName(VMException exception) {
    #define exname(e) case e: return #e

    switch (exception) {
        exname(VMEXCEPT_INVALID_OPERANDS);
        exname(VMEXCEPT_STACK_OVERFLOW);
        exname(VMEXCEPT_STACK_UNDERFLOW);
        exname(VMEXCEPT_INTEGER_OVERFLOW);
        exname(VMEXCEPT_FLOAT_OVERFLOW);
    }

    #undef exname

    return "";
}

#endif

