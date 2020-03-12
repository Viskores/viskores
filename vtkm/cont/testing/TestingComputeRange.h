//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_TestingComputeRange_h
#define vtk_m_cont_testing_TestingComputeRange_h

#include <vtkm/Types.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/testing/Testing.h>

// Required for implementation of ArrayRangeCompute for virtual arrays
#include <vtkm/cont/ArrayRangeCompute.hxx>

#include <algorithm>
#include <iostream>
#include <random>

namespace vtkm
{
namespace cont
{
namespace testing
{

using CustomTypeList = vtkm::List<vtkm::Vec<Int32, 3>,
                                  vtkm::Vec<Int64, 3>,
                                  vtkm::Vec<Float32, 3>,
                                  vtkm::Vec<Float64, 3>,
                                  vtkm::Vec<Int32, 9>,
                                  vtkm::Vec<Int64, 9>,
                                  vtkm::Vec<Float32, 9>,
                                  vtkm::Vec<Float64, 9>>;

template <typename DeviceAdapterTag>
class TestingComputeRange
{
private:
  template <typename T>
  static void TestScalarField()
  {
    const vtkm::Id nvals = 11;
    T data[nvals] = { 1, 2, 3, 4, 5, -5, -4, -3, -2, -1, 0 };
    std::random_device rng;
    std::mt19937 urng(rng());
    std::shuffle(data, data + nvals, urng);
    auto field =
      vtkm::cont::make_Field("TestField", vtkm::cont::Field::Association::POINTS, data, nvals);

    vtkm::Range result;
    field.GetRange(&result);

    std::cout << result << std::endl;
    VTKM_TEST_ASSERT((test_equal(result.Min, -5.0) && test_equal(result.Max, 5.0)),
                     "Unexpected scalar field range.");
  }

  template <typename T, vtkm::IdComponent NumberOfComponents>
  static void TestVecField()
  {
    const vtkm::Id nvals = 11;
    T data[nvals] = { 1, 2, 3, 4, 5, -5, -4, -3, -2, -1, 0 };
    vtkm::Vec<T, NumberOfComponents> fieldData[nvals];
    std::random_device rng;
    std::mt19937 urng(rng());
    for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
    {
      std::shuffle(data, data + nvals, urng);
      for (vtkm::Id j = 0; j < nvals; ++j)
      {
        fieldData[j][i] = data[j];
      }
    }
    auto field =
      vtkm::cont::make_Field("TestField", vtkm::cont::Field::Association::POINTS, fieldData, nvals);

    vtkm::Range result[NumberOfComponents];
    field.GetRange(result, CustomTypeList());

    for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
    {
      VTKM_TEST_ASSERT((test_equal(result[i].Min, -5.0) && test_equal(result[i].Max, 5.0)),
                       "Unexpected vector field range.");
    }
  }

  static void TestUniformCoordinateField()
  {
    vtkm::cont::CoordinateSystem field("TestField",
                                       vtkm::Id3(10, 20, 5),
                                       vtkm::Vec3f(0.0f, -5.0f, 4.0f),
                                       vtkm::Vec3f(1.0f, 0.5f, 2.0f));

    vtkm::Bounds result = field.GetBounds();

    VTKM_TEST_ASSERT(test_equal(result.X.Min, 0.0), "Min x wrong.");
    VTKM_TEST_ASSERT(test_equal(result.X.Max, 9.0), "Max x wrong.");
    VTKM_TEST_ASSERT(test_equal(result.Y.Min, -5.0), "Min y wrong.");
    VTKM_TEST_ASSERT(test_equal(result.Y.Max, 4.5), "Max y wrong.");
    VTKM_TEST_ASSERT(test_equal(result.Z.Min, 4.0), "Min z wrong.");
    VTKM_TEST_ASSERT(test_equal(result.Z.Max, 12.0), "Max z wrong.");
  }

  struct TestAll
  {
    VTKM_CONT void operator()() const
    {
      std::cout << "Testing (Int32, 1)..." << std::endl;
      TestingComputeRange::TestScalarField<vtkm::Int32>();
      std::cout << "Testing (Int64, 1)..." << std::endl;
      TestingComputeRange::TestScalarField<vtkm::Int64>();
      std::cout << "Testing (Float32, 1)..." << std::endl;
      TestingComputeRange::TestScalarField<vtkm::Float32>();
      std::cout << "Testing (Float64, 1)..." << std::endl;
      TestingComputeRange::TestScalarField<vtkm::Float64>();

      std::cout << "Testing (Int32, 3)..." << std::endl;
      TestingComputeRange::TestVecField<vtkm::Int32, 3>();
      std::cout << "Testing (Int64, 3)..." << std::endl;
      TestingComputeRange::TestVecField<vtkm::Int64, 3>();
      std::cout << "Testing (Float32, 3)..." << std::endl;
      TestingComputeRange::TestVecField<vtkm::Float32, 3>();
      std::cout << "Testing (Float64, 3)..." << std::endl;
      TestingComputeRange::TestVecField<vtkm::Float64, 3>();

      std::cout << "Testing (Int32, 9)..." << std::endl;
      TestingComputeRange::TestVecField<vtkm::Int32, 9>();
      std::cout << "Testing (Int64, 9)..." << std::endl;
      TestingComputeRange::TestVecField<vtkm::Int64, 9>();
      std::cout << "Testing (Float32, 9)..." << std::endl;
      TestingComputeRange::TestVecField<vtkm::Float32, 9>();
      std::cout << "Testing (Float64, 9)..." << std::endl;
      TestingComputeRange::TestVecField<vtkm::Float64, 9>();

      std::cout << "Testing UniformPointCoords..." << std::endl;
      TestingComputeRange::TestUniformCoordinateField();
    }
  };

public:
  static VTKM_CONT int Run(int argc, char* argv[])
  {
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapterTag());
    return vtkm::cont::testing::Testing::Run(TestAll(), argc, argv);
  }
};
}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_TestingComputeRange_h
