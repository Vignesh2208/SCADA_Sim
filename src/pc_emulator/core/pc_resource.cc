#include <assert.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <regex>
#include <vector>

#include "pc_emulator/include/pc_datatype.h"
#include "pc_emulator/include/pc_variable.h"
#include "pc_emulator/include/pc_resource.h"
#include "pc_emulator/include/pc_configuration.h"
#include "pc_emulator/include/utils.h"

using namespace std;
using namespace pc_emulator;

void PCResource::RegisterPoUVariable(string VariableName, PCVariable * Var) {
    std::unordered_map<std::string, PCVariable*>::const_iterator got = 
                        __ResourcePoUVars.find (VariableName);
    if (got != __ResourcePoUVars.end()) {
        
        __configuration->PCLogger->RaiseException("Variable already defined !");
    } else {
        __ResourcePoUVars.insert(std::make_pair(VariableName, Var));
       
        __configuration->PCLogger->LogMessage(LOG_LEVELS::LOG_INFO,
                                        "Registered new resource pou variable!");
    }

}

PCVariable * PCResource::GetVariable(string NestedFieldName) {
    assert(!NestedFieldName.empty());
    std::vector<string> results;
    boost::split(results, NestedFieldName, [](char c){return c == '.';});

    if  (results.size() == 1) {
        //not . was found

        std::unordered_map<std::string, PCVariable*>::const_iterator got = 
                        __ResourcePoUVars.find(NestedFieldName);
        if (got == __ResourcePoUVars.end()) {
            
            // this may belong to global variable
            PCVariable * global_var = __ResourcePoUVars.find(
                    "__RESOURCE_" + __ResourceName + " _GLOBAL__")->second;
            assert(global_var != nullptr);
            return global_var->GetPCVariableToField(NestedFieldName);
        } else {
            return got->second;
        }
    } else {
        // dot was found;
        std::unordered_map<std::string, PCVariable*>::const_iterator got = 
                        __ResourcePoUVars.find(results[0]);
        if (got == __ResourcePoUVars.end()) {
            
            // this may belong to global variable
            PCVariable * global_var = __ResourcePoUVars.find(
                    "__RESOURCE_" + __ResourceName + " _GLOBAL__")->second;
            assert(global_var != nullptr);
            return global_var->GetPCVariableToField(NestedFieldName);

        } else {
            PCVariable * Base = got->second;
            assert(Base != nullptr);
            string Field = NestedFieldName.substr(
                    NestedFieldName.find('.') + 1, string::npos);
            return Base->GetPCVariableToField(Field);
        }
    } 
}

void PCResource::InitializeAllPoUVars() {

    
    for (auto & resource_spec : 
            __configuration->__specification.machine_spec().resource_spec()) {
        if (resource_spec.resource_name == __ResourceName) {
            if (resource_spec.has_resource_global_var()) {
                PCDataType * global_var_type = new PCDataType(
                    __configuration, 
                    "__RESOURCE_" + __ResourceName + " _GLOBAL__",
                    "__RESOURCE_" + __ResourceName + " _GLOBAL__",
                    DataTypeCategories::POU);

                __configuration->RegisteredDataTypes.RegisterDataType(
                "__RESOURCE_" + __ResourceName + " _GLOBAL__", global_var_type);

                Utils::InitializeDataType(__configuration, global_var_type,
                        resource_spec.resource_global_var());
                
                PCVariable * __global_pou_var = new PCVariable(__configuration,
                        this, "__RESOURCE_" + __ResourceName + " _GLOBAL_VAR__",
                        "__RESOURCE_" + __ResourceName + " _GLOBAL__");

                RegisterPoUVariable(
                    "__RESOURCE_" + __ResourceName + " _GLOBAL_VAR__",
                    __global_pou_var);

            }

            for (auto& pou_var : resource_spec.pou_var()) {

                assert(pou_var.datatype_category() == DataTypeCategory::POU);
                assert(pou_var.has_pou_type()
                    && (pou_var.pou_type() == PoUType::FC || 
                        pou_var.pou_type() == PoUType::FB ||
                        pou_var.pou_type() == PoUType::PROGRAM));
                         
                PCDataType * new_var_type = new PCDataType(
                    __configuration, 
                    pou_var.name(),
                    pou_var.name(),
                    DataTypeCategories::POU);

                __configuration->RegisteredDataTypes.RegisterDataType(
                                            pou_var.name(), new_var_type);

                Utils::InitializeDataType(__configuration, new_var_type,
                                        pou_var);

                if (pou_var.pou_type() != PoUType::FC) {
                    PCVariable * new_pou_var = new PCVariable(
                        __configuration,
                        this, pou_var.name(), pou_var.name());

                    RegisterPoUVariable(pou_var.name(), new_pou_var);
                }
            }

            break;
        }
    }

}

PCVariable * PCResource::GetVariablePointerToMem(int MemType, int ByteOffset,
                        int BitOffset, string VariableDataTypeName) {

    assert(ByteOffset > 0 && BitOffset >= 0 && BitOffset < 8);
    assert(MemType == MEM_TYPE::INPUT_MEM || MemType == MEM_TYPE::OUTPUT_MEM);
    string VariableName = __ResourceName + std::to_string(MemType)
                            + "." + std::to_string(ByteOffset)
                            + "." + std::to_string(BitOffset);

    // need to track and delete this variable later on
    PCVariable* V = new PCVariable(__configuration, this, VariableName,
                                VariableDataTypeName);
    assert(V != nullptr);

    if(MemType == MEM_TYPE::INPUT_MEM)
        V->__MemoryLocation.SetMemUnitLocation(&__InputMemory);
    else 
        V->__MemoryLocation.SetMemUnitLocation(&__OutputMemory);

    V->__ByteOffset = ByteOffset;
    V->__BitOffset = BitOffset;
    V->__IsDirectlyRepresented = true;

    return V;     
}