# Build Ubuntu 18.10 image
FROM ubuntu:18.10

# see https://docs.docker.com/engine/userguide/eng-image/dockerfile_best-practices/#/run
RUN apt-get update && \
  apt-get install -y --no-install-recommends \
  acl-dev \
  autoconf \
  automake \
  build-essential \
  debhelper \
  devscripts \
  docbook-xsl \
  fakeroot \
  g++ \
  gettext \
  language-pack-de \
  language-pack-en \
  libboost-dev \
  libboost-system-dev \
  libboost-test-dev \
  libboost-thread-dev \
  libdbus-1-dev \
  libmount-dev \
  libpam-dev \
  libtool \
  libxml2-dev \
  libz-dev \
  xsltproc

RUN mkdir -p /usr/src/app
WORKDIR /usr/src/app

COPY . /usr/src/app
