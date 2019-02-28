# Added specialized operators for ArrayPortalValueReference

The ArrayPortalValueReference is supposed to behave just like the value it
encapsulates and does so by automatically converting to the base type when
necessary. However, when it is possible to convert that to something else,
it is possible to get errors about ambiguous overloads. To avoid these, add
specialized versions of the operators to specify which ones should be used.

Also consolidated the CUDA version of an ArrayPortalValueReference to the
standard one. The two implementations were equivalent and we would like
changes to apply to both.

