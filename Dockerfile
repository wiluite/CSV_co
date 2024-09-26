FROM ubuntu:22.04
LABEL Description="Build environment"

ENV HOME=/root
ENV SOCI_DB_SQLITE3="sqlite3://db=sample.sqlite"
ENV SOCI_DB_MYSQL="mysql://db=testdb user=root"

SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get -qy --no-install-recommends install \
    build-essential \
    cmake \
    gdb \
    wget \
    python3 \
    clang-15 \
    libc++-15-dev \
    libc++abi-15-dev \
    pip \
    libmysql++-dev

RUN apt-get install -y mysql-server && sleep 1
RUN service mysql start && mysql -u root -e "CREATE DATABASE testdb" && service mysql stop
RUN pip3 install csvkit
