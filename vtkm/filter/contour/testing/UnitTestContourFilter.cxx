//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Math.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/contour/Contour.h>
#include <vtkm/filter/contour/ContourFlyingEdges.h>
#include <vtkm/filter/contour/ContourMarchingCells.h>
#include <vtkm/filter/field_transform/GenerateIds.h>

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/source/Tangle.h>

namespace
{

class TestContourFilter
{
public:
  template <typename ContourFilterType>
  void TestContourUniformGrid(vtkm::IdComponent numPointsNoMergeDuplicate) const
  {
    std::cout << "Testing Contour filter on a uniform grid" << std::endl;

    vtkm::source::Tangle tangle;
    tangle.SetCellDimensions({ 4, 4, 4 });
    vtkm::filter::field_transform::GenerateIds genIds;
    genIds.SetUseFloat(true);
    genIds.SetGeneratePointIds(false);
    genIds.SetCellFieldName("cellvar");
    vtkm::cont::DataSet dataSet = genIds.Execute(tangle.Execute());

    ContourFilterType filter;

    filter.SetGenerateNormals(true);
    filter.SetIsoValue(0, 0.5);
    filter.SetActiveField("tangle");
    filter.SetFieldsToPass(vtkm::filter::FieldSelection::Mode::None);

    auto result = filter.Execute(dataSet);
    {
      VTKM_TEST_ASSERT(result.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset");
      //since normals is on we have one field
      VTKM_TEST_ASSERT(result.GetNumberOfFields() == 2,
                       "Wrong number of fields in the output dataset");
    }

    // let's execute with mapping fields.
    filter.SetFieldsToPass({ "tangle", "cellvar" });
    result = filter.Execute(dataSet);
    {
      const bool isMapped = result.HasField("tangle");
      VTKM_TEST_ASSERT(isMapped, "mapping should pass");

      VTKM_TEST_ASSERT(result.GetNumberOfFields() == 4,
                       "Wrong number of fields in the output dataset");

      //verify the cellvar result
      vtkm::cont::ArrayHandle<vtkm::FloatDefault> cellFieldArrayOut;
      result.GetField("cellvar").GetData().AsArrayHandle(cellFieldArrayOut);

      vtkm::cont::Algorithm::Sort(cellFieldArrayOut);
      {
        std::vector<vtkm::Id> correctcellIdStart = { 0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 5, 6, 6, 6 };
        std::vector<vtkm::Id> correctcellIdEnd = { 57, 57, 58, 58, 58, 59, 59,
                                                   60, 61, 61, 62, 62, 63 };

        auto id_portal = cellFieldArrayOut.ReadPortal();
        for (std::size_t i = 0; i < correctcellIdStart.size(); ++i)
        {
          VTKM_TEST_ASSERT(id_portal.Get(vtkm::Id(i)) == correctcellIdStart[i]);
        }

        vtkm::Id index = cellFieldArrayOut.GetNumberOfValues() - vtkm::Id(correctcellIdEnd.size());
        for (std::size_t i = 0; i < correctcellIdEnd.size(); ++i, ++index)
        {
          VTKM_TEST_ASSERT(id_portal.Get(index) == correctcellIdEnd[i]);
        }
      }

      vtkm::cont::CoordinateSystem coords = result.GetCoordinateSystem();
      vtkm::cont::UnknownCellSet dcells = result.GetCellSet();
      using CellSetType = vtkm::cont::CellSetSingleType<>;
      const CellSetType& cells = dcells.AsCellSet<CellSetType>();

      //verify that the number of points is correct (72)
      //verify that the number of cells is correct (160)
      VTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 72,
                       "Should have less coordinates than the unmerged version");
      VTKM_TEST_ASSERT(cells.GetNumberOfCells() == 160, "");
    }

    //Now try with vertex merging disabled.
    filter.SetMergeDuplicatePoints(false);
    filter.SetFieldsToPass(vtkm::filter::FieldSelection::Mode::All);
    result = filter.Execute(dataSet);
    {
      vtkm::cont::CoordinateSystem coords = result.GetCoordinateSystem();
      VTKM_TEST_ASSERT(coords.GetNumberOfPoints() == numPointsNoMergeDuplicate,
                       "Shouldn't have less coordinates than the unmerged version");

      //verify that the number of cells is correct (160)
      vtkm::cont::UnknownCellSet dcells = result.GetCellSet();

      using CellSetType = vtkm::cont::CellSetSingleType<>;
      const CellSetType& cells = dcells.AsCellSet<CellSetType>();
      VTKM_TEST_ASSERT(cells.GetNumberOfCells() == 160, "");
    }
  }

  template <typename ContourFilterType>
  void Test3DUniformDataSet0() const
  {
    vtkm::cont::testing::MakeTestDataSet maker;
    vtkm::cont::DataSet inputData = maker.Make3DUniformDataSet0();
    std::string fieldName = "pointvar";

    // Defend the test against changes to Make3DUniformDataSet0():
    VTKM_TEST_ASSERT(inputData.HasField(fieldName));
    vtkm::cont::Field pointField = inputData.GetField(fieldName);

    vtkm::Range range;
    pointField.GetRange(&range);
    vtkm::FloatDefault isovalue = 100.0;
    // Range = [10.1, 180.5]
    VTKM_TEST_ASSERT(range.Contains(isovalue));

    ContourFilterType filter;
    filter.SetGenerateNormals(false);
    filter.SetMergeDuplicatePoints(true);
    filter.SetIsoValue(isovalue);
    filter.SetActiveField(fieldName);
    vtkm::cont::DataSet outputData = filter.Execute(inputData);
    VTKM_TEST_ASSERT(outputData.GetNumberOfCells() == 8);
    VTKM_TEST_ASSERT(outputData.GetNumberOfPoints() == 9);
  }

