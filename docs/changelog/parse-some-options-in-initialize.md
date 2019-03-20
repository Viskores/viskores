# Allow Initialize to parse only some arguments

When a library requires reading some command line arguments through a
function like Initialize, it is typical that it will parse through
arguments it supports and then remove those arguments from `argc` and
`argv` so that the remaining arguments can be parsed by the calling
program. Recent changes to the `vtkm::cont::Initialize` function support
that.

## Use Case

Say you are creating a simple benchmark where you want to provide a command
line option `--size` that allows you to adjust the size of the data that
you are working on. However, you also want to support flags like `--device`
and `-v` that are performed by `vtkm::cont::Initialize`. Rather than have
to re-implement all of `Initialize`'s parsing, you can now first call
`Initialize` to handle its arguments and then parse the remaining objects.

The following is a simple (and rather incomplete) example:

```cpp
int main(int argc, char** argv)
{
  vtkm::cont::InitializeResult initResult = vtkm::cont::Initialize(argc, argv);
  
  if ((argc > 1) && (strcmp(argv[1], "--size") == 0))
  {
    if (argc < 3)
	{
	  std::cerr << "--size option requires a numeric argument" << std::endl;
	  std::cerr << "USAGE: " << argv[0] << " [options]" << std::endl;
	  std::cerr << "Options are:" << std::endl;
	  std::cerr << "  --size <number>\tSpecify the size of the data." << std::endl;
	  std::cerr << initResult.Usage << std::endl;
	  exit(1);
	}
	
	g_size = atoi(argv[2]);
  }
  
  std::cout << "Using device: " << initResult.Device.GetName() << std::endl;
```

## Additional Initialize Options

Because `Initialize` no longer has the assumption that it is responsible
for parsing _all_ arguments, some options have been added to
`vtkm::cont::InitializeOptions` to manage these different use cases. The
following options are now supported.

  * `None` A placeholder for having all options off, which is the default.
    (Same as before this change.)
  * `RequireDevice` Issue an error if the device argument is not specified.
    (Same as before this change.)
  * `DefaultAnyDevice` If no device is specified, treat it as if the user
    gave --device=Any. This means that DeviceAdapterTagUndefined will never
    be return in the result.
  * `AddHelp` Add a help argument. If `-h` or `--help` is provided, prints
    a usage statement. Of course, the usage statement will only print out
    arguments processed by VTK-m.
  * `ErrorOnBadOption` If an unknown option is encountered, the program
    terminates with an error and a usage statement is printed. If this
    option is not provided, any unknown options are returned in `argv`. If
    this option is used, it is a good idea to use `AddHelp` as well.
  * `ErrorOnBadArgument` If an extra argument is encountered, the program
    terminates with an error and a usage statement is printed. If this
    option is not provided, any unknown arguments are returned in `argv`.
  * `Strict` If supplied, Initialize treats its own arguments as the only
    ones supported by the application and provides an error if not followed
    exactly. This is a convenience option that is a combination of
    `ErrorOnBadOption`, `ErrorOnBadArgument`, and `AddHelp`.

## InitializeResult Changes

The changes in `Initialize` have also necessitated the changing of some of
the fields in the `InitializeResult` structure. The following fields are
now provided in the `InitializeResult` struct.

  * `Device` Returns the device selected in the command line arguments as a
    `DeviceAdapterId`. If no device was selected,
    `DeviceAdapterTagUndefined` is returned. (Same as before this change.)
  * `Usage` Returns a string containing the usage for the options
    recognized by `Initialize`. This can be used to build larger usage
    statements containing options for both `Initialize` and the calling
    program. See the example above.

Note that the `Arguments` field has been removed from `InitializeResult`.
This is because the unparsed arguments are now returned in the modified
`argc` and `argv`, which provides a more complete result than the
`Arguments` field did.

