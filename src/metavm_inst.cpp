#include "common.hpp"
#include "types.hpp"
#include "metavm.hpp"

void MetaVM::mov(VMInstruction &inst) {
    VMOperand src = inst.operand1;
    VMOperand dst = inst.operand2;
    u8 size = src.size;
    u8 dstSize = dst.size;
    if (dstSize < size || dst.type == VMOPTYPE_IMMEDIATE) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &srcWord = getVMWord(src);

    for (u8 i = 0; i < size; ++i) {
        dstWord.ubytes[i] = srcWord.ubytes[i];
    }
}

void MetaVM::push(VMInstruction &inst) {
    VMOperand src = inst.operand1;
    u8 size = src.size;
    if (_registers.stackPointer - size >= _memory.size()) {
        _exceptions.append(VMEXCEPT_STACK_OVERFLOW);
        return;
    }
    _registers.stackPointer -= size;
    VMWord &stackTop = getStackTop();
    VMWord &srcVMWord = getVMWord(src);

    for (u8 i = 0; i < size; ++i) {
        stackTop.ubytes[i] = srcVMWord.ubytes[i];
    }
}

void MetaVM::pop(VMInstruction &inst) {
    VMOperand dst = inst.operand1;
    u8 size = dst.size;
    if (_registers.stackPointer + size >= _memory.size()) {
        _exceptions.append(VMEXCEPT_STACK_UNDERFLOW);
        return;
    }
    VMWord &stackTop = getStackTop();
    _registers.stackPointer += size;
    VMWord &dstVMWord = getVMWord(dst);

    for (u8 i = 0; i < size; ++i) {
        dstVMWord.ubytes[i] = stackTop.ubytes[i];
    }
}

