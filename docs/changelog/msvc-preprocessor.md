## Use compliant MSVC preprocessor for device code

MSVC has some behavior in their "traditional" preprocessor that is not compliant
with the C99 and C++11 standards. Starting in MSVC 19 (the current earliest
version supported by Viskores), MSVC provides a "compliant" preprocessor that
does meet C and C++ standards. This is enabled with the `/Zc:preprocessor`
command line flag.

Some device libraries (in particular, Thrust) require this compliant preprocess
to be used for compatibility. (See bug [#276].) To support that, Viskores now
adds the `/Zc:preprocessor` flag to compilations of device code. This is done by
adding the compile option to the `viskores_exec` interface library.

[#276]: https://github.com/Viskores/viskores/issues/276
