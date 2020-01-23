if HAVE_ZYPP
TEST_LOG_DRIVER = $(SHELL) tap-driver.sh

check_SCRIPTS = 1-happy.test 2-malformed-xml.test 3-invalid-xml.test 4-badjson.test 99-shell.test
noinst_SCRIPTS = mock-snapperd test-helper.sh
noinst_DATA = \
	zypp-plugin-invalid.conf \
	zypp-plugin-malformed.conf \
	zypp-plugin-maxcover.conf

# Hack: these tests both use mock-snapperd which is not paralellized
# so serialize them
4-badjson.log: 1-happy.log

TESTS = $(check_SCRIPTS)

EXTRA_DIST = $(check_SCRIPTS) $(noinst_SCRIPTS) $(noinst_DATA) tap-driver.sh
endif
