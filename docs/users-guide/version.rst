==============================
Viskores Version
==============================

.. index:: version

As the |Viskores| code evolves, changes to the interface and behavior will
inevitably happen.
Consequently, code that links into |Viskores| might need a specific version of
|Viskores| or changes its behavior based on what version of |Viskores| it is using.
To facilitate this, |Viskores| software is managed with a versioning system and
advertises its version in multiple ways.
As with many software products, |Viskores| has three version numbers: major,
minor, and patch.
The major version represents significant changes in the |Viskores|
implementation and interface.
Changes in the major version include backward incompatible changes.
The minor version represents added functionality.
Generally, changes in the minor version to not introduce changes to the API.
The patch version represents fixes provided after a release occurs.
Patch versions represent minimal change and do not add features.

.. index::
   triple: CMake ; Viskores package ; version

If you are writing a software package that is managed by CMake and load |Viskores| with the :cmake:command:`find_package` command as described in :secref:`building:Linking to |Viskores|`, then you can query the |Viskores| version directly in the CMake configuration.
When you load |Viskores| with :cmake:command:`find_package`, CMake sets the variables :cmake:variable:`Viskores_VERSION_MAJOR`, :cmake:variable:`Viskores_VERSION_MINOR`, and :cmake:variable:`Viskores_VERSION_PATCH` to the major, minor, and patch versions, respectively.
Additionally, :cmake:variable:`Viskores_VERSION` is set to the "major.minor" version number and :cmake:variable:`Viskores_VERSION_FULL` is set to the "major.minor.patch" version number.
If the current version of |Viskores| is actually a development version that is in between releases of |Viskores|, then and abbreviated SHA of the git commit is also included as part of :cmake:variable:`Viskores_VERSION_FULL`.

.. didyouknow::
  If you have a specific version of |Viskores| required for your software, you can also use the version option to the :cmake:command:`find_package` CMake command.
  The :cmake:command:`find_package` command takes an optional version argument that causes the command to fail if the wrong version of the package is found.

.. index:: version ; macro

It is also possible to query the |Viskores| version directly in your code through preprocessor macros.
The :file:`viskores/Version.h` header file defines the following preprocessor macros to identify the |Viskores| version.

.. c:macro:: VISKORES_VERSION

   The version number of the loaded |Viskores| package.
   This is in the form "major.minor".

.. c:macro:: VISKORES_VERSION_FULL

   The extended version number of the |Viskores| package including patch and in-between-release information.
   This is in the form "major.minor.patch[.gitsha1]" where "gitsha" is only included if the source code is in between releases.

.. c:macro:: VISKORES_VERSION_MAJOR

   The major |Viskores| version number.

.. c:macro:: VISKORES_VERSION_MINOR

   The minor |Viskores| version number.

.. c:macro:: VISKORES_VERSION_PATCH

   The patch |Viskores| version number.

.. commonerrors::
  Note that the CMake variables all begin with ``Viskores_`` (lowercase "m") whereas the preprocessor macros begin with ``VISKORES_`` (all uppercase).
  This follows the respective conventions of CMake variables and preprocessor macros.

Note that :file:`viskores/Version.h` does not include any other |Viskores| header files.
This gives your code a chance to load, query, and react to the |Viskores| version before loading any |Viskores| code proper.
