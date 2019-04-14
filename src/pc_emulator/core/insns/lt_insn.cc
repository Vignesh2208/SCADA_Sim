#include "src/pc_emulator/include/insns/lt_insn.h"


using namespace std;
using namespace pc_emulator;
using namespace pc_specification;

/*
 * Sets the Current result accumulator to the passed operand.
 */
void LT_Insn::Execute(std::vector<PCVariable*>& Operands, bool isNegated) {
    auto Logger = __AssociatedResource->__configuration->PCLogger.get();

    if (Operands.size() != 1) {
        Logger->RaiseException("LT can take exactly one argument ! "
            " Actual number of arguments provided = " +
            std::to_string(Operands.size()));
        
    }

    PCVariable * Operand = Operands[0];
    assert(Operand != nullptr);
    if (Operand->__IsVariableContentTypeAPtr) {
        Operand = Operand->GetPtrStoredAtField("");
        assert(Operand != nullptr);
    }
    assert(Operand->__VariableDataType->__DataTypeCategory
            != DataTypeCategory::POU);

    assert(Operand->__VariableDataType->__DataTypeCategory
            != DataTypeCategory::DERIVED);

    assert(Operand->__VariableDataType->__DataTypeCategory
            != DataTypeCategory::ARRAY);    
    auto CurrentResult = __AssociatedResource->__CurrentResult;


    if(*CurrentResult  < *Operand) {
        *CurrentResult = * __AssociatedResource->GetTmpVariable("BOOL", "1");
    } else {
       *CurrentResult = * __AssociatedResource->GetTmpVariable("BOOL", "0"); 
    }
}