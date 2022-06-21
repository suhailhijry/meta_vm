#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utils/assert.hpp>
#include <utils/memory.hpp>
#include <backward_cpp/backward.hpp>

using namespace backward;
using namespace achilles;

void print_trace(void) {
    StackTrace st;
    st.load_here(32);
    Printer p;
    p.print(st);
}

bool aassert_handler(const char *conditionCode, const char *report) {
    std::printf("assertion %s failed, %s \n", conditionCode, report);
    print_trace();
    return true;
}

void *default_allocator(u64 size, u64 alignment = sizeof(u64), void *previousMemory = nullptr, u64 previousSize = 0) {
    if (size == previousSize) return previousMemory;
    u64 aligned = memory::align(size, alignment);
    if (previousMemory == nullptr) {
        void *result = std::malloc(aligned);
        std::memset(result, 0, aligned);
        return result;
    }
    void *newMemory = std::realloc(previousMemory, aligned);
    if (size > previousSize) {
        std::memset(((u8 *) newMemory) + previousSize, 0, size - previousSize);    
    }
    return newMemory;
}

void default_deallocator(void *mem, u64 size) {
    (void)size; // shutting off the compiler warning
    return free(mem);
}

template<typename T>
using region = memory::region<T, default_allocator, default_deallocator, true>;

template<typename T>
using array = memory::array<T, default_allocator, default_deallocator, true>;

template<typename T, u64 size>
using static_region = memory::static_region<T, size>;

template<typename T, u64 size>
using static_array = memory::static_array<T, size>;

template<typename T>
using memory_view = memory::memory_view<T>;

enum VMOPCode {
    // core and reserved
    VMOPCODE_HLT = 0,
    VMOPCODE_NOP,
    
    // move
    VMOPCODE_MOV = 0x20,
    VMOPCODE_MOVF,
    
    // stack push
    VMOPCODE_PUSH,      VMOPCODE_POP,

    // arithmetics opcodes
    VMOPCODE_ADD,       VMOPCODE_SUB,    VMOPCODE_MUL,    VMOPCODE_DIV,

    // floating points arithmetics opcodes
    VMOPCODE_ADDF,       VMOPCODE_SUBF,    VMOPCODE_MULF,    VMOPCODE_DIVF,

    // bitwise operations
    VMOPCODE_AND,        VMOPCODE_OR,    VMOPCODE_XOR,    VMOPCODE_NOT,

    // comparison
    VMOPCODE_CMP,

    // jumps
    VMOPCODE_JMP,       VMOPCODE_JME,    VMOPCODE_JNE,    VMOPCODE_JLT,
    VMOPCODE_JGT,       VMOPCODE_JLE,    VMOPCODE_JGE,

    // procedure calling
    VMOPCODE_CALL,      VMOPCODE_RET,

    // flags
    VMOPCODE_CLR,
};

union VMWord {
    u8   ubytes[8];
    s8   sbytes[8];

    u16  uwords[4];
    s16  swords[4];

    u32 udwords[2];
    s32 sdwords[2];

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
};

enum VMOPType {
    VMOPTYPE_UNUSED = 0b0,
    VMOPTYPE_DATA_REGISTER = 0b1,
    VMOPTYPE_SDATA_REGISTER = 0b10,
    VMOPTYPE_FLOAT_REGISTER = 0b100,
    VMOPTYPE_POINTER = 0b1000,
    VMOPTYPE_IMMEDIATE_UNSIGNED = 0b10000,
    VMOPTYPE_IMMEDIATE_SIGNED = 0b100000,
    VMOPTYPE_IMMEDIATE_FLOAT = 0b1000000,
    VMOPTYPE_IMMEDIATE = VMOPTYPE_IMMEDIATE_UNSIGNED | VMOPTYPE_IMMEDIATE_SIGNED | VMOPTYPE_IMMEDIATE_FLOAT,
    // NOTE: cannot be used on float registers
    VMOPTYPE_INDIRECT = 0b10000000, 
    VMOPTYPE_DISPLACEMENT = 0b100000000,
};

