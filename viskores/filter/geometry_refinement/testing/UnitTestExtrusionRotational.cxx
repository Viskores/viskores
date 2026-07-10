//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/CellShape.h>
#include <viskores/cont/CellSetExtrude.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/geometry_refinement/ExtrusionRotational.h>

#include <array>
#include <functional>
#include <vector>

namespace
{

using ExplicitWedgeCellSet = viskores::cont::CellSetSingleType<>;

viskores::cont::DataSet MakeTriangleDataSet(bool includeAxisPoint = false)
{
  std::vector<viskores::Vec3f> coordinates;
  if (includeAxisPoint)
  {
    coordinates = { { 0.0f, 0.0f, 0.0f }, { 2.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f },
                    { 3.0f, 0.0f, 0.0f }, { 4.0f, 0.0f, 0.0f }, { 3.0f, 0.0f, 1.0f } };
  }
  else
  {
    coordinates = { { 1.0f, 0.0f, 0.0f }, { 2.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f },
                    { 3.0f, 0.0f, 0.0f }, { 4.0f, 0.0f, 0.0f }, { 3.0f, 0.0f, 1.0f } };
  }

  const std::vector<viskores::UInt8> shapes{ viskores::CELL_SHAPE_TRIANGLE,
                                             viskores::CELL_SHAPE_TRIANGLE };
  const std::vector<viskores::IdComponent> numIndices{ 3, 3 };
  const std::vector<viskores::Id> connectivity{ 0, 1, 2, 3, 5, 4 };

  viskores::cont::DataSetBuilderExplicit builder;
  viskores::cont::DataSet dataSet =
    builder.Create(coordinates, shapes, numIndices, connectivity, "coords");

  dataSet.AddPointField("pointvar",
                        std::vector<viskores::Float32>{ 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f });
  dataSet.AddCellField("cellvar", std::vector<viskores::Float32>{ 70.0f, 80.0f });
  return dataSet;
}

viskores::cont::DataSet MakeQuadDataSet()
{
  const std::vector<viskores::Vec3f> coordinates{
    { 1.0f, 0.0f, 0.0f }, { 2.0f, 0.0f, 0.0f }, { 2.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f }
  };
  const std::vector<viskores::UInt8> shapes{ viskores::CELL_SHAPE_QUAD };
  const std::vector<viskores::IdComponent> numIndices{ 4 };
  const std::vector<viskores::Id> connectivity{ 0, 1, 2, 3 };

  viskores::cont::DataSetBuilderExplicit builder;
  return builder.Create(coordinates, shapes, numIndices, connectivity, "coords");
}

bool ThrowsFilterExecution(const std::function<void()>& function)
{
  try
  {
    function();
  }
  catch (const viskores::cont::ErrorFilterExecution&)
  {
    return true;
  }
  return false;
}

void AssertCellPointIds(const viskores::cont::DataSet& dataSet,
                        viskores::Id cellId,
                        const std::array<viskores::Id, 6>& expected)
{
  std::array<viskores::Id, 6> actual;
  dataSet.GetCellSet().GetCellPointIds(cellId, actual.data());
  for (viskores::IdComponent index = 0; index < 6; ++index)
  {
    VISKORES_TEST_ASSERT(actual[static_cast<std::size_t>(index)] ==
                           expected[static_cast<std::size_t>(index)],
                         "Wrong wedge point id.");
  }
}

viskores::Float64 SignedWedgeVolume(const viskores::cont::DataSet& dataSet, viskores::Id cellId)
{
  std::array<viskores::Id, 6> pointIds;
  dataSet.GetCellSet().GetCellPointIds(cellId, pointIds.data());

  viskores::cont::ArrayHandle<viskores::Vec3f> coordinates;
  dataSet.GetCoordinateSystem().GetData().AsArrayHandle(coordinates);
  auto coordinatePortal = coordinates.ReadPortal();

  const viskores::Vec3f point0 = coordinatePortal.Get(pointIds[0]);
  const viskores::Vec3f point1 = coordinatePortal.Get(pointIds[1]);
  const viskores::Vec3f point2 = coordinatePortal.Get(pointIds[2]);
  const viskores::Vec3f point3 = coordinatePortal.Get(pointIds[3]);
  const viskores::Vec3f point4 = coordinatePortal.Get(pointIds[4]);
  const viskores::Vec3f point5 = coordinatePortal.Get(pointIds[5]);

  viskores::Float64 volume =
    viskores::Dot(viskores::Cross(point1 - point0, point2 - point0), point3 - point0);
  volume += viskores::Dot(viskores::Cross(point4 - point1, point5 - point1), point3 - point1);
  volume += viskores::Dot(viskores::Cross(point5 - point1, point2 - point1), point3 - point1);
  return volume;
}

void AssertPositiveWedgeVolumes(const viskores::cont::DataSet& dataSet)
{
  for (viskores::Id cellId = 0; cellId < dataSet.GetNumberOfCells(); ++cellId)
  {
    VISKORES_TEST_ASSERT(SignedWedgeVolume(dataSet, cellId) > 0.0,
                         "Output wedge should have positive orientation.");
  }
}

void TestSweepAngleCloseSweepDefaults()
{
  std::cout << "Testing sweep angle close-sweep defaults" << std::endl;

  viskores::filter::geometry_refinement::ExtrusionRotational filter;
  VISKORES_TEST_ASSERT(filter.GetCloseSweep(), "Default full sweep should be closed.");

  filter.SetSweepAngle(viskores::Pi<viskores::FloatDefault>() / 2.0f);
  VISKORES_TEST_ASSERT(!filter.GetCloseSweep(), "Partial sweep should default open.");

  filter.SetSweepAngle(viskores::TwoPi<viskores::FloatDefault>());
  VISKORES_TEST_ASSERT(filter.GetCloseSweep(), "Full sweep should default closed.");

  viskores::filter::geometry_refinement::ExtrusionRotational explicitOpenFilter;
  explicitOpenFilter.SetCloseSweep(false);
  explicitOpenFilter.SetSweepAngle(viskores::TwoPi<viskores::FloatDefault>());
  VISKORES_TEST_ASSERT(!explicitOpenFilter.GetCloseSweep(),
                       "Explicit open sweep should not be overwritten by full sweep angle.");

  viskores::filter::geometry_refinement::ExtrusionRotational explicitClosedFilter;
  explicitClosedFilter.SetCloseSweep(true);
  explicitClosedFilter.SetSweepAngle(viskores::Pi<viskores::FloatDefault>() / 2.0f);
  VISKORES_TEST_ASSERT(explicitClosedFilter.GetCloseSweep(),
                       "Explicit closed sweep should not be overwritten by partial sweep angle.");
}

void TestClosedFullSweep()
{
  std::cout << "Testing closed full sweep" << std::endl;

  viskores::filter::geometry_refinement::ExtrusionRotational filter;
  filter.SetNumberOfPlanes(4);
  filter.SetSweepAngle(viskores::TwoPi<viskores::FloatDefault>());
  filter.SetCloseSweep(true);

  viskores::cont::DataSet output = filter.Execute(MakeTriangleDataSet());
  VISKORES_TEST_ASSERT(output.GetNumberOfPoints() == 24, "Wrong number of output points.");
  VISKORES_TEST_ASSERT(output.GetNumberOfCells() == 8, "Wrong number of output cells.");
  VISKORES_TEST_ASSERT(output.GetCellSet().IsType<ExplicitWedgeCellSet>(),
                       "Default output cell set is not explicit single-type topology.");

  const auto cellSet = output.GetCellSet().AsCellSet<ExplicitWedgeCellSet>();
  VISKORES_TEST_ASSERT(cellSet.GetCellShapeAsId() == viskores::CELL_SHAPE_WEDGE,
                       "Default output cells should be wedges.");
  VISKORES_TEST_ASSERT(cellSet.GetCellShape(0) == viskores::CELL_SHAPE_WEDGE,
                       "Output cells should be wedges.");
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfPointsInCell(0) == 6,
                       "Output wedges should have 6 points.");
  AssertCellPointIds(output, 0, { 0, 2, 1, 6, 8, 7 });
  AssertCellPointIds(output, 1, { 3, 5, 4, 9, 11, 10 });
  AssertCellPointIds(output, 6, { 18, 20, 19, 0, 2, 1 });
  AssertCellPointIds(output, 7, { 21, 23, 22, 3, 5, 4 });
  AssertPositiveWedgeVolumes(output);
}

