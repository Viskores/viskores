==============================
Regression Testing
==============================

.. index:: testing

|Viskores| has hundreds of built-in regression tests that test the functionality of the entire |Viskores| infrastructure on new platforms.
In this chapter, we discuss how to run regression tests in |Viskores| and how to create new regression tests.

.. didyouknow::
   |Viskores|'s regression test infrastructure is enabled by default if compiling from a git clone.
   If you do not need regression tests and want a faster compile time, you can disable it using the CMake configuration variable described in :secref:`building:Configuring |Viskores|`.


------------------------------
Running Regression Testing
------------------------------

.. _sec:RunningRegressionTests:

This section details how to run |Viskores|'s regression tests.
First, we explore how to use :command:`ctest` to run these tests.
:command:`ctest` is the easiest option for running regression tests because it automatically sets several arguments required by the testing infrastructure.
Second, we give an overview of how to run the regression tests without using :command:`ctest` and list the primary command-line arguments for doing so.


Regression Testing Using ctest
==============================

.. index::
   single: ctest
   pair: testing; ctest

The following code examples show how to run the regression tests in |Viskores| using :command:`ctest`.
:numref:`ex:RunningRegressionTests` shows how to run all the enabled regression tests in |Viskores|.

.. code-block:: bash
   :caption: Running all regression tests (Unix commands).
   :name: ex:RunningRegressionTests

   cd viskores-build
   ctest

You can get a list of all available tests by giving :command:`ctest` the ``-N`` option, which suppresses actually running the tests (see :numref:`ex:ListRegressionTests`).

.. code-block:: bash
   :caption: Listing all available regression tests (Unix commands).
   :name: ex:ListRegressionTests

   cd viskores-build
   ctest -N

Tests can be selected by using the ``-R`` option with :command:`ctest`.
The ``-R`` option is followed by a string or regular expression that matches the names of tests to run (see :numref:`ex:RunningSingleRegressionTests`).

.. code-block:: bash
   :caption: Running a single regression test (Unix commands).
   :name: ex:RunningSingleRegressionTests

   cd viskores-build
   ctest -R SystemInformation

Verbose testing output can be selected by using the ``-V`` option with :command:`ctest`.
The ``-V`` option causes the tests to print the underlying command used to launch each test, along with detailed test progression information (see :numref:`ex:RunningSingleRegressionTestVerbose`).

.. code-block:: bash
   :caption: Running a single regression test with verbose output (Unix commands). The verbose output first gives the exact command used to run the regression test, along with detailed test progression information.
   :name: ex:RunningSingleRegressionTestVerbose

   cd viskores-build
   ctest -R -V SystemInformation

.. commonerrors::
   Some of the regression tests in |Viskores| use data files stored in Git LFS.
   These files are automatically pulled when the |Viskores| repository is cloned.
   However, if the device on which you are compiling does not have Git LFS installed, these unit tests will fail.


Regression Testing Without ctest
================================

.. index::
   pair: testing; without ctest

It is also possible to run |Viskores| regression tests without using :command:`ctest`.
This can be accomplished by running individual unit-test wrappers located in the :file:`<path/to/viskores/build>/bin` directory.
These tests require specific command-line options to run correctly.

:numref:`ex:RunningSingleRegressionTestFullCommand` shows how to run a specific rendering test by passing the locations of the |Viskores| data and baseline directories.

.. code-block:: bash
   :caption: Running a single regression test without calling ctest (Unix commands).
   :name: ex:RunningSingleRegressionTestFullCommand

   UnitTests_viskores_rendering_testing \
     UnitTestMapperVolume \
     --data-dir=<path/to/viskores>/data \
     --baseline-dir=<path/to/viskores>/baseline


------------------------------
Creating Regression Tests
------------------------------

.. index::
   pair: testing; creating tests

This section details the process and expectations for new regression tests in |Viskores|.


How to Add Data to |Viskores|
==============================

|Viskores| uses Git LFS for all regression test data.
To download or add test data to |Viskores|, you must have Git LFS installed.
Once installed, add unit-test data to the :file:`data` directory in the |Viskores| repository.
Data in this directory is classified according to its type: :file:`structured` or :file:`unstructured`.

.. code-block:: bash
   :caption: Adding test data to the |Viskores| repository (Unix commands).
   :name: ex:addTestData

   cd viskores-src-dir
   cd data/data/<data-type>
   git add <file-name>

.. todo::
   Add further test creation instructions here.

..
   Topics for this section:

   * Developing a test for a specific device.
   * How to get the path to the data directory.
   * How to get the path to the regression image directory.
   * Where tests are located.
   * How to add them to CMake.
   * What the mandatory components of a unit test are.
   * How to add data to the repository.
   * General policies for unit testing, including code coverage and requiring tests for new algorithms.
   * How to add images for comparison.
