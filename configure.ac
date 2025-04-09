AC_INIT

AC_CONFIG_SRCDIR([snapper/Snapper.h])

VERSION=`cat ./VERSION`

LIBVERSION=`cat ./LIBVERSION`
LIBVERSION_MAJOR=`cut -d . -f 1 ./LIBVERSION`
LIBVERSION_MINOR=`cut -d . -f 2 ./LIBVERSION`
LIBVERSION_PATCHLEVEL=`cut -d . -f 3 ./LIBVERSION`

LIBVERSION_CURRENT=`expr $LIBVERSION_MAJOR + $LIBVERSION_MINOR`
LIBVERSION_REVISON=`expr $LIBVERSION_PATCHLEVEL`
LIBVERSION_AGE=`expr $LIBVERSION_MINOR`
LIBVERSION_INFO=$LIBVERSION_CURRENT:$LIBVERSION_REVISON:$LIBVERSION_AGE

AM_INIT_AUTOMAKE(snapper, $VERSION)
AC_CONFIG_HEADERS(config.h)

AC_DISABLE_STATIC

AC_PROG_CXX
LT_INIT

AC_PREFIX_DEFAULT(/usr)

AC_PATH_PROG([XSLTPROC], [xsltproc], [/usr/bin/xsltproc])

AC_PATH_PROG([BTRFS_BIN], [btrfs], [/usr/sbin/btrfs])
AC_PATH_PROG([CHATTR_BIN], [chattr], [/usr/bin/chattr])
AC_PATH_PROG([CHSNAP_BIN], [chsnap], [/sbin/chsnap])
AC_PATH_PROG([CP_BIN], [cp], [/bin/cp])
AC_PATH_PROG([DIFF_BIN], [diff], [/usr/bin/diff])
AC_PATH_PROG([FINDMNT_BIN], [findmnt], [/usr/bin/findmnt])
AC_PATH_PROG([LVCHANGE_BIN], [lvchange], [/sbin/lvchange])
AC_PATH_PROG([LVCREATE_BIN], [lvcreate], [/sbin/lvcreate])
AC_PATH_PROG([LVM_BIN], [lvm], [/sbin/lvm])
AC_PATH_PROG([LVREMOVE_BIN], [lvremove], [/sbin/lvremove])
AC_PATH_PROG([LVRENAME_BIN], [lvrename], [/sbin/lvrename])
AC_PATH_PROG([LVS_BIN], [lvs], [/sbin/lvs])
AC_PATH_PROG([MKDIR_BIN], [mkdir], [/bin/mkdir])
AC_PATH_PROG([REALPATH_BIN], [realpath], [/usr/bin/realpath])
AC_PATH_PROG([RM_BIN], [rm], [/bin/rm])
AC_PATH_PROG([RMDIR_BIN], [rmdir], [/bin/rmdir])
AC_PATH_PROG([TOUCH_BIN], [touch], [/usr/bin/touch])

AC_DEFINE_UNQUOTED([BTRFS_BIN], ["$BTRFS_BIN"], [Path of btrfs program.])
AC_DEFINE_UNQUOTED([CHATTR_BIN], ["$CHATTR_BIN"], [Path of chattr program.])
AC_DEFINE_UNQUOTED([CHSNAP_BIN], ["$CHSNAP_BIN"], [Path of chsnap program.])
AC_DEFINE_UNQUOTED([CP_BIN], ["$CP_BIN"], [Path of cp program.])
AC_DEFINE_UNQUOTED([DIFF_BIN], ["$DIFF_BIN"], [Path of diff program.])
AC_DEFINE_UNQUOTED([FINDMNT_BIN], ["$FINDMNT_BIN"], [Path of findmnt program.])
AC_DEFINE_UNQUOTED([LVCHANGE_BIN], ["$LVCHANGE_BIN"], [Path of lvchange program.])
AC_DEFINE_UNQUOTED([LVCREATE_BIN], ["$LVCREATE_BIN"], [Path of lvcreate program.])
AC_DEFINE_UNQUOTED([LVM_BIN], ["$LVM_BIN"], [Path of lvm program.])
AC_DEFINE_UNQUOTED([LVREMOVE_BIN], ["$LVREMOVE_BIN"], [Path of lvremove program.])
AC_DEFINE_UNQUOTED([LVRENAME_BIN], ["$LVRENAME_BIN"], [Path of lvrename program.])
AC_DEFINE_UNQUOTED([LVS_BIN], ["$LVS_BIN"], [Path of lvs program.])
AC_DEFINE_UNQUOTED([MKDIR_BIN], ["$MKDIR_BIN"], [Path of mkdir program.])
AC_DEFINE_UNQUOTED([REALPATH_BIN], ["$REALPATH_BIN"], [Path of realpath program.])
AC_DEFINE_UNQUOTED([RM_BIN], ["$RM_BIN"], [Path of rm program.])
AC_DEFINE_UNQUOTED([RMDIR_BIN], ["$RMDIR_BIN"], [Path of rmdir program.])
AC_DEFINE_UNQUOTED([TOUCH_BIN], ["$TOUCH_BIN"], [Path of touch program.])

