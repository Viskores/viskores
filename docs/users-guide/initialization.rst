==============================
Initialization
==============================

.. index:: initialization

When it comes to running |VTKm| code, there are a few ways in which various facilities, such as logging device connections, and device configuration parameters, can be initialized.
The preferred method of initializing these features is to run the :func:`vtkm::cont::Initialize` function.
Although it is not strictly necessary to call :func:`vtkm::cont::Initialize`, it is recommended to set up state and check for available devices.

.. doxygenfunction:: vtkm::cont::Initialize(int &argc, char *argv[], InitializeOptions opts)

.. index::
   single: argc
   single: argv

:func:`vtkm::cont::Initialize` can be called without any arguments, in which case |VTKm| will be initialized with defaults.
But it can also optionally take the ``argc`` and ``argv`` arguments to the ``main`` function to parse some options that control the state of |VTKm|.
|VTKm| accepts arguments that, for example, configure the compute device to use or establish logging levels.
Any arguments that are handled by |VTKm| are removed from the ``argc``/``argv`` list so that your program can then respond to the remaining arguments.

Many options can also be set with environment variables.
If both the environment variable and command line argument are provided, the command line argument is used.
The following table lists the currently supported options.

.. list-table:: |VTKm| command line arguments and environment variable options.
   :widths: 23 22 15 40
   :header-rows: 1

   * - Command Line Argument
     - Environment Variable
     - Default Value
     - Description
   * - ``--vtkm-help``
     -
     -
     - Causes the program to print information about |VTKm| command line arguments and then exits the program.
   * - ``--vtkm-log-level``
     - ``VTKM_LOG_LEVEL``
     - ``WARNING``
     - Specifies the logging level.
       Valid values are ``INFO``, ``WARNING``, ``ERROR``, ``FATAL``, and ``OFF``.
       This can also be set to a numeric value for the logging level.
   * - ``--vtkm-device``
     - ``VTKM_DEVICE``
     -
     - Force |VTKm| to use the specified device.
       If not specified or ``Any`` given, then any available device may be used.
   * - ``--vtkm-num-threads``
     - ``VTKM_NUM_THREADS``
     -
     - Set the number of threads to use on a multi-core device.
       If not specified, the device will use the cores available in the system.
   * - ``--vtkm-device-instance``
     - ``VTKM_DEVICE_INSTANCE``
     -
     - Selects the device to use when more than one device device of a given type is available.
       The device is specified with a numbered index.

:func:`vtkm::cont::Initialize` returns a :struct:`vtkm::cont::InitializeResult` structure.
This structure contains information about the supported arguments and options selected during initialization.

.. doxygenstruct:: vtkm::cont::InitializeResult
   :members:

:func:`vtkm::cont::Initialize` takes an optional third argument that specifies some options on the behavior of the argument parsing.
The options are specified as a bit-wise "or" of fields specified in the :enum:`vtkm::cont::InitializeOptions` enum.

.. doxygenenum:: vtkm::cont::InitializeOptions

.. load-example:: BasicInitialize
   :file: GuideExampleInitialization.cxx
   :caption: Calling :func:`vtkm::cont::Initialize`.
