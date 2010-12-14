### ##################################################

FIND_PROGRAM( DOT dot )
IF ( NOT DOT )
   MESSAGE( "dot not found: you should install graphviz." )
   SET( HAVE_DOT NO )
ELSE ( NOT DOT )
   SET( HAVE_DOT YES )
ENDIF ( NOT DOT )

SET( SNAPDOC_PACKAGE        libsnap )
SET( SNAPDOC_VERSION        ${LIBSNAP_VERSION} )
SET( SNAPDOC_SOURCE_DIR     ${CMAKE_SOURCE_DIR}/snapp )
SET( SNAPDOC_DOCINCLUDE_DIR ${CMAKE_SOURCE_DIR}/doc/autoinclude )
SET( SNAPDOC_EXAMPLE_DIR    ${CMAKE_SOURCE_DIR}/examples )

SET( DOXYGEN_INPUT    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile )
SET( DOXYGEN_OUTPUT   ${CMAKE_CURRENT_BINARY_DIR}/html/index.html )
FILE( MAKE_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/html )

### ##################################################

CONFIGURE_FILE(
   ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.cmake
   ${DOXYGEN_INPUT}
   @ONLY
   )

### ##################################################

ADD_CUSTOM_COMMAND (
   OUTPUT  ${DOXYGEN_OUTPUT}
   COMMAND ${CMAKE_COMMAND} -E echo_append "Building Documentation..."
   COMMAND ${DOXYGEN} ${DOXYGEN_INPUT}
   COMMAND ${CMAKE_COMMAND} -E echo "Done."
   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
   DEPENDS ${DOXYGEN_INPUT}
)

ADD_CUSTOM_TARGET( doc ALL
   DEPENDS ${DOXYGEN_OUTPUT}
)
ADD_DEPENDENCIES(doc zypp)

ADD_CUSTOM_TARGET( doc_forced
   COMMAND ${CMAKE_COMMAND} -E echo_append "Building Documentation..."
   COMMAND ${DOXYGEN} ${DOXYGEN_INPUT}
   COMMAND ${CMAKE_COMMAND} -E echo "Done."
   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
   DEPENDS ${DOXYGEN_INPUT}
)

### ##################################################

INSTALL( FILES
   ${CMAKE_CURRENT_BINARY_DIR}/libsnap.doxytag
   DESTINATION ${DOC_INSTALL_DIR}/libzypp/libsnap.doxytag
)

# We could use the thing below but it wont work with cmake older than 2.4.4
FILE(GLOB docfiles ${CMAKE_CURRENT_BINARY_DIR}/html/*)
INSTALL( FILES
   ${docfiles}
   DESTINATION ${DOC_INSTALL_DIR}/libzypp/html
)

#INSTALL( DIRECTORY
#   ${CMAKE_CURRENT_BINARY_DIR}/html
#   DESTINATION ${DOC_INSTALL_DIR}
#)

