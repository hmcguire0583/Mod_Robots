
cmake_minimum_required(VERSION 3.28)
project(Pathfinder)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Normal variables
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/Module Properties")
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
# Cache entries
set(INCLUDE_TESTS OFF CACHE BOOL "Whether or not unit tests should be built alongside the Pathfinder.")
set(LINK_TBB OFF CACHE BOOL "Whether or not TBB should be linked.")
# Environment variables
set(ENV{CTEST_OUTPUT_ON_FAILURE} ON)

if(${LINK_TBB})
    find_package(TBB REQUIRED COMPONENTS tbb)
endif()

find_package(Boost 1.83 REQUIRED COMPONENTS filesystem system)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(nlohmann_json REQUIRED)

if(${WIN32})
    find_package(unofficial-getopt-win32 REQUIRED)
endif()

if(NOT "${CMAKE_CURRENT_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_LIST_DIR}")
    if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/Moves)
        add_custom_target(clear_moves
                COMMAND rm -r "${CMAKE_CURRENT_BINARY_DIR}/Moves"
        )
    endif()

    add_custom_target(copy_moves
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Moves ${CMAKE_CURRENT_BINARY_DIR}/Moves
    )

    add_custom_target(copy_examples
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/docs/examples ${CMAKE_CURRENT_BINARY_DIR}/docs/examples
    )

    add_custom_target(copy_properties
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/Module Properties" "${CMAKE_CURRENT_BINARY_DIR}/Module Properties"
    )

    add_custom_target(copy_resources
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Test-Resources ${CMAKE_CURRENT_BINARY_DIR}/Test-Resources
    )
endif()

# Base Property Library

add_library(PropertyLib SHARED pathfinder/modules/ModuleProperties.cpp)

target_link_libraries(PropertyLib ${Boost_LIBRARIES})

target_link_libraries(PropertyLib Boost::system)

target_link_libraries(PropertyLib Boost::filesystem)

set_target_properties(PropertyLib PROPERTIES PREFIX "")

# Color Property

add_library(ColorPropertyLib MODULE pathfinder/properties/Colors.cpp pathfinder/utility/color_util.cpp)

set_target_properties(ColorPropertyLib PROPERTIES PREFIX "")

target_link_libraries(ColorPropertyLib PropertyLib)

add_dependencies(ColorPropertyLib PropertyLib)

# Orientation Property

add_library(OrientationPropertyLib MODULE EXCLUDE_FROM_ALL pathfinder/properties/Orientation.cpp)

set_target_properties(OrientationPropertyLib PROPERTIES PREFIX "")

target_link_libraries(OrientationPropertyLib PropertyLib)

add_dependencies(OrientationPropertyLib PropertyLib)

# Pathfinder

if(${INCLUDE_TESTS})
    set(Targets Pathfinder Pathfinder_Standard Pathfinder_Rhombic Pathfinder_Parallel Pathfinder_FullCheck)
else()
    set(Targets Pathfinder)
endif()

foreach (TARGET ${Targets})
    add_executable(${TARGET}
            pathfinder/main.cpp
            pathfinder/utility/debug_util.h
            pathfinder/modules/ModuleManager.h
            pathfinder/modules/ModuleManager.cpp
            pathfinder/moves/MoveManager.h
            pathfinder/moves/MoveManager.cpp
            pathfinder/lattice/Lattice.h
            pathfinder/lattice/Lattice.cpp
            pathfinder/lattice/LatticeSetup.h
            pathfinder/lattice/LatticeSetup.cpp
            pathfinder/moves/Scenario.h
            pathfinder/moves/Scenario.cpp
            pathfinder/search/ConfigurationSpace.h
            pathfinder/search/ConfigurationSpace.cpp
            pathfinder/search/SearchAnalysis.h
            pathfinder/search/SearchAnalysis.cpp
            pathfinder/moves/Isometry.h
            pathfinder/moves/Isometry.cpp
            pathfinder/search/HeuristicCache.cpp
            pathfinder/search/HeuristicCache.h
            pathfinder/utility/color_util.cpp
            pathfinder/utility/color_util.h)

    target_link_libraries(${TARGET} PropertyLib)

    if(${LINK_TBB})
        target_link_libraries(${TARGET} tbb)
    endif()

    if(WIN32)
        target_link_libraries(${TARGET} unofficial::getopt-win32::getopt)
    endif()
