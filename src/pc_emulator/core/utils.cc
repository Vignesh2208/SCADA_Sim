#include <iostream>
#include <cstdint>
#include <cstring>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <vector>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <cerrno>


#include "src/pc_emulator/include/pc_variable.h"
#include "src/pc_emulator/include/pc_datatype.h"
#include "src/pc_emulator/include/pc_configuration.h"
#include "src/pc_emulator/include/pc_resource.h"
#include "src/pc_emulator/include/utils.h"
#include "src/pc_emulator/proto/configuration.pb.h"

using namespace std;
using namespace pc_emulator;
using namespace pc_specification;

using MemType  = pc_specification::MemType;
using DataTypeCategory = pc_specification::DataTypeCategory;
using FieldIntfType = pc_specification::FieldInterfaceType;
using PoUType = pc_specification::PoUType;
using FieldQualifiers = pc_specification::FieldQualifiers;

#define STRING(s) #s



bool Utils::ReadAccessCheck(PCConfiguration * configuration,
        string CallingPoUType, string NestedFieldName) {
    auto CallingPoU = configuration->LookupDataType(CallingPoUType);

    assert(!NestedFieldName.empty());
    std::vector<string> results;
    bool AccessedPoUField = false;

    boost::split(results, NestedFieldName,
                boost::is_any_of("."), boost::token_compress_on);

    if (!CallingPoU) {
        configuration->PCLogger->LogMessage(LogLevels::LOG_INFO,
            "READ: CallingPoU: " + CallingPoUType + " not found !");
        return false;
    }

    if (!CallingPoU->IsFieldPresent(NestedFieldName)){
        configuration->PCLogger->LogMessage(LogLevels::LOG_INFO,
            "READ: NestedField: " + NestedFieldName + " not found !");
        return false;
    }

    string CurrNestedFieldName = "";
    for (int i = 0; i < results.size(); i++) {
        
        PCDataTypeField Result;

        
        CurrNestedFieldName += (results[i] + ".");
           
        CallingPoU->GetPCDataTypeField(CurrNestedFieldName, Result);

        if (CallingPoUType != Result.__HoldingPoUType) {

            if (Result.__FieldTypeCategory == DataTypeCategory::POU
                && !AccessedPoUField) {
                AccessedPoUField = true;

                continue;

            } else {
                if (Result.__FieldInterfaceType 
                        == FieldInterfaceType::VAR_OUTPUT ||
                    Result.__FieldInterfaceType 
                        == FieldInterfaceType::VAR_IN_OUT ||
                    Result.__FieldInterfaceType 
                        == FieldInterfaceType::VAR_EXPLICIT_STORAGE ||
                    Result.__FieldInterfaceType 
                        == FieldInterfaceType::VAR_EXTERNAL) {
                    //CallingPoUType = Result.__HoldingPoUType;
                    continue;
                } else {
                    return false;
                }
            }
        } else {
            continue;
        }
    }

    return true;
}

