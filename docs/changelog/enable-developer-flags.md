# Add a new cmake option: VTKm_ENABLE_DEVELOPER_FLAGS

The new cmake option VTKm_ENABLE_DEVELOPER_FLAGS can be used to enable/disable
warnings in VTK-m. It is useful to disable VTK-m's warning flags when VTK-m is
directly embedded by a project as sub project (add_subdirectory), and the
warnings are too strict for the project. This does not apply when using an
installed version of VTK-m.

For example, this flag is disabled in VTK.

This flag is enabled by default.
