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

    VMInstruction fetch();
};

#endif