bool Utils::WriteAccessCheck(PCConfiguration * configuration,
        string CallingPoUType, string NestedFieldName) {
    auto CallingPoU = configuration->LookupDataType(CallingPoUType);
    std::vector<string> results;
    bool AccessedPoUField = false;

    boost::split(results, NestedFieldName,
                boost::is_any_of("."), boost::token_compress_on);


 
    if (!CallingPoU) {
        configuration->PCLogger->LogMessage(LogLevels::LOG_INFO,
            "WRITE: CallingPoU: " + CallingPoUType + " not found !");
        return false;
    }

    if (!CallingPoU->IsFieldPresent(NestedFieldName)){
        configuration->PCLogger->LogMessage(LogLevels::LOG_INFO,
            "WRITE: NestedField: " + NestedFieldName + " not found !");
        return false;
    }

    string CurrNestedFieldName = "";
    for (int i = 0; i < results.size(); i++) {
        CurrNestedFieldName += (results[i] + ".");

        PCDataTypeField Result;

        CallingPoU->GetPCDataTypeField(CurrNestedFieldName, Result);

        if (CallingPoUType != Result.__HoldingPoUType) {
            if (Result.__FieldTypeCategory == DataTypeCategory::POU
                && !AccessedPoUField) {
                AccessedPoUField = true;

                if (Result.__FieldInterfaceType 
                        == FieldInterfaceType::VAR_INPUT)
                    return false;
                continue;

            } else {
                if (Result.__FieldInterfaceType 
                        == FieldInterfaceType::VAR_INPUT ||
                    Result.__FieldInterfaceType 
                        == FieldInterfaceType::VAR_IN_OUT ||
                    Result.__FieldInterfaceType 
                        == FieldInterfaceType::VAR_EXTERNAL ||
                    Result.__FieldInterfaceType 
                        == FieldInterfaceType::VAR_EXPLICIT_STORAGE) {
                    //CallingPoUType = Result.__HoldingPoUType;
                    continue;
                } else {
                    return false;
                }
            }
        } else {
            if (Result.__FieldInterfaceType == FieldInterfaceType::VAR_INPUT)
                return false;
            continue;
        }
    }
    return true;
}
bool Utils::does_file_exist(const char * filename){
    std::ifstream infile(filename);
    return infile.good();
}
char * Utils::make_mmap_shared(int nElements, string FileName) {
        constexpr auto PROT_RW = PROT_READ | PROT_WRITE;
        constexpr auto MAP_ALLOC = MAP_SHARED;

        bool file_exists = does_file_exist(FileName.c_str());

        FILE * fp;
        
        if(file_exists)
            fp = fopen((FileName).c_str(), "a+");
        else
            fp = fopen((FileName).c_str(), "w+");

        
        fseek(fp,0,SEEK_END);
        int file_size = ftell(fp);
        std::cout << "Initializing MMAP file: " 
            << FileName 
            << " Does it already exist?: " << file_exists
            << " Current Size: " << file_size
            << " Required Size: " << sizeof(char)*nElements << std::endl;
        if (!file_exists || file_size < sizeof(char)*nElements) {
            
            std::cout << "Setting File to 0s" << std::endl;
            
            //if (file_size < sizeof(char)*nElements) {
                char init = '\0';
                for(int i = 0; i < nElements; i++)
                    fwrite(&init, sizeof(char), 1, fp);
            //}
        }
        //fseek(fp, 0, SEEK_SET);
        int fd = fileno(fp);

        assert (nElements > 0 && fd != -1);
        
        auto ptr = mmap(0,
                        sizeof(char)*nElements, PROT_RW, MAP_ALLOC, fd, 0);
        std::cout << "MMAP Status: " << std::strerror(errno) << '\n';
        fclose(fp);
        return (char *)ptr;
    
        //std::cout << "MMAP failed !" << std::endl;
        //throw std::bad_alloc();
    }


