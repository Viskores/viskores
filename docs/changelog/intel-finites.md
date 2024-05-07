# Enable non-finite values with Intel compiler

The Intel compiler by default turns on an optimization that assumes that
all floating point values are finite. This breaks any ligitimate uses of
non-finite values including checking values with functions like `isnan`
and `isinf`. Turn off this feature for the intel compiler.
