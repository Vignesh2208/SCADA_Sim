#ifndef __PC_EMULATOR_INCLUDE_PC_VARIABLE_H__
#define __PC_EMULATOR_INCLUDE_PC_VARIABLE_H__
#include <iostream>
#include <memory>
#include <unordered_map>
#include "pc_datatype.h"
#include "pc_mem_unit.h"

using namespace std;

namespace pc_emulator {

    class PCConfiguration;
    class PCResource;
    class PCVariable;

    enum VariableOps {
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        AND,
        OR,
        XOR,
        LS,
        RS,
        EQ,
        GT,
        LT,
        GE,
        LE
    };  

    enum VarOpType {
        RELATIONAL,
        ARITHMETIC,
        BITWISE
    };

    typedef struct DataTypeFieldAttributesStruct {
        string NestedFieldName;
        unsigned long RelativeOffset;
        PCDataTypeField FieldDetails;
        PCVariable* HoldVariablePtr;
        PCVariable* ParentVariablePtr;
    } DataTypeFieldAttributes;

    class PCVariable {

        private:
            

            void CopyToPCVariableFieldFromPointer(DataTypeFieldAttributes&
                                                Attributes, PCVariable * From);
            void GetAndStoreValue(string NestedFieldName, void * Value,
                                int CopySize, int CategoryOfDataType);
            void CheckOperationValidity(int CategoryOfDataType, int VarOp);

            template <typename T> bool ArithmeticOpOnVariables(T var1, T var2,
                                        int CategoryOfDataType, int VarOp);
            template <typename T> bool RelationalOpOnVariables(T var1, T var2,
                                        int CategoryOfDataType, int VarOp);
            template <typename T> bool BitwiseOpOnVariables(T var1, T var2,
                                        int CategoryOfDataType, int VarOp);
            template <typename T> bool AllOpsOnVariables(T var1, T var2,
                                        int CategoryOfDataType, int VarOp);

            

            void InitializeVariable(PCVariable * V, string InitialValue);
            void InitializeAllNonPtrFields();
            void InitializeAllDirectlyRepresentedFields();
            
            void CheckValidity();
            void ParseRemFieldAttributes(std::vector<string>& Fields,
                        int StartPos, DataTypeFieldAttributes& FieldAttributes,
                        PCVariable * HolderVariable, PCDataType * Current);

            
            void SafeMemRead(void * dst, PCMemUnit * From, int CopySizeBytes,
                                int Offset);
            void SafeMemWrite(PCMemUnit * To, int CopySizeBytes, int Offset,
                            void * src);
            bool SafeBitRead(PCMemUnit * From, int ByteOffset, int BitOffset);
            void SafeBitWrite(PCMemUnit * To, int ByteOffset, int BitOffset,
                            bool value);
            

        public:
            int __ByteOffset;
            int __BitOffset;
            int __TotalSizeInBits;

            bool __IsVariableContentTypeAPtr; // 1 - pointer, 0 - non_pointer
            bool __PrevValue;
            string __VariableName;
            PCDataType* __VariableDataType;
            PCMemUnit __MemoryLocation;
            PCConfiguration * __configuration;
            PCResource * __AssociatedResource; // if null, associated with all
                                               // resources or entire configuration
            std::unordered_map<std::string,
            std::unique_ptr<PCVariable>> __AccessedFields;
            bool __MemAllocated, __IsDirectlyRepresented;
            DataTypeFieldAttributes __VariableAttributes;


            PCVariable(PCConfiguration * configuration,
                    PCResource * AssociatedResource,
                    string VariableName,
                    string VariableDataTypeName);

            void Cleanup(); 
            void AllocateStorage();
            void AllocateStorage(string SharedMemFileName);   
            void AllocateAndInitialize();
            void AllocateAndInitialize(string SharedMemFileName);
            void ResolveAllExternalFields();
            void OnExecutorStartup();

            std::unique_ptr<PCVariable> GetCopy();
            

            void SetField(string NestedFieldName, string value);
            void SetField(string NestedFieldName, void * value,
                            int CopySizeBytes);
            void SetField(string NestedFieldName, PCVariable * From);

            PCVariable* GetPtrToField(string NestedFieldName);
            template <typename T> T GetValueStoredAtField(string NestedFieldName,
                                                        int CategoryOfDataType);
            PCVariable * GetPtrStoredAtField(string NestedFieldName);
            void GetFieldAttributes(string NestedFieldName, 
                            DataTypeFieldAttributes& FieldAttributes);


            bool InitiateOperationOnVariables(PCVariable& V, int VarOp);
            PCVariable& operator=(PCVariable& V);
            PCVariable& operator+(PCVariable& V );
            PCVariable& operator-(PCVariable& V );
            PCVariable& operator/(PCVariable& V );
            PCVariable& operator*(PCVariable& V );
            PCVariable& operator%(PCVariable& V );
            PCVariable& operator&(PCVariable& V );
            PCVariable& operator|(PCVariable& V );
            PCVariable& operator^(PCVariable& V );
            PCVariable& operator<<(PCVariable& V );
            PCVariable& operator>>(PCVariable& V );
            PCVariable& operator!();


            friend bool operator==(PCVariable& V1, PCVariable& V2) {
                int CopySize = V1.__TotalSizeInBits / 8;

                if (V1.__VariableDataType->__DataTypeCategory 
                        == DataTypeCategory::ARRAY
                    || V1.__VariableDataType->__DataTypeCategory 
                        == DataTypeCategory::DERIVED
                    || V1.__VariableDataType->__DataTypeCategory 
                        == DataTypeCategory::POU) {
                    if (V1.__TotalSizeInBits != V2.__TotalSizeInBits)
                        return false;
                    for (int i = 0 ; i < CopySize; i++) {
                        char * val1 
                            = V1.__MemoryLocation.GetPointerToMemory(
                                V1.__ByteOffset + i);
                        char * val2 = V2.__MemoryLocation.GetPointerToMemory(
                                V2.__ByteOffset + i);
                        if (*val1 != *val2)
                            return false;
                    }
                    return true;
                }
                
                return V1.InitiateOperationOnVariables(V2, VariableOps::EQ);
            }

            friend bool operator>(PCVariable& V1, PCVariable& V2) {
                return V1.InitiateOperationOnVariables(V2, VariableOps::GT);
            }

            friend bool operator>=(PCVariable& V1, PCVariable& V2) {
                return V1.InitiateOperationOnVariables(V2, VariableOps::GE);
            }

            friend bool operator<(PCVariable& V1, PCVariable& V2) {
                return V1.InitiateOperationOnVariables(V2, VariableOps::LT);
            }

            friend bool operator<=(PCVariable& V1, PCVariable& V2) {
                return V1.InitiateOperationOnVariables(V2, VariableOps::LE);
            }

            
    };
    
}

#endif