void Utils::ValidatePOUDefinition(PCVariable * POUVar, 
                                            PCConfiguration * configuration) {

    PCDataType * PoUDataType = POUVar->__VariableDataType;
    if (PoUDataType->__PoUType == PoUType::NOA) {
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_ACCESS].size()) {
               configuration->PCLogger->RaiseException("A non POU datatype: "
                + POUVar->__VariableName + " cannot have access fields!");
        }
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_EXPLICIT_STORAGE].size()) {
               configuration->PCLogger->RaiseException("A non POU datatype: "
                + POUVar->__VariableName + " cannot have Directly "
                "Represented fields!");
        }
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_EXTERNAL].size()) {
               configuration->PCLogger->RaiseException("A non POU datatype: "
                + POUVar->__VariableName + " cannot have external fields!");
        }
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_IN_OUT].size()) {
               configuration->PCLogger->RaiseException("A non POU datatype: "
                + POUVar->__VariableName + " cannot have IN_OUT fields!");
        }
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_GLOBAL].size()) {
               configuration->PCLogger->RaiseException("A non POU datatype: "
                + POUVar->__VariableName + " cannot have GLOBAL fields!");
        }
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_INPUT].size()) {
               configuration->PCLogger->RaiseException("A non POU datatype: "
                + POUVar->__VariableName + " cannot have INPUT fields!");
        }
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_OUTPUT].size()) {
               configuration->PCLogger->RaiseException("A non POU datatype: "
                + POUVar->__VariableName + " cannot have OUTPUT fields!");
        }
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR].size()) {
               configuration->PCLogger->RaiseException("A non POU datatype: "
                + POUVar->__VariableName + " cannot have VAR fields!");
        }
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_TEMP].size()) {
               configuration->PCLogger->RaiseException("A non POU datatype: "
                + POUVar->__VariableName + " cannot have VAR_TEMP fields!");
        }
       
        return;

    } else if (PoUDataType->__PoUType == PoUType::FC) {
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_ACCESS].size()) {
               configuration->PCLogger->RaiseException("A FC: "
                + POUVar->__VariableName + " cannot have access fields!");
        }
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_EXPLICIT_STORAGE].size()) {
               configuration->PCLogger->RaiseException("A FC: "
                + POUVar->__VariableName + " cannot have Directly "
                "Represented fields!");
        }
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_EXTERNAL].size()) {
               configuration->PCLogger->RaiseException("A FC: "
                + POUVar->__VariableName + " cannot have external fields!");
        }
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_GLOBAL].size()) {
               configuration->PCLogger->RaiseException("A FC: "
                + POUVar->__VariableName + " cannot have GLOBAL fields!");
        }
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR].size()) {
               configuration->PCLogger->RaiseException("A FC: "
                + POUVar->__VariableName + " cannot have VAR fields!");
        }   
    } else if (PoUDataType->__PoUType == PoUType::FB) {
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_ACCESS].size()) {
               configuration->PCLogger->RaiseException("A FB: "
                + POUVar->__VariableName + " cannot have access fields!");
        }
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_EXPLICIT_STORAGE].size()) {
               configuration->PCLogger->RaiseException("A FB: "
                + POUVar->__VariableName + " cannot have Directly "
                "Represented fields!");
        }
        if (PoUDataType->__FieldsByInterfaceType[
           FieldIntfType::VAR_GLOBAL].size()) {
               configuration->PCLogger->RaiseException("A FB: "
                + POUVar->__VariableName + " cannot have GLOBAL fields!");
        }
    } 
}

bool Utils::IsFieldTypePtr(int FieldInterfaceType) {
    if (FieldInterfaceType == FieldIntfType::VAR_IN_OUT
        || FieldInterfaceType == FieldIntfType::VAR_ACCESS
        || FieldInterfaceType == FieldIntfType::VAR_EXTERNAL
        || FieldInterfaceType == FieldIntfType::VAR_EXPLICIT_STORAGE)
        return true;
    return false;
}

bool Utils::TestEQPtrs(PCVariable * Var1, PCVariable *  Var2) {
    if (!Var1 || !Var2)
        return false;

    if (Var1->__MemoryLocation == Var2->__MemoryLocation
        && Var1->__ByteOffset == Var2->__ByteOffset &&
        Var1->__BitOffset == Var2->__BitOffset)
        return true;
    
    if (Var1->__MemoryLocation == Var2->__MemoryLocation) {
        if (Var1->__ByteOffset == Var2->__ByteOffset)
            std::cout << "Unequal Byte Offset " << std::endl;
        else
            std::cout << "Unequal Bit Offset " << std::endl;
    } else {
        std::cout << "Unequal MemLocations " << std::endl;
    }

    return false;
}

string Utils::GetInitialValueForArrayIdx(int Idx, string InitialValue,
                                            PCDataType * ElementDataType,
                                            PCConfiguration * configuration) {
    std::vector<string> InitialValues;
    string Init;


    boost::trim_if(InitialValue,boost::is_any_of("\t ,{}"));
    boost::split(InitialValues, InitialValue,
    boost::is_any_of(",{}"), boost::token_compress_on);
    if (!InitialValue.empty() &&  Idx < InitialValues.size()) {
        Init = InitialValues[Idx];
    }
    else { 
        auto got 
        = configuration->__DataTypeDefaultInitialValues.find(
                ElementDataType->__DataTypeCategory);
        assert(got 
            != configuration->__DataTypeDefaultInitialValues.end());
        Init = got->second;
    }
    return Init;
}

string Utils::GetInstallationDirectory() {

    string InstallationDir = getpwuid(getuid())->pw_dir;
    return InstallationDir + "/OpenSCADA";
}

