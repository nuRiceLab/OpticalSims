# basic source code CMakeLists.txt

art_make(
        USE_PROJECT_NAME
        MODULE_LIBRARIES
        larsim::LArG4_LArG4
        larcore::Geometry_Geometry
        nusimdata::SimulationBase
        ${XERCES_C}
        ${XERCES_C_LIBRARIES}
        ${Geant4_LIBRARIES}
        ${ROOT_LIBRARIES}
        Opticks::G4CX
        Opticks::U4
)

install_headers()
install_source()
install_fhicl()
