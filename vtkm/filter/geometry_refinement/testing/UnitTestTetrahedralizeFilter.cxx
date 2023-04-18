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

#include <vtkm/filter/geometry_refinement/Tetrahedralize.h>

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

    vtkm::filter::geometry_refinement::Tetrahedralize tetrahedralize;
    tetrahedralize.SetFieldsToPass({ "pointvar", "cellvar" });

    vtkm::cont::DataSet output = tetrahedralize.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 20), "Wrong result for Tetrahedralize");
    VTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 18),
                     "Wrong number of points for Tetrahedralize");

    vtkm::cont::ArrayHandle<vtkm::Float32> outData =
      output.GetField("cellvar").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float32>>();

    VTKM_TEST_ASSERT(outData.ReadPortal().Get(5) == 100.2f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.ReadPortal().Get(6) == 100.2f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.ReadPortal().Get(7) == 100.2f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.ReadPortal().Get(8) == 100.2f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.ReadPortal().Get(9) == 100.2f, "Wrong cell field data");
  }

  void TestExplicit() const
  {
    std::cout << "Testing tetrahedralize explicit" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    vtkm::filter::geometry_refinement::Tetrahedralize tetrahedralize;
    tetrahedralize.SetFieldsToPass({ "pointvar", "cellvar" });

    vtkm::cont::DataSet output = tetrahedralize.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 11), "Wrong result for Tetrahedralize");
    VTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 11),
                     "Wrong number of points for Tetrahedralize");

    vtkm::cont::ArrayHandle<vtkm::Float32> outData =
      output.GetField("cellvar").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float32>>();

    VTKM_TEST_ASSERT(outData.ReadPortal().Get(5) == 110.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.ReadPortal().Get(6) == 110.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.ReadPortal().Get(8) == 130.5f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.ReadPortal().Get(9) == 130.5f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outData.ReadPortal().Get(10) == 130.5f, "Wrong cell field data");
  }

  void TestCellSetSingleTypeTetra() const
  {
    vtkm::cont::DataSet dataset;
    vtkm::cont::CellSetSingleType<> cellSet;

    auto connectivity = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 1, 2, 3, 3, 2, 1, 4 });
    cellSet.Fill(5, vtkm::CELL_SHAPE_TETRA, 4, connectivity);

    dataset.SetCellSet(cellSet);

    vtkm::filter::geometry_refinement::Tetrahedralize tetrahedralize;
    vtkm::cont::DataSet output = tetrahedralize.Execute(dataset);

    VTKM_TEST_ASSERT(dataset.GetCellSet().GetCellSetBase() == output.GetCellSet().GetCellSetBase(),
                     "Pointer to the CellSetSingleType has changed.");
  }

  void TestCellSetExplicitTetra() const
  {
    std::vector<vtkm::Vec3f_32> coords{
      vtkm::Vec3f_32(0.0f, 0.0f, 0.0f), vtkm::Vec3f_32(2.0f, 0.0f, 0.0f),
      vtkm::Vec3f_32(2.0f, 4.0f, 0.0f), vtkm::Vec3f_32(0.0f, 4.0f, 0.0f),
      vtkm::Vec3f_32(1.0f, 0.0f, 3.0f),
    };
    std::vector<vtkm::UInt8> shapes{ vtkm::CELL_SHAPE_TETRA, vtkm::CELL_SHAPE_TETRA };
    std::vector<vtkm::IdComponent> indices{ 4, 4 };
    std::vector<vtkm::Id> connectivity{ 0, 1, 2, 3, 1, 2, 3, 4 };

    vtkm::cont::DataSetBuilderExplicit dsb;
    vtkm::cont::DataSet dataset = dsb.Create(coords, shapes, indices, connectivity);

    vtkm::filter::geometry_refinement::Tetrahedralize tetrahedralize;
    vtkm::cont::DataSet output = tetrahedralize.Execute(dataset);
    vtkm::cont::UnknownCellSet outputCellSet = output.GetCellSet();

    VTKM_TEST_ASSERT(outputCellSet.IsType<vtkm::cont::CellSetSingleType<>>(),
                     "Output CellSet is not CellSetSingleType");
    VTKM_TEST_ASSERT(output.GetNumberOfCells() == 2, "Wrong number of cells");
    VTKM_TEST_ASSERT(outputCellSet.GetCellShape(0) == vtkm::CellShapeTagTetra::Id,
                     "Cell is not tetra");
    VTKM_TEST_ASSERT(outputCellSet.GetCellShape(1) == vtkm::CellShapeTagTetra::Id,
                     "Cell is not tetra");
  }

  void operator()() const
  {
    this->TestStructured();
    this->TestExplicit();
    this->TestCellSetSingleTypeTetra();
    this->TestCellSetExplicitTetra();
  }
};
}

int UnitTestTetrahedralizeFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingTetrahedralize(), argc, argv);
}
