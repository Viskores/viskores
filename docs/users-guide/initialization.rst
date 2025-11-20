==============================
Initialization
==============================

.. index:: initialization

When it comes to running |Viskores| code, there are a few ways in which various facilities, such as logging device connections, and device configuration parameters, can be initialized.
The preferred method of initializing these features is to run the :func:`viskores::cont::Initialize` function.
Although it is not strictly necessary to call :func:`viskores::cont::Initialize`, it is recommended to set up state and check for available devices.

.. doxygenfunction:: viskores::cont::Initialize(int &argc, char *argv[], InitializeOptions opts)

.. index::
   single: argc
   single: argv

:func:`viskores::cont::Initialize` can be called without any arguments, in which case |Viskores| will be initialized with defaults.
But it can also optionally take the ``argc`` and ``argv`` arguments to the ``main`` function to parse some options that control the state of |Viskores|.
|Viskores| accepts arguments that, for example, configure the compute device to use or establish logging levels.
Any arguments that are handled by |Viskores| are removed from the ``argc``/``argv`` list so that your program can then respond to the remaining arguments.

Many options can also be set with environment variables.
If both the environment variable and command line argument are provided, the command line argument is used.
The following table lists the currently supported options.

.. list-table:: |Viskores| command line arguments and environment variable options.
   :widths: 23 22 15 40
   :header-rows: 1

   * - Command Line Argument
     - Environment Variable
     - Default Value
     - Description
   * - ``--viskores-help``
     -
     -
     - Causes the program to print information about |Viskores| command line arguments and then exits the program.
   * - ``--viskores-log-level``
     - ``VISKORES_LOG_LEVEL``
     - ``WARNING``
     - Specifies the logging level.
       Valid values are ``INFO``, ``WARNING``, ``ERROR``, ``FATAL``, and ``OFF``.
       This can also be set to a numeric value for the logging level.
   * - ``--viskores-device``
     - ``VISKORES_DEVICE``
     -
     - Force |Viskores| to use the specified device.
       If not specified or ``Any`` given, then any available device may be used.
   * - ``--viskores-num-threads``
     - ``VISKORES_NUM_THREADS``
     -
     - Set the number of threads to use on a multi-core device.
       If not specified, the device will use the cores available in the system.
   * - ``--viskores-device-instance``
     - ``VISKORES_DEVICE_INSTANCE``
     -
     - Selects the device to use when more than one device device of a given type is available.
       The device is specified with a numbered index.
   * - ``--viskores-use-unified-memory``
     - ``VISKORES_USE_UNIFIED_MEMORY``
     - 0
     - If set to 1, prefer using unified memory to transfer data between host and device.
       When set to 0, prefer explicit host/device memory management.

:func:`viskores::cont::Initialize` returns a :struct:`viskores::cont::InitializeResult` structure.
This structure contains information about the supported arguments and options selected during initialization.

.. doxygenstruct:: viskores::cont::InitializeResult
   :members:

:func:`viskores::cont::Initialize` takes an optional third argument that specifies some options on the behavior of the argument parsing.
The options are specified as a bit-wise "or" of fields specified in the :enum:`viskores::cont::InitializeOptions` enum.

.. doxygenenum:: viskores::cont::InitializeOptions

.. load-example:: BasicInitialize
   :file: GuideExampleInitialization.cxx
   :caption: Calling :func:`viskores::cont::Initialize`.

:func:`viskores::cont::Initialize` should be called exactly once before using any other Viskores features.
Often this is done in the ``main`` function during program initialization as shown in :numref:`ex:BasicInitialize`.
However, if the program structure does not allow this, you can use the :func:`viskores::cont::IsInitialized` to check to see if :func:`viskores::cont::Initialize` still needs to be installed.

.. doxygenfunction:: viskores::cont::IsInitialized

.. load-example:: CheckInitialization
   :file: GuideExampleInitialization.cxx
   :caption: Using :func:`viskores::cont::IsInitialized` to see if initialization still needs to happen.
