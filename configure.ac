
AC_INIT(snapper/Snapper.h)

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

AC_PATH_PROG([CHSNAPBIN], [chsnap], [/sbin/chsnap])
AC_PATH_PROG([CPBIN], [cp], [/bin/cp])
AC_PATH_PROG([TOUCHBIN], [touch], [/usr/bin/touch])
AC_PATH_PROG([RMBIN], [rm], [/bin/rm])
AC_PATH_PROG([DIFFBIN], [diff], [/usr/bin/diff])
AC_PATH_PROG([CHATTRBIN], [chattr], [/usr/bin/chattr])
AC_PATH_PROG([LVCREATEBIN], [lvcreate], [/sbin/lvcreate])
AC_PATH_PROG([LVREMOVEBIN], [lvremove], [/sbin/lvremove])
AC_PATH_PROG([LVSBIN], [lvs], [/sbin/lvs])
AC_PATH_PROG([LVCHANGEBIN], [lvchange], [/sbin/lvchange])
AC_PATH_PROG([LVMBIN], [lvm], [/sbin/lvm])
AC_PATH_PROG([LVRENAMEBIN], [lvrename], [/sbin/lvrename])

AC_DEFINE_UNQUOTED([CHSNAPBIN], ["$CHSNAPBIN"], [Path of chsnap program.])
AC_DEFINE_UNQUOTED([CPBIN], ["$CPBIN"], [Path of cp program.])
AC_DEFINE_UNQUOTED([TOUCHBIN], ["$TOUCHBIN"], [Path of touch program.])
AC_DEFINE_UNQUOTED([RMBIN], ["$RMBIN"], [Path of rm program.])
AC_DEFINE_UNQUOTED([DIFFBIN], ["$DIFFBIN"], [Path of diff program.])
AC_DEFINE_UNQUOTED([CHATTRBIN], ["$CHATTRBIN"], [Path of chattr program.])
AC_DEFINE_UNQUOTED([LVCREATEBIN], ["$LVCREATEBIN"], [Path of lvcreate program.])
AC_DEFINE_UNQUOTED([LVREMOVEBIN], ["$LVREMOVEBIN"], [Path of lvremove program.])
AC_DEFINE_UNQUOTED([LVSBIN], ["$LVSBIN"], [Path of lvs program.])
AC_DEFINE_UNQUOTED([LVCHANGEBIN], ["$LVCHANGEBIN"], [Path of lvchange program.])
AC_DEFINE_UNQUOTED([LVMBIN], ["$LVMBIN"], [Path of lvm program.])
AC_DEFINE_UNQUOTED([LVRENAMEBIN], ["$LVRENAMEBIN"], [Path of lvrename program.])

dnl Automake 1.11 enables silent compilation
dnl Disable it by "configure --disable-silent-rules" or "make V=1"
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

CFLAGS="${CFLAGS} -std=c99 -Wall -Wextra -Wformat=2 -Wmissing-prototypes -Wno-unused-parameter"
CXXFLAGS="${CXXFLAGS} -std=c++11 -Wall -Wextra -Wformat=2 -Wnon-virtual-dtor -Wno-unused-parameter"

fillupdir=/var/adm/fillup-templates

AC_ARG_WITH([conf], AC_HELP_STRING([--with-conf], [Use a custom sysconfig directory (default is /etc/sysconfig)]),
	[with_conf=$withval], [with_conf=no])

AS_IF([test "x$with_conf" != xno], [CPPFLAGS="${CPPFLAGS} -DCONFDIR='\"${with_conf}\"'"],
	[CPPFLAGS="${CPPFLAGS} -DCONFDIR='\"/etc/sysconfig\"'"])

AC_ARG_ENABLE([btrfs], AC_HELP_STRING([--disable-btrfs],[Disable Btrfs internal snapshots support]),
		[with_btrfs=$enableval],[with_btrfs=yes])

AM_CONDITIONAL(ENABLE_BTRFS, [test "x$with_btrfs" = "xyes"])

if test "x$with_btrfs" = "xyes"; then
	AC_DEFINE(ENABLE_BTRFS, 1, [Enable Btrfs internal snapshots support])
fi

AC_ARG_ENABLE([ext4], AC_HELP_STRING([--disable-ext4],[Disable ext4 snapshots support]),
		[with_ext4=$enableval],[with_ext4=yes])

AM_CONDITIONAL(ENABLE_EXT4, [test "x$with_ext4" = "xyes"])

if test "x$with_ext4" = "xyes"; then
	AC_DEFINE(ENABLE_EXT4, 1, [Enable Ext4 snapshots support])
fi

AC_ARG_ENABLE([lvm], AC_HELP_STRING([--disable-lvm],[Disable LVM thinprovisioned snapshots support]),
		[with_lvm=$enableval],[with_lvm=yes])

AM_CONDITIONAL(ENABLE_LVM, [test "x$with_lvm" = "xyes"])

if test "x$with_lvm" = "xyes"; then
	AC_DEFINE(ENABLE_LVM, 1, [Enable LVM thin-provisioned snapshots support])
fi

