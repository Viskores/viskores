# Logging support via loguru.

The loguru project has been integrated with VTK-m to provide runtime logging
facilities. A sample of the log output can be found at
https://gitlab.kitware.com/snippets/427.

Logging is enabled by setting the CMake variable VTKm_ENABLE_LOGGING. When
this flag is enabled, any messages logged to the Info, Warn, Error, and
Fatal levels are printed to stderr by default.

Additional logging features are enabled by calling vtkm::cont::InitLogging
in an executable. This will:
- Set human-readable names for the log levels in the output.
- Allow the stderr logging level to be set at runtime by passing a
  '-v [level]' argument to the executable.
- Name the main thread.
- Print a preamble with details of the program's startup (args, etc).
- Install signal handlers to automatically print stacktraces and error
  contexts (linux only) on crashes.

The main logging entry points are the macros VTKM_LOG_S and VTKM_LOG_F,
which using C++ stream and printf syntax, repectively. Other variants exist,
including conditional logging and special-purpose logs for writing specific
events, such as DynamicObject cast results and TryExecute failures.

The logging backend supports the concept of "Scopes". By creating a new
scope with the macros VTKM_LOG_SCOPE or VTKM_LOG_SCOPE_FUNCTION, a new
"logging scope" is opened within the C++ scope the macro is called from. New
messages will be indented in the log until the scope ends, at which point
a message is logged with the elapsed time that the scope was active. Scopes
may be nested to arbitrary depths.

The logging implementation is thread-safe. When working in a multithreaded
environment, each thread may be assigned a human-readable name using
vtkm::cont::SetThreadName. This will appear in the log output so that
per-thread messages can be easily tracked.

By default, only Info, Warn, Error, and Fatal messages are printed to
stderr. This can be changed at runtime by passing the '-v' flag to an
executable that calls vtkm::cont::InitLogging. Alternatively, the
application can explicitly call vtkm::cont::SetStderrLogLevel to change the
verbosity. When specifying a verbosity, all log levels with enum values
less-than-or-equal-to the requested level are printed.
vtkm::cont::LogLevel::Off (or "-v Off") may be used to silence the log
completely.

The helper functions vtkm::cont::GetHumanReadableSize and
vtkm::cont::GetSizeString assist in formating byte sizes to a more readable
format. Similarly, the vtkm::cont::TypeName template functions provide RTTI
based type-name information. When logging is enabled, these use the logging
backend to demangle symbol names on supported platforms.

The more verbose VTK-m log levels are:
- Perf: Logs performance information, using the scopes feature to track
  execution time of filters, worklets, and device algorithms with
  microsecond resolution.
- MemCont / MemExec: These levels log memory allocations in the control and
  execution environments, respectively.
- MemTransfer: This level logs memory transfers between the control and host
  environments.
- Cast: Logs details of dynamic object resolution.

The log may be shared and extended by applications that use VTK-m. There
are two log level ranges left available for applications: User and
UserVerbose. The User levels may be enabled without showing any of the
verbose VTK-m levels, while UserVerbose levels will also enable all VTK-m
levels.
