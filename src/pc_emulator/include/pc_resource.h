#ifndef __PC_EMULATOR_INCLUDE_PC_RESOURCE_H__
#define __PC_EMULATOR_INCLUDE_PC_RESOURCE_H__

#include <iostream>
#include <vector>
#include <climits>
#include <cstdlib>
#include <unordered_map>
#include <queue>
#include "pc_variable.h"
#include "pc_mem_unit.h"
#include "pc_pou_code_container.h"
#include "pc_clock.h"
#include "configuration.h"
#include "resource.h"
#include "task.h"
#include "src/pc_emulator/proto/configuration.pb.h"

using namespace std;
using namespace pc_specification;
using MemType  = pc_specification::MemType;
using DataTypeCategory = pc_specification::DataTypeCategory;
using FieldInterfaceType = pc_specification::FieldInterfaceType;

namespace pc_emulator {
    class PCConfigurationImpl;
    class Task;
    class InsnRegistry;

    class CompactTaskDescription {
        public :
            string __TaskName;
            double __nxt_schedule_time_ms;

            CompactTaskDescription(string TaskName, double nx_schedule_time_ms)
                : __TaskName(TaskName), 
                __nxt_schedule_time_ms(nx_schedule_time_ms) {};

        friend bool operator==(const CompactTaskDescription& b,
                            const CompactTaskDescription& a) {
            return b.__nxt_schedule_time_ms == a.__nxt_schedule_time_ms;
        }

        friend bool operator>(const CompactTaskDescription& b,
                            const CompactTaskDescription& a) {
            return b.__nxt_schedule_time_ms > a.__nxt_schedule_time_ms;
        }

        friend bool operator<(const CompactTaskDescription& b, 
                            const CompactTaskDescription& a) {
            return b.__nxt_schedule_time_ms < a.__nxt_schedule_time_ms;
        }

        friend bool operator<=(const CompactTaskDescription& b,
                            const CompactTaskDescription& a) {
            return b.__nxt_schedule_time_ms <= a.__nxt_schedule_time_ms;
        }

        friend bool operator>=(const CompactTaskDescription& b,
                            const CompactTaskDescription& a) {
            return b.__nxt_schedule_time_ms >= a.__nxt_schedule_time_ms;
        }

    };

    class PCResourceImpl: public PCResource {
        private:

            InsnRegistry *  __InsnRegistry;
            std::unordered_map<std::string,
                    std::unique_ptr<PoUCodeContainer>> __CodeContainers;
            std::unordered_map<std::string, std::unique_ptr<Task>> __Tasks;
            std::unordered_map<int, priority_queue<CompactTaskDescription>> 
                                            __IntervalTasksByPriority;
            std::unordered_map<string, std::vector<Task*>> __InterruptTasks;
                   
            
            PCVariable * GetPOUGlobalVariable(string NestedFieldName);
            void RegisterPoUVariable(string VariableName,
                                    std::unique_ptr<PCVariable> Var);

        public :
            PCConfigurationImpl * __configuration;
            std::unique_ptr<Clock> clock;
            PCVariable * __CurrentResult;
            
            PCResourceImpl(PCConfigurationImpl * configuration,
                string ResourceName, int InputMemSize, int OutputMemSize);
            void InitializeClock() ;
            void InitializeAllPoUVars();
            void ResolveAllExternalFields();
            void InitializeAllTasks();
            void OnStartup();
            void AddTask(std::unique_ptr<Task> Tsk);
            Task * GetTask(string TskName);
            Task * GetInterruptTaskToExecute();
            Task * GetIntervalTaskToExecuteAt(double schedule_time);
            void ExecuteInsn(string InsnName, std::vector<PCVariable*>& Ops,
                            bool isNegated);
            void QueueTask(Task* Tsk);   
            PCVariable * GetExternVariable(string NestedFieldName);
            PCVariable * GetPOU(string PoUName);
            PCVariable * GetVariablePointerToMem(int MemType, int ByteOffset,
                                int BitOffset, string VariableDataTypeName);
            PCVariable * GetTmpVariable(string VariableDataTypeName,
                                        string InitialValue);
            PCVariable * GetVariableForImmediateOperand(string OperandValue);
            PoUCodeContainer * CreateNewCodeContainer(string PoUDataTypeName);
            PoUCodeContainer * GetCodeContainer(string PoUDataTypeName);
            void Cleanup();
    };

}

#endif