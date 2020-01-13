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
rpmbuild -bb --with coverage -D "fedora_version 25" -D "jobs `nproc`" package/*.spec

# test the %pre/%post scripts by installing/updating/removing the built packages
# ignore the dependencies to make the test easier, as a smoke test it's good enough
rpm -iv --force --nodeps /root/rpmbuild/RPMS/*/*.rpm
rpm -Uv --force --nodeps /root/rpmbuild/RPMS/*/*.rpm

# smoke test, make sure snapper at least starts
snapper --version

# Run the integration test
# Running it in the source tree ensures that the coverage report finds it
pushd /root/rpmbuild/BUILD/snapper-*/testsuite-real
./setup-and-run-all
popd

# Coverage report
pushd /root/rpmbuild/BUILD/snapper-*
make coverage
popd

# get the plain package names and remove all packages at once
rpm -ev --nodeps `rpm -q --qf '%{NAME} ' -p /root/rpmbuild/RPMS/**/*.rpm`
