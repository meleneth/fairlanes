FROM debian:trixie

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    locales \
    ca-certificates \
 && sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen \
 && locale-gen en_US.UTF-8 \
 && apt-get install -y --no-install-recommends \
    bash \
    build-essential \
    cmake \
    ninja-build \
    bubblewrap \
    gdb \
    clang \
    clangd \
    lldb \
    git \
    pkg-config \
    gcovr \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /workspaces/fairlanes
