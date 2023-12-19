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
