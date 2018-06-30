# VTK-m Vec< Vec<T> > can't be constructed from Vec<U>

 
When you have a Vec<Vec<float,3>> it was possible to incorrectly initialize
it with the contents of a Vec<double,3>. An example of this is:
```cpp
using Vec3d = vtkm::Vec<double, 3>;
using Vec3f = vtkm::Vec<float, 3>;
using Vec3x3f = vtkm::Vec<Vec3f, 3>;

Vec3d x(0.0, 1.0, 2.0);
Vec3x3f b(x); // becomes [[0,0,0],[1,1,1],[2,2,2]]
Vec3x3f c(x, x, x); // becomes [[0,1,2],[0,1,2],[0,1,2]]
Vec3x3f d(Vec3f(0.0f,1.0f,2.0f)) //becomes [[0,0,0],[1,1,1],[2,2,2]]
```

So the solution we have chosen is to disallow the construction of objects such
as b. This still allows the free implicit cast to go from double to float.