struct VMOPCodeOperand {
    VMOPType          type;
    u8       registerIndex;
    VMWord           value;
};

struct VMInstruction {
    VMOPCode          opcode;
    VMOPCodeOperand operand1;
    VMOPCodeOperand operand2;
    VMOPCodeOperand operand3;
};

union VMRegisters {
    struct {
        u64 data[16];
        s64 flags[5];
        f64 floats[16];
    };

    struct {
        u64 rip, rsp, rbp, r03,
            r04, r05, r06, r07,
            r08, r09, r10, r11,
            r12, r13, r14, r15;

        s64 cf, sf, zf, of, mf;

        f64 f00, f01, f02, f03,
            f04, f05, f06, f07,
            f08, f09, f10, f11,
            f12, f13, f14, f15;
    };

    struct {
        s64 sdata[16];
    };

    VMRegisters() {
        std::memset(data, 0, 16);
        std::memset(flags, 0, 5);
        std::memset(floats, 0, 16);
    }
};

const char *getOPName(VMOPCode op) {
    #define opname(o) case o: return #o
    switch(op) {
        // core and reserved
        opname(VMOPCODE_HLT);
        opname(VMOPCODE_NOP);

        // move opcodes
        opname(VMOPCODE_MOV);      opname(VMOPCODE_MOVF);

        // push opcodes
        opname(VMOPCODE_PUSH);      opname(VMOPCODE_POP);

        // arithmetics opcodes
        opname(VMOPCODE_ADD);       opname(VMOPCODE_SUB);        opname(VMOPCODE_MUL);      opname(VMOPCODE_DIV);

        // floating points arithmetics opcodes
        opname(VMOPCODE_ADDF);     opname(VMOPCODE_SUBF);       opname(VMOPCODE_MULF);     opname(VMOPCODE_DIVF);

        // bitwise operations
        opname(VMOPCODE_AND);        opname(VMOPCODE_OR);        opname(VMOPCODE_XOR);      opname(VMOPCODE_NOT);

        // comparison
        opname(VMOPCODE_CMP);

        // jumps
        opname(VMOPCODE_JMP);       opname(VMOPCODE_JME);        opname(VMOPCODE_JNE);      opname(VMOPCODE_JLT);
        opname(VMOPCODE_JGT);       opname(VMOPCODE_JLE);        opname(VMOPCODE_JGE);

        // procedure calling
        opname(VMOPCODE_CALL);      opname(VMOPCODE_RET);

        // flags
        opname(VMOPCODE_CLR);
    }
    #undef opname

    return "";
}

enum VMState {
    VMSTATE_HALT,
    VMSTATE_EXECUTE,
};

enum VMException {
    VMEXCEPT_TRUNCATED_CODE,
    VMEXCEPT_INVALID_OPERANDS,
    VMEXCEPT_STACK_OVERFLOW,
    VMEXCEPT_INTEGER_OVERFLOW,
};

const char *getExceptionName(VMException exception) {
    #define exname(e) case e: return #e

    switch (exception) {
        exname(VMEXCEPT_TRUNCATED_CODE);
        exname(VMEXCEPT_INVALID_OPERANDS);
        exname(VMEXCEPT_STACK_OVERFLOW);
        exname(VMEXCEPT_INTEGER_OVERFLOW);
    }

    #undef exname

    return "";
}

template<
    typename TByteCode,
    typename TMemory,
    typename TExceptions 
>
struct MetaVM {
    MetaVM (
            TByteCode    &bytecode,
            TMemory        &memory,
            TExceptions &exceptions
    ) :      _bytecode(bytecode),
                 _memory(memory),
         _exceptions(exceptions) {}

