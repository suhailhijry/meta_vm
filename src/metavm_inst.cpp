#include "common.hpp"
#include "types.hpp"
#include "metavm.hpp"

// NOTE: many opcodes implementations can be reduced to a macro, but I don't have the time to it, so copy pasting for now :)

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

u64 getUnsigned(VMOperandSize size, VMWord &value) {
    switch (size) {
        case VMOPSIZE_QWORD: return value.u;
        case VMOPSIZE_DWORD: return value.udwords[0];
        case VMOPSIZE_WORD: return value.uwords[0];
        case VMOPSIZE_BYTE: return value.ubytes[0];
        default: return 0;
    }
}

s64 getSigned(VMOperandSize size, VMWord &value) {
    switch (size) {
        case VMOPSIZE_QWORD: return value.s;
        case VMOPSIZE_DWORD: return value.sdwords[0];
        case VMOPSIZE_WORD: return value.swords[0];
        case VMOPSIZE_BYTE: return value.sbytes[0];
        default: return 0;
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
            u16 lhsValue = getUnsigned(lhs.size, lhsWord);
            u16 rhsValue = getUnsigned(lhs.size, rhsWord);
            dstWord.uwords[0] = lhsValue + rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            u32 lhsValue = getUnsigned(lhs.size, lhsWord);
            u32 rhsValue = getUnsigned(lhs.size, rhsWord);
            dstWord.udwords[0] = lhsValue + rhsValue;
        } break;
        case VMOPSIZE_QWORD: {
            u64 lhsValue = getUnsigned(lhs.size, lhsWord);
            u64 rhsValue = getUnsigned(lhs.size, rhsWord);
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
            u16 lhsValue = getUnsigned(lhs.size, lhsWord);
            u16 rhsValue = getUnsigned(lhs.size, rhsWord);
            dstWord.uwords[0] = lhsValue - rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            u32 lhsValue = getUnsigned(lhs.size, lhsWord);
            u32 rhsValue = getUnsigned(lhs.size, rhsWord);
            dstWord.udwords[0] = lhsValue - rhsValue;
        } break;
        case VMOPSIZE_QWORD: {
            u64 lhsValue = getUnsigned(lhs.size, lhsWord);
            u64 rhsValue = getUnsigned(lhs.size, rhsWord);
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
            u16 lhsValue = getUnsigned(lhs.size, lhsWord);
            u16 rhsValue = getUnsigned(lhs.size, rhsWord);
            dstWord.uwords[0] = lhsValue * rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            u32 lhsValue = getUnsigned(lhs.size, lhsWord);
            u32 rhsValue = getUnsigned(lhs.size, rhsWord);
            dstWord.udwords[0] = lhsValue * rhsValue;
        } break;
        case VMOPSIZE_QWORD: {
            u64 lhsValue = getUnsigned(lhs.size, lhsWord);
            u64 rhsValue = getUnsigned(lhs.size, rhsWord);
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
            u16 lhsValue = getUnsigned(lhs.size, lhsWord);
            u16 rhsValue = getUnsigned(lhs.size, rhsWord);
            dstWord.uwords[0] = lhsValue / rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            u32 lhsValue = getUnsigned(lhs.size, lhsWord);
            u32 rhsValue = getUnsigned(lhs.size, rhsWord);
            dstWord.udwords[0] = lhsValue / rhsValue;
        } break;
        case VMOPSIZE_QWORD: {
            u64 lhsValue = getUnsigned(lhs.size, lhsWord);
            u64 rhsValue = getUnsigned(lhs.size, rhsWord);
            dstWord.u = lhsValue / rhsValue;
        } break;
    }
}

void MetaVM::divr(VMInstruction &inst) {
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
            dstWord.ubytes[0] = lhsWord.ubytes[0] % rhsWord.ubytes[0];
        } break;
        case VMOPSIZE_WORD: {
            u16 lhsValue = getUnsigned(lhs.size, lhsWord);
            u16 rhsValue = getUnsigned(lhs.size, rhsWord);
            dstWord.uwords[0] = lhsValue % rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            u32 lhsValue = getUnsigned(lhs.size, lhsWord);
            u32 rhsValue = getUnsigned(lhs.size, rhsWord);
            dstWord.udwords[0] = lhsValue % rhsValue;
        } break;
        case VMOPSIZE_QWORD: {
            u64 lhsValue = getUnsigned(lhs.size, lhsWord);
            u64 rhsValue = getUnsigned(lhs.size, rhsWord);
            dstWord.u = lhsValue % rhsValue;
        } break;
    }
}

