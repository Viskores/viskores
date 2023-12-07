==============================
VTK-m Version
==============================

.. index:: version

As the |VTKm| code evolves, changes to the interface and behavior will
inevitably happen.
Consequently, code that links into |VTKm| might need a specific version of
|VTKm| or changes its behavior based on what version of |VTKm| it is using.
To facilitate this, |VTKm| software is managed with a versioning system and
advertises its version in multiple ways.
As with many software products, |VTKm| has three version numbers: major,
minor, and patch.
The major version represents significant changes in the |VTKm|
implementation and interface.
Changes in the major version include backward incompatible changes.
The minor version represents added functionality.
Generally, changes in the minor version to not introduce changes to the API.
The patch version represents fixes provided after a release occurs.
Patch versions represent minimal change and do not add features.

.. index::
   triple: CMake ; VTK-m package ; version

If you are writing a software package that is managed by CMake and load |VTKm| with the :cmake:command:`find_package` command as described in :secref:`building:Linking to |VTKm|`, then you can query the |VTKm| version directly in the CMake configuration.
When you load |VTKm| with :cmake:command:`find_package`, CMake sets the variables :cmake:variable:`VTKm_VERSION_MAJOR`, :cmake:variable:`VTKm_VERSION_MINOR`, and :cmake:variable:`VTKm_VERSION_PATCH` to the major, minor, and patch versions, respectively.
Additionally, :cmake:variable:`VTKm_VERSION` is set to the "major.minor" version number and :cmake:variable:`VTKm_VERSION_FULL` is set to the "major.minor.patch" version number.
If the current version of |VTKm| is actually a development version that is in between releases of |VTKm|, then and abbreviated SHA of the git commit is also included as part of :cmake:variable:`VTKm_VERSION_FULL`.

.. didyouknow::
  If you have a specific version of |VTKm| required for your software, you can also use the version option to the :cmake:command:`find_package` CMake command.
  The :cmake:command:`find_package` command takes an optional version argument that causes the command to fail if the wrong version of the package is found.

.. index:: version ; macro

It is also possible to query the |VTKm| version directly in your code through preprocessor macros.
The :file:`vtkm/Version.h` header file defines the following preprocessor macros to identify the |VTKm| version.

.. c:macro:: VTKM_VERSION

   The version number of the loaded |VTKm| package.
   This is in the form "major.minor".

.. c:macro:: VTKM_VERSION_FULL

   The extended version number of the |VTKm| package including patch and in-between-release information.
   This is in the form "major.minor.patch[.gitsha1]" where "gitsha" is only included if the source code is in between releases.

.. c:macro:: VTKM_VERSION_MAJOR

   The major |VTKm| version number.

.. c:macro:: VTKM_VERSION_MINOR

   The minor |VTKm| version number.

.. c:macro:: VTKM_VERSION_PATCH

   The patch |VTKm| version number.

.. commonerrors::
  Note that the CMake variables all begin with ``VTKm_`` (lowercase "m") whereas the preprocessor macros begin with ``VTKM_`` (all uppercase).
  This follows the respective conventions of CMake variables and preprocessor macros.

Note that :file:`vtkm/Version.h` does not include any other |VTKm| header files.
This gives your code a chance to load, query, and react to the |VTKm| version before loading any |VTKm| code proper.
