//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/Tetrahedralize.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace
{

class TestingTetrahedralize
{
public:
  void TestStructured() const
  {
    std::cout << "Testing tetrahedralize structured" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet0();

    vtkm::filter::Tetrahedralize tetrahedralize;
    tetrahedralize.SetFieldsToPass({ "pointvar", "cellvar" });

    vtkm::cont::DataSet output = tetrahedralize.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 20), "Wrong result for Tetrahedralize");
    VTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 18),
                     "Wrong number of points for Tetrahedralize");

    vtkm::cont::ArrayHandle<vtkm::Float32> outData =
      output.GetField("cellvar").GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32>>();

    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(5) == 100.2f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(6) == 100.2f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(7) == 100.2f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(8) == 100.2f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(9) == 100.2f, "Wrong cell field data");
  }

  void TestExplicit() const
  {
    std::cout << "Testing tetrahedralize explicit" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    vtkm::filter::Tetrahedralize tetrahedralize;
    tetrahedralize.SetFieldsToPass({ "pointvar", "cellvar" });

    vtkm::cont::DataSet output = tetrahedralize.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 11), "Wrong result for Tetrahedralize");
    VTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 11),
                     "Wrong number of points for Tetrahedralize");

    vtkm::cont::ArrayHandle<vtkm::Float32> outData =
      output.GetField("cellvar").GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32>>();

    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(5) == 110.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(6) == 110.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(8) == 130.5f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(9) == 130.5f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(10) == 130.5f, "Wrong cell field data");
  }

  void operator()() const
  {
    this->TestStructured();
    this->TestExplicit();
  }
};
}

int UnitTestTetrahedralizeFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingTetrahedralize(), argc, argv);
}
