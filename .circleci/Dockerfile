FROM debian:stretch

# Convenience packages not needed for build: gdb, git, nano
RUN echo "deb http://ftp.debian.org/debian stretch-backports main" >> /etc/apt/sources.list \
    && apt-get update \
    && apt-get -y -t stretch-backports install \
        cmake \
        gdb \
        git \
        libprotobuf-dev \
        libsqlite3-dev \
        nano \
        ninja-build \
        pkg-config \
        protobuf-compiler \
        python3 \
        python3-pip \
    && rm -rf /var/lib/apt/lists/*

# PATH="$(python3 -c 'import site; print(site.USER_BASE)')/bin:${PATH}" 
ENV PATH=/root/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

RUN python3 -m pip install --user --upgrade \
        pip \
        pipenv

# One of pipenv's dependencies complains if we don't set these
ENV LC_ALL=C.UTF-8 \
    LANG=C.UTF-8
