FROM ubuntu:xenial
MAINTAINER Edmund Mok <edmundmok@outlook.com>

# install essential system progs
RUN apt-get update && apt-get install -y \
  gcc \
  g++ \
  g++-5 \
  git \
  make \
  man \
  python \
  python-pip \
  vim

# create labs folder
CMD mkdir /labs
