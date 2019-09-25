# VTK-c #

VTK-c is a lightweight collection of cell types and cell functionality
that is designed be used scientific visualization libraries of
VTK-m, and VTK.

You can find out more about the design of VTK-c in [DESIGN.md].

## Contributing ##

There are many ways to contribute to [VTK-c]:

  + Submit new or add to discussions of a feature requests or bugs on the
    [VTK-c Issue Tracker].

  + Submit a Pull Request
      + See [CONTRIBUTING.md] for detailed instructions on how to create a
        Pull Request.
      + See the [VTK-c Coding Conventions] that must be followed for
        contributed code.

## Compiler Requirements ##

  + C++11 Compiler. VTK-c has been confirmed to work with the following
      + GCC 4.8+
      + Clang 3.3+
      + XCode 5.0+
      + MSVC 2015+

## Example##

Below is a simple example of using VTK-c to get derivatives and
parametric coordinates for different cell types:

```cpp
#incude <vtkc/vtkc.h>

std::array<float, 3> pcoords;
auto status = vtkc::parametricCenter(vtkc::Hexahedron{}, pcoords);

std::vector<std::array<float, 3>> points = { ... };
std::vector<std::array<double, 4>> field = { ... };
std::array<double, 4> derivs[3];
status = vtkc::derivative(vtkc::Hexahedron{},
                          vtkc::makeFieldAccessorNestedSOAConst(points, 3),
                          vtkc::makeFieldAccessorNestedSOAConst(field, 4),
                          pcoords,
                          derivs[0],
                          derivs[1],
                          derivs[2]);
```

## License ##

VTK-c is distributed under the OSI-approved BSD 3-clause License.
See [LICENSE.md] for details.


[VTK-c]:                    https://gitlab.kitware.com/sujin.philip/vtk-c/
[VTK-c Issue Tracker]:      https://gitlab.kitware.com/sujin.philip/vtk-c/issues

[CONTRIBUTING.md]:          CONTRIBUTING.md
[DESIGN.md]:                docs/Design.md
[LICENSE.md]:               LICENSE.md
[VTK-c Coding Conventions]: docs/CodingConventions.md
