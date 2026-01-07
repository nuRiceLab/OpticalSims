#!/bin/bash
## Running events with different gdml files at once
#source ../OpticksEnv
export Build_Dir=build
## override some of the enviroment variables

export OPTICKS_MAX_PHOTON=50000000 # Max Amount of photons
export OPTICKS_MAX_SLOT=$OPTICKS_MAX_PHOTON
export ProjectDir=$HOME/Projects/OpticalSims


export GEOM="dunevd10kt_fullvd_wires"
echo "Simulating ${GEOM} "
nohup "${ProjectDir}/${Build_Dir}/gdml_det" "${ProjectDir}/GDML/dunevd10kt_fullvd_wires.gdml" "${ProjectDir}/macros/${GEOM}.mac" > "${GEOM}.out" 2>&1 &
sleep 5

export GEOM="dune10kt_v6_refactored"
echo "Simulating ${GEOM} "
nohup "${ProjectDir}/${Build_Dir}/gdml_det" "${ProjectDir}/GDML/dune10kt_v6_refactored_nowires.gdml" "${ProjectDir}/macros/${GEOM}.mac " > "${GEOM}.out" 2>&1 &
sleep 5

export GEOM="protodunehd_v6"
echo "Simulating ${GEOM} "
nohup "${ProjectDir}/${Build_Dir}/gdml_det" "${ProjectDir}/GDML/protodunehd_v6_refactored_nowires.gdml" "${ProjectDir}/macros/${GEOM}.mac" > "${GEOM}.out" 2>&1 &
sleep 5

export GEOM="g04"
echo "Simulating ${GEOM} "
nohup "${ProjectDir}/${Build_Dir}/gdml_det" "${ProjectDir}/GDML/dune10kt_v5_refactored_1x2x6_nowires_NoField.gdml" "${ProjectDir}/macros/${GEOM}.mac" > "${GEOM}.out" 2>&1 &
sleep 5