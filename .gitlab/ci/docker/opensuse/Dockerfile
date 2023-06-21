FROM opensuse/tumbleweed
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@kitware.com>"

# Base dependencies for building VTK-m projects
RUN zypper refresh && \
    zypper update -y && \
    zypper install -y --no-recommends \
      cmake \
      curl \
      gcc13-c++ \
      git \


      hdf5-devel \
      libgomp1 \
      mpich \
      mpich-devel \
      ninja \
      python311 \
      python311-scipy \
      tbb-devel && \
    zypper clean --all





