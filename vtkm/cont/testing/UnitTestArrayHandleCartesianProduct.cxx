//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace ArrayHandleCartesianProductNamespace
{

template <typename T>
void ArrayHandleCPBasic(vtkm::cont::ArrayHandle<T> x,
                        vtkm::cont::ArrayHandle<T> y,
                        vtkm::cont::ArrayHandle<T> z)

{
  vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<T>,
                                          vtkm::cont::ArrayHandle<T>,
                                          vtkm::cont::ArrayHandle<T>>
    cpArray;

  vtkm::Id nx = x.GetNumberOfValues();
  vtkm::Id ny = y.GetNumberOfValues();
  vtkm::Id nz = z.GetNumberOfValues();
  vtkm::Id n = nx * ny * nz;

  cpArray = vtkm::cont::make_ArrayHandleCartesianProduct(x, y, z);

  //Make sure we have the right number of values.
  VTKM_TEST_ASSERT(cpArray.GetNumberOfValues() == (nx * ny * nz),
                   "Cartesian array constructor has wrong number of values");

  //Make sure the values are correct.
  vtkm::Vec<T, 3> val;
  for (vtkm::Id i = 0; i < n; i++)
  {
    vtkm::Id idx0 = (i % (nx * ny)) % nx;
    vtkm::Id idx1 = (i % (nx * ny)) / nx;
    vtkm::Id idx2 = i / (nx * ny);

    val = vtkm::Vec<T, 3>(x.GetPortalConstControl().Get(idx0),
                          y.GetPortalConstControl().Get(idx1),
                          z.GetPortalConstControl().Get(idx2));
    VTKM_TEST_ASSERT(test_equal(cpArray.GetPortalConstControl().Get(i), val),
                     "Wrong value in array");
  }
}

template <typename T>
void createArr(std::vector<T>& arr, std::size_t n)
{
  arr.resize(n);
  for (std::size_t i = 0; i < n; i++)
    arr[i] = static_cast<T>(i);
}

template <typename T>
void RunTest()
{
  std::size_t nX = 11, nY = 13, nZ = 11;

  for (std::size_t i = 1; i < nX; i += 2)
  {
    for (std::size_t j = 1; j < nY; j += 4)
    {
      for (std::size_t k = 1; k < nZ; k += 5)
      {
        std::vector<T> X, Y, Z;
        createArr(X, nX);
        createArr(Y, nY);
        createArr(Z, nZ);

        ArrayHandleCPBasic(vtkm::cont::make_ArrayHandle(X),
                           vtkm::cont::make_ArrayHandle(Y),
                           vtkm::cont::make_ArrayHandle(Z));
      }
    }
  }
}

void TestArrayHandleCartesianProduct()
{
  RunTest<vtkm::Float32>();
  RunTest<vtkm::Float64>();
  RunTest<vtkm::Id>();
}

} // namespace ArrayHandleCartesianProductNamespace

int UnitTestArrayHandleCartesianProduct(int argc, char* argv[])
{
  using namespace ArrayHandleCartesianProductNamespace;
  return vtkm::cont::testing::Testing::Run(TestArrayHandleCartesianProduct, argc, argv);
}
