PROJECT(SNAPPER)
SET( PACKAGE "snapper" )
SET( LIBSNAP_PACKAGE "libsnap" )
SET( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules )
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

INCLUDE(SnapperCommon)

INCLUDE( ${CMAKE_SOURCE_DIR}/VERSION.cmake )
MATH( EXPR LIBSNAP_CURRENT "${LIBSNAP_MAJOR} * 100 + ${LIBSNAP_MINOR}" )
MATH( EXPR LIBSNAP_AGE     "${LIBSNAP_MINOR} - ${LIBSNAP_COMPATMINOR}" )
# Libtool wanted current:patch:age
# But cmake is not libtool, it wants the verbatim suffix to libsnap.so
MATH( EXPR LIBSNAP_SO_FIRST  "${LIBSNAP_CURRENT}-${LIBSNAP_AGE}" )
SET( LIBSNAP_VERSION "${LIBSNAP_MAJOR}.${LIBSNAP_MINOR}.${LIBSNAP_PATCH}" )
SET( LIBSNAP_VERSION_INFO "${LIBSNAP_SO_FIRST}.${LIBSNAP_AGE}.${LIBSNAP_PATCH}" )
SET( LIBSNAP_SOVERSION_INFO "${LIBSNAP_SO_FIRST}" )

SET( VERSION "${SNAPPER_MAJOR}.${SNAPPER_MINOR}.${SNAPPER_PATCH}" )

SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g -Wall -Werror=format-security -Woverloaded-virtual -Wnon-virtual-dtor -Wl,-as-needed" )
SET( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -fPIC -g -Wall -Werror=format-security -Wl,-as-needed" )
set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3" )

ADD_DEFINITIONS( -D_FILE_OFFSET_BITS=64 )

####################################################################
# make package based on git archive

GENERATE_PACKAGING(${PACKAGE} ${VERSION})
#INCLUDE(CTest)
#ENABLE_TESTING()

####################################################################

FIND_PACKAGE(Boost REQUIRED)
IF (Boost_FOUND)
  MESSAGE( STATUS "boost found: includes in ${Boost_INCLUDE_DIRS}, library in ${Boost_LIBRARY_DIRS}")
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

FIND_PACKAGE(Gettext REQUIRED)
IF (GETTEXT_FOUND)
  MESSAGE(STATUS "Found Gettext: ${GETTEXT_MSGFMT_EXECUTABLE}")
  INCLUDE_DIRECTORIES(${GETTEXT_INCLUDE_DIR})
ELSE (GETTEXT_FOUND)
  MESSAGE( FATAL_ERROR "Gettext not found" )
ENDIF (GETTEXT_FOUND)

#FIND_PACKAGE(Libxml REQUIRED)
#IF ( NOT LIBXML_FOUND)
#  MESSAGE( FATAL_ERROR " libxml not found" )
#ELSE ( NOT LIBXML_FOUND)
#  INCLUDE_DIRECTORIES(${LIBXML_INCLUDE_DIR})
#ENDIF( NOT LIBXML_FOUND)

FIND_PROGRAM( DOXYGEN doxygen )
IF ( NOT DOXYGEN )
  MESSAGE( FATAL_ERROR "doxygen not found: install doxygen to build the documentation." )
ELSE ( NOT DOXYGEN )
  MESSAGE( STATUS "doxygen found: ${DOXYGEN}" )
ENDIF ( NOT DOXYGEN )

####################################################################

MESSAGE(STATUS "soname: ${LIBSNAP_VERSION_INFO}")
MESSAGE(STATUS "version: ${VERSION}")

SPECFILE()

MESSAGE(STATUS "Writing pkg-config file...")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/package/${LIBSNAP_PACKAGE}.pc.cmake ${CMAKE_BINARY_DIR}/${LIBSNAP_PACKAGE}.pc @ONLY)
INSTALL( FILES ${CMAKE_BINARY_DIR}/${LIBSNAP_PACKAGE}.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig )

MESSAGE(STATUS "FindSnap.cmake will be installed in ${CMAKE_INSTALL_PREFIX}/share/cmake/Modules")
INSTALL( FILES ${CMAKE_SOURCE_DIR}/cmake/modules/FindSnap.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/share/cmake/Modules )

####################################################################
# SUBDIRECTORIES                                                   #
####################################################################

ADD_SUBDIRECTORY( snap )
ADD_SUBDIRECTORY( snapper )
ADD_SUBDIRECTORY( doc )
ADD_SUBDIRECTORY( examples )
ADD_SUBDIRECTORY( tests EXCLUDE_FROM_ALL )