CFLAGS="${CFLAGS} -std=c99 -Wall -Wextra -Wformat -Wmissing-prototypes -Wno-unused-parameter"
CXXFLAGS="${CXXFLAGS} -std=c++11 -Wall -Wextra -Wformat -Wnon-virtual-dtor -Wno-unused-parameter -Wsuggest-override"

SYSCONFIG=/etc/sysconfig

AC_ARG_WITH([conf], AS_HELP_STRING([--with-conf], [Use a custom sysconfig directory (default is /etc/sysconfig)]),
	[with_conf=$withval], [with_conf=no])
AS_IF([test "x$with_conf" != xno], [SYSCONFIG="${with_conf}"])

CPPFLAGS="${CPPFLAGS} -DCONF_DIR='\"${SYSCONFIG}\"'"

PAM_SECURITY=${libdir}/security

AC_ARG_WITH([pam-security], AS_HELP_STRING([--pam-security], [Use a custom pam security directory (default is $libdir/security)]),
	[with_pam_security=$withval], [with_pam_security=no])
AS_IF([test "x$with_pam_security" != xno], [PAM_SECURITY="${with_pam_security}"])

AC_ARG_ENABLE([btrfs], AS_HELP_STRING([--disable-btrfs], [Disable Btrfs internal snapshots support]),
		[with_btrfs=$enableval], [with_btrfs=yes])
AM_CONDITIONAL(ENABLE_BTRFS, [test "x$with_btrfs" = "xyes"])

if test "x$with_btrfs" = "xyes"; then
	AC_DEFINE(ENABLE_BTRFS, 1, [Enable Btrfs internal snapshots support])
fi

AC_ARG_ENABLE([bcachefs], AS_HELP_STRING([--disable-bcachefs], [Disable Bcachefs internal snapshots support]),
		[with_bcachefs=$enableval], [with_bcachefs=yes])
AM_CONDITIONAL(ENABLE_BCACHEFS, [test "x$with_bcachefs" = "xyes"])

if test "x$with_bcachefs" = "xyes"; then
	AC_DEFINE(ENABLE_BCACHEFS, 1, [Enable Bcachefs internal snapshots support])
fi

AC_ARG_ENABLE([ext4], AS_HELP_STRING([--disable-ext4], [Disable ext4 snapshots support]),
		[with_ext4=$enableval], [with_ext4=yes])
AM_CONDITIONAL(ENABLE_EXT4, [test "x$with_ext4" = "xyes"])

if test "x$with_ext4" = "xyes"; then
	AC_DEFINE(ENABLE_EXT4, 1, [Enable Ext4 snapshots support])
fi

AC_ARG_ENABLE([lvm], AS_HELP_STRING([--disable-lvm], [Disable LVM thinprovisioned snapshots support]),
		[with_lvm=$enableval], [with_lvm=yes])
AM_CONDITIONAL(ENABLE_LVM, [test "x$with_lvm" = "xyes"])

if test "x$with_lvm" = "xyes"; then
	AC_DEFINE(ENABLE_LVM, 1, [Enable LVM thin-provisioned snapshots support])
fi