string Utils::GetElementaryDataTypeName(int Category) {
    switch(Category) {
        case DataTypeCategory::BOOL :   
                        return "BOOL";
        case DataTypeCategory::BYTE :     
                        return "BYTE";
        case DataTypeCategory::WORD :
                        return "WORD";  
        case DataTypeCategory::DWORD : 
                        return "DWORD";    
        case DataTypeCategory::LWORD :  
                        return "LWORD";  
        case DataTypeCategory::CHAR : 
                        return "CHAR";   
        case DataTypeCategory::INT :
                        return "INT";      
        case DataTypeCategory::SINT : 
                        return "SINT";    
        case DataTypeCategory::DINT : 
                        return "DINT";    
        case DataTypeCategory::LINT : 
                        return "LINT";    
        case DataTypeCategory::UINT :  
                        return "UINT";   
        case DataTypeCategory::USINT : 
                        return "USINT";    
        case DataTypeCategory::UDINT : 
                        return "UDINT";    
        case DataTypeCategory::ULINT :  
                        return "ULINT";   
        case DataTypeCategory::REAL : 
                        return "REAL";   
        case DataTypeCategory::LREAL :
                        return "LREAL";     
        case DataTypeCategory::TIME :
                        return "TIME";     
        case DataTypeCategory::DATE :
                        return "DATE";     
        case DataTypeCategory::TIME_OF_DAY : 
                        return "TOD";    
        case DataTypeCategory::DATE_AND_TIME :  
                        return "DT";   
    }
    return "NA";
}

int Utils::GetVarOpType(int varop) {
    switch(varop) {
        case VariableOps::ADD:
        case VariableOps::SUB:
        case VariableOps::MUL:
        case VariableOps::DIV:
        case VariableOps::MOD:    return VarOpType::ARITHMETIC;

        case VariableOps::AND:
        case VariableOps::OR:
        case VariableOps::XOR:
        case VariableOps::LS:
        case VariableOps::RS:     return VarOpType::BITWISE;

        default :   return VarOpType::RELATIONAL;
    }
}

bool Utils::ExtractFromStorageSpec(string StorageSpec, 
                        int * memType, int * ByteOffset, int * BitOffset) {
    if (!boost::starts_with(StorageSpec, "%") || StorageSpec.length() < 4) {
        std::cout << "Incorrect storage specification: " << StorageSpec << std::endl;
        return false;
    }

    assert(memType != nullptr && ByteOffset != nullptr && BitOffset != nullptr);
    if (StorageSpec[1] == 'M') 
        *memType = (int)MemType::RAM_MEM;
    else if (StorageSpec[1] == 'I') 
        *memType = (int)MemType::INPUT_MEM;
    else if (StorageSpec[1] == 'Q') 
        *memType = (int)MemType::OUTPUT_MEM;
    else
        return false;

    if (StorageSpec[2] == 'W') {
        *ByteOffset = std::stoi(StorageSpec.substr(3,  string::npos));
        *BitOffset = 0;
    } else {
        *ByteOffset = std::stoi(StorageSpec.substr(2, StorageSpec.find('.')));
        *BitOffset = std::stoi(StorageSpec.substr(StorageSpec.find('.') + 1,
                                                    string::npos));
    }

    
    return true;
}


bool Utils::ExtractFromAccessStorageSpec(PCConfiguration * __configuration,
                        string StorageSpec,  int * memType, int * ByteOffset,
                        int * BitOffset, string& CandidateResourceName) {

    CandidateResourceName = "NONE";
    if (StorageSpec.length() < 4)
        return false;

    assert(memType != nullptr && ByteOffset != nullptr && BitOffset != nullptr);

    if (boost::starts_with(StorageSpec,"%")) {
        assert(StorageSpec[1] == 'M');
        *memType = (int)MemType::RAM_MEM;
        if (StorageSpec[2] == 'W') {
            *ByteOffset = std::stoi(StorageSpec.substr(3,  string::npos));
            *BitOffset = 0;
        } else {
            *ByteOffset = std::stoi(StorageSpec.substr(2, StorageSpec.find('.')));
            *BitOffset = std::stoi(StorageSpec.substr(StorageSpec.find('.') + 1,
                                                        string::npos));
        }
        std::cout << "Storage Spec: " << StorageSpec << " MemType: " << *memType
            << " Byte: " << *ByteOffset << " Bit: " << *BitOffset << std::endl;

        return true;
    } else {
        std::vector<string> results;
        boost::split(results, StorageSpec, [](char c){return c == '.';});

        if(results.size() == 1) {// no dot found, this must be some field of global variable
            return false;
        } else {
            string candidate_resource = results[0];
            PCResource * resource = 
                __configuration->RegisteredResources->GetResource(
                                                    candidate_resource);

            if (resource == nullptr) {// no resource by this name, must be some
                                     // nested field of global variable
                return false;
            } else {
                string RemStorageSpec = StorageSpec.substr(
                    StorageSpec.find('.') + 1, string::npos);
                CandidateResourceName = candidate_resource;
                if (!Utils::ExtractFromStorageSpec(RemStorageSpec, memType,
                            ByteOffset, BitOffset))
                    return false; // this must be a nested field of some resource variable
                else
                    return true;
            }
        }
    }

    return false;
}

