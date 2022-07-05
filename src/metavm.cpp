#include "common.hpp"
#include "types.hpp"
#include "metavm.hpp"

VMInstruction MetaVM::fetch() {
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
                0,
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
                VMOPSIZE_BYTE,
                0,
                64,
            };
        break;
        case 3:
            inst.opcode = VMOPCODE_POP;
            inst.operand1 = VMOperand {
                VMOPTYPE_REGISTER,
                VMOPSIZE_BYTE,
                2,
                0,
            };
        break;
        case 4:
            inst.opcode = VMOPCODE_MUL;
            inst.operand1 = VMOperand {
                VMOPTYPE_REGISTER,
                VMOPSIZE_BYTE,
                2,
                0,
            };
            inst.operand2 = VMOperand {
                VMOPTYPE_IMMEDIATE,
                VMOPSIZE_BYTE,
                0,
                64,
            };
            inst.operand3 = VMOperand {
                VMOPTYPE_REGISTER,
                VMOPSIZE_QWORD,
                17,
                0,
            };
        break;
        default:
            inst.opcode = VMOPCODE_HLT;
        break;
    }

    _registers.instructionPointer++;
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
        }
    }
}

void MetaVM::printRegisters() {
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
    return _registers.data[operand.registerIndex].u + operand.value.u;
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
    return *reinterpret_cast<VMWord *>(&_memory[_registers.stackPointer]);
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

