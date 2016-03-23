//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_testing_TestingComputeBounds_h
#define vtk_m_cont_testing_TestingComputeBounds_h

#include <vtkm/Types.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/testing/Testing.h>

#include <algorithm>
#include <iostream>

namespace vtkm {
namespace cont {
namespace testing {

struct CustomTypeList : vtkm::ListTagBase<vtkm::Vec<Int32, 3>,
                                          vtkm::Vec<Int64, 3>,
                                          vtkm::Vec<Float32, 3>,
                                          vtkm::Vec<Float64, 3>,
                                          vtkm::Vec<Int32, 9>,
                                          vtkm::Vec<Int64, 9>,
                                          vtkm::Vec<Float32, 9>,
                                          vtkm::Vec<Float64, 9> >
{};

template <typename DeviceAdapterTag>
class TestingComputeBounds
{
private:
  template <typename T>
  static void TestScalarField()
  {
    const vtkm::Id nvals = 11;
    T data[nvals] = { 1, 2, 3, 4, 5, -5, -4, -3, -2, -1, 0 };
    std::random_shuffle(data, data + nvals);
    vtkm::cont::Field field("TestField", vtkm::cont::Field::ASSOC_POINTS, data,
                            nvals);

    vtkm::Float64 result[2];
    field.GetBounds(result, DeviceAdapterTag());

    if (result[0] == -5.0 && result[1] == 5.0)
    {
      std::cout << "Success" << std::endl;
    }
    else
    {
      std::cout << "Expected: -5.0, 5.0; Got: " << result[0] << ", " << result[1]
                << std::endl;
      VTKM_TEST_FAIL("Failed");
    }
  }

  template <typename T, vtkm::Id NumberOfComponents>
  static void TestVecField()
  {
    const vtkm::Id nvals = 11;
    T data[nvals] = { 1, 2, 3, 4, 5, -5, -4, -3, -2, -1, 0 };
    vtkm::Vec<T, NumberOfComponents> fieldData[nvals];
    for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
    {
      std::random_shuffle(data, data + nvals);
      for (vtkm::Id j = 0; j < nvals; ++j)
      {
        fieldData[j][i] = data[j];
      }
    }
    vtkm::cont::Field field("TestField", vtkm::cont::Field::ASSOC_POINTS, fieldData,
                            nvals);

    vtkm::Float64 result[NumberOfComponents * 2];
    field.GetBounds(result, DeviceAdapterTag(), CustomTypeList(),
                    VTKM_DEFAULT_STORAGE_LIST_TAG());

    bool success = true;
    for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
    {
      if (result[i * 2] != -5.0 || result[i * 2 + 1] != 5.0)
      {
        success = false;
        break;
      }
    }

    if (success)
    {
      std::cout << "Success" << std::endl;
    }
    else
    {
      std::cout << "Expected: -5.0s and 5.0s; Got: ";
      for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
      {
        std::cout << result[i * 2] << ",";
      }
      std::cout << " and ";
      for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
      {
        std::cout << result[i * 2 + 1] << ",";
      }
      std::cout << std::endl;
      VTKM_TEST_FAIL("Failed");
    }
  }

  static void TestUniformCoordinateField()
  {
    vtkm::cont::CoordinateSystem field(
          "TestField",
          vtkm::Id3(10, 20, 5),
          vtkm::Vec<vtkm::FloatDefault,3>(0.0f,-5.0f,4.0f),
          vtkm::Vec<vtkm::FloatDefault,3>(1.0f,0.5f,2.0f));

    vtkm::Float64 result[6];
    field.GetBounds(result, DeviceAdapterTag());

    VTKM_TEST_ASSERT(test_equal(result[0], 0.0), "Min x wrong.");
    VTKM_TEST_ASSERT(test_equal(result[1], 9.0), "Max x wrong.");
    VTKM_TEST_ASSERT(test_equal(result[2], -5.0), "Min y wrong.");
    VTKM_TEST_ASSERT(test_equal(result[3], 4.5), "Max y wrong.");
    VTKM_TEST_ASSERT(test_equal(result[4], 4.0), "Min z wrong.");
    VTKM_TEST_ASSERT(test_equal(result[5], 12.0), "Max z wrong.");
  }

  struct TestAll
  {
    VTKM_CONT_EXPORT void operator()() const
    {
      std::cout << "Testing (Int32, 1)..." << std::endl;
      TestingComputeBounds::TestScalarField<vtkm::Int32>();
      std::cout << "Testing (Int64, 1)..." << std::endl;
      TestingComputeBounds::TestScalarField<vtkm::Int64>();
      std::cout << "Testing (Float32, 1)..." << std::endl;
      TestingComputeBounds::TestScalarField<vtkm::Float32>();
      std::cout << "Testing (Float64, 1)..." << std::endl;
      TestingComputeBounds::TestScalarField<vtkm::Float64>();

      std::cout << "Testing (Int32, 3)..." << std::endl;
      TestingComputeBounds::TestVecField<vtkm::Int32, 3>();
      std::cout << "Testing (Int64, 3)..." << std::endl;
      TestingComputeBounds::TestVecField<vtkm::Int64, 3>();
      std::cout << "Testing (Float32, 3)..." << std::endl;
      TestingComputeBounds::TestVecField<vtkm::Float32, 3>();
      std::cout << "Testing (Float64, 3)..." << std::endl;
      TestingComputeBounds::TestVecField<vtkm::Float64, 3>();

      std::cout << "Testing (Int32, 9)..." << std::endl;
      TestingComputeBounds::TestVecField<vtkm::Int32, 9>();
      std::cout << "Testing (Int64, 9)..." << std::endl;
      TestingComputeBounds::TestVecField<vtkm::Int64, 9>();
      std::cout << "Testing (Float32, 9)..." << std::endl;
      TestingComputeBounds::TestVecField<vtkm::Float32, 9>();
      std::cout << "Testing (Float64, 9)..." << std::endl;
      TestingComputeBounds::TestVecField<vtkm::Float64, 9>();

      std::cout << "Testing UniformPointCoords..." << std::endl;
      TestingComputeBounds::TestUniformCoordinateField();
    }
  };

public:
  static VTKM_CONT_EXPORT int Run()
  {
    return vtkm::cont::testing::Testing::Run(TestAll());
  }
};

}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_TestingComputeBounds_h
