## Fixed the SetupForDevelopment.sh script

The [CONTRIBUTING.md] document instructs developers to run the
`Utilities/SetupForDevelopment.sh` script. However, when you ran this script,
you got an error:

```bash
$ ./Utilities/SetupForDevelopment.sh
Setting up useful Git aliases...
./Utilities/SetupForDevelopment.sh: line 22: syntax error: unexpected end of file
```

This error has been fixed, and the `SetupForDevelopment.sh` script now correctly
sets up git aliases.

[CONTRIBUTING.md]: https://github.com/Viskores/viskores/blob/main/CONTRIBUTING.md#setup
