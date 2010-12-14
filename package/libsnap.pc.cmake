prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@LIB_INSTALL_DIR@/snap
libdir=@LIB_INSTALL_DIR@
includedir=@CMAKE_INSTALL_PREFIX@/include

Name: @LIBSNAP_PACKAGE@
Version: @LIBSNAP_VERSION@
Description: A system to allow undo of system modifications

Libs: -L${libdir} -lsnap
Cflags: -I${includedir} @ZYPP_CFLAGS@
