FROM opensuse/tumbleweed

RUN zypper refresh && \
    zypper update -y && \
    zypper install -y --no-recommends \
      clang15 \
      curl \
      git \
      ShellCheck \
      && \
    zypper clean --all

# Check that the target programs has been installed
RUN command -v clang-format > /dev/null && \
    command -v shellcheck > /dev/null
