
Snapper
=======

Snapper is a tool for Linux file system snapshot management. Apart from the
obvious creation and deletion of snapshots it can compare snapshots and
revert differences between them. In simple terms, this allows root and
non-root users to view older versions of files and revert changes.

For more information visit [snapper.io](http://snapper.io/).


Development
-----------

For compiling and developing Snapper you need to setup the development
environment first.

### Development Environment

In the SUSE Linux Enterprise and openSUSE distributions you can install the needed
packages by using these commands:

```sh
# install the basic development environment (SUSE Linux Enterprise, the SDK extension is needed)
sudo zypper install -t pattern SDK-C-C++
# install the basic development environment (openSUSE)
sudo zypper install -t pattern devel_C_C++
# install the extra packages for snapper development (both SLE and openSUSE)
sudo zypper install git libmount-devel dbus-1-devel libacl-devel \
  docbook-xsl-stylesheets libxml2-devel libbtrfs-devel
```

### Building Snapper

You can download the sources and build Snapper by using these commands:

```sh
git clone git@github.com:<your_fork>/snapper.git
cd snapper
make -f Makefile.repo
# parallelize the build using more processors, use plain `make` if it does not work
make -j`nproc`
```

### Installing and Running Snapper

To run the freshly built Snapper use this:

```sh
sudo make install
# kill the currently running DBus process if present
sudo killall snapperd
# try your changes (the DBus service is started automatically)
(sudo) snapper ...
```

### Releasing

- Before releasing the Snapper package ensure that the changes made to the package
are mentioned in the `package/snapper.changes` file.

- When the version is increased then the Git repo has to be tagged, use the
`vX.Y.Z` format for the tag. Also the
[filesystems:snapper](https://build.opensuse.org/project/show/filesystems:snapper)
OBS project has to be updated.

- To create the package use command `make package`. Then use the common work-flow to submit
package to the build service. For [openSUSE:Factory](https://build.opensuse.org/project/show/openSUSE:Factory)
send at first the package to the devel project
[YaST:Head](https://build.opensuse.org/project/show/YaST:Head) in OBS.  
*Please note that this OBS project builds for more distributions so more metadata
files have to be updated. See the OBS documentation for more info ([cross distribution
howto](https://en.opensuse.org/openSUSE:Build_Service_cross_distribution_howto),
[Debian builds](https://en.opensuse.org/openSUSE:Build_Service_Debian_builds)).*

- The generated bzip2 tarball has to be also placed at
[ftp.suse.com/pub/projects/snapper](ftp://ftp.suse.com/pub/projects/snapper).

- When the documentation changes e.g. the man page or an important functionality then also
the [snapper.io](http://snapper.io/) web pages have to be updated. They are hosted
as GitHub pages in the [gh-pages branch](https://github.com/openSUSE/snapper/tree/gh-pages)
in the Snapper Git repository.