    void run() {
        bool isRunning = true;
        while (isRunning) {
            VMInstruction inst = fetch();
            VMOPCode op = inst.opcode;
            switch (op) {
                case VMOPCODE_HLT:
                    isRunning = false;
                break;
                case VMOPCODE_NOP:
                    // do nothing
                break;
                case VMOPCODE_MOV:
                    mov(inst);
                break;
                case VMOPCODE_PUSH:
                    push(inst);
                break;
                case VMOPCODE_POP:
                    pop(inst);
                break;
                case VMOPCODE_ADD:
                    add(inst);
                break;
                case VMOPCODE_SUB:
                    sub(inst);
                break;
                case VMOPCODE_MUL:
                    mul(inst);
                break;
                case VMOPCODE_DIV:
                    div(inst);
                break;
                case VMOPCODE_AND:
                    vand(inst);
                break;
                case VMOPCODE_OR:
                    vor(inst);
                break;
                case VMOPCODE_XOR:
                    vxor(inst);
                break;
                case VMOPCODE_NOT:
                    vnot(inst);
                break;
                case VMOPCODE_CMP:
                    cmp(inst);
                break;
                case VMOPCODE_JMP:
                    jmp(inst);
                break;
                case VMOPCODE_JME:
                    jme(inst);
                break;
                case VMOPCODE_JNE:
                    jne(inst);
                break;
                case VMOPCODE_JLT:
                    jlt(inst);
                break;
                case VMOPCODE_JGT:
                    jgt(inst);
                break;
                case VMOPCODE_JLE:
                    jle(inst);
                break;
                case VMOPCODE_JGE:
                    jge(inst);
                break;
                case VMOPCODE_CALL:
                    call(inst);
                break;
                case VMOPCODE_RET:
                    ret(inst);
                break;
                case VMOPCODE_CLR:
                    clr(inst);
                break;
            }
        }
    }

    void printRegisters() {
        std::printf("REGISTERS:\n");
        std::printf("DATA:\n");
        for (u8 i = 0; i < 16; ++i) {
            if (i != 0 && i % 8 == 0) {
                std::printf("\n");
            }
            if (i < 10) {
                std::printf("data0%i: %#8llx ", i, _registers.data[i]);
            } else {
                std::printf("data%i: %#8llx ", i, _registers.data[i]);
            }
        }
        std::printf("\nFLOATS:\n");
        for (u8 i = 0; i < 16; ++i) {
            if (i != 0 && i % 8 == 0) {
                std::printf("\n");
            }
            if (i < 10) {
                std::printf("float0%i: %lf ", i, _registers.floats[i]);
            } else {
                std::printf("float%i: %lf ", i, _registers.floats[i]);
            }
        }
        std::printf("\n");
    }

    void printMemory() {
        std::printf("MEMORY:\n");
        for (u64 i = 0; i < _memory.length(); ++i) {
            if (i != 0 && i % 8 == 0) {
                std::printf("\n");
            }
            std::printf("%#8llx ", _memory[i].u);
        }
        std::printf("\n");
    }

    void printStack() {
        std::printf("STACK:\n");
        u64 size = _memory.size();
        u64 top = size - 1;
        for (u64 i = 0; i < _registers.rsp; ++i) {
            if (i != 0 && i % 8 == 0) {
                std::printf("\n");
            }
            std::printf("%#8llx ", _memory[top - i]);
        }
        std::printf("\n");
    }

    void printExceptions() {
        printf("EXCEPTIONS:\n");
        for (u64 i = 0; i < _exceptions.length(); ++i) {
            std::printf("%s\n", getExceptionName(_exceptions[i]));
        }
        std::printf("\n");
    }

    void printAll() {
        printRegisters();
        printMemory();
        printStack();
        printExceptions();
    }
private:
    VMRegisters       _registers;
    TByteCode         &_bytecode;
    TMemory             &_memory;
    TExceptions     &_exceptions;

    u64 getLocation(VMOPCodeOperand const &operand) const {
        return _registers.data[operand.registerIndex];
    }

    u64 getDisplaced(VMOPCodeOperand const &operand) const {
        return _registers.data[operand.registerIndex] + operand.value.u;
    }

    u64 &getDataRegister(VMOPCodeOperand const &operand) {
        return _registers.data[operand.registerIndex];
    }

    s64 &getSignedDataRegister(VMOPCodeOperand const &operand) {
        return _registers.sdata[operand.registerIndex];
    }

    f64 &getFloatRegister(VMOPCodeOperand const &operand) {
        return _registers.floats[operand.registerIndex];
    }