void TestCompactOutput()
{
  std::cout << "Testing compact output" << std::endl;

  viskores::filter::geometry_refinement::ExtrusionRotational filter;
  filter.SetCompactOutput(true);
  filter.SetNumberOfPlanes(4);
  filter.SetSweepAngle(viskores::TwoPi<viskores::FloatDefault>());
  filter.SetCloseSweep(true);

  viskores::cont::DataSet output = filter.Execute(MakeTriangleDataSet());
  VISKORES_TEST_ASSERT(output.GetNumberOfPoints() == 24, "Wrong compact output point count.");
  VISKORES_TEST_ASSERT(output.GetNumberOfCells() == 8, "Wrong compact output cell count.");
  VISKORES_TEST_ASSERT(output.GetCellSet().IsType<viskores::cont::CellSetExtrude>(),
                       "Compact output cell set is not CellSetExtrude.");

  const auto cellSet = output.GetCellSet().AsCellSet<viskores::cont::CellSetExtrude>();
  VISKORES_TEST_ASSERT(cellSet.GetIsPeriodic(), "Closed compact sweep should be periodic.");
  VISKORES_TEST_ASSERT(cellSet.GetCellShape(0) == viskores::CELL_SHAPE_WEDGE,
                       "Compact output cells should be wedges.");
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfPointsInCell(0) == 6,
                       "Compact output wedges should have 6 points.");
  AssertCellPointIds(output, 0, { 0, 2, 1, 6, 8, 7 });
  AssertCellPointIds(output, 1, { 3, 5, 4, 9, 11, 10 });
  AssertCellPointIds(output, 6, { 18, 20, 19, 0, 2, 1 });
  AssertCellPointIds(output, 7, { 21, 23, 22, 3, 5, 4 });
  AssertPositiveWedgeVolumes(output);
}