void MetaVM::adds(VMInstruction &inst) {
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
            dstWord.sbytes[0] = lhsWord.sbytes[0] + rhsWord.sbytes[0];
        } break;
        case VMOPSIZE_WORD: {
            s16 lhsValue = getSigned(lhs.size, lhsWord);
            s16 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.swords[0] = lhsValue + rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            s32 lhsValue = getSigned(lhs.size, lhsWord);
            s32 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.sdwords[0] = lhsValue + rhsValue;
        } break;
        case VMOPSIZE_QWORD: {
            s64 lhsValue = getSigned(lhs.size, lhsWord);
            s64 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.s = lhsValue + rhsValue;
        } break;
    }
}

void MetaVM::subs(VMInstruction &inst) {
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
            dstWord.sbytes[0] = lhsWord.sbytes[0] - rhsWord.sbytes[0];
        } break;
        case VMOPSIZE_WORD: {
            s16 lhsValue = getSigned(lhs.size, lhsWord);
            s16 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.swords[0] = lhsValue - rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            s32 lhsValue = getSigned(lhs.size, lhsWord);
            s32 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.sdwords[0] = lhsValue - rhsValue;
        } break;
        case VMOPSIZE_QWORD: {
            s64 lhsValue = getSigned(lhs.size, lhsWord);
            s64 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.s = lhsValue - rhsValue;
        } break;
    }
}

void MetaVM::muls(VMInstruction &inst) {
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
            dstWord.sbytes[0] = lhsWord.sbytes[0] * rhsWord.sbytes[0];
        } break;
        case VMOPSIZE_WORD: {
            s16 lhsValue = getSigned(lhs.size, lhsWord);
            s16 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.swords[0] = lhsValue * rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            s32 lhsValue = getSigned(lhs.size, lhsWord);
            s32 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.sdwords[0] = lhsValue * rhsValue;
        } break;
        case VMOPSIZE_QWORD: {
            s64 lhsValue = getSigned(lhs.size, lhsWord);
            s64 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.s = lhsValue * rhsValue;
        } break;
    }
}

void MetaVM::divs(VMInstruction &inst) {
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
            dstWord.sbytes[0] = lhsWord.sbytes[0] / rhsWord.sbytes[0];
        } break;
        case VMOPSIZE_WORD: {
            s16 lhsValue = getSigned(lhs.size, lhsWord);
            s16 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.swords[0] = lhsValue / rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            s32 lhsValue = getSigned(lhs.size, lhsWord);
            s32 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.sdwords[0] = lhsValue / rhsValue;
        } break;
        case VMOPSIZE_QWORD: {
            s64 lhsValue = getSigned(lhs.size, lhsWord);
            s64 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.s = lhsValue / rhsValue;
        } break;
    }
}

void MetaVM::divsr(VMInstruction &inst) {
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
            dstWord.sbytes[0] = lhsWord.sbytes[0] % rhsWord.sbytes[0];
        } break;
        case VMOPSIZE_WORD: {
            s16 lhsValue = getSigned(lhs.size, lhsWord);
            s16 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.swords[0] = lhsValue % rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            s32 lhsValue = getSigned(lhs.size, lhsWord);
            s32 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.sdwords[0] = lhsValue % rhsValue;
        } break;
        case VMOPSIZE_QWORD: {
            s64 lhsValue = getSigned(lhs.size, lhsWord);
            s64 rhsValue = getSigned(lhs.size, rhsWord);
            dstWord.s = lhsValue % rhsValue;
        } break;
    }
}

void MetaVM::addf(VMInstruction &inst) {
    VMOperand lhs = inst.operand1;
    VMOperand rhs = inst.operand2;
    VMOperand dst = inst.operand3;

    if (
        dst.size != VMOPSIZE_QWORD ||
        lhs.size != VMOPSIZE_QWORD ||
        rhs.size != VMOPSIZE_QWORD ||
        dst.type == VMOPTYPE_IMMEDIATE
    ) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    dstWord.f = lhsWord.f + rhsWord.f;
}

void MetaVM::subf(VMInstruction &inst) {
    VMOperand lhs = inst.operand1;
    VMOperand rhs = inst.operand2;
    VMOperand dst = inst.operand3;

    if (
        dst.size != VMOPSIZE_QWORD ||
        lhs.size != VMOPSIZE_QWORD ||
        rhs.size != VMOPSIZE_QWORD ||
        dst.type == VMOPTYPE_IMMEDIATE
    ) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    dstWord.f = lhsWord.f - rhsWord.f;
}