    void copyImmediateToMemory(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[dst.value.u].u = src.value.u;
    }

    void copyImmediateToIndirect(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[dst.value.u] = _memory[getLocation(src)];
    }

    void copyImmediateToDisplaced(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[dst.value.u] = _memory[getDisplaced(src)];
    }

    void copyMemoryToMemory(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[dst.value.u] = _memory[src.value.u];
    }

    void copyMemoryToIndirect(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[getLocation(dst)] = _memory[src.value.u];
    }

    void copyMemoryToDisplaced(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[getDisplaced(dst)] = _memory[src.value.u];
    }

    void copyIndirectToMemory(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[dst.value.u] = _memory[getLocation(src)];
    }

    void copyDisplacedToMemory(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[dst.value.u] = _memory[getDisplaced(src)];
    }

    void copyIndirectToIndirect(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[getLocation(src)] = _memory[getLocation(src)];
    }

    void copyIndirectToDisplaced(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[getDisplaced(src)] = _memory[getLocation(src)];
    }

    void copyDisplacedToIndirect(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[getLocation(dst)] = _memory[getDisplaced(src)];
    }

    void copyDisplacedToDisplaced(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[getDisplaced(dst)] = _memory[getDisplaced(src)];
    }

    void copyDataRegister(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _registers.data[dst.registerIndex] = _registers.data[src.registerIndex];
    }

    void copyImmediateToDataRegister(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _registers.data[dst.registerIndex] = src.value.u;
    }

    void copyMemoryToDataRegister(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _registers.data[dst.registerIndex] = _memory[src.value.u].u;
    }

    void copyIndirectToDataRegister(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _registers.data[dst.registerIndex] = _memory[getLocation(src)].u;
    }

    void copyDisplacedToDataRegister(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _registers.data[dst.registerIndex] = _memory[getDisplaced(src)].u;
    }
    
    void copyDataRegisterToMemory(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[dst.value.u].u = _registers.data[src.registerIndex];
    }

    void copyDataRegisterToIndirect(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[getLocation(dst)].u = _registers.data[src.registerIndex];
    }

    void copyDataRegisterToDisplaced(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[getDisplaced(dst)].u = _registers.data[src.registerIndex];
    }

    void copyFloatRegister(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _registers.floats[dst.registerIndex] = _registers.floats[src.registerIndex];
    }

    void copyImmediateToFloatRegister(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _registers.floats[dst.registerIndex] = src.value.f;
    }

    void copyMemoryToFloatRegister(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _registers.floats[dst.registerIndex] = _memory[src.value.u].f;
    }

    void copyIndirectToFloatRegister(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _registers.floats[dst.registerIndex] = _memory[getLocation(src)].f;
    }

    void copyDisplacedToFloatRegister(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _registers.floats[dst.registerIndex] = _memory[getDisplaced(src)].f;
    }
    
    void copyFloatRegisterToMemory(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[dst.value.u].f = _registers.floats[src.registerIndex];
    }

    void copyFloatRegisterToIndirect(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[getLocation(dst)].f = _registers.floats[src.registerIndex];
    }

    void copyFloatRegisterToDisplaced(VMOPCodeOperand &src, VMOPCodeOperand &dst) {
        _memory[getDisplaced(dst)].f = _registers.floats[src.registerIndex];
    }

    VMWord &getStackTop() {
        return _memory[(_memory.size() - 1) - _registers.rsp];
    }

    VMWord &getStackTopAndIncrement() {
        return _memory[(_memory.size() - 1) - _registers.rsp++];
    }

    VMWord &decrementAndGetStackTop() {
        return _memory[(_memory.size() - 1) - --_registers.rsp];
    }

    void pushImmediateToStack(VMOPCodeOperand &src) {
        getStackTopAndIncrement() = src.value;
    }

    void pushMemoryToStack(VMOPCodeOperand &src) {
        getStackTopAndIncrement() = _memory[src.value.u];
    }

    void pushDataRegisterToStack(VMOPCodeOperand &src) {
        getStackTopAndIncrement().u = _registers.data[src.registerIndex]; 
    }