string Utils::ResolveAliasName(string AliasName,
                                        PCConfiguration * __configuration) {
    PCDataType * field_type_ptr 
                = __configuration->LookupDataType(AliasName);
    while (true) {
        if (field_type_ptr->__AliasName != field_type_ptr->__DataTypeName) {
            field_type_ptr 
                = __configuration->LookupDataType(
                            field_type_ptr->__DataTypeName);
            assert(field_type_ptr != nullptr);
        } else {
            return field_type_ptr->__DataTypeName;
        }
    }                                    
}

void Utils::InitializeDataType(PCConfiguration * __configuration,
                            PCDataType * __new_data_type,
                            const pc_specification::DataType& DataTypeSpec) {
    for (auto& field : DataTypeSpec.datatype_field()) {

        string initial_value;
        s64 range_min;
        s64 range_max;
        int field_qualifier;
        string field_datatype_name = field.field_datatype_name();

        
        PCDataType * field_type_ptr 
            = __configuration->LookupDataType(field.field_datatype_name());

        assert(field_type_ptr != nullptr);
        initial_value = field.has_initial_value() ? field.initial_value()
                                : field_type_ptr->__InitialValue;
        range_min = field.has_range_min() ? field.range_min() 
                                : field_type_ptr->__RangeMin;
        range_max = field.has_range_max() ? field.range_max()
                                : field_type_ptr->__RangeMax;

        field_qualifier = field.has_field_qualifier() ? field.field_qualifier()
                                : FieldQualifiers::NONE;

        // READ_ONLY and READ_WRITE can only be specified for access variables
        if (field_qualifier == FieldQualifiers::READ_ONLY
            || field_qualifier == FieldQualifiers::READ_WRITE)
            field_qualifier = FieldQualifiers::NONE;

        // field qualifiers R_EDGE and F_EDGE can only be specified for
        // VAR_INPUT or VAR_EXPLICIT_STORAGE bool variables 
        if (field_type_ptr->__DataTypeCategory != DataTypeCategory::BOOL
            || field.intf_type() != FieldIntfType::VAR_INPUT
            || field.intf_type() != FieldIntfType::VAR_EXPLICIT_STORAGE) {
            field_qualifier = FieldQualifiers::NONE; 
        }
        

        
        if (field.has_initial_value())
            assert(field.intf_type() != VAR_EXPLICIT_STORAGE
                    && field.intf_type() != VAR_ACCESS
                    && field.intf_type() != VAR_EXTERNAL);
    

        if (field.intf_type() != FieldIntfType::VAR_EXPLICIT_STORAGE) {
            if (field.has_dimension_1() && !field.has_dimension_2()) {
                __new_data_type->AddArrayDataTypeField(field.field_name(),
                        field_datatype_name, field.dimension_1(),
                        initial_value,
                        field.intf_type(),
                        field_qualifier,
                        range_min,
                        range_max);
            } else if (field.has_dimension_1() && field.has_dimension_2()) {

                __new_data_type->AddArrayDataTypeField(field.field_name(),
                        field_datatype_name, field.dimension_1(),
                        field.dimension_2(),
                        initial_value,
                        field.intf_type(),
                        field_qualifier,
                        range_min,
                        range_max);

            } else {
                __new_data_type->AddDataTypeField(field.field_name(),
                        field_datatype_name, initial_value,
                        field.intf_type(),
                        field_qualifier,
                        range_min,
                        range_max);
            }
        }
        else if (field.intf_type() 
                    == FieldIntfType::VAR_EXPLICIT_STORAGE
                && field.has_field_storage_spec()) {
                
                assert(DataTypeSpec.datatype_category() 
                        == DataTypeCategory::POU);
                assert(DataTypeSpec.pou_type() 
                        == pc_specification::PoUType::PROGRAM);
            int mem_type = 0;
            int ByteOffset = 0;
            int BitOffset = 0;
            if (field.field_storage_spec().has_full_storage_spec()) {
                //extract memtype, byte and bit offsets from string specification
                
                if (!Utils::ExtractFromStorageSpec(
                        field.field_storage_spec().full_storage_spec(),
                        &mem_type, &ByteOffset, &BitOffset))
                    __configuration->PCLogger->RaiseException(
                        "Incorrectly formatted storage specification !");

            } else {
                mem_type = (int)field.field_storage_spec().mem_type();
                ByteOffset = field.field_storage_spec().byte_offset();
                BitOffset = field.field_storage_spec().bit_offset();
            }

            if (field_type_ptr->__DataTypeCategory != DataTypeCategory::BOOL)
                assert(BitOffset == 0); //ignore bit offset
            if (field.has_dimension_1() && !field.has_dimension_2()) {
                __new_data_type->AddArrayDataTypeFieldAT(field.field_name(),
                        field_datatype_name, field.dimension_1(),
                        initial_value,
                        field_qualifier,
                        range_min,
                        range_max,
                        mem_type, ByteOffset, BitOffset);
            } else if (field.has_dimension_1() && field.has_dimension_2()) {

                __new_data_type->AddArrayDataTypeFieldAT(field.field_name(),
                        field_datatype_name, field.dimension_1(),
                        field.dimension_2(),
                        initial_value,
                        field_qualifier,
                        range_min,
                        range_max,
                        mem_type, ByteOffset, BitOffset);

            } else {
                __new_data_type->AddDataTypeFieldAT(field.field_name(),
                        field_datatype_name, initial_value,
                        field_qualifier,
                        range_min,
                        range_max,
                        mem_type, ByteOffset, BitOffset);
            }

        }
    }
}

