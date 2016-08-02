Any distribution interested in enabling selinux support in snapper should be aware
of following requirements to be able to run snapper in confined environment
properly.

The snapper with enabled selinux support requires following symbol provided by
distributed libselinux package: selinux_snapperd_contexts_path. The symbol is
available in libselinux upstream [1] since commit "b2c1b0baaf52" which should
land in libselinux version 2.6 and higher.

Also distribution is expected to install a file located on a path acquired via the
call above. Usually the file is packaged together with selinux-policy. Currently
the minimal required file content is as follows:

snapperd_data = system_u:object_r:snapperd_data_t:s0

Content description:

a) the selinux context referenced by key 'snapperd_data' is used to label all
snapper metadata stored in (including) /mnt/dir/.snapshots subvolume or directory.

Keep this file up to date whenever requirements on a selinux enabled snapper gets
changed!

[1] https://github.com/SELinuxProject/selinux