void MetaVM::add(VMInstruction &inst) {
    VMOperand lhs = inst.operand1;
    VMOperand rhs = inst.operand2;
    VMOperand dst = inst.operand3;

    if (dst.type == VMOPTYPE_IMMEDIATE || dst.size < lhs.size || dst.size < rhs.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    switch (dst.size) {
        case VMOPSIZE_BYTE: {
            // lhs and rhs are guaranteed to be 1 byte long
            dstWord.ubytes[0] = lhsWord.ubytes[0] + rhsWord.ubytes[0];
        } break;
        case VMOPSIZE_WORD: {
            // lhs and rhs can at least be 1 byte long and at most 1 word long
            u16 lhsValue = 0;
            switch (lhs.size) {
                case VMOPSIZE_WORD:
                    lhsValue = lhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    lhsValue = lhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            u16 rhsValue = 0;
            switch (rhs.size) {
                case VMOPSIZE_WORD:
                    rhsValue = rhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    rhsValue = rhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            dstWord.uwords[0] = lhsValue + rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            // lhs and rhs can at least be 1 byte long and at most 1 dword long
            u32 lhsValue = 0;
            switch (lhs.size) {
                case VMOPSIZE_DWORD:
                    lhsValue = lhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    lhsValue = lhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    lhsValue = lhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            u32 rhsValue = 0;
            switch (rhs.size) {
                case VMOPSIZE_DWORD:
                    rhsValue = rhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    rhsValue = rhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    rhsValue = rhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            dstWord.udwords[0] = lhsValue + rhsValue;
        } break;
        case VMOPSIZE_QWORD: {
            // lhs and rhs can at least be 1 byte long and at most 1 qword long
            u64 lhsValue = 0;
            switch (lhs.size) {
                case VMOPSIZE_QWORD:
                    lhsValue = lhsWord.u;
                break;
                case VMOPSIZE_DWORD:
                    lhsValue = lhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    lhsValue = lhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    lhsValue = lhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            u64 rhsValue = 0;
            switch (rhs.size) {
                case VMOPSIZE_QWORD:
                    rhsValue = rhsWord.u;
                break;
                case VMOPSIZE_DWORD:
                    rhsValue = rhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    rhsValue = rhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    rhsValue = rhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            dstWord.u = lhsValue + rhsValue;
        } break;
    }
}

void MetaVM::sub(VMInstruction &inst) {
    VMOperand lhs = inst.operand1;
    VMOperand rhs = inst.operand2;
    VMOperand dst = inst.operand3;

    if (dst.type == VMOPTYPE_IMMEDIATE || dst.size < lhs.size || dst.size < rhs.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    switch (dst.size) {
        case VMOPSIZE_BYTE: {
            // lhs and rhs are guaranteed to be 1 byte long
            dstWord.ubytes[0] = lhsWord.ubytes[0] - rhsWord.ubytes[0];
        } break;
        case VMOPSIZE_WORD: {
            // lhs and rhs can at least be 1 byte long and at most 1 word long
            u16 lhsValue = 0;
            switch (lhs.size) {
                case VMOPSIZE_WORD:
                    lhsValue = lhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    lhsValue = lhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            u16 rhsValue = 0;
            switch (rhs.size) {
                case VMOPSIZE_WORD:
                    rhsValue = rhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    rhsValue = rhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            dstWord.uwords[0] = lhsValue - rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            // lhs and rhs can at least be 1 byte long and at most 1 dword long
            u32 lhsValue = 0;
            switch (lhs.size) {
                case VMOPSIZE_DWORD:
                    lhsValue = lhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    lhsValue = lhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    lhsValue = lhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            u32 rhsValue = 0;
            switch (rhs.size) {
                case VMOPSIZE_DWORD:
                    rhsValue = rhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    rhsValue = rhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    rhsValue = rhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            dstWord.udwords[0] = lhsValue - rhsValue;
        } break;
        case VMOPSIZE_QWORD: {
            // lhs and rhs can at least be 1 byte long and at most 1 qword long
            u64 lhsValue = 0;
            switch (lhs.size) {
                case VMOPSIZE_QWORD:
                    lhsValue = lhsWord.u;
                break;
                case VMOPSIZE_DWORD:
                    lhsValue = lhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    lhsValue = lhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    lhsValue = lhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            u64 rhsValue = 0;
            switch (rhs.size) {
                case VMOPSIZE_QWORD:
                    rhsValue = rhsWord.u;
                break;
                case VMOPSIZE_DWORD:
                    rhsValue = rhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    rhsValue = rhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    rhsValue = rhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            dstWord.u = lhsValue - rhsValue;
        } break;
    }
}

void MetaVM::mul(VMInstruction &inst) {
    VMOperand lhs = inst.operand1;
    VMOperand rhs = inst.operand2;
    VMOperand dst = inst.operand3;

    if (dst.type == VMOPTYPE_IMMEDIATE || dst.size < lhs.size || dst.size < rhs.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    switch (dst.size) {
        case VMOPSIZE_BYTE: {
            // lhs and rhs are guaranteed to be 1 byte long
            dstWord.ubytes[0] = lhsWord.ubytes[0] * rhsWord.ubytes[0];
        } break;
        case VMOPSIZE_WORD: {
            // lhs and rhs can at least be 1 byte long and at most 1 word long
            u16 lhsValue = 0;
            switch (lhs.size) {
                case VMOPSIZE_WORD:
                    lhsValue = lhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    lhsValue = lhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            u16 rhsValue = 0;
            switch (rhs.size) {
                case VMOPSIZE_WORD:
                    rhsValue = rhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    rhsValue = rhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            dstWord.uwords[0] = lhsValue * rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            // lhs and rhs can at least be 1 byte long and at most 1 dword long
            u32 lhsValue = 0;
            switch (lhs.size) {
                case VMOPSIZE_DWORD:
                    lhsValue = lhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    lhsValue = lhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    lhsValue = lhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            u32 rhsValue = 0;
            switch (rhs.size) {
                case VMOPSIZE_DWORD:
                    rhsValue = rhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    rhsValue = rhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    rhsValue = rhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            dstWord.udwords[0] = lhsValue * rhsValue;
        } break;
        case VMOPSIZE_QWORD: {
            // lhs and rhs can at least be 1 byte long and at most 1 qword long
            u64 lhsValue = 0;
            switch (lhs.size) {
                case VMOPSIZE_QWORD:
                    lhsValue = lhsWord.u;
                break;
                case VMOPSIZE_DWORD:
                    lhsValue = lhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    lhsValue = lhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    lhsValue = lhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            u64 rhsValue = 0;
            switch (rhs.size) {
                case VMOPSIZE_QWORD:
                    rhsValue = rhsWord.u;
                break;
                case VMOPSIZE_DWORD:
                    rhsValue = rhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    rhsValue = rhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    rhsValue = rhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            dstWord.u = lhsValue * rhsValue;
        } break;
    }
}

void MetaVM::div(VMInstruction &inst) {
    VMOperand lhs = inst.operand1;
    VMOperand rhs = inst.operand2;
    VMOperand dst = inst.operand3;

    if (dst.type == VMOPTYPE_IMMEDIATE || dst.size < lhs.size || dst.size < rhs.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    switch (dst.size) {
        case VMOPSIZE_BYTE: {
            // lhs and rhs are guaranteed to be 1 byte long
            dstWord.ubytes[0] = lhsWord.ubytes[0] / rhsWord.ubytes[0];
        } break;
        case VMOPSIZE_WORD: {
            // lhs and rhs can at least be 1 byte long and at most 1 word long
            u16 lhsValue = 0;
            switch (lhs.size) {
                case VMOPSIZE_WORD:
                    lhsValue = lhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    lhsValue = lhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            u16 rhsValue = 0;
            switch (rhs.size) {
                case VMOPSIZE_WORD:
                    rhsValue = rhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    rhsValue = rhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            dstWord.uwords[0] = lhsValue / rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            // lhs and rhs can at least be 1 byte long and at most 1 dword long
            u32 lhsValue = 0;
            switch (lhs.size) {
                case VMOPSIZE_DWORD:
                    lhsValue = lhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    lhsValue = lhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    lhsValue = lhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            u32 rhsValue = 0;
            switch (rhs.size) {
                case VMOPSIZE_DWORD:
                    rhsValue = rhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    rhsValue = rhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    rhsValue = rhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            dstWord.udwords[0] = lhsValue / rhsValue;
        } break;
        case VMOPSIZE_QWORD: {
            // lhs and rhs can at least be 1 byte long and at most 1 qword long
            u64 lhsValue = 0;
            switch (lhs.size) {
                case VMOPSIZE_QWORD:
                    lhsValue = lhsWord.u;
                break;
                case VMOPSIZE_DWORD:
                    lhsValue = lhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    lhsValue = lhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    lhsValue = lhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            u64 rhsValue = 0;
            switch (rhs.size) {
                case VMOPSIZE_QWORD:
                    rhsValue = rhsWord.u;
                break;
                case VMOPSIZE_DWORD:
                    rhsValue = rhsWord.udwords[0];
                break;
                case VMOPSIZE_WORD:
                    rhsValue = rhsWord.uwords[0];
                break;
                case VMOPSIZE_BYTE:
                    rhsValue = rhsWord.ubytes[0];
                break;
                // NOTE: unreachable
                default: return;
            }

            dstWord.u = lhsValue / rhsValue;
        } break;
    }
}

