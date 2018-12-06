# vtkm::cont::Initialize

A new initialization function, vtkm::cont::Initialize, has been added.
Initialization is not required, but will configure the logging utilities (when
enabled) and allows forcing a device via a `-d` or `--device` command line
option.


Usage:

```
#include <vtkm/cont/Initialize.h>

int main(int argc, char *argv[])
{
  auto config = vtkm::cont::Initialize(argc, argv);

  ...
}
```