if test "x$with_btrfs" != "xyes" -a "x$with_lvm" != "xyes" -a "x$with_ext4" != "xyes"; then
	AC_MSG_ERROR([You have to enable at least one snapshot type (remove some --disable-xxx parameter)])
fi

AC_ARG_ENABLE([zypp], AS_HELP_STRING([--disable-zypp], [Disable zypp plugin support]),
		[with_zypp=$enableval], [with_zypp=yes])
AM_CONDITIONAL(HAVE_ZYPP, [test "x$with_zypp" = "xyes"])

AC_ARG_ENABLE([systemd], AS_HELP_STRING([--disable-systemd], [Disable systemd support]),
		[enable_systemd=$enableval], [enable_systemd=yes])
AM_CONDITIONAL(ENABLE_SYSTEMD, [test "x$enable_systemd" = "xyes"])

AC_CHECK_LIB(btrfs, btrfs_read_and_process_send_stream)
AC_CHECK_HEADERS([btrfs/version.h])

AC_ARG_ENABLE([doc], AS_HELP_STRING([--disable-doc], [Disable Build DOC support]),
		[enable_doc=$enableval], [enable_doc=yes])
AM_CONDITIONAL(ENABLE_DOC, [test "x$enable_doc" = "xyes"])

if test "x$enable_doc" = "xyes"; then
	AC_DEFINE(ENABLE_DOC, 1, [Enable Build DOC support])
fi

AC_ARG_ENABLE([xattrs], AS_HELP_STRING([--disable-xattrs], [Disable extended attributes support]),
		[with_xattrs=$enableval], [with_xattrs=yes])
AM_CONDITIONAL(HAVE_XATTRS, [test "x$with_xattrs" = "xyes"])

if test "x$with_xattrs" = "xyes"; then
	AC_DEFINE(ENABLE_XATTRS, 1, [Enable extended attributes support])
fi

AC_ARG_ENABLE([rollback], AS_HELP_STRING([--disable-rollback], [Disable rollback support]),
		[enable_rollback=$enableval], [enable_rollback=yes])
AM_CONDITIONAL(ENABLE_ROLLBACK, [test "x$enable_rollback" = "xyes"])

if test "x$enable_rollback" = "xyes"; then
	if test "x$with_btrfs" != "xyes"; then
		AC_MSG_ERROR([rollback support needs btrfs support])
	fi

	AC_DEFINE(ENABLE_ROLLBACK, 1, [Enable rollback support])

	# libmount is needed for rollback feature
	AC_CHECK_HEADER(libmount/libmount.h,[],[AC_MSG_ERROR([Cannout find libmount headers. Please install libmount-devel])])
fi

AC_ARG_ENABLE([btrfs-quota], AS_HELP_STRING([--disable-btrfs-quota], [Disable btrfs quota support]),
		[enable_btrfs_quota=$enableval], [enable_btrfs_quota=yes])
AM_CONDITIONAL(ENABLE_BTRFS_QUOTA, [test "x$enable_btrfs_quota" = "xyes"])

if test "x$enable_btrfs_quota" = "xyes"; then
	if test "x$with_btrfs" != "xyes"; then
		AC_MSG_ERROR([btrfs quota support needs btrfs support])
	fi

	AC_DEFINE(ENABLE_BTRFS_QUOTA, 1, [Enable btrfs quota support])
fi

AC_ARG_ENABLE([pam], AS_HELP_STRING([--disable-pam], [Disable pam plugin support]),
                [with_pam=$enableval], [with_pam=yes])
AM_CONDITIONAL(HAVE_PAM, [test "x$with_pam" = "xyes"])

AC_ARG_ENABLE([selinux], AS_HELP_STRING([--enable-selinux], [Enable support for SELinux LSM]),
		[with_selinux=$enableval], [with_selinux=no])
AM_CONDITIONAL(ENABLE_SELINUX, [test "x$enable_selinux" = "xyes"])

if test "x$with_selinux" = "xyes"; then
	AC_DEFINE(ENABLE_SELINUX, 1, [Enable SELinux support])
	AC_CHECK_HEADER(selinux/selinux.h,[],[AC_MSG_ERROR([Cannot find libselinux headers. Please install libselinux-devel])])
	AC_CHECK_LIB(selinux, selinux_snapperd_contexts_path, [], [AC_MSG_ERROR([selinux library does not provide selinux_snapperd_contexts_path symbol])])
