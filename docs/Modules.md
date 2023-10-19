# Specifying modules in the VTK-m build system

The VTK-m build system comes with a module mechanism that allows a library
or other target be optionally compiled based on CMake configuration
variables. Additionally, modules can be optionally compiled based on their
dependencies. That is, a module can be turned on if a module that depends
on it wants to be compiled. Likewise, a module can be turned off if a
module that it depends on cannot be compiled.

## Module configuration

All modules have a "name" that is the same as the target created by the
module (for example, the name of a library). Every module has an associated
(advanced) CMake variable named `VTKm_MODULE_ENABLE_<name>`. For example,
the module that builds the `vtkm_filter_entity_extraction` filter has an
associated CMake variable named
`VTKm_MODULE_ENABLE_vtkm_filter_entity_extraction`. This CMake variable can
be used to control whether the module should be included in the build. It
can be set to one of the following values.

  * `YES`: Always create the module. If it is not possible to create the
    module, the CMake configuration fails.
  * `WANT`: Create the module if possible. If it is possible to create all
    modules it depends on, then this module will be created. If this is not
    possible, this module will not be created, but this will not cause an
    error in the configuration.
  * `DONT_WANT`: Create the module only if there is a dependency that
    requires it. This is useful for stripping out modules not directly
    needed but required for a select list of modules desired.
  * `NO`: Never create the module. Any module that depends on this module
    will also not be built.
  * `DEFAULT`: Does the default behavior. This is typically either `WANT`
    or `DONT_WANT` depending on other configuration.

The advantage of having these multiple options is that it becomes possible
to turn off all modules except a select desired few and have the CMake
configuration automatically determine dependencies.

### Module groups

Modules can also declare themselves as part of a group. Module groups
provide a way to turn on/off the build of several related modules. For
example, there is a module group named `FiltersCommon` that contains
modules with the most commonly used filters in VTK-m.

Every module group has an associated (advanced) CMake variable named
`VTKm_GROUP_ENABLE_<name>`. For example, the `FiltersCommon` group has an
associated CMake variable named `VTKm_GROUP_ENABLE_FiltersCommon`. This
variable can be set to the same `YES`/`WANT`/`DONT_WANT`/`NO`/`DEFAULT`
values as those for the `VTKm_MODULE_ENABLE` variables described earlier.

### Default behavior

If a `VTKm_MODULE_ENABLE_*` variable is set to `DEFAULT`, then the
configuration first checks all the `VTKm_GROUP_ENABLE_*` variables
associated with the groups the module belongs to. It will use the first
value not set to `DEFAULT` that it encounters.

If all the module's group are also set to `DEFAULT` (or the module does not
belong to any groups) then the behavior is based on the
`VTKm_BUILD_ALL_LIBRARIES` CMake variable. If `VTKm_BUILD_ALL_LIBRARIES` is
`ON`, then the default behavior becomes `WANT`. Otherwise, it becomes
`DONT_WANT`.

## Specifying a module

A module is created in much the same way as a normal target is made in
CMake: Create a directory with the appropriate source material and add a
`CMakeLists.txt` file to specify how they are built. However, the main
difference is that you do _not_ link to the directory with a CMake
`add_subdirectory` command (or any other command like `include` or
`subdirs`).

Instead, you simply create a file named `vtkm.module` and place it in the
same directory with the `CMakeLists.txt` file. The VTK-m configuration will
automatically find this `vtkm.module` file, recognize the directory as
containing a module, and automatically include the associated
`CMakeLists.txt` in the build (given that the CMake configuration turns on
the module to be compiled).

Each `vtkm.module` is a simple text file that contains a list of options.
Each option is provided by giving the name of the option followed by the
arguments for that option. The following options can be defined in a
`vtkm.module` file. `NAME` is required, but the rest are optional.

  * `NAME`: The name of the target created by the module.
  * `GROUPS`: A list of all groups the module belongs to. If a module's
    enable flag is set to `DEFAULT`, then the enable option is taken from
    the groups it belongs to.
  * `DEPENDS`: A list of all modules (or other libraries) on which this
    module depends. Everything in this list is added as a link library to
    the library created with `vtkm_library`.
  * `PRIVATE_DEPENDS`: Same as `DEPENDS` except that these libraries are
    added as private link libraries.
  * `OPTIONAL_DEPENDS`: A list of all modules that that are not strictly
    needed but will be used if available.
  * `TEST_DEPENDS`: A list of all modules (or other libraries) on which the
    tests for this module depends.
  * `TEST_OPTIONAL_DEPENDS`: A list of all modules that the test executable
    will like to if they exist, but are not necessary.
  * `NO_TESTING`: Normally, a module is expected to have a subdirectory
    named `testing`, which will build any necessary testing executables and
    add ctest tests. If this option is given, no tests are added. (Note,
    modules generally should have tests.)
  * `TESTING_DIR`: Specify the name of the testing subdirectory. If not
    provided, `testing` is used.
	