endforeach()

if(NOT "${CMAKE_CURRENT_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_LIST_DIR}")
    add_dependencies(Pathfinder copy_moves copy_examples copy_properties)
    add_dependencies(ColorPropertyLib PropertyLib copy_properties)
    if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/Moves")
        add_dependencies(copy_moves clear_moves)
    endif()
endif()

if(${INCLUDE_TESTS})
    add_dependencies(copy_examples copy_resources)

    target_compile_definitions(Pathfinder_Standard PUBLIC
            LATTICE_RD_EDGECHECK=false
            CONFIG_PARALLEL_MOVES=false
            MOVEMANAGER_CHECK_BY_OFFSET=true)
#    target_compile_definitions(Pathfinder_Rhombic PUBLIC
#            LATTICE_RD_EDGECHECK=true
#            CONFIG_PARALLEL_MOVES=false
#            MOVEMANAGER_CHECK_BY_OFFSET=true)
    target_compile_definitions(Pathfinder_Parallel PUBLIC
            LATTICE_RD_EDGECHECK=false
            CONFIG_PARALLEL_MOVES=true
            MOVEMANAGER_CHECK_BY_OFFSET=true)
    target_compile_definitions(Pathfinder_FullCheck PUBLIC
            LATTICE_RD_EDGECHECK=false
            CONFIG_PARALLEL_MOVES=false
            MOVEMANAGER_CHECK_BY_OFFSET=false)

    add_dependencies(Pathfinder_FullCheck OrientationPropertyLib)

    enable_testing()

    # Following are some very basic tests to ensure functionality of A* search and associated heuristics.
    # The tests work by making sure A* final depth matches that of BFS.
    # In certain cases it also checks the amounts of states processed, this is mostly to ensure that certain
    # desirable properties of the MRSH-1 heuristic remain intact.

    add_test(NAME Test2DSolo COMMAND Pathfinder_Standard
            -I ./Test-Resources/Heuristic-Checker_initial.json
            -F ./Test-Resources/Heuristic-Checker_final.json
            -e ./Test-Resources/Output/2DSolo.scen
            -a ./Test-Resources/Output/2DSolo_analysis.json
            -m ./Test-Resources/Moves_Pivot)

    set_property(TEST Test2DSolo PROPERTY PASS_REGULAR_EXPRESSION "States Processed: 9999")
    set_property(TEST Test2DSolo PROPERTY PASS_REGULAR_EXPRESSION "A* Final Depth: 36")

    add_test(NAME Test2D COMMAND Pathfinder_Standard
            -i
            -I ./Test-Resources/Z-Pentomino_initial.json
            -F ./Test-Resources/Z-Pentomino_final.json
            -e ./Test-Resources/Output/2D.scen
            -a ./Test-Resources/Output/2D_analysis.json
            -m ./Test-Resources/Moves_Pivot)

    set_property(TEST Test2D PROPERTY PASS_REGULAR_EXPRESSION "A* Final Depth: 14")

    add_test(NAME TestColor2D COMMAND Pathfinder_Standard
            -I ./Test-Resources/Color-Shuffle_initial.json
            -F ./Test-Resources/Color-Shuffle_final.json
            -e ./Test-Resources/Output/Color2D.scen
            -a ./Test-Resources/Output/Color2D_analysis.json
            -m ./Test-Resources/Moves_Pivot)

    set_property(TEST TestColor2D PROPERTY PASS_REGULAR_EXPRESSION "A* Final Depth: 10")

    add_test(NAME TestColor3D COMMAND Pathfinder_Standard
            -I ./Test-Resources/Color-Shuffle-3D_initial.json
            -F ./Test-Resources/Color-Shuffle-3D_final.json
            -e ./Test-Resources/Output/Color3D.scen
            -a ./Test-Resources/Output/Color3D_analysis.json
            -m ./Test-Resources/Moves_Pivot)

    set_property(TEST TestColor3D PROPERTY PASS_REGULAR_EXPRESSION "A* Final Depth: 8")

    add_test(NAME TestRhombicDodecahedron COMMAND Pathfinder_Standard
            -I ./Test-Resources/Rhombic-Color_initial.json
            -F ./Test-Resources/Rhombic-Color_final.json
            -e ./Test-Resources/Output/RhombicDodecahedron.scen
            -a ./Test-Resources/Output/RhombicDodecahedron_analysis.json
            -m ./Test-Resources/Moves_Rhombic
            -c RD)

    set_property(TEST TestRhombicDodecahedron PROPERTY PASS_REGULAR_EXPRESSION "A* Final Depth: 16")

    add_test(NAME TestParallelSolo COMMAND Pathfinder_Parallel
            -I ./Test-Resources/Heuristic-Checker_initial.json
            -F ./Test-Resources/Heuristic-Checker_final.json
            -e ./Test-Resources/Output/ParallelSolo.scen
            -a ./Test-Resources/Output/ParallelSolo_analysis.json
            -m ./Test-Resources/Moves_Pivot)

    set_property(TEST TestParallelSolo PROPERTY PASS_REGULAR_EXPRESSION "States Processed: 9")
    set_property(TEST TestParallelSolo PROPERTY PASS_REGULAR_EXPRESSION "A* Final Depth: 9")

    add_test(NAME TestParallelCoop COMMAND Pathfinder_Parallel
            -I ./Test-Resources/Color-Shuffle_initial.json
            -F ./Test-Resources/Color-Shuffle_final.json
            -e ./Test-Resources/Output/ParallelCoop.scen
            -a ./Test-Resources/Output/ParallelCoop_analysis.json
            -m ./Test-Resources/Moves_Pivot)

    set_property(TEST TestParallelCoop PROPERTY PASS_REGULAR_EXPRESSION "A* Final Depth: 5")

    add_test(NAME TestMovePropertyChecks COMMAND Pathfinder_Standard
            -I ./Test-Resources/Mixed-Modules_initial.json
            -F ./Test-Resources/Mixed-Modules_final.json
            -e ./Test-Resources/Output/MovePropertyChecks.scen
            -a ./Test-Resources/Output/MovePropertyChecks_analysis.json
            -m ./Test-Resources/Moves_ColorRestricted)

    set_property(TEST TestMovePropertyChecks PROPERTY PASS_REGULAR_EXPRESSION "A* Final Depth: 14")

    # Disabled until orientation property is repaired
