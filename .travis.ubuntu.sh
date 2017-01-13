#! /bin/bash

set -e -x

make -f Makefile.repo

# copy the Debian files to the expected place
cp -a dists/debian debian

# build binary packages
dpkg-buildpackage -j`nproc` -rfakeroot -b

# install the built packages
dpkg -i ../*.deb

# smoke test, make sure snapper at least starts
snapper --version

