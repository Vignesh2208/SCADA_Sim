
#include <iostream>
#include <cstdint>
#include <cstring>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <vector>
#include "src/pc_emulator/include/pc_configuration.h"
#include "src/pc_emulator/include/pc_resource.h"
#include "src/pc_emulator/include/executor.h"
#include "src/pc_emulator/include/task.h"
#include "src/pc_emulator/include/utils.h"

using namespace std;
using namespace pc_emulator;
using namespace pc_specification;

using MemType  = pc_specification::MemType;
using DataTypeCategory = pc_specification::DataTypeCategory;
using FieldInterfaceType = pc_specification::FieldInterfaceType;


void Executor::SetExecPoUVariable(PCVariable* ExecPoUVariable) {
    __ExecPoUVariable = ExecPoUVariable;
    __CodeContainer 
            = __AssociatedResource->GetCodeContainer(
                ExecPoUVariable->__VariableDataType->__DataTypeName);
    if (!__CodeContainer) {
        __configuration->PCLogger->RaiseException("Error initializing executor"
                " cannot get required code container!");
    }

    if (!__ExecPoUVariable) {
        __configuration->PCLogger->RaiseException("Error initializing executor"
                " Exec PoU variable is null!");
    }
    __Initialized = true;
};

void Executor::Run() {
    assert(__Initialized);

    __ExecPoUVariable->OnExecutorStartup();
    int idx = 0;
    while (true) {
        auto insn_container = __CodeContainer->GetInsn(idx);
        idx = RunInsn(*insn_container);
        SaveCPURegisters();
        if (__AssociatedTask->type == TaskType::INTERRUPT) {
            Task * EligibleTask = nullptr;
            do {
                EligibleTask = __AssociatedResource->GetInterruptTaskToExecute();
                if (EligibleTask != nullptr
                    && EligibleTask->__priority < __AssociatedTask->__priority) {
                    EligibleTask->Execute();
                }

            } while(EligibleTask != nullptr);
            
            
        } else {
            Task * EligibleTask = nullptr;
            do {
                EligibleTask = __AssociatedResource->GetInterruptTaskToExecute();
                if (EligibleTask != nullptr)
                    EligibleTask->Execute();

            } while(EligibleTask != nullptr);

            do {
                EligibleTask 
                    = __AssociatedResource->GetIntervalTaskToExecuteAt(
                        __AssociatedResource->clock->GetCurrentTime());
                if (EligibleTask != nullptr) {
                    EligibleTask->Execute();
                    EligibleTask->__nxt_schedule_time_ms 
                                += EligibleTask->__interval_ms;
                    __AssociatedResource->QueueTask(EligibleTask); //requeue   
                }

            } while(EligibleTask != nullptr);

        }

        RestoreCPURegisters();
        if (idx < 0)
            break;

    }
}

void Executor::CleanUp() {
    __CR->Cleanup();
    delete __CR;
}

// returns -1 to exit executor
int Executor::RunInsn(InsnContainer& insn_container) {
    // this might create and run other executors as well

    // update clock here as well.
    //__AssociatedResource->clock->UpdateCurrentTime(1);
    return -1;
}


void Executor::SaveCPURegisters() {
    *__CR = *__AssociatedResource->__CurrentResult;
}

void Executor::RestoreCPURegisters() {
    *__AssociatedResource->__CurrentResult = *__CR;
}

void Executor::ResetCPURegisters() {
    __AssociatedResource->__CurrentResult->__configuration = __configuration;
    __AssociatedResource->__CurrentResult->__VariableDataType
         = __configuration->RegisteredDataTypes->GetDataType("BOOL");
    __AssociatedResource->__CurrentResult->__AssociatedResource 
            = __AssociatedResource;
    __AssociatedResource->__CurrentResult->__ByteOffset = 0;
    __AssociatedResource->__CurrentResult->__BitOffset = 0;
    __AssociatedResource->__CurrentResult->__VariableName = "__CurrentResult";
    
    __AssociatedResource->__CurrentResult->__MemAllocated = false;
    __AssociatedResource->__CurrentResult->__IsDirectlyRepresented = false;
    __AssociatedResource->__CurrentResult->__IsVariableContentTypeAPtr = false;
    __AssociatedResource->__CurrentResult->AllocateAndInitialize();
}