#
# Makefile.am for snapper/completion/scripts
#

EXTRA_DIST = snapper-completion.bash snapper-completion.zsh snapper-completion.fish snbk-completion.bash

install-data-local:
	install -D --mode a+r,u+w snapper-completion.bash $(DESTDIR)/usr/share/bash-completion/completions/snapper
	install -D --mode a+r,u+w snapper-completion.zsh $(DESTDIR)/usr/share/zsh/site-functions/_snapper
	install -D --mode a+r,u+w snapper-completion.fish $(DESTDIR)/usr/share/fish/vendor_completions.d/snapper.fish
	install -D --mode a+r,u+w snbk-completion.bash $(DESTDIR)/usr/share/bash-completion/completions/snbk
