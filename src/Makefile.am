# Makefile.am for snapper/src

yncludedir = @yncludedir@/snapper

client_DATA =			\
	snapper.ycp

ynclude_DATA =			\
	helps.ycp		\
	wizards.ycp		\
	dialogs.ycp

desktop_DATA =			\
	snapper.desktop

module_DATA =			\
	Snapper.ycp

# create a symlink for local build, #145327
snapper:
	ln -sf . $@
ycpchook = snapper

EXTRA_DIST = $(client_DATA) $(ynclude_DATA) $(module_DATA) $(desktop_DATA)

include $(top_srcdir)/Makefile.am.common
