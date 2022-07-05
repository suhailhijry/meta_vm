#if !defined(METAVM_HPP)
#define METAVM_HPP

#include "common.hpp"
#include "types.hpp"

struct MetaVM {
    MetaVM (
        memory_view<VMInstruction> &bytecode,
        memory_view<u8> &memory,
        array_view<VMException> &exceptions
    ) :      _bytecode(bytecode),
                 _memory(memory),
         _exceptions(exceptions)
    {
        _registers.stackPointer = _memory.size();
    }

    void run();
    void printRegisters();
    void printMemory();
    void printExceptions();
    void printAll(); 

private:
    VMRegisters                  _registers {};
    memory_view<VMInstruction>   &_bytecode;
    memory_view<u8>                &_memory;
    array_view<VMException>    &_exceptions;

    u64 getIndirect(VMOperand const &operand) const;
    u64 getDisplaced(VMOperand const &operand) const;
    VMWord &getRegister(VMOperand const &operand);
    VMWord &getMemoryFromPointer(VMOperand const &operand);
    VMWord &getMemoryFromIndirect(VMOperand const &operand);
    VMWord &getMemoryFromDisplaced(VMOperand const &operand);
    VMWord &getStackTop() const;
    VMWord &getVMWord(VMOperand &operand);

    void mov(VMInstruction &inst);
    void push(VMInstruction &inst);
    void pop(VMInstruction &inst);
    void add(VMInstruction &inst);
    void sub(VMInstruction &inst);
    void mul(VMInstruction &inst);
    void div(VMInstruction &inst);
    void divr(VMInstruction &inst);
    void adds(VMInstruction &inst);
    void subs(VMInstruction &inst);
    void muls(VMInstruction &inst);
    void divs(VMInstruction &inst);
    void divsr(VMInstruction &inst);
    void addf(VMInstruction &inst);
    void subf(VMInstruction &inst);
    void mulf(VMInstruction &inst);
    void divf(VMInstruction &inst);
    void addfs(VMInstruction &inst);
    void subfs(VMInstruction &inst);
    void mulfs(VMInstruction &inst);
    void divfs(VMInstruction &inst);
    void neg(VMInstruction &inst);
    void bitwise_and(VMInstruction &inst);
    void bitwise_or(VMInstruction &inst);
    void bitwise_xor(VMInstruction &inst);
    void bitwise_not(VMInstruction &inst);
    void jmp(VMInstruction &inst);
    void jeq(VMInstruction &inst);
    void jne(VMInstruction &inst);
    void jgt(VMInstruction &inst);
    void jlt(VMInstruction &inst);
    void jge(VMInstruction &inst);
    void jle(VMInstruction &inst);

    VMInstruction fetch();
};

#endif

