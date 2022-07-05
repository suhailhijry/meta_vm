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

