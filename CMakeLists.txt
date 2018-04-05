cmake_minimum_required (VERSION 3.7)
project (rnv)

set(RNV_VERSION "2.0.0")

# Target architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET(TARGET_ARCH_NAME "amd64")
else() 
    SET(TARGET_ARCH_NAME "i386")
endif()

# Build configuration
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set(ENABLE_DBG ON)
set(ENABLE_REL ON)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ENABLE_REL OFF)
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(ENABLE_DBG OFF)
endif()

set(RNV_REL_SUFFIX "")
set(RNV_DBG_SUFFIX "d")

# Header files
set(RNV_HEADER_FILES
    ary.h
    drv.h
    er.h
    erbit.h
    ht.h
    m.h
    librnv.h
    rn.h
    rnc.h
    rnd.h
    rnl.h
    rnv.h
    rnx.h
    rx.h
    s.h
    sc.h
    u.h
    xmlc.h
    xsd.h
    xsd_tm.h
)

# Source files
set(RNV_SRC_FILES
    ary.c
    drv.c
    er.c
    ht.c
    librnv.c
    m.c
    rn.c
    rnc.c
    rnd.c
    rnl.c
    rnv.c
    rnx.c
    rx.c
    s.c
    sc.c
    u.c
    xmlc.c
    xsd.c
    xsd_tm.c
)

# Create file groups for IDEs like Visual Studio
source_group("Header Files" FILES ${RNV_HEADER_FILES})
source_group("Source Files" FILES ${RNV_SRC_FILES})

# External libraries
if(NOT DEFINED EXTERNAL_LIBRARIES_BASE_DIR)
    if(NOT $ENV{EXTERNAL_LIBRARIES_BASE_DIR} STREQUAL "")
        set(EXTERNAL_LIBRARIES_BASE_DIR $ENV{EXTERNAL_LIBRARIES_BASE_DIR})
    else()
        set(EXTERNAL_LIBRARIES_BASE_DIR "\\\\server03.swissqprint.inhouse\\Daten3\\development\\libraries")
    endif()
endif()
message(STATUS "Search path for external libraries: ${EXTERNAL_LIBRARIES_BASE_DIR}")

set(LIBEXPAT_DIR ${EXTERNAL_LIBRARIES_BASE_DIR}/libexpat/2.2.0)
include_directories(${LIBEXPAT_DIR}/include)

# Definitions
if(MSVC)
    add_definitions(-DUNISTD_H=<io.h>)
else()
    add_definitions(-DUNISTD_H=<unistd.h>)
endif()
add_definitions(-DRNV_VERSION="${RNV_VERSION}" -DEXPAT_H=<expat.h>)

## Build shared library
add_library(rnv SHARED
    ${RNV_SRC_FILES}
    ${RNV_HEADER_FILES}
)
target_compile_definitions(rnv PRIVATE LIBRNV)

if(ENABLE_DBG)
    unset(EXPAT_LIB_DBG CACHE)

    find_library(EXPAT_LIB_DBG
            NAMES expatd
            PATHS ${LIBEXPAT_DIR}/lib.${TARGET_ARCH_NAME}
            PATH_SUFFIXES debug Debug
    )
    
    message(STATUS "libexpat (debug): ${EXPAT_LIB_DBG}")
    
    target_link_libraries(rnv debug ${EXPAT_LIB_DBG})
endif()

if(ENABLE_REL)
    unset(EXPAT_LIB_REL CACHE)

    find_library(EXPAT_LIB_REL
            NAMES expat
            PATHS ${LIBEXPAT_DIR}/lib.${TARGET_ARCH_NAME}
            PATH_SUFFIXES release Release
    )
    
    message(STATUS "libexpat (debug): ${EXPAT_LIB_REL}")
    
    target_link_libraries(rnv optimized ${EXPAT_LIB_REL})
endif()

set_target_properties(rnv PROPERTIES
    VERSION ${RNV_VERSION}
    SOVERSION ${RNV_VERSION}
#    WINDOWS_EXPORT_ALL_SYMBOLS ON
	RELEASE_POSTFIX "${RNV_REL_SUFFIX}"
	DEBUG_POSTFIX 	"${RNV_DBG_SUFFIX}"
)

## Build command line tool
add_executable(rnvclc
    xcl.c
)

target_link_libraries(rnvclc rnv)

set_target_properties(rnvclc PROPERTIES
#    OUTPUT_NAME "rnv"
    VERSION ${RNV_VERSION}
)
