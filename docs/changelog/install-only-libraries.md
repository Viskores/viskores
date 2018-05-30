# Add a new cmake option: VTKm_INSTALL_ONLY_LIBRARIES

The new cmake option VTKm_INSTALL_ONLY_LIBRARIES when enabled will cause
VTK-m to only install libraries. This is useful for projects that are
producing an application and don't want to ship headers or CMake infrastructure.

For example, this flag is enabled by ParaView for releases.

This flag is disabled by default.
