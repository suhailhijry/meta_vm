#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utils/assert.hpp>
#include <utils/memory.hpp>

using namespace achilles;

bool aassert_handler(const char *conditionCode, const char *report) {
    std::printf("assertion %s failed, %s \n", conditionCode, report);
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

constexpr u64 REGISTER_COUNT = 16;
constexpr u64 FLOAT_REGISTER_COUNT = 16;

struct registers_t {
    u64 ip; // instruction pointer
    u64 sp; // stack pointer
    u64 sr; // status register
    u64 d[REGISTER_COUNT] = {0}; // data registers - general purpose
    f64 f[FLOAT_REGISTER_COUNT] = {0}; // floating point registers
};

// op src dst
enum VM_OP {
    VM_OP_HLT = 0,
    VM_OP_NOP,
    VM_OP_MOV = 0x20,
    VM_OP_MOVF,
    VM_OP_PUSH,
    VM_OP_POP,
    VM_OP_PUSHF,
    VM_OP_POPF,
    VM_OP_ADD,
    VM_OP_SUB,
    VM_OP_MUL,
    VM_OP_DIV,
    VM_OP_ADDF,
    VM_OP_SUBF,
    VM_OP_MULF,
    VM_OP_DIVF,
    VM_OP_JMP,
    VM_OP_CMP,
    VM_OP_JME,
    VM_OP_JNE,
    VM_OP_CALL,
    VM_OP_RET,
};

enum VM_STATE {
    VM_STATE_HALT,
    VM_STATE_EXECUTE,
};

template<typename stack_t, typename memory_t>
struct MetaVM {
    VM_STATE state = VM_STATE_EXECUTE; 
    registers_t registers {0};
    memory_view<u64> &bytecode;
    memory_t &memory;
    stack_t &stack;

    MetaVM (
            memory_view<u64> &bytecode,
            stack_t &stack,
            memory_t &memory
    ) : bytecode(bytecode),
        stack(stack),
        memory(memory) {}

    void run() {
        while (state != VM_STATE_HALT) {
            switch (static_cast<VM_OP>(byte())) {
                case VM_OP_HLT:
                    hlt();
                break;
                case VM_OP_MOV:
                    mov();
                break;
                case VM_OP_MOVF:
                    movf();
                break;
                case VM_OP_PUSH:
                    push();
                break;
                case VM_OP_POP:
                    pop();
                break;
                case VM_OP_PUSHF:
                    pushf();
                break;
                case VM_OP_POPF:
                    popf();
                break;
                case VM_OP_NOP:
                    advance();
                break;
                case VM_OP_ADD:
                    add();
                break;
                case VM_OP_SUB:
                    sub();
                break;
                case VM_OP_MUL:
                    mul();
                break;
                case VM_OP_DIV:
                    div();
                break;
                case VM_OP_ADDF:
                    addf();
                break;
                case VM_OP_SUBF:
                    subf();
                break;
                case VM_OP_MULF:
                    mulf();
                break;
                case VM_OP_DIVF:
                    divf();
                break;
                case VM_OP_JMP:
                    jmp();
                break;
                case VM_OP_CMP:
                    cmp();
                break;
                case VM_OP_JME:
                    jme();
                break;
                case VM_OP_JNE:
                    jne();
                break;
                case VM_OP_CALL:
                    call();
                break;
                case VM_OP_RET:
                    ret();
                break;
            }
            advance();
        }
    }
    
    void hlt() {
        state = VM_STATE_HALT;
        advance();
    }
    
    void mov() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 operand = byte();
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        registers.d[ri] = operand;
    }

    void movf() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 b = byte();
        f64 operand = *reinterpret_cast<f64 *>(&b);
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        registers.f[ri] = operand;
    }

    void push() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        stack.push(registers.d[ri]);
        registers.sp++;
    }

    void pop() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        registers.d[ri] = stack.pop();
        registers.sp--;
    }

    void pushf() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        stack.push(registers.f[ri]);
        registers.sp++;
    }

    void popf() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        u64 b = stack.pop();
        f64 f = *reinterpret_cast<f64 *>(&b);
        registers.f[ri] = f;
        registers.sp--;
    }

    void add() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 operand = byte();
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        registers.d[ri] += operand;
    }

    void sub() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 operand = byte();
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        registers.d[ri] -= operand;
    }

    void mul() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 operand = byte();
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        registers.d[ri] *= operand;
    }

    void div() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 operand = byte();
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        registers.d[ri] /= operand;
    }

    void addf() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 b = byte();
        f64 operand = *reinterpret_cast<f64 *>(&b);
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        registers.f[ri] += operand;
    }

    void subf() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 b = byte();
        f64 operand = *reinterpret_cast<f64 *>(&b);
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        registers.f[ri] -= operand;
    }

    void mulf() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 b = byte();
        f64 operand = *reinterpret_cast<f64 *>(&b);
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        registers.f[ri] *= operand;
    }

    void divf() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 b = byte();
        f64 operand = *reinterpret_cast<f64 *>(&b);
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        registers.f[ri] /= operand;
    }

    void jmp() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 operand = byte();
        aassert(operand < bytecode.length(), "segfault");
        registers.ip = operand - 1;
    }

    void cmp() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 operand = byte();
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 ri = byte();
        registers.sr = operand == registers.d[ri];
    }

    void jme() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 operand = byte();
        aassert(operand < bytecode.length(), "segfault");
        if (registers.sr) registers.ip = operand - 1;
    }

    void jne() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 operand = byte();
        aassert(operand < bytecode.length(), "segfault");
        if (!registers.sr) registers.ip = operand - 1;
    }

    void call() {
        advance();
        aassert(state != VM_STATE_HALT, "invalid operation");
        u64 operand = byte();
        aassert(operand < bytecode.length(), "segfault");
        stack.push(registers.ip);
        registers.sp++;
        registers.ip = operand - 1;
    }

    void ret() {
        registers.ip = stack.pop();
        registers.sp--;
    }

    u64 byte() {
        return bytecode[registers.ip];
    }

    void advance() {
        registers.ip++;
        if (registers.sp >= stack.capacity()) {
            printf("stack overflow\n");
            state = VM_STATE_HALT;
        }
        if (registers.ip >= bytecode.length()) {
            printf("program finished\n");
            state = VM_STATE_HALT;
        }
    }

    void printRegisters() {
        printf("REGISTERS:\n");
        printf("ip: %llu ", registers.ip);
        printf("sp: %llu ", registers.sp);
        printf("sr: %llu ", registers.sr);
        printf("\n");
        for (u64 i = 0; i < REGISTER_COUNT; ++i) {
            if (i != 0 && i % 8 == 0) {
                printf("\n");
            }
            if (i < 10) {
                printf("d0%llu: %llu ", i, registers.d[i]);
            } else {
                printf("d%llu: %llu ", i, registers.d[i]);
            }
        }
        printf("\n");
        for (u64 i = 0; i < FLOAT_REGISTER_COUNT; ++i) {
            if (i != 0 && i % 8 == 0) {
                printf("\n");
            }
            if (i < 10) {
                printf("f0%llu: %lf ", i, registers.f[i]);
            } else {
                printf("f%llu: %lf ", i, registers.f[i]);
            }
        }
        printf("\n");
    }

    void printStack() {
        printf("STACK:\n");
        for (u64 i = 0; i < stack.capacity(); ++i) {
            if (i != 0 && i % 8 == 0) {
                printf("\n");
            }
            printf("0x%llX ", stack[i]);
        }
        printf("\n");
    }

    void printMemory() {
        printf("Memory:\n");
        for (u64 i = 0; i < memory.length(); ++i) {
            if (i != 0 && i % 8 == 0) {
                printf("\n");
            }
            printf("0x%llX ", memory[i]);
        }
        printf("\n");
    }

    void printAll() {
        printRegisters();
        printStack();
        printMemory();
    }
};

int main() {
    f64 v = .1;
    u64 b = *reinterpret_cast<u64 *>(&v);
    f64 v2 = .2;
    u64 b2 = *reinterpret_cast<u64 *>(&v2);
    static_array<u64, 32> code {
        VM_OP_MOV,   32, 0,
        VM_OP_ADD,    2, 0,
        VM_OP_DIV,    8, 0,
        VM_OP_CALL, 0xF,
        VM_OP_DIV,    2, 0,
        VM_OP_HLT,
        VM_OP_MOVF,   b, 0,
        VM_OP_ADDF,  b2, 0,
        VM_OP_RET,
    };
    static_array<u64, 1024> stack {};
    static_array<u64, KB(8)> memory {};
    memory_view<u64> codeView = code.view(0, code.length() - 1);
    MetaVM<decltype(stack), decltype(memory)> vm { codeView, stack, memory };
    vm.run();
    vm.printRegisters();
}

