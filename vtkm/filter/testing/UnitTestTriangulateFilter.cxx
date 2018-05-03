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

#include <vtkm/filter/Triangulate.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace
{

class TestingTriangulate
{
public:
  void TestStructured() const
  {
    std::cout << "Testing triangulate structured" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();
    vtkm::filter::Triangulate triangulate;
    triangulate.SetFieldsToPass({ "pointvar", "cellvar" });
    vtkm::cont::DataSet output = triangulate.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfCells(), 32),
                     "Wrong result for Triangulate");
    VTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetData().GetNumberOfValues(), 25),
                     "Wrong number of points for Triangulate");

    vtkm::cont::ArrayHandle<vtkm::Float32> outData =
      output.GetField("cellvar").GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32>>();

    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(2) == 1.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(3) == 1.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(30) == 15.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(31) == 15.f, "Wrong cell field data");
  }

  void TestExplicit() const
  {
    std::cout << "Testing triangulate explicit" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DExplicitDataSet0();
    vtkm::filter::Triangulate triangulate;
    triangulate.SetFieldsToPass({ "pointvar", "cellvar" });
    vtkm::cont::DataSet output = triangulate.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfCells(), 14),
                     "Wrong result for Triangulate");
    VTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetData().GetNumberOfValues(), 16),
                     "Wrong number of points for Triangulate");

    vtkm::cont::ArrayHandle<vtkm::Float32> outData =
      output.GetField("cellvar").GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32>>();

    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(1) == 1.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(2) == 1.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(5) == 3.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(6) == 3.f, "Wrong cell field data");
  }

  void operator()() const
  {
    this->TestStructured();
    this->TestExplicit();
  }
};
}

int UnitTestTriangulateFilter(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestingTriangulate());
}
