#ifndef __PC_EMULATOR_INCLUDE_SFC_LOG_H
#define __PC_EMULATOR_INCLUDE_SFC_LOG_H
#include <iostream>
#include <fcntl.h>
#include <fstream>
#include <unordered_map>
#include "src/pc_emulator/proto/configuration.pb.h"
#include "src/pc_emulator/include/pc_configuration.h"
#include "src/pc_emulator/include/pc_resource.h"
#include "src/pc_emulator/include/pc_logger.h"
#include "src/pc_emulator/include/sfc/sfc.h"

using namespace std;
using namespace pc_specification;

namespace pc_emulator {

    //! Definition of LOG SFC
    class LOG: public SFC {
        public:

            LOG(PCResourceImpl * AssociatedResource) {
                __AssociatedResource = AssociatedResource;
                __SfcName = "LOG";
            }
            //! Called to execute the sfc
            /*!
                \param CurrentResult    The CurrentResult register
                    of the task executing this SFC
                \param Operands     Operands to the sfc
            */
            void Execute(PCVariable *CurrentResult,
                std::vector<PCVariable*>& Operands);
    };

}


#endif