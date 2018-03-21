//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/Mask.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace
{

class TestingMask
{
public:
  void TestUniform2D() const
  {
    std::cout << "Testing mask cells uniform grid :" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();

    // Setup and run filter to extract by stride
    vtkm::filter::Mask mask;
    vtkm::Id stride = 2;
    mask.SetStride(stride);

    vtkm::cont::DataSet output = mask.Execute(dataset);

    VTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfCells(), 8),
                     "Wrong result for Mask");


    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 8 &&
                       cellFieldArray.GetPortalConstControl().Get(7) == 14.f,
                     "Wrong mask data");
  }

  void TestUniform3D() const
  {
    std::cout << "Testing mask cells uniform grid :" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Setup and run filter to extract by stride
    vtkm::filter::Mask mask;
    vtkm::Id stride = 9;
    mask.SetStride(stride);

    vtkm::cont::DataSet output = mask.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfCells(), 7),
                     "Wrong result for Mask");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 7 &&
                       cellFieldArray.GetPortalConstControl().Get(2) == 18.f,
                     "Wrong mask data");
  }

  void TestExplicit() const
  {
    std::cout << "Testing mask cells explicit:" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    // Setup and run filter to extract by stride
    vtkm::filter::Mask mask;
    vtkm::Id stride = 2;
    mask.SetStride(stride);

    vtkm::cont::DataSet output = mask.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfCells(), 2),
                     "Wrong result for Mask");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                       cellFieldArray.GetPortalConstControl().Get(1) == 120.2f,
                     "Wrong mask data");
  }

  void operator()() const
  {
    this->TestUniform2D();
    this->TestUniform3D();
    this->TestExplicit();
  }
};
}

int UnitTestMaskFilter(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestingMask());
}