bool Utils::GetFieldAttributesForAccessPath(string AccessPathStorageSpec,
                PCConfiguration * configuration,
                DataTypeFieldAttributes& FieldAttributes){

    assert(!AccessPathStorageSpec.empty());
    std::vector<string> results;

    boost::split(results, AccessPathStorageSpec,
                boost::is_any_of("."), boost::token_compress_on);
    std::cout << "Resource Name: " << results[0] << std::endl;
    PCResource * resource = configuration->RegisteredResources->GetResource(
                        results[0]);
    if (resource == nullptr) {
        if (configuration->__global_pou_var != nullptr
        && configuration->__global_pou_var->__VariableDataType->IsFieldPresent(
                                                AccessPathStorageSpec)){
            configuration->__global_pou_var->GetFieldAttributes(
                        AccessPathStorageSpec, FieldAttributes);
            return true;
        }
        std::cout << "Resource is NULL!" << std::endl;
        return false;
        
    } else {
        
        if (results.size() <= 2) {
            configuration->PCLogger->RaiseException(
            "Improperly formatted AccessPathStorageSpec: "
            + AccessPathStorageSpec);
        }

        string PoUName = results[1];

        string PoUVariableName = AccessPathStorageSpec.substr(
                        results[0].length() + results[1].length() + 2,
                        string::npos);

        std::cout << "POU NAME: = " << PoUName << " VarName: "
                << PoUVariableName << std::endl;

        auto pou_var = resource->__ResourcePoUVars.find(PoUName)->second.get();
        if (pou_var == nullptr) {
            configuration->PCLogger->RaiseException(
                "POU NAME: " + PoUName + " not found !");
        }

        if (pou_var->__VariableDataType->IsFieldPresent(PoUVariableName)) {
            DataTypeFieldAttributes FieldAttributes;
            pou_var->GetFieldAttributes(PoUVariableName, FieldAttributes);
            return true;
        } 
    }
    return false;

};

