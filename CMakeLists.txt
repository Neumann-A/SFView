cmake_minimum_required (VERSION 3.7 FATAL_ERROR)
include(cmake/init_vcpkg.cmake)
include(cmake/init_cmake.cmake)
# Find the QtWidgets library


project(SFView 
        VERSION 1.0 
        DESCRIPTION "This is a simple viewer for MPI system functions acquired on the Bruker Preclinical MPI system." 
        HOMEPAGE_URL "http://it.hs-pforzheim.de/personen/heinen/software/sfview.html"
        LANGUAGES CXX)

find_package(Qt5 CONFIG REQUIRED COMPONENTS Core Widgets)

file(GLOB CPP_FILES *.cpp)
file(GLOB UI_FILES *.ui)
file(GLOB H_FILES *.h)
file(GLOB QRC_FILES *.qrc)

add_executable(${PROJECT_NAME} ${CPP_FILES} ${UI_FILES} ${H_FILES} ${QRC_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core Qt5::Widgets)
set_property(TARGET ${PROJECT_NAME} PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")