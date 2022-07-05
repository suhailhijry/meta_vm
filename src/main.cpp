#include "common.hpp"
#include "types.hpp"
#include "metavm.hpp"

int main() {
    using TCode = static_array<VMInstruction, 64>;
    using TMemory = static_array<u8, KB(1)>;
    using TExceptions = static_array<VMException, KB(1)>;

    TCode code {};
    TMemory memory {};
    TExceptions exceptions {};
    auto codeView = code.view(0, code.size());
    auto memoryView = memory.view(0, memory.size());
    auto exceptionsView = exceptions.arrayView();
    MetaVM vm { codeView, memoryView, exceptionsView };

    vm.run();
    vm.printRegisters();
    vm.printExceptions();
    vm.printMemory();
}

