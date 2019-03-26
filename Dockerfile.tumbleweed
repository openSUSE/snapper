# Build the latest openSUSE Tumbleweed image
FROM opensuse/tumbleweed

RUN zypper --non-interactive in --no-recommends \
  autoconf \
  automake \
  dbus-1-devel \
  docbook-xsl-stylesheets \
  e2fsprogs-devel \
  gcc-c++ \
  grep \
  libacl-devel \
  libboost_system-devel \
  libboost_test-devel \
  libboost_thread-devel \
  libbtrfs-devel \
  libmount-devel \
  libtool \
  libxml2-devel \
  libxslt \
  obs-service-source_validator \
  pam-devel \
  rpm-build \
  which

RUN mkdir -p /usr/src/app
WORKDIR /usr/src/app

COPY . /usr/src/app
