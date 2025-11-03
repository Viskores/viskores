## Removed redundant version information from libraries

The Viskores library files were created with redundant version information. The
library name added the major.minor numbers to the filename, but then CMake
automatically added these numbers a second time. This redundant information has
been removed.