    void pushFloatRegisterToStack(VMOPCodeOperand &src) {
        getStackTopAndIncrement().u = _registers.floats[src.registerIndex]; 
    }

    void pushIndirectToStack(VMOPCodeOperand &src) {
        getStackTopAndIncrement() = _memory[getLocation(src)];
    }

    void pushDisplacedToStack(VMOPCodeOperand &src) {
        getStackTopAndIncrement() = _memory[getDisplaced(src)];
    }

    void popStackToDataRegister(VMOPCodeOperand &dst) {
        _registers.data[dst.registerIndex] = decrementAndGetStackTop().u;
    }

    void popStackToFloatRegister(VMOPCodeOperand &dst) {
        _registers.floats[dst.registerIndex] = decrementAndGetStackTop().f;
    }

    void popStackToMemory(VMOPCodeOperand &dst) {
        _memory[dst.value.u] = decrementAndGetStackTop();
    }

    void popStackToIndirect(VMOPCodeOperand &dst) {
        _memory[getLocation(dst)] = decrementAndGetStackTop();
    }

    void popStackToDisplaced(VMOPCodeOperand &dst) {
        _memory[getDisplaced(dst)] = decrementAndGetStackTop();
    }

    void mov(VMInstruction &inst) {
        VMOPCodeOperand src = inst.operand1;
        VMOPCodeOperand dst = inst.operand2;
        switch (dst.type) {
            case VMOPTYPE_DATA_REGISTER:
                switch (src.type) {
                    case VMOPTYPE_DATA_REGISTER:
                        copyDataRegister(src, dst);
                    break;
                    case VMOPTYPE_IMMEDIATE:
                        copyImmediateToDataRegister(src, dst);
                    break;
                    case VMOPTYPE_POINTER:
                        copyMemoryToDataRegister(src, dst);
                    break;
                    case VMOPTYPE_INDIRECT:
                        copyIndirectToDataRegister(src, dst);
                    break;
                    case VMOPTYPE_DISPLACEMENT:
                        copyDisplacedToDataRegister(src, dst);
                    break;
                    default:
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    break;
                }
            break;
            case VMOPTYPE_FLOAT_REGISTER:
                switch (src.type) {
                    case VMOPTYPE_FLOAT_REGISTER:
                        copyFloatRegister(src, dst);
                    break;
                    case VMOPTYPE_IMMEDIATE:
                        copyImmediateToFloatRegister(src, dst);
                    break;
                    case VMOPTYPE_POINTER:
                        copyMemoryToFloatRegister(src, dst);
                    break;
                    case VMOPTYPE_INDIRECT:
                        copyIndirectToFloatRegister(src, dst);
                    break;
                    case VMOPTYPE_DISPLACEMENT:
                        copyDisplacedToFloatRegister(src, dst);
                    break;
                    default:
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    break;
                }
            break;
            case VMOPTYPE_POINTER:
                switch (src.type) {
                    case VMOPTYPE_DATA_REGISTER:
                        copyDataRegisterToMemory(src, dst);
                    break;
                    case VMOPTYPE_FLOAT_REGISTER:
                        copyFloatRegisterToMemory(src, dst);
                    break;
                    case VMOPTYPE_IMMEDIATE:
                        copyImmediateToMemory(src, dst);
                    break;
                    case VMOPTYPE_POINTER:
                        copyMemoryToMemory(src, dst);
                    break;
                    case VMOPTYPE_INDIRECT:
                        copyIndirectToMemory(src, dst);
                    break;
                    case VMOPTYPE_DISPLACEMENT:
                        copyDisplacedToMemory(src, dst);
                    break;
                    default:
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    break;
                }
            break;
            case VMOPTYPE_INDIRECT:
                switch (src.type) {
                    case VMOPTYPE_DATA_REGISTER:
                        copyDataRegisterToIndirect(src, dst);
                    break;
                    case VMOPTYPE_FLOAT_REGISTER:
                        copyFloatRegisterToIndirect(src, dst);
                    break;
                    case VMOPTYPE_IMMEDIATE:
                        copyImmediateToIndirect(src, dst);
                    break;
                    case VMOPTYPE_POINTER:
                        copyMemoryToIndirect(src, dst);
                    break;
                    case VMOPTYPE_INDIRECT:
                        copyIndirectToIndirect(src, dst);
                    break;
                    case VMOPTYPE_DISPLACEMENT:
                        copyDisplacedToIndirect(src, dst);
                    break;
                    default:
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    break;
                }
            break;
            case VMOPTYPE_DISPLACEMENT:
                switch (src.type) {
                    case VMOPTYPE_DATA_REGISTER:
                        copyDataRegisterToDisplaced(src, dst);
                    break;
                    case VMOPTYPE_FLOAT_REGISTER:
                        copyFloatRegisterToDisplaced(src, dst);
                    break;
                    case VMOPTYPE_IMMEDIATE:
                        copyImmediateToDisplaced(src, dst);
                    break;
                    case VMOPTYPE_POINTER:
                        copyMemoryToDisplaced(src, dst);
                    break;
                    case VMOPTYPE_INDIRECT:
                        copyIndirectToDisplaced(src, dst);
                    break;
                    case VMOPTYPE_DISPLACEMENT:
                        copyDisplacedToDisplaced(src, dst);
                    break;
                    default:
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    break;
                }
            break;
            default:
                _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
            break;
        }
    }

