if HAVE_ZYPP
LOG_COMPILER = dbus-run-session
AM_LOG_FLAGS = --

check_SCRIPTS = test1 test2-malformed-xml test3-invalid-xml test4-badjson
noinst_SCRIPTS = mock-snapperd
noinst_DATA = zypp-plugin-malformed.conf zypp-plugin-invalid.conf

TESTS = $(check_SCRIPTS)
XFAIL_TESTS = test2-malformed-xml test3-invalid-xml test4-badjson

EXTRA_DIST = $(check_SCRIPTS) $(noinst_SCRIPTS) $(noinst_DATA)
endif