void MetaVM::mulf(VMInstruction &inst) {
    VMOperand lhs = inst.operand1;
    VMOperand rhs = inst.operand2;
    VMOperand dst = inst.operand3;

    if (
        dst.size != VMOPSIZE_QWORD ||
        lhs.size != VMOPSIZE_QWORD ||
        rhs.size != VMOPSIZE_QWORD ||
        dst.type == VMOPTYPE_IMMEDIATE
    ) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    dstWord.f = lhsWord.f * rhsWord.f;
}

void MetaVM::divf(VMInstruction &inst) {
    VMOperand lhs = inst.operand1;
    VMOperand rhs = inst.operand2;
    VMOperand dst = inst.operand3;

    if (
        dst.size != VMOPSIZE_QWORD ||
        lhs.size != VMOPSIZE_QWORD ||
        rhs.size != VMOPSIZE_QWORD ||
        dst.type == VMOPTYPE_IMMEDIATE
    ) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    dstWord.f = lhsWord.f / rhsWord.f;
}

void MetaVM::addfs(VMInstruction &inst) {
    VMOperand lhs = inst.operand1;
    VMOperand rhs = inst.operand2;
    VMOperand dst = inst.operand3;

    if (
        dst.size != VMOPSIZE_DWORD ||
        lhs.size != VMOPSIZE_DWORD ||
        rhs.size != VMOPSIZE_DWORD ||
        dst.type == VMOPTYPE_IMMEDIATE
    ) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    dstWord.fsingles[0] = lhsWord.fsingles[0] + rhsWord.fsingles[0];
}

void MetaVM::subfs(VMInstruction &inst) {
    VMOperand lhs = inst.operand1;
    VMOperand rhs = inst.operand2;
    VMOperand dst = inst.operand3;

    if (
        dst.size != VMOPSIZE_DWORD ||
        lhs.size != VMOPSIZE_DWORD ||
        rhs.size != VMOPSIZE_DWORD ||
        dst.type == VMOPTYPE_IMMEDIATE
    ) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    dstWord.fsingles[0] = lhsWord.fsingles[0] - rhsWord.fsingles[0];
}

void MetaVM::mulfs(VMInstruction &inst) {
    VMOperand lhs = inst.operand1;
    VMOperand rhs = inst.operand2;
    VMOperand dst = inst.operand3;

    if (
        dst.size != VMOPSIZE_DWORD ||
        lhs.size != VMOPSIZE_DWORD ||
        rhs.size != VMOPSIZE_DWORD ||
        dst.type == VMOPTYPE_IMMEDIATE
    ) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    dstWord.fsingles[0] = lhsWord.fsingles[0] * rhsWord.fsingles[0];
}

void MetaVM::divfs(VMInstruction &inst) {
    VMOperand lhs = inst.operand1;
    VMOperand rhs = inst.operand2;
    VMOperand dst = inst.operand3;

    if (
        dst.size != VMOPSIZE_DWORD ||
        lhs.size != VMOPSIZE_DWORD ||
        rhs.size != VMOPSIZE_DWORD ||
        dst.type == VMOPTYPE_IMMEDIATE
    ) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    dstWord.fsingles[0] = lhsWord.fsingles[0] / rhsWord.fsingles[0];
}

