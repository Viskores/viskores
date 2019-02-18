# VTK-m thirdparty diy now can coexist with external diy

Previously VTK-m would leak macros that would cause an
external diy to be incorrectly mangled breaking consumers
of VTK-m that used diy.

Going forward to use `diy` from VTK-m all calls must use the
`vtkmdiy` namespace instead of the `diy` namespace. This
allows for VTK-m to properly forward calls to either
the external or internal version correctly.

