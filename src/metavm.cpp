#include "common.hpp"
#include "types.hpp"
#include "metavm.hpp"

VMInstruction MetaVM::fetch() {
    VMInstruction inst {}; 
    switch (_registers.data[31].u) {
        default:
            inst.opcode = VMOPCODE_HLT;
        break;
    }

    _registers.data[31].u++;
    return inst;
}

void MetaVM::run() {
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
            case VMOPCODE_DIVR:
                divr(inst);
            break;
            case VMOPCODE_ADDS:
                adds(inst);
            break;
            case VMOPCODE_SUBS:
                subs(inst);
            break;
            case VMOPCODE_MULS:
                muls(inst);
            break;
            case VMOPCODE_DIVS:
                divs(inst);
            break;
            case VMOPCODE_DIVSR:
                divsr(inst);
            break;
            case VMOPCODE_ADDF:
                addf(inst);
            break;
            case VMOPCODE_SUBF:
                subf(inst);
            break;
            case VMOPCODE_MULF:
                mulf(inst);
            break;
            case VMOPCODE_DIVF:
                divf(inst);
            break;
            case VMOPCODE_ADDFS:
                addfs(inst);
            break;
            case VMOPCODE_SUBFS:
                subfs(inst);
            break;
            case VMOPCODE_MULFS:
                mulfs(inst);
            break;
            case VMOPCODE_DIVFS:
                divfs(inst);
            break;
            case VMOPCODE_NEG:
                neg(inst);
            break;
            case VMOPCODE_AND:
                bitwise_and(inst);
            break;
            case VMOPCODE_OR:
                bitwise_or(inst);
            break;
            case VMOPCODE_XOR:
                bitwise_xor(inst);
            break;
            case VMOPCODE_NOT:
                bitwise_not(inst);
            break;
            case VMOPCODE_JMP:
                jmp(inst);
            break;
            case VMOPCODE_JEQ:
                jeq(inst);
            break;
            case VMOPCODE_JNE:
                jne(inst);
            break;
            case VMOPCODE_JGT:
                jgt(inst);
            break;
            case VMOPCODE_JLT:
                jlt(inst);
            break;
            case VMOPCODE_JGE:
                jge(inst);
            break;
            case VMOPCODE_JLE:
                jle(inst);
            break;
            case VMOPCODE_CALL:
                call(inst);
            break;
            case VMOPCODE_RET:
                ret(inst);
            break;
        }
    }
}

void MetaVM::printRegisters() {
    std::printf("REGISTERS:\n");
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

void MetaVM::printMemory() {
    std::printf("MEMORY:\n");
    for (u64 i = 0; i < _memory.length(); ++i) {
        if (i != 0 && i % 8 == 0) {
            std::printf("\n");
        }

        if (_memory[i] == 0) {
            std::printf("0x00 ");
        } else {
            std::printf("%#4hhx ", _memory [i]);
        }
    }
    std::printf("\n");
}

void MetaVM::printExceptions() {
    printf("EXCEPTIONS:\n");
    for (u64 i = 0; i < _exceptions.length(); ++i) {
        std::printf("%s\n", getExceptionName(_exceptions[i]));
    }
    std::printf("\n");
}

void MetaVM::printAll() {
    printRegisters();
    printMemory();
    printExceptions();
}

u64 MetaVM::getIndirect(VMOperand const &operand) const {
    return _registers.data[operand.registerIndex];
}

u64 MetaVM::getDisplaced(VMOperand const &operand) const {
    return _registers.data[operand.registerIndex].u + operand.value.s;
}

VMWord &MetaVM::getRegister(VMOperand const &operand) {
    u8 index = operand.registerIndex;
    switch (operand.size) {
        case VMOPSIZE_QWORD:
            return _registers.data[index];
        case VMOPSIZE_DWORD: {
            index = (u8) ((index / (REGISTER_COUNT * 2.0f)) * REGISTER_COUNT);
            VMWord &actualRegister = _registers.data[index];
            return *reinterpret_cast<VMWord *>(&actualRegister.udwords[operand.registerIndex % 2]);
        } break;
        case VMOPSIZE_WORD: {
            index = (u8) ((index / (REGISTER_COUNT * 4.0f)) * REGISTER_COUNT);
            VMWord &actualRegister = _registers.data[index];
            return *reinterpret_cast<VMWord *>(&actualRegister.uwords[operand.registerIndex % 4]);
        } break;
        default: {
            index = (u8) ((index / (REGISTER_COUNT * 8.0f)) * REGISTER_COUNT);
            VMWord &actualRegister = _registers.data[index];
            return *reinterpret_cast<VMWord *>(&actualRegister.ubytes[operand.registerIndex % 8]);
        } break;
    }
}

VMWord &MetaVM::getMemoryFromPointer(VMOperand const &operand) {
    return *reinterpret_cast<VMWord *>(&_memory[operand.value.u]);
}

VMWord &MetaVM::getMemoryFromIndirect(VMOperand const &operand) {
    return *reinterpret_cast<VMWord *>(&_memory[getIndirect(operand)]);
}

VMWord &MetaVM::getMemoryFromDisplaced(VMOperand const &operand) {
    return *reinterpret_cast<VMWord *>(&_memory[getDisplaced(operand)]);
}

VMWord &MetaVM::getStackTop() const {
    return *reinterpret_cast<VMWord *>(&_memory[_registers.data[30].u]);
}

VMWord &MetaVM::getVMWord(VMOperand &operand) {
    switch (operand.type) {
        case VMOPTYPE_REGISTER:     return getRegister(operand);
        case VMOPTYPE_POINTER:      return getMemoryFromPointer(operand);
        case VMOPTYPE_INDIRECT:     return getMemoryFromIndirect(operand);
        case VMOPTYPE_DISPLACEMENT: return getMemoryFromDisplaced(operand);
        default:                    return operand.value;
    }
}