  template <typename ContourFilterType>
  void TestContourWedges() const
  {
    std::cout << "Testing Contour filter on wedge cells" << std::endl;

    auto pathname = vtkm::cont::testing::Testing::DataPath("unstructured/wedge_cells.vtk");
    vtkm::io::VTKDataSetReader reader(pathname);

    vtkm::cont::DataSet dataSet = reader.ReadDataSet();

    vtkm::cont::CellSetSingleType<> cellSet;
    dataSet.GetCellSet().AsCellSet(cellSet);

    vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
    dataSet.GetPointField("gyroid").GetData().AsArrayHandle(fieldArray);

    ContourFilterType isosurfaceFilter;
    isosurfaceFilter.SetActiveField("gyroid");
    isosurfaceFilter.SetMergeDuplicatePoints(false);
    isosurfaceFilter.SetIsoValue(0.0);

    auto result = isosurfaceFilter.Execute(dataSet);
    VTKM_TEST_ASSERT(result.GetNumberOfCells() == 52);
  }

  void TestUnsupportedFlyingEdges() const
  {
    vtkm::cont::testing::MakeTestDataSet maker;
    vtkm::cont::DataSet explicitDataSet = maker.Make3DExplicitDataSet0();

    vtkm::filter::contour::ContourFlyingEdges filter;
    filter.SetIsoValue(2.0);
    filter.SetActiveField("pointvar");

    try
    {
      filter.Execute(explicitDataSet);
      VTKM_TEST_FAIL("Flying Edges filter should not run on explicit datasets");
    }
    catch (vtkm::cont::ErrorFilterExecution&)
    {
      std::cout << "Execution successfully aborted" << std::endl;
    }
  }

  template <typename ContourFilterType>
  void TestNonUniformStructured() const
  {
    auto pathname =
      vtkm::cont::testing::Testing::DataPath("rectilinear/simple_rectilinear1_ascii.vtk");
    vtkm::io::VTKDataSetReader reader(pathname);
    vtkm::cont::DataSet rectilinearDataset = reader.ReadDataSet();

    // Single-cell contour
    ContourFilterType filter;
    filter.SetActiveField("var");
    filter.SetIsoValue(2.0);
    vtkm::cont::DataSet outputSingleCell = filter.Execute(rectilinearDataset);
    auto coordinates = outputSingleCell.GetCoordinateSystem()
                         .GetData()
                         .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>();

    VTKM_TEST_ASSERT(outputSingleCell.GetNumberOfPoints() == 3,
                     "Wrong number of points in rectilinear contour");
    VTKM_TEST_ASSERT(outputSingleCell.GetNumberOfCells() == 1,
                     "Wrong number of cells in rectilinear contour");
    VTKM_TEST_ASSERT(outputSingleCell.GetCellSet().GetCellShape(0) == vtkm::CELL_SHAPE_TRIANGLE,
                     "Wrong contour cell shape");

    auto expectedCoordinates =
      vtkm::cont::make_ArrayHandle<vtkm::Vec3f>({ vtkm::Vec3f{ 10.0f, -10.0f, 9.66341f },
                                                  vtkm::Vec3f{ 9.30578f, -10.0f, 10.0f },
                                                  vtkm::Vec3f{ 10.0f, -9.78842f, 10.0f } });
    VTKM_TEST_ASSERT(test_equal_ArrayHandles(coordinates, expectedCoordinates),
                     "Wrong contour coordinates");

    // Generating normals triggers a different worklet for Flying Edges pass 4,
    // But it should not change anything on the contour itself.
    filter.SetGenerateNormals(true);
    vtkm::cont::DataSet outputNormals = filter.Execute(rectilinearDataset);
    coordinates = outputNormals.GetCoordinateSystem()
                    .GetData()
                    .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>();
    VTKM_TEST_ASSERT(test_equal_ArrayHandles(coordinates, expectedCoordinates),
                     "Wrong contour coordinates");

    // Full contour
    filter.SetIsoValue(3.0);
    filter.SetGenerateNormals(false);
    vtkm::cont::DataSet output = filter.Execute(rectilinearDataset);

    VTKM_TEST_ASSERT(output.GetNumberOfPoints() == 93,
                     "Wrong number of points in rectilinear contour");
    VTKM_TEST_ASSERT(output.GetNumberOfCells() == 144,
                     "Wrong number of cells in rectilinear contour");
  }

  void operator()() const
  {
    this->TestContourUniformGrid<vtkm::filter::contour::Contour>(72);
    this->TestContourUniformGrid<vtkm::filter::contour::ContourFlyingEdges>(72);
    // Unlike flying edges, marching cells does not have point merging for free,
    // So the number of points should increase when disabling duplicate point merging.
    this->TestContourUniformGrid<vtkm::filter::contour::ContourMarchingCells>(480);

    this->Test3DUniformDataSet0<vtkm::filter::contour::Contour>();
    this->Test3DUniformDataSet0<vtkm::filter::contour::ContourMarchingCells>();
    this->Test3DUniformDataSet0<vtkm::filter::contour::ContourFlyingEdges>();

    this->TestContourWedges<vtkm::filter::contour::Contour>();
    this->TestContourWedges<vtkm::filter::contour::ContourMarchingCells>();

    this->TestNonUniformStructured<vtkm::filter::contour::Contour>();
    this->TestNonUniformStructured<vtkm::filter::contour::ContourFlyingEdges>();
    this->TestNonUniformStructured<vtkm::filter::contour::ContourMarchingCells>();

    this->TestUnsupportedFlyingEdges();
  }

}; // class TestContourFilter
} // namespace

int UnitTestContourFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestContourFilter{}, argc, argv);
}
