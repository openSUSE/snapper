#!/bin/bash -xe

make -f Makefile.repo
make clean
make check


./configure --disable-btrfs --disable-btrfs-quota --disable-rollback 
make clean
make check

./configure --disable-btrfs --disable-btrfs-quota --disable-rollback --enable-selinux
make clean
make check

./configure --disable-btrfs-quota --disable-rollback
make clean
make check

./configure --disable-btrfs-quota --disable-rollback --enable-selinux
make clean
make check

./configure --disable-btrfs-quota
make clean
make check

./configure --disable-rollback
make clean
make check

./configure --disable-zypp
make clean
make check


./configure --disable-lvm
make clean
make check


./configure --disable-ext4
make clean
make check