    void push(VMInstruction &inst) {
        VMOPCodeOperand src = inst.operand1;
        switch (src.type) {
            case VMOPTYPE_IMMEDIATE:
                pushImmediateToStack(src);
            break;
            case VMOPTYPE_DATA_REGISTER:
                pushDataRegisterToStack(src);
            break;
            case VMOPTYPE_FLOAT_REGISTER:
                pushFloatRegisterToStack(src);
            break;
            case VMOPTYPE_POINTER:
                pushMemoryToStack(src);
            break;
            case VMOPTYPE_INDIRECT:
                pushIndirectToStack(src);
            break;
            case VMOPTYPE_DISPLACEMENT:
                pushDisplacedToStack(src);
            break;
            default:
                _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
            break;
        }
    }

    void pop(VMInstruction &inst) {
        VMOPCodeOperand dst = inst.operand1;
        switch (dst.type) {
            case VMOPTYPE_DATA_REGISTER:
                popStackToDataRegister(dst);
            break;
            case VMOPTYPE_FLOAT_REGISTER:
                popStackToFloatRegister(dst);
            break;
            case VMOPTYPE_POINTER:
                popStackToMemory(dst);
            break;
            case VMOPTYPE_INDIRECT:
                popStackToIndirect(dst);
            break;
            case VMOPTYPE_DISPLACEMENT:
                popStackToDisplaced(dst);
            break;
            default:
                _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
            break;
        }
    }