fi

AC_ARG_ENABLE([coverage], AS_HELP_STRING([--enable-coverage], [Enable test coverage measurement]),
                [enable_coverage=$enableval], [enable_coverage=no])
AM_CONDITIONAL(ENABLE_COVERAGE, [test "x$enable_coverage" = "xyes"])
if test "x$enable_coverage" = "xyes"; then
        CFLAGS="${CFLAGS} --coverage"
        CXXFLAGS="${CXXFLAGS} --coverage"
        LDFLAGS="${LDFLAGS} --coverage"
fi

PKG_CHECK_MODULES(DBUS, dbus-1)
PKG_CHECK_MODULES(XML2, libxml-2.0)
PKG_CHECK_MODULES(JSON_C, json-c, [], [AC_MSG_WARN([Cannot find json-c. Please install libjson-c-devel])])
PKG_CHECK_MODULES(ZLIB, zlib)

# Conditional support for libbtrfsutil based on existence of pkg-config file
PKG_CHECK_MODULES([LIBBTRFSUTIL], [libbtrfsutil], [have_libbtrfsutil=yes], [have_libbtrfsutil=no])
if test "x$have_libbtrfsutil" = "xyes"; then
	AC_DEFINE(HAVE_LIBBTRFSUTIL, 1, [Define to 1 if you have the libbtrfsutil.pc file.])
fi

AC_CHECK_HEADER(acl/libacl.h,[],[AC_MSG_ERROR([Cannout find libacl headers. Please install libacl-devel])])

AC_SUBST(VERSION)
AC_SUBST(LIBVERSION)
AC_SUBST(LIBVERSION_MAJOR)
AC_SUBST(LIBVERSION_MINOR)
AC_SUBST(LIBVERSION_PATCHLEVEL)
AC_SUBST(LIBVERSION_INFO)
AC_SUBST(SYSCONFIG)
AC_SUBST(PAM_SECURITY)
AC_SUBST(docdir)

AC_CONFIG_FILES([
	Makefile
	snapper/Makefile
	snapper/Version.h:snapper/Version.h.in
	examples/Makefile
	examples/c/Makefile
	examples/c++-lib/Makefile
	dbus/Makefile
	dbus/testsuite/Makefile
	server/Makefile
	client/Makefile
	client/utils/Makefile
	client/proxy/Makefile
	client/snapper/Makefile
	client/mksubvolume/Makefile
	client/installation-helper/Makefile
	client/systemd-helper/Makefile
	client/snbk/Makefile
	scripts/Makefile
	pam/Makefile
	data/Makefile
	doc/Makefile
	doc/snapper.xml:doc/snapper.xml.in
	doc/snapperd.xml:doc/snapperd.xml.in
	doc/snapper-configs.xml:doc/snapper-configs.xml.in
	doc/snbk.xml:doc/snbk.xml.in
	doc/snapper-backup-configs.xml:doc/snapper-backup-configs.xml.in
	doc/snapper-zypp-plugin.xml:doc/snapper-zypp-plugin.xml.in
	doc/snapper-zypp-plugin.conf.xml:doc/snapper-zypp-plugin.conf.xml.in
	doc/pam_snapper.xml:doc/pam_snapper.xml.in
	doc/mksubvolume.xml:doc/mksubvolume.xml.in
	po/Makefile
	testsuite/Makefile
	testsuite-real/Makefile
	testsuite-cmp/Makefile
	stomp/Makefile
	stomp/testsuite/Makefile
	zypp-plugin/Makefile
	zypp-plugin/testsuite/Makefile
	package/snapper.spec:snapper.spec.in
	dists/debian/snapper-Debian.dsc.in:dists/debian/snapper-Debian.dsc.in.in
	dists/debian/snapper-xUbuntu.dsc.in:dists/debian/snapper-xUbuntu.dsc.in.in
	dists/debian/snapper-Raspbian.dsc.in:dists/debian/snapper-Raspbian.dsc.in.in
])

AC_OUTPUT