if test "x$with_btrfs" != "xyes" -a "x$with_lvm" != "xyes" -a "x$with_ext4" != "xyes"; then
	AC_MSG_ERROR([You have to enable at least one snapshot type (remove some --disable-xxx parameter)])
fi

AC_ARG_ENABLE([zypp], AC_HELP_STRING([--disable-zypp],[Disable zypp plugin support]),
		[with_zypp=$enableval],[with_zypp=yes])
AM_CONDITIONAL(HAVE_ZYPP, [test "x$with_zypp" = "xyes"])

AC_CHECK_LIB(btrfs, btrfs_read_and_process_send_stream)
AC_CHECK_HEADERS([btrfs/version.h])

AC_ARG_ENABLE([xattrs], AC_HELP_STRING([--disable-xattrs],[Disable extended attributes support]),
		[with_xattrs=$enableval],[with_xattrs=yes])
AM_CONDITIONAL(HAVE_XATTRS, [test "x$with_xattrs" = "xyes"])

if test "x$with_xattrs" = "xyes"; then
	AC_DEFINE(ENABLE_XATTRS, 1, [Enable extended attributes support])
fi

AC_ARG_ENABLE([rollback], AC_HELP_STRING([--disable-rollback],[Disable rollback support]),
		[enable_rollback=$enableval],[enable_rollback=yes])
AM_CONDITIONAL(ENABLE_ROLLBACK, [test "x$enable_rollback" = "xyes"])

if test "x$enable_rollback" = "xyes"; then
	AC_DEFINE(ENABLE_ROLLBACK, 1, [Enable rollback support])
        # libmount is needed for rollback feature
        AC_CHECK_HEADER(libmount/libmount.h,[],[AC_MSG_ERROR([Cannout find libmount headers. Please install libmount-devel])])
fi

AC_ARG_ENABLE([btrfs-quota], AC_HELP_STRING([--disable-btrfs-quota],[Disable btrfs quota support]),
		[enable_btrfs_quota=$enableval],[enable_btrfs_quota=yes])
AM_CONDITIONAL(ENABLE_BTRFS_QUOTA, [test "x$enable_btrfs_quota" = "xyes"])

if test "x$enable_btrfs_quota" = "xyes"; then
	AC_DEFINE(ENABLE_BTRFS_QUOTA, 1, [Enable btrfs quota support])
fi

AC_ARG_ENABLE([pam], AC_HELP_STRING([--disable-pam],[Disable pam plugin support]),
                [with_pam=$enableval],[with_pam=yes])
AM_CONDITIONAL(HAVE_PAM, [test "x$with_pam" = "xyes"])

AC_ARG_ENABLE([selinux], AC_HELP_STRING([--enable-selinux],[Enable support for SELinux LSM]),
		[with_selinux=$enableval],[with_selinux=no])
AM_CONDITIONAL(ENABLE_SELINUX, [test "x$enable_selinux" = "xyes"])

if test "x$with_selinux" = "xyes"; then
	AC_DEFINE(ENABLE_SELINUX, 1, [Enable SELinux support])
	AC_CHECK_HEADER(selinux/selinux.h,[],[AC_MSG_ERROR([Cannot find libselinux headers. Please install libselinux-devel])])
	AC_CHECK_LIB(selinux, selinux_snapperd_contexts_path, [], [AC_MSG_ERROR([selinux library does not provide selinux_snapperd_contexts_path symbol])])
fi

PKG_CHECK_MODULES(DBUS, dbus-1)

AC_CHECK_HEADER(acl/libacl.h,[],[AC_MSG_ERROR([Cannout find libacl headers. Please install libacl-devel])])

AC_SUBST(VERSION)
AC_SUBST(LIBVERSION_MAJOR)
AC_SUBST(LIBVERSION_MINOR)
AC_SUBST(LIBVERSION_PATCHLEVEL)
AC_SUBST(LIBVERSION_INFO)
AC_SUBST(docdir)
AC_SUBST(fillupdir)

AC_OUTPUT(
	Makefile
	snapper/Makefile
	snapper/Version.h:snapper/Version.h.in
	examples/Makefile
	examples/c/Makefile
	examples/c++-lib/Makefile
	dbus/Makefile
	server/Makefile
	client/Makefile
	client/utils/Makefile
	scripts/Makefile
	pam/Makefile
	data/Makefile
	doc/Makefile
	doc/snapper.xml:doc/snapper.xml.in
	doc/snapperd.xml:doc/snapperd.xml.in
	doc/snapper-configs.xml:doc/snapper-configs.xml.in
	doc/snapper-zypp-plugin.xml:doc/snapper-zypp-plugin.xml.in
	doc/snapper-zypp-plugin.conf.xml:doc/snapper-zypp-plugin.conf.xml.in
	doc/pam_snapper.xml:doc/pam_snapper.xml.in
	doc/mksubvolume.xml:doc/mksubvolume.xml.in
	po/Makefile
	testsuite/Makefile
	testsuite-real/Makefile
	testsuite-cmp/Makefile
	package/snapper.spec:snapper.spec.in
)
