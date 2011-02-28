#
# Makefile.am for snapper/data
#

fillup_DATA = sysconfig.snapper

logrotate_DATA = snapper

EXTRA_DIST = $(fillup_DATA) $(logrotate_DATA)

install-data-local:
	mkdir -p $(DESTDIR)/etc/sysconfig/snapper.d