    void add(VMInstruction &inst) {
        VMOPCodeOperand src = inst.operand1;
        VMOPCodeOperand dst = inst.operand2;
        switch (dst.type) {
            case VMOPTYPE_DATA_REGISTER:
                switch (src.type) {
                    case VMOPTYPE_IMMEDIATE_UNSIGNED:
                        getDataRegister(dst) += src.value.u;
                    break;
                    case VMOPTYPE_DATA_REGISTER:
                        getDataRegister(dst) += getDataRegister(src);
                    break;
                    case VMOPTYPE_POINTER:
                        getDataRegister(dst) += _memory[src.value.u].u;
                    break;
                    case VMOPTYPE_INDIRECT:
                        getDataRegister(dst) += _memory[getLocation(src)].u;
                    break;
                    case VMOPTYPE_DISPLACEMENT:
                        getDataRegister(dst) += _memory[getDisplaced(src)].u;
                    break;
                    default:
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    break;
                }
            break;
            case VMOPTYPE_SDATA_REGISTER:
                switch(src.type) {
                    case VMOPTYPE_IMMEDIATE_SIGNED:
                        getSignedDataRegister(dst) += src.value.s;
                    break;
                    case VMOPTYPE_SDATA_REGISTER:
                        getSignedDataRegister(dst) += getSignedDataRegister(src);
                    break;
                    case VMOPTYPE_POINTER:
                        getSignedDataRegister(dst) += _memory[src.value.u].s;
                    break;
                    case VMOPTYPE_INDIRECT:
                        getSignedDataRegister(dst) += _memory[getLocation(src)].s;
                    break;
                    case VMOPTYPE_DISPLACEMENT:
                        getSignedDataRegister(dst) += _memory[getDisplaced(src)].s;
                    break;
                    default:
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    break;
                }
            break;
            case VMOPTYPE_FLOAT_REGISTER:
                switch (src.type) {
                    case VMOPTYPE_IMMEDIATE_FLOAT:
                        getFloatRegister(dst) += src.value.f;
                    break;
                    case VMOPTYPE_FLOAT_REGISTER:
                        getFloatRegister(dst) += getFloatRegister(src);
                    break;
                    case VMOPTYPE_POINTER:
                        getFloatRegister(dst) += _memory[src.value.u].f;
                    break;
                    case VMOPTYPE_INDIRECT:
                        getFloatRegister(dst) += _memory[getLocation(src)].f;
                    break;
                    case VMOPTYPE_DISPLACEMENT:
                        getFloatRegister(dst) += _memory[getDisplaced(src)].f;
                    break;
                    default:
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    break;
                }
            break;
            case VMOPTYPE_POINTER:
                switch (src.type) {
                    // TODO: maybe make this support floats and signed integers as well
                    case VMOPTYPE_IMMEDIATE:
                        _memory[dst.value.u].u += src.value.u;
                    break;
                    case VMOPTYPE_DATA_REGISTER:
                        _memory[dst.value.u].u += getDataRegister(src);
                    break;
                    case VMOPTYPE_SDATA_REGISTER:
                        _memory[dst.value.u].s += getSignedDataRegister(src);
                    break;
                    case VMOPTYPE_FLOAT_REGISTER:
                        _memory[dst.value.u].f += getFloatRegister(src);
                    break;
                    case VMOPTYPE_POINTER:
                        _memory[dst.value.u].u += _memory[src.value.u].u;
                    break;
                    case VMOPTYPE_INDIRECT:
                        _memory[dst.value.u].u += _memory[getLocation(src)].u;
                    break;
                    case VMOPTYPE_DISPLACEMENT:
                        _memory[dst.value.u].u += _memory[getDisplaced(src)].u;
                    break;
                    default:
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    break;
                }
            break;
            case VMOPTYPE_INDIRECT:
                switch (src.type) {
                    // TODO: maybe make this support floats and signed integers as well
                    case VMOPTYPE_IMMEDIATE:
                        _memory[getLocation(dst)].u += src.value.u;
                    break;
                    case VMOPTYPE_DATA_REGISTER:
                        _memory[getLocation(dst)].u += getDataRegister(src);
                    break;
                    case VMOPTYPE_SDATA_REGISTER:
                        _memory[getLocation(dst)].s += getSignedDataRegister(src);
                    break;
                    case VMOPTYPE_FLOAT_REGISTER:
                        _memory[getLocation(dst)].f += getFloatRegister(src);
                    break;
                    case VMOPTYPE_POINTER:
                        _memory[getLocation(dst)].u += _memory[src.value.u].u;
                    break;
                    case VMOPTYPE_INDIRECT:
                        _memory[getLocation(dst)].u += _memory[getLocation(src)].u;
                    break;
                    case VMOPTYPE_DISPLACEMENT:
                        _memory[getLocation(dst)].u += _memory[getDisplaced(src)].u;
                    break;
                    default:
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    break;
                }
            break;
            case VMOPTYPE_DISPLACEMENT:
                switch (src.type) {
                    // TODO: maybe make this support floats and signed integers as well
                    case VMOPTYPE_IMMEDIATE:
                        _memory[getDisplaced(dst)].u += src.value.u;
                    break;
                    case VMOPTYPE_DATA_REGISTER:
                        _memory[getDisplaced(dst)].u += getDataRegister(src);
                    break;
                    case VMOPTYPE_SDATA_REGISTER:
                        _memory[getDisplaced(dst)].s += getSignedDataRegister(src);
                    break;
                    case VMOPTYPE_FLOAT_REGISTER:
                        _memory[getDisplaced(dst)].f += getFloatRegister(src);
                    break;
                    case VMOPTYPE_POINTER:
                        _memory[getDisplaced(dst)].u += _memory[src.value.u].u;
                    break;
                    case VMOPTYPE_INDIRECT:
                        _memory[getDisplaced(dst)].u += _memory[getLocation(src)].u;
                    break;
                    case VMOPTYPE_DISPLACEMENT:
                        _memory[getDisplaced(dst)].u += _memory[getDisplaced(src)].u;
                    break;
                    default:
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    break;
                }
            break;
        }
    }

