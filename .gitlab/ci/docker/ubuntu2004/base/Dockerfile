FROM ubuntu:20.04
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@kitware.com>"

ENV TZ=America/New_York

# Base dependencies for building VTK-m projects
RUN apt update && DEBIAN_FRONTEND="noninteractive" apt install -y --no-install-recommends \
      cmake \
      curl \
      g++ \
      git \


      libmpich-dev \
      libomp-dev \
      libtbb-dev \
      libhdf5-dev \
      mpich \
      ninja-build \
      python \
      python3-scipy \
      software-properties-common && \
      apt clean



# system packaged version