A `vtkm.module` file may also have comments. Everything between a `#` and
the end of the line will be ignored.

As an example, the `vtkm_filter_entity_extraction` module (located in
`vtkm/filter/entity_extraction` has a `vtkm.module` file that looks like
the following.

``` cmake
NAME
  vtkm_filter_entity_extraction
GROUPS
  FiltersCommon
  Filters
DEPENDS
  vtkm_worklet
  vtkm_filter_core
  vtkm_filter_clean_grid
TEST_DEPENDS
  vtkm_filter_clean_grid
  vtkm_filter_entity_extraction
  vtkm_source
```

## Building the module

As mentioned earlier, a VTK-m module directory has its own
`CMakeLists.txt`. There does not have to be anything particularly special
about the `CMakeLists.txt`. If the module is building a library target
(which is typical), it should use the `vtkm_library` CMake command to do so
to make sure the proper compiler flags are added.

Here is an example portion of the `CMakeLists.txt` for the
`vtkm_filter_entity_extraction` module. (Mainly, the definition of
variables containing source and header files is left out.)

``` cmake
vtkm_library(
  NAME vtkm_filter_entity_extraction
  HEADERS ${entity_extraction_headers}
  DEVICE_SOURCES ${entity_extraction_sources_device}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_entity_extraction)
```

Note that if a library created by a module depends on the library created
by another module, it should be in the `DEPENDS` list of `vtkm.module`. For
example, the `vtkm.module` contains `vtkm_filter_clean_grid` in its
`DEPENDS` list, and that library will automatically be added as a target
link library to `vtkm_filter_entity_extraction`. You should avoid using
`target_link_libraries` to link one module to another as the modules will
not be able to guarantee that all the targets will be created correctly.

Also note that the `CMakeLists.txt` file should _not_ include its testing
directory with `add_subdirectory`. As described in the next section, the
testing directory will automatically be added, if possible. (Using
`add_subdirectory` for other subdirectories on which the module depends is
OK.)

## Module testing directory

All modules are expected to have a `testing` subdirectory. This
subdirectory should contain its own `CMakeLists.txt` that, typically,
builds a testing executable and adds the appropriate tests. (This is
usually done with the `vtkm_unit_tests` CMake function.)

However, a module should _not_ include its own `testing` directory with
`add_subdirectory`. This is because the tests for a module might have
dependencies that the module itself does not. For example, it is common for
filter tests to use a source to generate some test data. But what if the
CMake configuration has the source module turned off? Should the filter
module be turned off because the tests need the source module? No. Should
the source module be turned on just because some tests want it? No.

To resolve this issue, VTK-m modules allow for an extended set of
dependencies for the tests. This is specified with the `TEST_DEPENDS`
variable in `vtkm.module`. It will then add the test only if all the test
dependencies are met.

If the dependencies for both the module itself and the module's tests are
met, then the `testing` subdirectory of the module will be added to the
build. Like for the module itself, the `CMakeLists.txt` in the `testing`
directory should build tests just like any other CMake directory. Here is
an example `CMakeLists.txt` for the `vtkm_filter_entity_extraction` module.

``` cmake
set(unit_tests
  UnitTestExternalFacesFilter.cxx
  UnitTestExtractGeometryFilter.cxx
  UnitTestExtractPointsFilter.cxx
  UnitTestExtractStructuredFilter.cxx
  UnitTestGhostCellRemove.cxx
  UnitTestMaskFilter.cxx
  UnitTestMaskPointsFilter.cxx
  UnitTestThresholdFilter.cxx
  UnitTestThresholdPointsFilter.cxx
  )

set(libraries
  vtkm_filter_clean_grid
  vtkm_filter_entity_extraction
  vtkm_source
  )

vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  USE_VTKM_JOB_POOL
)
```

## Testing if a module is being built

The easiest way to test if a module is being built (in CMake) is to check
whether the associated target exists.

``` cmake
if(TARGET vtkm_filter_entity_extraction)
  # Do stuff dependent on vtkm_filter_entity_extraction library/module
endif()
```

Note that this only works in a module if the module properly depends on the
named target. It only works outside of modules if modules have already been
processed.

## Debugging modules

Because modules depend on each other, and these dependencies affect whether
a particular module will be built, it can sometimes be difficult to
understand why a particular module is or is not built. To help diagnose
problems with modules, you can turn on extra reporting with the
`VTKm_VERBOSE_MODULES` CMake variable.

When `VTKm_VERBOSE_MODULES` is set to `OFF` (the default), then the parsing
and dependency resolution of the modules is silent unless there is an
error. When `VTKm_VERBOSE_MODULES` is set to `ON`, then information about
what modules are found, which modules are built, and why they are or are
not built are added as status messages during CMake configuration.
