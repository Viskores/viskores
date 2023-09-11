# Added Makefile contract tests

Added Makefile contract tests to ensure that the VTK-m smoke test example
application can be built and run using a Makefile against a VTK-m install tree.
This will help users who use bare Make as their build system. Additionally,
fixed both the VTK-m pkg-config `vtkm.pc` and the `vtkm_config.mk` file to
ensure that both files are correctly generated and added CI coverage to ensure
that they are always up-to-date and correct. This improves support for users
who use bare Make as their build system, and increases confidence in the
correctness of both the VTK-m pkg-config file `vtkm.pc` and of the Makefile
`vtkm_config.mk`.

You can run these tests with: `ctest -R smoke_test`