PCVariable * Utils::GetVariable(string NestedFieldName,
                PCConfiguration * configuration) {
    auto desired_variable = configuration->GetExternVariable(NestedFieldName);

    if(desired_variable)
        return desired_variable;

    std::vector<string> results;

    boost::split(results, NestedFieldName,
                boost::is_any_of("."), boost::token_compress_on);
    DataTypeFieldAttributes FieldAttributes;
    PCResource * resource = configuration->RegisteredResources->GetResource(
                        results[0]);
    if (resource == nullptr || results.size() <= 2) {
        configuration->PCLogger->LogMessage(LogLevels::LOG_INFO,
            resource == nullptr ? "Resource: " + results[0] + " Not found!" :
            "Nested FieldName: " + NestedFieldName + " in incorrect format !");
        return nullptr;
    }


    // NestedFieldName of the form: resource_name.pou_name.nested_variable_name
    string PoUName = results[1];

    string PoUVariableName = NestedFieldName.substr(
                    results[0].length() + results[1].length() + 2,
                    string::npos);

    std::cout << "POU NAME: = " << PoUName << " VarName: "
                << PoUVariableName << std::endl;

    auto pou_var = resource->__ResourcePoUVars.find(PoUName)->second.get();
    if (pou_var == nullptr) {
        configuration->PCLogger->LogMessage(LogLevels::LOG_INFO,
            "POU NAME: " + PoUName + " not found !");
        return nullptr;
    }

    if (pou_var->__VariableDataType->IsFieldPresent(PoUVariableName)) {
        DataTypeFieldAttributes FieldAttributes;
        pou_var->GetFieldAttributes(PoUVariableName, FieldAttributes);
        if (!Utils::IsFieldTypePtr(
            FieldAttributes.FieldDetails.__FieldInterfaceType))
            return pou_var->GetPtrToField(PoUVariableName);
        else
            return pou_var->GetPtrStoredAtField(PoUVariableName);
    } else {
        configuration->PCLogger->LogMessage(LogLevels::LOG_INFO,
            "POU NAME: " + PoUName + " Variable: " + PoUVariableName 
            + " not found !");
        return nullptr;
    }
}