void MetaVM::neg(VMInstruction &inst) {
    VMOperand src = inst.operand1;
    VMOperand dst = inst.operand1;
    
    if (dst.size < src.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &srcWord = getVMWord(src);
    VMWord &dstWord = getVMWord(dst);

    switch (dst.size) {
        case VMOPSIZE_QWORD: {
            s64 value = getSigned(src.size, srcWord);
            dstWord.s = -value;
        } break;
        case VMOPSIZE_DWORD: {
            s32 value = getSigned(src.size, srcWord);
            dstWord.sdwords[0] = -value;
        } break;
        case VMOPSIZE_WORD: {
            s16 value = getSigned(src.size, srcWord);
            dstWord.swords[0] = -value;
        } break;
        default: {
            s8 value = getSigned(src.size, srcWord);
            dstWord.sbytes[0] = -value;
        } break;
    }
}

void MetaVM::bitwise_and(VMInstruction &inst) {
    VMOperand lhs = inst.operand2;
    VMOperand rhs = inst.operand3;
    VMOperand dst = inst.operand1;
    
    if (dst.size < lhs.size || lhs.size != rhs.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    switch (dst.size) {
        case VMOPSIZE_QWORD: {
            u64 lhsValue = getUnsigned(lhs.size, lhsWord);
            u64 rhsValue = getUnsigned(rhs.size, rhsWord);
            dstWord.u = lhsValue & rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            u32 lhsValue = getUnsigned(lhs.size, lhsWord);
            u32 rhsValue = getUnsigned(rhs.size, rhsWord);
            dstWord.udwords[0] = lhsValue & rhsValue;
        } break;
        case VMOPSIZE_WORD: {
            u16 lhsValue = getUnsigned(lhs.size, lhsWord);
            u16 rhsValue = getUnsigned(rhs.size, rhsWord);
            dstWord.uwords[0] = lhsValue & rhsValue;
        } break;
        default: {
            u8 lhsValue = getUnsigned(lhs.size, lhsWord);
            u8 rhsValue = getUnsigned(rhs.size, rhsWord);
            dstWord.ubytes[0] = lhsValue & rhsValue;
        } break;
    }
}

void MetaVM::bitwise_or(VMInstruction &inst) {
    VMOperand lhs = inst.operand2;
    VMOperand rhs = inst.operand3;
    VMOperand dst = inst.operand1;
    
    if (dst.size < lhs.size || lhs.size != rhs.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    switch (dst.size) {
        case VMOPSIZE_QWORD: {
            u64 lhsValue = getUnsigned(lhs.size, lhsWord);
            u64 rhsValue = getUnsigned(rhs.size, rhsWord);
            dstWord.u = lhsValue | rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            u32 lhsValue = getUnsigned(lhs.size, lhsWord);
            u32 rhsValue = getUnsigned(rhs.size, rhsWord);
            dstWord.udwords[0] = lhsValue | rhsValue;
        } break;
        case VMOPSIZE_WORD: {
            u16 lhsValue = getUnsigned(lhs.size, lhsWord);
            u16 rhsValue = getUnsigned(rhs.size, rhsWord);
            dstWord.uwords[0] = lhsValue | rhsValue;
        } break;
        default: {
            u8 lhsValue = getUnsigned(lhs.size, lhsWord);
            u8 rhsValue = getUnsigned(rhs.size, rhsWord);
            dstWord.ubytes[0] = lhsValue | rhsValue;
        } break;
    }
}

void MetaVM::bitwise_xor(VMInstruction &inst) {
    VMOperand lhs = inst.operand2;
    VMOperand rhs = inst.operand3;
    VMOperand dst = inst.operand1;
    
    if (dst.size < lhs.size || lhs.size != rhs.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &dstWord = getVMWord(dst);
    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    switch (dst.size) {
        case VMOPSIZE_QWORD: {
            u64 lhsValue = getUnsigned(lhs.size, lhsWord);
            u64 rhsValue = getUnsigned(rhs.size, rhsWord);
            dstWord.u = lhsValue ^ rhsValue;
        } break;
        case VMOPSIZE_DWORD: {
            u32 lhsValue = getUnsigned(lhs.size, lhsWord);
            u32 rhsValue = getUnsigned(rhs.size, rhsWord);
            dstWord.udwords[0] = lhsValue ^ rhsValue;
        } break;
        case VMOPSIZE_WORD: {
            u16 lhsValue = getUnsigned(lhs.size, lhsWord);
            u16 rhsValue = getUnsigned(rhs.size, rhsWord);
            dstWord.uwords[0] = lhsValue ^ rhsValue;
        } break;
        default: {
            u8 lhsValue = getUnsigned(lhs.size, lhsWord);
            u8 rhsValue = getUnsigned(rhs.size, rhsWord);
            dstWord.ubytes[0] = lhsValue ^ rhsValue;
        } break;
    }
}

void MetaVM::bitwise_not(VMInstruction &inst) {
    VMOperand src = inst.operand1;
    VMOperand dst = inst.operand1;
    
    if (dst.size < src.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &srcWord = getVMWord(src);
    VMWord &dstWord = getVMWord(dst);

    switch (dst.size) {
        case VMOPSIZE_QWORD: {
            u64 value = getUnsigned(src.size, srcWord);
            dstWord.u = ~value;
        } break;
        case VMOPSIZE_DWORD: {
            u32 value = getSigned(src.size, srcWord);
            dstWord.udwords[0] = ~value;
        } break;
        case VMOPSIZE_WORD: {
            u16 value = getSigned(src.size, srcWord);
            dstWord.uwords[0] = ~value;
        } break;
        default: {
            u8 value = getSigned(src.size, srcWord);
            dstWord.ubytes[0] = ~value;
        } break;
    }
}

void MetaVM::jmp(VMInstruction &inst) {
    VMOperand dst = inst.operand1;
    
    if (dst.value.u >= _bytecode.size()) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    _registers.instructionPointer = dst.value.u;
}

void MetaVM::jeq(VMInstruction &inst) {
    VMOperand dst = inst.operand1;
    VMOperand lhs = inst.operand2;
    VMOperand rhs = inst.operand3;
    
    if (dst.value.u >= _bytecode.size() || lhs.size != rhs.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    if (getUnsigned(lhs.size, lhsWord) != getUnsigned(rhs.size, rhsWord)) return;

    _registers.instructionPointer = dst.value.u;
}

void MetaVM::jne(VMInstruction &inst) {
    VMOperand dst = inst.operand1;
    VMOperand lhs = inst.operand2;
    VMOperand rhs = inst.operand3;
    
    if (dst.value.u >= _bytecode.size() || lhs.size != rhs.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    if (getUnsigned(lhs.size, lhsWord) == getUnsigned(rhs.size, rhsWord)) return;

    _registers.instructionPointer = dst.value.u;
}

void MetaVM::jgt(VMInstruction &inst) {
    VMOperand dst = inst.operand1;
    VMOperand lhs = inst.operand2;
    VMOperand rhs = inst.operand3;
    
    if (dst.value.u >= _bytecode.size() || lhs.size != rhs.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    if (!(getUnsigned(lhs.size, lhsWord) > getUnsigned(rhs.size, rhsWord))) return;

    _registers.instructionPointer = dst.value.u;
}

void MetaVM::jlt(VMInstruction &inst) {
    VMOperand dst = inst.operand1;
    VMOperand lhs = inst.operand2;
    VMOperand rhs = inst.operand3;
    
    if (dst.value.u >= _bytecode.size() || lhs.size != rhs.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    if (!(getUnsigned(lhs.size, lhsWord) < getUnsigned(rhs.size, rhsWord))) return;

    _registers.instructionPointer = dst.value.u;
}

void MetaVM::jge(VMInstruction &inst) {
    VMOperand dst = inst.operand1;
    VMOperand lhs = inst.operand2;
    VMOperand rhs = inst.operand3;
    
    if (dst.value.u >= _bytecode.size() || lhs.size != rhs.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    if (!(getUnsigned(lhs.size, lhsWord) >= getUnsigned(rhs.size, rhsWord))) return;

    _registers.instructionPointer = dst.value.u;
}

void MetaVM::jle(VMInstruction &inst) {
    VMOperand dst = inst.operand1;
    VMOperand lhs = inst.operand2;
    VMOperand rhs = inst.operand3;
    
    if (dst.value.u >= _bytecode.size() || lhs.size != rhs.size) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    VMWord &lhsWord = getVMWord(lhs);
    VMWord &rhsWord = getVMWord(rhs);

    if (!(getUnsigned(lhs.size, lhsWord) <= getUnsigned(rhs.size, rhsWord))) return;

    _registers.instructionPointer = dst.value.u;
}

void MetaVM::call(VMInstruction &inst) {
    VMOperand address = inst.operand1;

    VMWord &addressWord = getVMWord(address);
    VMOperandSize addressSize = address.size;

    u64 value = getUnsigned(addressSize, addressWord);
    if (value >= _bytecode.length()) {
        _exceptions.append(VMEXCEPT_INVALID_OPERANDS);
        return;
    }

    u8 size = VMOPSIZE_QWORD; 

    // make sure there's enough room for (at least) the instruction pointer
    if (_registers.stackPointer - size >= _memory.size()) {
        _exceptions.append(VMEXCEPT_STACK_OVERFLOW);
        return;
    }

    // push the current instruction pointer to the stack
    _registers.stackPointer -= size;
    VMWord &stackTop = getStackTop();
    stackTop.u = _registers.instructionPointer;

    // make the next instruction the called code
    _registers.instructionPointer = value;
}

void MetaVM::ret(VMInstruction &) {
    VMWord &stackTop = getStackTop();

    // make sure the stack top is a sane address
    if (stackTop.u >= _bytecode.length()) {
        _exceptions.append(VMEXCEPT_UNEXPECTED_OPCODE);
        return;
    }

    // take the top of the stack
    u8 size = VMOPSIZE_QWORD;
    _registers.stackPointer += size;

    // get save it back to the register
    _registers.instructionPointer = stackTop.u;
}

