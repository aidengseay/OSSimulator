// protect from multiple compiling
#ifndef SIMULATOR_H
#define SIMULATOR_H

// header files
#include "StandardConstants.h"
#include "Configops.h"
#include "Metadataops.h"

// function prototypes

/*
Name: runSim
Process: primary simulation driver
Function Input/Parameters: configuration data (configDataType *),
                           metadata (OpCodeType *)
Function Output/Parameters: none
Function Output/Returned: none
Device Input/device: none
Device Output/device: none
Dependencies: tbd 
*/
void runSim(ConfigDataType *configPtr, OpCodeType *metaDataMstrPtr);

#endif // SIMULATOR_H