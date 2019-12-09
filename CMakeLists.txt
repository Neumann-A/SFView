if(WIN32)
    cmake_minimum_required (VERSION 3.15 FATAL_ERROR)
elseif(UNIX)
    cmake_minimum_required (VERSION 3.7 FATAL_ERROR)
endif()
include(cmake/init_vcpkg.cmake)
include(cmake/init_cmake.cmake)
# Find the QtWidgets library


project(SFView 
        VERSION 1.0 
        DESCRIPTION "This is a simple viewer for MPI system functions acquired on the Bruker Preclinical MPI system." 
        HOMEPAGE_URL "http://it.hs-pforzheim.de/personen/heinen/software/sfview.html"
        LANGUAGES CXX)

find_package(Qt5 CONFIG REQUIRED COMPONENTS Core Widgets Charts Gui DataVisualization)



file(GLOB CPP_FILES *.cpp)
file(GLOB UI_FILES *.ui)
file(GLOB H_FILES *.h)
file(GLOB QRC_FILES *.qrc)

add_executable(${PROJECT_NAME} ${CPP_FILES} ${UI_FILES} ${H_FILES} ${QRC_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core Qt5::Widgets Qt5::Charts Qt5::Gui Qt5::DataVisualization)


if(TRUE) # if static build:
    if(WIN32)
        set_property(TARGET ${PROJECT_NAME} PROPERTY
                     MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::QXdgDesktopPortalThemePlugin Qt5::QWindowsIntegrationPlugin)
    elseif(UNIX)
        find_package(Qt5 CONFIG REQUIRED COMPONENTS QXdgDesktopPortalThemePlugin)
    endif()
    
    find_package(TIFF REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::QICOPlugin Qt5::QJpegPlugin Qt5::QICNSPlugin Qt5::QGifPlugin Qt5::QTgaPlugin Qt5::QTiffPlugin Qt5::QXdgDesktopPortalThemePlugin TIFF::TIFF)
endif()
