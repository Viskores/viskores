//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/geometry_refinement/Triangulate.h>

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
    vtkm::filter::geometry_refinement::Triangulate triangulate;
    triangulate.SetFieldsToPass({ "pointvar", "cellvar" });
    vtkm::cont::DataSet output = triangulate.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 32), "Wrong result for Triangulate");
    VTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 25),
                     "Wrong number of points for Triangulate");

    vtkm::cont::ArrayHandle<vtkm::Float32> outData =
      output.GetField("cellvar").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float32>>();

    VTKM_TEST_ASSERT(outData.ReadPortal().Get(2) == 1.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.ReadPortal().Get(3) == 1.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.ReadPortal().Get(30) == 15.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.ReadPortal().Get(31) == 15.f, "Wrong cell field data");
  }

  void TestExplicit() const
  {
    std::cout << "Testing triangulate explicit" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DExplicitDataSet0();
    vtkm::filter::geometry_refinement::Triangulate triangulate;
    triangulate.SetFieldsToPass({ "pointvar", "cellvar" });
    vtkm::cont::DataSet output = triangulate.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 14), "Wrong result for Triangulate");
    VTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 16),
                     "Wrong number of points for Triangulate");

    vtkm::cont::ArrayHandle<vtkm::Float32> outData =
      output.GetField("cellvar").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float32>>();

    VTKM_TEST_ASSERT(outData.ReadPortal().Get(1) == 1.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.ReadPortal().Get(2) == 1.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.ReadPortal().Get(5) == 3.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.ReadPortal().Get(6) == 3.f, "Wrong cell field data");
  }

  void TestCellSetSingleTypeTriangle() const
  {
    vtkm::cont::DataSet dataset;
    vtkm::cont::CellSetSingleType<> cellSet;

    auto connectivity = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 1, 2, 1, 2, 3 });
    cellSet.Fill(4, vtkm::CELL_SHAPE_TRIANGLE, 3, connectivity);

    dataset.SetCellSet(cellSet);

    vtkm::filter::geometry_refinement::Triangulate triangulate;
    vtkm::cont::DataSet output = triangulate.Execute(dataset);

    VTKM_TEST_ASSERT(dataset.GetCellSet().GetCellSetBase() == output.GetCellSet().GetCellSetBase(),
                     "Pointer to the CellSetSingleType has changed.");
  }

  void TestCellSetExplicitTriangle() const
  {
    std::vector<vtkm::Vec3f_32> coords{ vtkm::Vec3f_32(0.0f, 0.0f, 0.0f),
                                        vtkm::Vec3f_32(2.0f, 0.0f, 0.0f),
                                        vtkm::Vec3f_32(2.0f, 4.0f, 0.0f),
                                        vtkm::Vec3f_32(0.0f, 4.0f, 0.0f) };
    std::vector<vtkm::UInt8> shapes{ vtkm::CELL_SHAPE_TRIANGLE, vtkm::CELL_SHAPE_TRIANGLE };
    std::vector<vtkm::IdComponent> indices{ 3, 3 };
    std::vector<vtkm::Id> connectivity{ 0, 1, 2, 1, 2, 3 };

    vtkm::cont::DataSetBuilderExplicit dsb;
    vtkm::cont::DataSet dataset = dsb.Create(coords, shapes, indices, connectivity);

    vtkm::filter::geometry_refinement::Triangulate triangulate;
    vtkm::cont::DataSet output = triangulate.Execute(dataset);
    vtkm::cont::UnknownCellSet outputCellSet = output.GetCellSet();

    VTKM_TEST_ASSERT(outputCellSet.IsType<vtkm::cont::CellSetSingleType<>>(),
                     "Output CellSet is not CellSetSingleType");
    VTKM_TEST_ASSERT(output.GetNumberOfCells() == 2, "Wrong number of cells");
    VTKM_TEST_ASSERT(outputCellSet.GetCellShape(0) == vtkm::CellShapeTagTriangle::Id,
                     "Cell is not triangular");
    VTKM_TEST_ASSERT(outputCellSet.GetCellShape(1) == vtkm::CellShapeTagTriangle::Id,
                     "Cell is not triangular");
  }

  void operator()() const
  {
    this->TestStructured();
    this->TestExplicit();
    this->TestCellSetSingleTypeTriangle();
    this->TestCellSetExplicitTriangle();
  }
};
}

int UnitTestTriangulateFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingTriangulate(), argc, argv);
}
