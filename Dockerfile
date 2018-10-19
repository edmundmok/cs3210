FROM ubuntu:xenial
MAINTAINER Edmund Mok <edmundmok@outlook.com>

# install essential system progs
RUN apt-get update && apt-get install -y \
  gcc \
  g++ \
  g++-5 \
  git \
  htop \
  libomp-dev \
  linux-tools-common \
  linux-tools-generic \
  make \
  man \
  openmpi-bin \
  openmpi-common \
  openmpi-doc \
  openssh-client \
  openssh-server \
  libopenmpi-dev \
  python \
  python-pip \
  vim

# create labs folder
CMD mkdir /cs3210

# link perf in linux-tools-generic
RUN rm /usr/bin/perf
RUN ln -s /usr/lib/linux-tools/4.4.0-135-generic/perf /usr/bin/perf
