#! /bin/bash

set -e -x

make -f Makefile.repo
make package

# Build the binary package locally, use plain "rpmbuild" to make it simple.
# "osc build" is too resource hungry (builds a complete chroot from scratch).
# Moreover it does not work in a Docker container (it fails when trying to mount
# /proc and /sys in the chroot).
mkdir -p /root/rpmbuild/SOURCES
cp package/* /root/rpmbuild/SOURCES
rpmbuild -bb -D "fedora_version 25" -D "jobs `nproc`" package/*.spec

# test the %pre/%post scripts by installing/updating/removing the built packages
# ignore the dependencies to make the test easier, as a smoke test it's good enough
rpm -iv --force --nodeps /root/rpmbuild/RPMS/**/*.rpm

# smoke test, make sure snapper at least starts
snapper --version

rpm -Uv --force --nodeps /root/rpmbuild/RPMS/**/*.rpm
# get the plain package names and remove all packages at once
rpm -ev --nodeps `rpm -q --qf '%{NAME} ' -p /root/rpmbuild/RPMS/**/*.rpm`
