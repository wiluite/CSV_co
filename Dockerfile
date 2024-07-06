FROM ubuntu:22.04
LABEL Description="Build environment"

ENV HOME /root

SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get -y --no-install-recommends install \
    build-essential \
    cmake \
    gdb \
    wget \
    python3 \
    clang-15 \
    libc++-15-dev \
    libc++abi-15-dev \
    pip

RUN pip3 install csvkit
