## pkg-config files now install to libdir/pkgconfig

Viskores now installs pkg-config files to `${Viskores_INSTALL_LIB_DIR}/pkgconfig` instead of the
share directory. This is the default location where pkg-config searches for
.pc files, allowing pkg-config to find Viskores without additional configuration.
