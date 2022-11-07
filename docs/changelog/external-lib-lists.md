# Output complete list of libraries for external Makefiles

There is a Makefile include, `vtkm_config.mk`, and a package include,
`vtkm.pc`, that are configured so that external programs that do not use
CMake have a way of importing VTK-m's configuration. However, the set of
libraries was hardcoded. In particular, many of the new filter libraries
were missing.

Rather than try to maintain this list manually, the new module mechanism
in the CMake configuration is used to get a list of libraries built and
automatically build these lists.
