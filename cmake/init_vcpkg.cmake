if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND NOT DONOTUSE_VPCKG)
    if(NOT DEFINED VCPKG_TARGET_TRIPLET)
        if(WIN32)
            message(STATUS "No vcpkg triplet set. Forcing x64-windows-static")
            set(VCPKG_TARGET_TRIPLET x64-windows-static CACHE STRING "VCPKG target triplet to use" FORCE)
        elseif(UNIX)
            message(STATUS "No vcpkg triplet set. Forcing x64-linux")
            set(VCPKG_TARGET_TRIPLET x64-linux CACHE STRING "VCPKG target triplet to use" FORCE)
        endif()
    endif()
    set(VCPKG_VERBOSE OFF CACHE STRING "VCPKG Verbose logging" FORCE)
    if(NOT VCPKG_SEARCH_PATHS)
        set(VCPKG_SEARCH_PATHS
                    ${CMAKE_CURRENT_LIST_DIR}/thirdparty/vcpkg
                    ${CMAKE_CURRENT_LIST_DIR}/thirdparty/mpi_vcpkg
                    ${CMAKE_CURRENT_LIST_DIR}/external/vcpkg
                    ${CMAKE_CURRENT_LIST_DIR}/external/mpi_vcpkg 
                    ${CMAKE_CURRENT_LIST_DIR}/../vcpkg
                    ${CMAKE_CURRENT_LIST_DIR}/../mpi_vcpkg 
                    ${CMAKE_CURRENT_LIST_DIR}/../../vcpkg
                    ${CMAKE_CURRENT_LIST_DIR}/../../mpi_vcpkg)
    endif()
    set(VCPKG_SEARCH_PATHS "G:/cgns") #Internal for testing <- ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''REMOVE ME
    find_path(VCPKG_ROOT NAMES .vcpkg-root vcpkg.exe vcpkg
                         PATHS ${VCPKG_SEARCH_PATHS}
                         NO_DEFAULT_PATH)
    if(EXISTS "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        set(VCPKG_TOOLCHAIN_FILE ${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
        set(CMAKE_TOOLCHAIN_FILE ${VCPKG_TOOLCHAIN_FILE})
    else()
        message(WARNING "VCPKG toolchain file not found. Please set CMAKE_TOOLCHAIN_FILE to the VCPKG toolchain file! VCPKG_ROOT:${VCPKG_ROOT}; VCPKG_SEARCH_PATHS:${VCPKG_SEARCH_PATHS}")
    endif()
endif()
message(STATUS "Using toolchain: ${CMAKE_TOOLCHAIN_FILE}")
