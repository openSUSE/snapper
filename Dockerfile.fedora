# Build Fedora 27 image
FROM fedora:27

RUN dnf -y install \
  autoconf \
  automake \
  boost-devel \
  dbus-devel \
  docbook-style-xsl \
  e2fsprogs-devel \
  gcc-c++ \
  gettext \
  glibc-langpack-de \
  glibc-langpack-en \
  libacl-devel \
  libmount-devel \
  libtool \
  libxml2-devel \
  libxslt \
  make \
  pam-devel \
  pkgconfig \
  rpm-build

RUN mkdir -p /usr/src/app
WORKDIR /usr/src/app

COPY . /usr/src/app