void TestOpenPartialSweep()
{
  std::cout << "Testing open partial sweep" << std::endl;

  viskores::filter::geometry_refinement::ExtrusionRotational filter;
  filter.SetNumberOfPlanes(3);
  filter.SetSweepAngle(viskores::Pi<viskores::FloatDefault>() / 2.0f);
  filter.SetCloseSweep(false);

  viskores::cont::DataSet output = filter.Execute(MakeTriangleDataSet());
  VISKORES_TEST_ASSERT(output.GetNumberOfPoints() == 18, "Wrong number of output points.");
  VISKORES_TEST_ASSERT(output.GetNumberOfCells() == 4, "Wrong number of output cells.");

  const auto cellSet = output.GetCellSet().AsCellSet<ExplicitWedgeCellSet>();
  VISKORES_TEST_ASSERT(cellSet.GetCellShapeAsId() == viskores::CELL_SHAPE_WEDGE,
                       "Open output cells should be wedges.");
  AssertPositiveWedgeVolumes(output);
}

void TestCoordinates()
{
  std::cout << "Testing generated coordinates" << std::endl;

  viskores::filter::geometry_refinement::ExtrusionRotational filter;
  filter.SetNumberOfPlanes(3);
  filter.SetSweepAngle(viskores::Pi<viskores::FloatDefault>() / 2.0f);
  filter.SetCloseSweep(false);

  viskores::cont::DataSet output = filter.Execute(MakeTriangleDataSet());
  viskores::cont::ArrayHandle<viskores::Vec3f> coordinates;
  output.GetCoordinateSystem().GetData().AsArrayHandle(coordinates);
  auto portal = coordinates.ReadPortal();

  VISKORES_TEST_ASSERT(test_equal(portal.Get(0), viskores::Vec3f(1.0f, 0.0f, 0.0f)),
                       "Plane 0 coordinate is wrong.");
  VISKORES_TEST_ASSERT(
    test_equal(portal.Get(6), viskores::Vec3f(viskores::Sqrt(0.5f), viskores::Sqrt(0.5f), 0.0f)),
    "Middle plane coordinate is wrong.");
  VISKORES_TEST_ASSERT(test_equal(portal.Get(12), viskores::Vec3f(0.0f, 1.0f, 0.0f)),
                       "Final plane coordinate is wrong.");
}

void TestOffsetAxisCoordinates()
{
  std::cout << "Testing generated coordinates around an offset axis" << std::endl;

  viskores::filter::geometry_refinement::ExtrusionRotational filter;
  filter.SetNumberOfPlanes(2);
  filter.SetSweepAngle(viskores::Pi<viskores::FloatDefault>() / 2.0f);
  filter.SetCloseSweep(false);
  filter.SetCenter({ 1.0f, 0.0f, 0.0f });

  viskores::cont::DataSet output = filter.Execute(MakeTriangleDataSet());
  viskores::cont::ArrayHandle<viskores::Vec3f> coordinates;
  output.GetCoordinateSystem().GetData().AsArrayHandle(coordinates);
  auto portal = coordinates.ReadPortal();

  VISKORES_TEST_ASSERT(test_equal(portal.Get(0), viskores::Vec3f(1.0f, 0.0f, 0.0f)),
                       "Axis point coordinate is wrong.");
  VISKORES_TEST_ASSERT(test_equal(portal.Get(7), viskores::Vec3f(1.0f, 1.0f, 0.0f)),
                       "Offset-axis rotation coordinate is wrong.");
}

