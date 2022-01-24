cmake_minimum_required (VERSION 3.12)
project (RNV)

set(RNV_VERSION "2.0.0")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/expat)

set(EXPAT_SRCS
  expat/xmltok.c
  expat/xmlrole.c
  expat/xmlparse.c
  )

# Header files
set(HDR_FILES
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
set(SRC_FILES
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


# Definitions
add_definitions(-DRNV_VERSION="${RNV_VERSION}")

## Build shared library
add_library(lrnv SHARED ${EXPAT_SRCS} ${SRC_FILES} ${HDR_FILES})
target_compile_definitions(lrnv PRIVATE LIBRNV_LIBRARY)

## Build command line tool
add_executable(rnv xcl.c)
target_link_libraries(rnv lrnv)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