#    add_test(NAME TestOrientationProperty COMMAND Pathfinder_FullCheck
#            -I ./Test-Resources/Orientation_initial.json
#            -F ./Test-Resources/Orientation_final.json
#            -e ./Test-Resources/Output/OrientationProperty.scen
#            -a ./Test-Resources/Output/OrientationProperty_analysis.json
#            -m ./Test-Resources/Moves_Orientation
#            -s BDBFS)
#
#    set_property(TEST TestOrientationProperty PROPERTY PASS_REGULAR_EXPRESSION "BDBFS Final Depth: 4")
#
#    add_test(NAME TestOrientationProperty3D COMMAND Pathfinder_FullCheck
#            -I ./Test-Resources/Orientation3D_initial.json
#            -F ./Test-Resources/Orientation3D_final.json
#            -e ./Test-Resources/Output/OrientationProperty3D.scen
#            -a ./Test-Resources/Output/OrientationProperty3D_analysis.json
#            -m ./Test-Resources/Moves_Orientation
#            -s BDBFS)
#
#    set_property(TEST TestOrientationProperty3D PROPERTY PASS_REGULAR_EXPRESSION "BDBFS Final Depth: 6")

    # This test takes too long to complete
#    add_test(NAME 15PuzzleGen COMMAND Pathfinder_Standard
#            -I ./Test-Resources/15_Puzzle_initial.json
#            -e ./Test-Resources/Output/15_Puzzle.scen
#            -a ./Test-Resources/Output/15_Puzzle_analysis.json
#            -m ./Test-Resources/Moves_15_Puzzle)
#
#    set_property(TEST 15PuzzleGen PROPERTY PASS_REGULAR_EXPRESSION "Exporting moves...   Done.")
endif()