    void sub(VMInstruction &inst) {
        // TODO:
    }

    void mul(VMInstruction &inst) {
        // TODO:
    }

    void div(VMInstruction &inst) {
        // TODO:
    }

    void vand(VMInstruction &inst) {
        // TODO:
    }

    void vor(VMInstruction &inst) {
        // TODO:
    }

    void vxor(VMInstruction &inst) {
        // TODO:
    }

    void vnot(VMInstruction &inst) {
        // TODO:
    }

    void cmp(VMInstruction &inst) {
        // TODO:
    }

    void jmp(VMInstruction &inst) {
        // TODO:
    }

    void jme(VMInstruction &inst) {
        // TODO:
    }

    void jne(VMInstruction &inst) {
        // TODO:
    }

    void jlt(VMInstruction &inst) {
        // TODO:
    }

    void jgt(VMInstruction &inst) {
        // TODO:
    }

    void jle(VMInstruction &inst) {
        // TODO:
    }

    void jge(VMInstruction &inst) {
        // TODO:
    }

    void call(VMInstruction &inst) {
        // TODO:
    }

    void ret(VMInstruction &inst) {
        // TODO:
    }

    void clr(VMInstruction &inst) {
        // TODO:
    }

    VMInstruction fetch() {
        static u8 state = 0;
        VMInstruction inst {}; 
        switch (state) {
            case 0:
                inst.opcode = VMOPCODE_MOV;
                inst.operand1 = VMOPCodeOperand {
                    VMOPTYPE_IMMEDIATE,
                    0,
                    32,
                };
                inst.operand2 = VMOPCodeOperand {
                    VMOPTYPE_DATA_REGISTER,
                    8,
                    0,
                };
                state++;
            break;
            case 1:
                inst.opcode = VMOPCODE_PUSH;
                inst.operand1 = VMOPCodeOperand {
                    VMOPTYPE_IMMEDIATE,
                    0,
                    32,
                };
                state++;
            break;
            case 2:
                inst.opcode = VMOPCODE_PUSH;
                inst.operand1 = VMOPCodeOperand {
                    VMOPTYPE_IMMEDIATE,
                    0,
                    64,
                };
                state++;
            break;
            case 3:
                inst.opcode = VMOPCODE_POP;
                inst.operand1 = VMOPCodeOperand {
                    VMOPTYPE_DATA_REGISTER,
                    15,
                };
                state++;
            break;
            case 4:
                inst.opcode = VMOPCODE_ADD;
                inst.operand1 = VMOPCodeOperand {
                    VMOPTYPE_IMMEDIATE_UNSIGNED,
                    0,
                    64,
                };
                inst.operand2 = VMOPCodeOperand {
                    VMOPTYPE_DATA_REGISTER,
                    15,
                };
                state++;
            break;
            default:
                inst.opcode = VMOPCODE_HLT;
            break;
        }

        _registers.rip++;
        return inst;
    }
};

int main() {
    using TCode = static_array<VMWord, 64>;
    using TMemory = static_array<VMWord, KB(8)>;
    using TExceptions = static_array<VMException, KB(1)>;

    using MetaVMStatic = MetaVM<TCode, TMemory, TExceptions>;
    TCode code {};
    TMemory memory {};
    TExceptions exceptions {};
    MetaVMStatic vm { code, memory, exceptions };

    vm.run();
    vm.printRegisters();
    vm.printStack();
    vm.printExceptions();
}