void Utils::InitializeAccessDataType(PCConfiguration * __configuration,
                                PCDataType * __new_data_type,
                                const pc_specification::DataType& DataTypeSpec) {

    for (auto& field : DataTypeSpec.datatype_field()) {
        string initial_value;
        s64 range_min;
        s64 range_max;
        int field_qualifier;
        string field_datatype_name = field.field_datatype_name();

        
        PCDataType * field_type_ptr 
            = __configuration->LookupDataType(field.field_datatype_name());

        assert(field_type_ptr != nullptr);
        initial_value = field.has_initial_value() ? field.initial_value()
                                : field_type_ptr->__InitialValue;
        range_min = field.has_range_min() ? field.range_min() 
                                : field_type_ptr->__RangeMin;
        range_max = field.has_range_max() ? field.range_max()
                                : field_type_ptr->__RangeMax;

        field_qualifier = field.has_field_qualifier() 
                ? field.field_qualifier()
                : FieldQualifiers::READ_ONLY;

        // R_EDGE and F_EDGE cannot be specified for access variables
        if (field_qualifier == FieldQualifiers::R_EDGE
            || field_qualifier == FieldQualifiers::F_EDGE)
            field_qualifier = FieldQualifiers::READ_ONLY;

        assert(!field.has_initial_value());
                

        int mem_type = 0;
        int ByteOffset = 0;
        int BitOffset = 0;
        string CandidateResourceName;
        if (field.field_storage_spec().has_full_storage_spec()) {
            //extract memtype, byte and bit offsets from string specification

            assert(DataTypeSpec.datatype_category() 
                    == DataTypeCategory::POU);
            assert(DataTypeSpec.pou_type() 
                    == pc_specification::PoUType::PROGRAM);
            
            if (!Utils::ExtractFromAccessStorageSpec(
                    __configuration,
                    field.field_storage_spec().full_storage_spec(),
                    &mem_type, &ByteOffset, &BitOffset,
                    CandidateResourceName)) {

                DataTypeFieldAttributes FieldAttributes;

                assert(Utils::GetFieldAttributesForAccessPath(
                    field.field_storage_spec().full_storage_spec(),
                    __configuration, FieldAttributes));

                if (field.has_dimension_1() && !field.has_dimension_2()) {
                    __new_data_type->AddArrayDataTypeField(field.field_name(),
                            field_datatype_name, field.dimension_1(),
                            initial_value,
                            //field.intf_type(),
                            FieldInterfaceType::VAR_IN_OUT,
                            field_qualifier,
                            range_min,
                            range_max,
                            field.field_storage_spec().full_storage_spec());
                } else if (field.has_dimension_1() && field.has_dimension_2()) {

                    __new_data_type->AddArrayDataTypeField(field.field_name(),
                            field_datatype_name, field.dimension_1(),
                            field.dimension_2(),
                            initial_value,
                            //field.intf_type(),
                            FieldInterfaceType::VAR_IN_OUT,
                            field_qualifier,
                            range_min,
                            range_max,
                            field.field_storage_spec().full_storage_spec());

                } else {

                    if (FieldAttributes
                        .FieldDetails.__FieldTypeCategory 
                        != DataTypeCategory::ARRAY) {

                        __new_data_type->AddDataTypeField(
                            field.field_name(),
                            field_datatype_name, initial_value,
                            //field.intf_type(),
                            FieldInterfaceType::VAR_IN_OUT,
                            field_qualifier,
                            range_min,
                            range_max,
                            field.field_storage_spec().full_storage_spec());
                    } else if(FieldAttributes.FieldDetails
                            .__NDimensions == 2){
                        __new_data_type->AddArrayDataTypeField(
                            field.field_name(),
                            FieldAttributes
                            .FieldDetails.__FieldTypePtr->__DataTypeName,
                            FieldAttributes.FieldDetails.__Dimension1,
                            FieldAttributes.FieldDetails.__Dimension2,
                            initial_value,
                            //field.intf_type(),
                            FieldInterfaceType::VAR_IN_OUT,
                            field_qualifier,
                            range_min,
                            range_max,
                            field.field_storage_spec().full_storage_spec());
                    } else {
                        __new_data_type->AddArrayDataTypeField(
                            field.field_name(),
                            FieldAttributes
                            .FieldDetails.__FieldTypePtr->__DataTypeName,
                            FieldAttributes.FieldDetails.__Dimension1,
                            initial_value,
                            //field.intf_type(),
                            FieldInterfaceType::VAR_IN_OUT,
                            field_qualifier,
                            range_min,
                            range_max,
                            field.field_storage_spec().full_storage_spec());
                    }
                }


                continue; // these fields are added as a pointer. we will
                            // set these pointers later when a variable of this
                            // data type is created.
            }

        } else {

            assert(DataTypeSpec.datatype_category() 
                    == DataTypeCategory::POU);
            assert(DataTypeSpec.pou_type() 
                    == pc_specification::PoUType::PROGRAM);

            mem_type = (int)field.field_storage_spec().mem_type();
            ByteOffset = field.field_storage_spec().byte_offset();
            BitOffset = field.field_storage_spec().bit_offset();

            assert(field.field_storage_spec().mem_type() == MemType::RAM_MEM);
        }

        if (field_type_ptr->__DataTypeCategory != DataTypeCategory::BOOL)
                assert(BitOffset == 0); //ignore bit offset

        if (field.has_dimension_1() && !field.has_dimension_2()) {
            __new_data_type->AddArrayDataTypeFieldAT(field.field_name(),
                    field_datatype_name, field.dimension_1(),
                    initial_value,
                    field_qualifier,
                    range_min,
                    range_max,
                    mem_type, ByteOffset, BitOffset, CandidateResourceName,
                    field.field_storage_spec().full_storage_spec());
        } else if (field.has_dimension_1() && field.has_dimension_2()) {

            __new_data_type->AddArrayDataTypeFieldAT(field.field_name(),
                    field_datatype_name, field.dimension_1(),
                    field.dimension_2(),
                    initial_value,
                    field_qualifier,
                    range_min,
                    range_max,
                    mem_type, ByteOffset, BitOffset, CandidateResourceName,
                    field.field_storage_spec().full_storage_spec());

        } else {
            __new_data_type->AddDataTypeFieldAT(field.field_name(),
                    field_datatype_name,
                    initial_value,
                    field_qualifier,
                    range_min,
                    range_max,
                    mem_type, ByteOffset, BitOffset, CandidateResourceName,
                    field.field_storage_spec().full_storage_spec());
        }

    }

}