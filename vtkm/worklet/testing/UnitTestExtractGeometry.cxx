//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/ExtractGeometry.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

using vtkm::cont::testing::MakeTestDataSet;

class TestingExtractGeometry
{
public:
  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestExplicitById() const
  {
    std::cout << "Testing extract cell explicit by id:" << std::endl;

    using CellSetType = vtkm::cont::CellSetExplicit<>;
    using OutCellSetType = vtkm::cont::CellSetPermutation<CellSetType>;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Cells to extract
    const int nCells = 2;
    vtkm::Id cellids[nCells] = { 1, 2 };
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds = vtkm::cont::make_ArrayHandle(cellids, nCells);

    // Output data set with cell set containing extracted cells and all points
    vtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = extractGeometry.Run(cellSet, cellIds);

    auto cellvar =
      dataset.GetField("cellvar").GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32>>();
    auto cellFieldArray = extractGeometry.ProcessCellField(cellvar);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), nCells),
                     "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == nCells &&
                       cellFieldArray.GetPortalConstControl().Get(0) == 110.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestExplicitByBox() const
  {
    std::cout << "Testing extract cells with implicit function (box) on explicit:" << std::endl;

    using CellSetType = vtkm::cont::CellSetExplicit<>;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Implicit function
    vtkm::Vec3f minPoint(0.5f, 0.0f, 0.0f);
    vtkm::Vec3f maxPoint(2.0f, 2.0f, 2.0f);

    bool extractInside = true;
    bool extractBoundaryCells = false;
    bool extractOnlyBoundaryCells = false;

    // Output data set with cell set containing extracted cells and all points
    vtkm::worklet::ExtractGeometry extractGeometry;
    vtkm::cont::DynamicCellSet outCellSet =
      extractGeometry.Run(cellSet,
                          dataset.GetCoordinateSystem("coordinates"),
                          vtkm::cont::make_ImplicitFunctionHandle<vtkm::Box>(minPoint, maxPoint),
                          extractInside,
                          extractBoundaryCells,
                          extractOnlyBoundaryCells);

    auto cellvar =
      dataset.GetField("cellvar").GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32>>();
    auto cellFieldArray = extractGeometry.ProcessCellField(cellvar);


    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 2), "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                       cellFieldArray.GetPortalConstControl().Get(1) == 120.2f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniformById2D() const
  {
    std::cout << "Testing extract cells structured by id:" << std::endl;

    using CellSetType = vtkm::cont::CellSetStructured<2>;
    using OutCellSetType = vtkm::cont::CellSetPermutation<CellSetType>;


    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Cells to extract
    const int nCells = 5;
    vtkm::Id cellids[nCells] = { 0, 4, 5, 10, 15 };
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds = vtkm::cont::make_ArrayHandle(cellids, nCells);

    // Output data set permutation of with only extracted cells
    vtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = extractGeometry.Run(cellSet, cellIds);

    auto cellvar =
      dataset.GetField("cellvar").GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32>>();
    auto cellFieldArray = extractGeometry.ProcessCellField(cellvar);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), nCells),
                     "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == nCells &&
                       cellFieldArray.GetPortalConstControl().Get(1) == 4.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniformById3D() const
  {
    std::cout << "Testing extract cells structured by id:" << std::endl;

    using CellSetType = vtkm::cont::CellSetStructured<3>;
    using OutCellSetType = vtkm::cont::CellSetPermutation<CellSetType>;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Cells to extract
    const int nCells = 5;
    vtkm::Id cellids[nCells] = { 0, 4, 5, 10, 15 };
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds = vtkm::cont::make_ArrayHandle(cellids, nCells);

    // Output data set with cell set containing extracted cells and all points
    vtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = extractGeometry.Run(cellSet, cellIds);

    auto cellvar =
      dataset.GetField("cellvar").GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32>>();
    auto cellFieldArray = extractGeometry.ProcessCellField(cellvar);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), nCells),
                     "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == nCells &&
                       cellFieldArray.GetPortalConstControl().Get(2) == 5.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniformByBox() const
  {
    std::cout << "Testing extract cells with implicit function (box):" << std::endl;

    using CellSetType = vtkm::cont::CellSetStructured<3>;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Implicit function
    vtkm::Vec3f minPoint(1.0f, 1.0f, 1.0f);
    vtkm::Vec3f maxPoint(3.0f, 3.0f, 3.0f);

    bool extractInside = true;
    bool extractBoundaryCells = false;
    bool extractOnlyBoundaryCells = false;

    // Output data set with cell set containing extracted points
    vtkm::worklet::ExtractGeometry extractGeometry;
    vtkm::cont::DynamicCellSet outCellSet =
      extractGeometry.Run(cellSet,
                          dataset.GetCoordinateSystem("coords"),
                          vtkm::cont::make_ImplicitFunctionHandle<vtkm::Box>(minPoint, maxPoint),
                          extractInside,
                          extractBoundaryCells,
                          extractOnlyBoundaryCells);

    auto cellvar =
      dataset.GetField("cellvar").GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32>>();
    auto cellFieldArray = extractGeometry.ProcessCellField(cellvar);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8), "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 8 &&
                       cellFieldArray.GetPortalConstControl().Get(0) == 21.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniformBySphere() const
  {
    std::cout << "Testing extract cells with implicit function (sphere):" << std::endl;

    using CellSetType = vtkm::cont::CellSetStructured<3>;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Implicit function
    vtkm::Vec3f center(2.f, 2.f, 2.f);
    vtkm::FloatDefault radius(1.8f);

    bool extractInside = true;
    bool extractBoundaryCells = false;
    bool extractOnlyBoundaryCells = false;

    // Output data set with cell set containing extracted cells
    vtkm::worklet::ExtractGeometry extractGeometry;
    vtkm::cont::DynamicCellSet outCellSet =
      extractGeometry.Run(cellSet,
                          dataset.GetCoordinateSystem("coords"),
                          vtkm::cont::make_ImplicitFunctionHandle<vtkm::Sphere>(center, radius),
                          extractInside,
                          extractBoundaryCells,
                          extractOnlyBoundaryCells);

    auto cellvar =
      dataset.GetField("cellvar").GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32>>();
    auto cellFieldArray = extractGeometry.ProcessCellField(cellvar);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8), "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 8 &&
                       cellFieldArray.GetPortalConstControl().Get(1) == 22.f,
                     "Wrong cell field data");
  }

  void operator()() const
  {
    this->TestUniformById2D();
    this->TestUniformById3D();
    this->TestUniformBySphere();
    this->TestUniformByBox();
    this->TestExplicitById();
    this->TestExplicitByBox();
  }
};

int UnitTestExtractGeometry(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingExtractGeometry(), argc, argv);
}