void TestFieldReplication()
{
  std::cout << "Testing field replication" << std::endl;

  viskores::filter::geometry_refinement::ExtrusionRotational filter;
  filter.SetNumberOfPlanes(4);
  filter.SetCloseSweep(true);

  viskores::cont::DataSet output = filter.Execute(MakeTriangleDataSet());

  viskores::cont::ArrayHandle<viskores::Float32> pointField =
    output.GetField("pointvar")
      .GetData()
      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>();
  auto pointPortal = pointField.ReadPortal();
  VISKORES_TEST_ASSERT(pointPortal.GetNumberOfValues() == 24, "Wrong point field size.");
  for (viskores::Id plane = 0; plane < 4; ++plane)
  {
    for (viskores::Id point = 0; point < 6; ++point)
    {
      VISKORES_TEST_ASSERT(test_equal(pointPortal.Get((plane * 6) + point),
                                      static_cast<viskores::Float32>((point + 1) * 10)),
                           "Wrong replicated point field value.");
    }
  }

  viskores::cont::ArrayHandle<viskores::Float32> cellField =
    output.GetField("cellvar")
      .GetData()
      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>();
  auto cellPortal = cellField.ReadPortal();
  VISKORES_TEST_ASSERT(cellPortal.GetNumberOfValues() == 8, "Wrong cell field size.");
  for (viskores::Id plane = 0; plane < 4; ++plane)
  {
    VISKORES_TEST_ASSERT(test_equal(cellPortal.Get((plane * 2) + 0), 70.0f),
                         "Wrong replicated cell field value.");
    VISKORES_TEST_ASSERT(test_equal(cellPortal.Get((plane * 2) + 1), 80.0f),
                         "Wrong replicated cell field value.");
  }
}

void TestTriangulateInput()
{
  std::cout << "Testing optional triangulation" << std::endl;

  VISKORES_TEST_ASSERT(ThrowsFilterExecution(
                         []()
                         {
                           viskores::filter::geometry_refinement::ExtrusionRotational filter;
                           filter.SetNumberOfPlanes(4);
                           filter.Execute(MakeQuadDataSet());
                         }),
                       "Quad input should fail without triangulation.");

  viskores::filter::geometry_refinement::ExtrusionRotational filter;
  filter.SetNumberOfPlanes(4);
  filter.SetTriangulateInput(true);
  viskores::cont::DataSet output = filter.Execute(MakeQuadDataSet());
  VISKORES_TEST_ASSERT(output.GetNumberOfPoints() == 16, "Wrong triangulated output point count.");
  VISKORES_TEST_ASSERT(output.GetNumberOfCells() == 8, "Wrong triangulated output cell count.");
}

void TestValidation()
{
  std::cout << "Testing validation failures" << std::endl;

  VISKORES_TEST_ASSERT(ThrowsFilterExecution(
                         []()
                         {
                           viskores::filter::geometry_refinement::ExtrusionRotational filter;
                           filter.SetAxis({ 0.0f, 0.0f, 0.0f });
                           filter.Execute(MakeTriangleDataSet());
                         }),
                       "Zero axis should fail.");

  VISKORES_TEST_ASSERT(ThrowsFilterExecution(
                         []()
                         {
                           viskores::filter::geometry_refinement::ExtrusionRotational filter;
                           filter.SetCloseSweep(false);
                           filter.SetNumberOfPlanes(1);
                           filter.Execute(MakeTriangleDataSet());
                         }),
                       "Invalid open plane count should fail.");

  VISKORES_TEST_ASSERT(ThrowsFilterExecution(
                         []()
                         {
                           viskores::filter::geometry_refinement::ExtrusionRotational filter;
                           filter.SetCloseSweep(true);
                           filter.SetSweepAngle(viskores::Pi<viskores::FloatDefault>());
                           filter.Execute(MakeTriangleDataSet());
                         }),
                       "Closed non-full sweep should fail.");

  VISKORES_TEST_ASSERT(ThrowsFilterExecution(
                         []()
                         {
                           viskores::filter::geometry_refinement::ExtrusionRotational filter;
                           filter.SetAxisPointTolerance(-1.0);
                           filter.Execute(MakeTriangleDataSet());
                         }),
                       "Negative axis point tolerance should fail.");

  viskores::filter::geometry_refinement::ExtrusionRotational filter;
  filter.SetNumberOfPlanes(4);
  viskores::cont::DataSet output = filter.Execute(MakeTriangleDataSet(true));
  VISKORES_TEST_ASSERT(output.GetNumberOfCells() == 8, "Axis points should be allowed by default.");

  VISKORES_TEST_ASSERT(ThrowsFilterExecution(
                         []()
                         {
                           viskores::filter::geometry_refinement::ExtrusionRotational strictFilter;
                           strictFilter.SetAllowAxisPoints(false);
                           strictFilter.Execute(MakeTriangleDataSet(true));
                         }),
                       "Axis point should fail when axis points are disallowed.");
}

struct TestingExtrusionRotational
{
  void operator()() const
  {
    TestSweepAngleCloseSweepDefaults();
    TestClosedFullSweep();
    TestCompactOutput();
    TestOpenPartialSweep();
    TestCoordinates();
    TestOffsetAxisCoordinates();
    TestFieldReplication();
    TestTriangulateInput();
    TestValidation();
  }
};
} // anonymous namespace

int UnitTestExtrusionRotational(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestingExtrusionRotational{}, argc, argv);
}
