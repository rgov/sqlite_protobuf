FROM debian:stretch

RUN echo "deb http://ftp.debian.org/debian stretch-backports main" >> /etc/apt/sources.list \
    && apt-get update \
    && apt-get -y -t stretch-backports install \
        cmake \
        libprotobuf-dev \
        libsqlite3-dev \
        ninja-build \
        protobuf-compiler \
        python3 \
        python3-pip \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m pip install pipenv

# One of pipenv's dependencies complains if we don't set these
ENV LC_ALL=C.UTF-8 \
    LANG=C.UTF-8
