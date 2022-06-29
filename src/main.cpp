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

union VMWord {
    u8   ubytes[8];
    s8   sbytes[8];

    u16  uwords[4];
    s16  swords[4];

    u32 udwords[2];
    s32 sdwords[2];

    f32 fsingle[2];

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
        fsingle[0] = value;
    }
};

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
    
    // stack push
    VMOPCODE_PUSH,      VMOPCODE_POP,
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

const char *getExceptionName(VMException exception) {
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

template<
    typename TExceptions 
>
struct MetaVM {
    MetaVM (
            memory_view<VMInstruction> &bytecode,
            memory_view<u8> &memory,
            TExceptions &exceptions
    ) :      _bytecode(bytecode),
                 _memory(memory),
         _exceptions(exceptions)
    {
        _registers.stackPointer = _memory.size();
    }

    void run() {
        bool isRunning = true;
        while (isRunning && _exceptions.length() == 0) {
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
            }
        }
    }

    void printRegisters() {
        std::printf("REGISTERS:\n");
        std::printf("IP: %#018llx ", _registers.instructionPointer);
        std::printf("SP: %#018llx ", _registers.stackPointer);
        std::printf("FP: %#018llx ", _registers.framePointer);
        std::printf("LP: %#018llx\n", _registers.linkPointer);
        std::printf("FLAGS: %#02x\n", _registers.flags);
        std::printf("DATA:\n");
        for (u8 i = 0; i < REGISTER_COUNT; ++i) {
            if (i != 0 && i % 8 == 0) {
                std::printf("\n");
            }
            if (i < 10) {
                std::printf("data0%i: %#8llx ", i, _registers.data[i].u);
            } else {
                std::printf("data%i: %#8llx ", i, _registers.data[i].u);
            }
        }
        std::printf("\n");
    }

    void printMemory() {
        std::printf("MEMORY:\n");
        u64 zeroesCount = 0;
        for (u64 i = 0; i < _memory.length(); ++i) {
            if (i != 0 && i % 8 == 0) {
                std::printf("\n");
            }

            if (_memory[i] == 0) {
                std::printf("0x00 ");
            } else {
                std::printf("%#4hhx ", _memory [i]);
            }

            // if (!_memory[i]) {
            //     if (zeroesCount > 0) {
            //         zeroesCount++;
            //         continue;
            //     }
            //     
            //     if (i != 0 && i % 8 == 0) {
            //         std::printf("\n");
            //     }

            //     zeroesCount++;
            //     std::printf("0x00 ");
            // } else {
            //     if (zeroesCount > 0) {
            //         printf("...%llu \n", zeroesCount);
            //         zeroesCount = 0;
            //     }

            // }
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
        printExceptions();
    }
private:
    VMRegisters                  _registers;
    memory_view<VMInstruction>   &_bytecode;
    memory_view<u8>                &_memory;
    TExceptions                &_exceptions;

    u64 getIndirect(VMOperand const &operand) const {
        return _registers.data[operand.registerIndex];
    }

    u64 getDisplaced(VMOperand const &operand) const {
        return _registers.data[operand.registerIndex].u + operand.value.u;
    }

    VMWord &getRegister(VMOperand const &operand) {
        return _registers.data[operand.registerIndex];
    }

    VMWord &getMemoryFromPointer(VMOperand const &operand) const {
        return *reinterpret_cast<VMWord *>(&_memory[operand.value.u]);
    }

    VMWord &getMemoryFromIndirect(VMOperand const &operand) const {
        return *reinterpret_cast<VMWord *>(&_memory[getIndirect(operand)]);
    }

    VMWord &getMemoryFromDisplaced(VMOperand const &operand) const {
        return *reinterpret_cast<VMWord *>(&_memory[getDisplaced(operand)]);
    }

    VMWord &getStackTop() const {
        return *reinterpret_cast<VMWord *>(&_memory[_registers.stackPointer]);
    }

    void mov(VMInstruction &inst) {
        VMOperand src = inst.operand1;
        VMOperand dst = inst.operand2;
        u8 size = src.size;
        u8 dstSize = dst.size;
        if (dstSize < size) {
            _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
            return;
        }
        switch (dst.type) {
            case VMOPTYPE_REGISTER: {
                VMWord &dstRegister = getRegister(dst);
                switch (src.type) {
                    case VMOPTYPE_IMMEDIATE: {
                        for (u8 i = 0; i < size; ++i) {
                            dstRegister.ubytes[i] = src.value.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_REGISTER: {
                        VMWord &srcRegister = getRegister(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstRegister.ubytes[i] = srcRegister.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_POINTER: {
                        VMWord &srcMemory = getMemoryFromPointer(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstRegister.ubytes[i] = srcMemory.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_INDIRECT: {
                        VMWord &srcMemory = getMemoryFromIndirect(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstRegister.ubytes[i] = srcMemory.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_DISPLACEMENT: {
                        VMWord &srcMemory = getMemoryFromDisplaced(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstRegister.ubytes[i] = srcMemory.ubytes[i];
                        }
                    } break;
                    default: {
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    } break;
                }
            } break;
            case VMOPTYPE_POINTER: {
                VMWord &dstMemory = getMemoryFromPointer(dst);
                switch (src.type) {
                    case VMOPTYPE_IMMEDIATE: {
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = src.value.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_REGISTER: {
                        VMWord &srcRegister = getRegister(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = srcRegister.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_POINTER: {
                        VMWord &srcMemory = getMemoryFromPointer(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = srcMemory.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_INDIRECT: {
                        VMWord &srcMemory = getMemoryFromIndirect(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = srcMemory.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_DISPLACEMENT: {
                        VMWord &srcMemory = getMemoryFromDisplaced(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = srcMemory.ubytes[i];
                        }
                    } break;
                    default: {
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    } break;
                }
            } break;
            case VMOPTYPE_INDIRECT: {
                VMWord &dstMemory = getMemoryFromIndirect(dst);
                switch (src.type) {
                    case VMOPTYPE_IMMEDIATE: {
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = src.value.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_REGISTER: {
                        VMWord &srcRegister = getRegister(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = srcRegister.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_POINTER: {
                        VMWord &srcMemory = getMemoryFromPointer(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = srcMemory.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_INDIRECT: {
                        VMWord &srcMemory = getMemoryFromIndirect(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = srcMemory.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_DISPLACEMENT: {
                        VMWord &srcMemory = getMemoryFromDisplaced(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = srcMemory.ubytes[i];
                        }
                    } break;
                    default: {
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    } break;
                }
            } break;
            case VMOPTYPE_DISPLACEMENT: {
                VMWord &dstMemory = getMemoryFromDisplaced(dst);
                switch (src.type) {
                    case VMOPTYPE_IMMEDIATE: {
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = src.value.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_REGISTER: {
                        VMWord &srcRegister = getRegister(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = srcRegister.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_POINTER: {
                        VMWord &srcMemory = getMemoryFromPointer(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = srcMemory.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_INDIRECT: {
                        VMWord &srcMemory = getMemoryFromIndirect(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = srcMemory.ubytes[i];
                        }
                    } break;
                    case VMOPTYPE_DISPLACEMENT: {
                        VMWord &srcMemory = getMemoryFromDisplaced(src);
                        for (u8 i = 0; i < size; ++i) {
                            dstMemory.ubytes[i] = srcMemory.ubytes[i];
                        }
                    } break;
                    default: {
                        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
                    } break;
                }
            } break;
            default: {
                _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
            } break;
        }
    }

    void push(VMInstruction &inst) {
        VMOperand src = inst.operand1;
        u8 size = src.size;
        if (_registers.stackPointer - size >= _memory.size()) {
            _exceptions.append(VMEXCEPT_STACK_OVERFLOW);
            return;
        }
        _registers.stackPointer -= size;
        VMWord &stackTop = getStackTop();
        switch (src.type) {
            case VMOPTYPE_IMMEDIATE: {
                for (u8 i = 0; i < size; ++i) {
                    stackTop.ubytes[i] = src.value.ubytes[i];
                }
            } break;
            case VMOPTYPE_REGISTER: {
                VMWord &srcRegister = getRegister(src);
                for (u8 i = 0; i < size; ++i) {
                    stackTop.ubytes[i] = srcRegister.ubytes[i];
                }
            } break;
            case VMOPTYPE_POINTER: {
                VMWord &srcMemory = getMemoryFromPointer(src);
                for (u8 i = 0; i < size; ++i) {
                    stackTop.ubytes[i] = srcMemory.ubytes[i];
                }
            } break;
            case VMOPTYPE_INDIRECT: {
                VMWord &srcMemory = getMemoryFromIndirect(src);
                for (u8 i = 0; i < size; ++i) {
                    stackTop.ubytes[i] = srcMemory.ubytes[i];
                }
            } break;
            case VMOPTYPE_DISPLACEMENT: {
                VMWord &srcMemory = getMemoryFromDisplaced(src);
                for (u8 i = 0; i < size; ++i) {
                    stackTop.ubytes[i] = srcMemory.ubytes[i];
                }
            } break;
            default: {
                _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
            } break;
        }
    }

    void pop(VMInstruction &inst) {
        VMOperand dst = inst.operand1;
        u8 size = dst.size;
        if (_registers.stackPointer + size >= _memory.size()) {
            _exceptions.append(VMEXCEPT_STACK_UNDERFLOW);
            return;
        }
        VMWord &stackTop = getStackTop();
        _registers.stackPointer += size;
        switch (dst.type) {
            case VMOPTYPE_REGISTER: {
                VMWord &dstRegister = getRegister(dst);
                for (u8 i = 0; i < size; ++i) {
                    dstRegister.ubytes[i] = stackTop.ubytes[i];
                }
            } break;
            case VMOPTYPE_POINTER: {
                VMWord &dstMemory = getMemoryFromPointer(dst);
                for (u8 i = 0; i < size; ++i) {
                    dstMemory.ubytes[i] = stackTop.ubytes[i];
                }
            } break;
            case VMOPTYPE_INDIRECT: {
                VMWord &dstMemory = getMemoryFromIndirect(dst);
                for (u8 i = 0; i < size; ++i) {
                    dstMemory.ubytes[i] = stackTop.ubytes[i];
                }
            } break;
            case VMOPTYPE_DISPLACEMENT: {
                VMWord &dstMemory = getMemoryFromDisplaced(dst);
                for (u8 i = 0; i < size; ++i) {
                    dstMemory.ubytes[i] = stackTop.ubytes[i];
                }
            } break;
            default: {
                _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
            } break;
        }
    }

    VMInstruction fetch() {
        VMInstruction inst {}; 
        switch (_registers.instructionPointer) {
            case 0:
                inst.opcode = VMOPCODE_MOV;
                inst.operand1 = VMOperand {
                    VMOPTYPE_IMMEDIATE,
                    VMOPSIZE_QWORD,
                    0,
                    32,
                };
                inst.operand2 = VMOperand {
                    VMOPTYPE_REGISTER,
                    VMOPSIZE_QWORD,
                    8,
                };
            break;
            case 1:
                inst.opcode = VMOPCODE_PUSH;
                inst.operand1 = VMOperand {
                    VMOPTYPE_IMMEDIATE,
                    VMOPSIZE_DWORD,
                    0,
                    32,
                };
            break;
            case 2:
                inst.opcode = VMOPCODE_PUSH;
                inst.operand1 = VMOperand {
                    VMOPTYPE_IMMEDIATE,
                    VMOPSIZE_QWORD,
                    0,
                    64,
                };
            break;
            case 3:
                inst.opcode = VMOPCODE_POP;
                inst.operand1 = VMOperand {
                    VMOPTYPE_REGISTER,
                    VMOPSIZE_QWORD,
                    15,
                };
            break;
            default:
                inst.opcode = VMOPCODE_HLT;
            break;
        }

        _registers.instructionPointer++;
        return inst;
    }
};

int main() {
    using TCode = static_array<VMInstruction, 64>;
    using TMemory = static_array<u8, KB(8)>;
    using TExceptions = static_array<VMException, KB(1)>;

    TCode code {};
    TMemory memory {};
    TExceptions exceptions {};
    auto codeView = code.view(0, code.size());
    auto memoryView = memory.view(0, memory.size());
    MetaVM<TExceptions> vm { codeView, memoryView, exceptions };

    vm.run();
    vm.printRegisters();
    vm.printExceptions();
    vm.printMemory();
}

