
The "old" way using the steps 1 to 5 is deprecated.


The "new" way is to use the two steps "filesystem" and "config".

The step "filesystem" does the following:

  create btrfs subvolume /<root-prefix>/.snapshots
  create directory /<root-prefix>/.snapshots/1
  create btrfs subvolume /<root-prefix>/.snapshots/1/snapshot
  set default btrfs subvolume to /<root-prefix>/.snapshots/1/snapshot
  create directory /<root-prefix>/.snapshots/1/snapshot/.snapshots

The step "config" does the following:

  create snapper sysconfig /<root-prefix>/etc/sysconfig/snapper
  create snapper config /<root-prefix>/etc/snapper/configs/root
  create snapper info file /<root-prefix>/.snapshots/1/info.xml

The installer has to mount the filesystem before the step
"filesystem". Between the two steps the filesystem must be remounted
(since the default subvolume was changes). Additionally the .snapshots
subvolume must be mounted by the installer.

The installer must also handle /etc/fstab or similar.

Works with and without the optional "@" subvolume.

No plugins are run.

Example workflow can be seen in test1.sh and test2.sh.

The "filesystem" step can of course be implemented somewhere
else. E.g. libstorage-ng is capable of doing this (see example there).

