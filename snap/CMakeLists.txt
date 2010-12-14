####################################################################
# SOURCES                                                          #
####################################################################

ADD_DEFINITIONS(-DVERSION=${LIBSNAP_VERSION} )
ADD_DEFINITIONS(-DLOCALEDIR="${CMAKE_INSTALL_PREFIX}/share/locale" -DTEXTDOMAIN="libsnap" )
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET( snap_SRCS
  Snap.cc
)

SET( snap_HEADERS
  Snap.h
)

INSTALL( FILES ${snap_HEADERS} DESTINATION "${CMAKE_INSTALL_PREFIX}/include/snap" )

####################################################################

SET( snap_lib_SRCS
  ${snap_SRCS}
)

SET( snap_lib_HEADERS
  ${snap_HEADERS}
)

#
# Macro to set the log group for a list of files
#
MACRO( SET_LOGGROUP _group _files  )
  SET_SOURCE_FILES_PROPERTIES( ${_files} COMPILE_FLAGS -DSNAPP_BASE_LOGGER_LOGGROUP=\\"${_group}\\" )
  FOREACH (_currentFile ${ARGN})
    SET_SOURCE_FILES_PROPERTIES( ${_currentFile} COMPILE_FLAGS -DSNAP_BASE_LOGGER_LOGGROUP=\\"${_group}\\" )
  ENDFOREACH (_currentFile ${ARGN})
ENDMACRO( SET_LOGGROUP )

# Default loggroup for all files
SET_LOGGROUP( "snap" ${snap_lib_SRCS} )


ADD_LIBRARY(snap SHARED ${snap_lib_SRCS})
SET_TARGET_PROPERTIES( snap PROPERTIES VERSION "${LIBSNAP_VERSION_INFO}" )
SET_TARGET_PROPERTIES( snap PROPERTIES SOVERSION "${LIBSNAP_SOVERSION_INFO}" )
SET_TARGET_PROPERTIES( snap PROPERTIES LINK_FLAGS "--as-needed" )

INSTALL(TARGETS snap LIBRARY DESTINATION ${LIB_INSTALL_DIR} )
