##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

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
