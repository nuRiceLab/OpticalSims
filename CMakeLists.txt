cmake_minimum_required(VERSION 3.16...3.27)
project(GDMLOpticks)

# --- 1. Options ---
option(With_Opticks "Enable Opticks GPU Support" OFF)
option(With_LArSoft "Enable LArSoft/Art Integration" OFF)
option(WITH_GEANT4_VIS "Build with Geant4 Vis drivers" ON)


# --- 2. Geant4 Setup ---
if(WITH_GEANT4_VIS)
    if(WITH_LARSOFT)
        find_ups_package(Geant4 REQUIRED gdml vis_all)
    else()
        find_package(Geant4 REQUIRED gdml vis_all)
    endif ()
else()
    if(WITH_LARSOFT)
        find_ups_package(Geant4 REQUIRED gdml)
    else()
        find_package(Geant4 REQUIRED gdml)
    endif ()
endif()

include(${Geant4_USE_FILE})

# --- 3. ROOT Setup ---
if(WITH_LARSOFT)
    find_ups_package(ROOT REQUIRED COMPONENTS RIO Net Tree)
else()
    find_package(ROOT REQUIRED COMPONENTS RIO Net Tree)
endif ()

include(${ROOT_USE_FILE})



# --- 4. Opticks Conditional Setup ---
if(With_Opticks)
    message(STATUS "Opticks Enabled. Home: $ENV{OPTICKS_HOME}")
    list(APPEND CMAKE_MODULE_PATH "$ENV{OPTICKS_HOME}/cmake/Modules")
    message(STATUS "OPTICKS_PREFIX : $ENV{OPTICKS_PREFIX} ")
    include(OpticksBuildOptions)
    message(STATUS "---- OPTICKS ----")
    message(STATUS "CMAKE_MODULE_PATH --> ${CMAKE_MODULE_PATH}")
    message(STATUS "OPTICKS HOME --> $ENV{OPTICKS_HOME}")
    message(STATUS "---- OPTICKS END ----")
    # Use find_package for PLog
    find_package(PLog REQUIRED)
    find_package(G4CX REQUIRED CONFIG)
    find_package(U4 REQUIRED CONFIG)

endif()

# --- 5. LArSoft Conditional Setup ---
if(With_LArSoft)
    find_package(cetmodules 3.24.00 REQUIRED)
    include(CetCMakeEnv)
    cet_cmake_env()
    # -Wno-unused-variable needed because of CLHEP
    cet_set_compiler_flags(DIAGS CAUTIOUS
            WERROR
            NO_UNDEFINED
            EXTRA_FLAGS -pedantic -Wno-unused-local-typedefs -Wno-unused-variable
    )
    cet_report_compiler_flags(REPORT_THRESHOLD VERBOSE)
    cet_cmake_module_directories(Modules BINARY)

    find_package(art REQUIRED)
    find_package(larsim REQUIRED)
    find_package(larcore REQUIRED)
    find_ups_product(xerces_c)
    # loading macros to cmake
    include(ArtMake)
    include(ArtDictionary)
    include(CetMake)
    include(BuildPlugins)
endif()

# --- 6. Include Directories ---
include_directories(${PROJECT_SOURCE_DIR}/include
        ${Geant4_INCLUDE_DIR}
        ${ROOT_INCLUDE_DIRS}:
        ${CMAKE_CURRENT_BINARY_DIR})

# --- 7. Target Handling ---
# Only build standalone executable if NOT building a LArSoft module
# OR if you want both, ensure they use different source files.
if(NOT With_LArSoft)
    file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
    file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

    add_executable(gdml_det gdml_det.cc ${sources})
    target_link_libraries(gdml_det ${Geant4_LIBRARIES} ${ROOT_LIBRARIES})

    if(With_Opticks)
        target_link_libraries(gdml_det Opticks::G4CX Opticks::U4)
    endif()
else()
    # In LArSoft mode, we use art_make.
    # NOTE: art_make will look for all .cc files in the current dir.
    # Exclude your standalone main (gdml_det.cc) from the art_make build!
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
endif()

# --- 8. Config & Scripts ---
configure_file(config.h.in ${CMAKE_BINARY_DIR}/include/config.h)

set(G04_SCRIPTS macros/iso.mac macros/test.mac GDML/test.gdml) # shortened for brevity

foreach(_script ${G04_SCRIPTS})
    configure_file(${PROJECT_SOURCE_DIR}/${_script} ${PROJECT_BINARY_DIR}/${_script} COPYONLY)
endforeach()
