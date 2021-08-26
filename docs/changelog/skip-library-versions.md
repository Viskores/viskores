# skip library versions

The `VTKm_SKIP_LIBRARY_VERSIONS` variable is now available to skip the SONAME
and SOVERSION fields (or the equivalent for non-ELF platforms).

Some deployments (e.g., Python wheels or Java `.jar` files) do not support
symlinks reliably and the way the libraries get loaded just leads to
unnecessary files in the packaged artifact.
