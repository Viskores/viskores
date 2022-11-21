//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/VTKDataSetReader.h>

#include <string>

namespace
{

inline vtkm::cont::DataSet readVTKDataSet(const std::string& fname)
{
  vtkm::cont::DataSet ds;
  vtkm::io::VTKDataSetReader reader(fname);
  try
  {
    ds = reader.ReadDataSet();
  }
  catch (vtkm::io::ErrorIO& e)
  {
    std::string message("Error reading: ");
    message += fname;
    message += ", ";
    message += e.GetMessage();

    VTKM_TEST_FAIL(message.c_str());
  }

  return ds;
}

enum Format
{
  FORMAT_ASCII,
  FORMAT_BINARY
};

} // anonymous namespace

void TestReadingPolyData(Format format)
{
  std::string testFileName = (format == FORMAT_ASCII)
    ? vtkm::cont::testing::Testing::DataPath("unstructured/simple_poly_ascii.vtk")
    : vtkm::cont::testing::Testing::DataPath("unstructured/simple_poly_bin.vtk");

  vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 6, "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 8, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 8,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.GetNumberOfCells() == 6, "Incorrect number of cells");
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetSingleType<>>(),
                   "Incorrect cellset type");
}

void TestReadingPolyDataEmpty()
{
  vtkm::cont::DataSet data =
    readVTKDataSet(vtkm::cont::testing::Testing::DataPath("unstructured/empty_poly.vtk"));

  VTKM_TEST_ASSERT(data.GetNumberOfPoints() == 8);
  VTKM_TEST_ASSERT(data.GetNumberOfCells() == 0);
  VTKM_TEST_ASSERT(data.GetCellSet().GetNumberOfPoints() == 8);
  VTKM_TEST_ASSERT(data.GetNumberOfFields() == 2);
}

void TestReadingStructuredPoints(Format format)
{
  std::string testFileName = (format == FORMAT_ASCII)
    ? vtkm::cont::testing::Testing::DataPath("uniform/simple_structured_points_ascii.vtk")
    : vtkm::cont::testing::Testing::DataPath("uniform/simple_structured_points_bin.vtk");

  vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 72, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 72,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.GetNumberOfCells() == 30, "Incorrect number of cells");
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetStructured<3>>(),
                   "Incorrect cellset type");
}

void TestReadingStructuredPointsVisIt(Format format)
{
  VTKM_TEST_ASSERT(format == FORMAT_ASCII);

  std::string testFileName =
    vtkm::cont::testing::Testing::DataPath("uniform/simple_structured_points_visit_ascii.vtk");

  vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 64, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 64,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.GetNumberOfCells() == 27, "Incorrect number of cells");
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetStructured<3>>(),
                   "Incorrect cellset type");
}

void TestReadingUnstructuredGrid(Format format)
{
  std::string testFileName = (format == FORMAT_ASCII)
    ? vtkm::cont::testing::Testing::DataPath("unstructured/simple_unstructured_ascii.vtk")
    : vtkm::cont::testing::Testing::DataPath("unstructured/simple_unstructured_bin.vtk");

  vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 26, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 26,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.GetNumberOfCells() == 15, "Incorrect number of cells");
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetExplicit<>>(),
                   "Incorrect cellset type");
}

void TestReadingV5Format(Format format)
{
  std::string testFileName = (format == FORMAT_ASCII)
    ? vtkm::cont::testing::Testing::DataPath("unstructured/simple_unstructured_ascii_v5.vtk")
    : vtkm::cont::testing::Testing::DataPath("unstructured/simple_unstructured_bin_v5.vtk");

  vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 7, "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 26, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 26,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.GetNumberOfCells() == 15, "Incorrect number of cells");
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetExplicit<>>(),
                   "Incorrect cellset type");

  for (vtkm::IdComponent fieldIdx = 0; fieldIdx < ds.GetNumberOfFields(); ++fieldIdx)
  {
    vtkm::cont::Field field = ds.GetField(fieldIdx);
    switch (field.GetAssociation())
    {
      case vtkm::cont::Field::Association::Points:
        VTKM_TEST_ASSERT(field.GetData().GetNumberOfValues() == ds.GetNumberOfPoints(),
                         "Field ",
                         field.GetName(),
                         " is the wrong size");
        break;
      case vtkm::cont::Field::Association::Cells:
        VTKM_TEST_ASSERT(field.GetData().GetNumberOfValues() == ds.GetNumberOfCells(),
                         "Field ",
                         field.GetName(),
                         " is the wrong size");
        break;
      default:
        // Could be any size.
        break;
    }
  }
}

void TestReadingUnstructuredGridEmpty()
{
  vtkm::cont::DataSet data =
    readVTKDataSet(vtkm::cont::testing::Testing::DataPath("unstructured/empty_unstructured.vtk"));

  VTKM_TEST_ASSERT(data.GetNumberOfPoints() == 26);
  VTKM_TEST_ASSERT(data.GetNumberOfCells() == 0);
  VTKM_TEST_ASSERT(data.GetCellSet().GetNumberOfPoints() == 26);
  VTKM_TEST_ASSERT(data.GetNumberOfFields() == 3);
}

void TestReadingUnstructuredPixels()
{
  // VTK has a special pixel cell type that is the same as a quad but with a different
  // vertex order. The reader must convert pixels to quads. Make sure this is happening
  // correctly. This file has only axis-aligned pixels.
  vtkm::cont::DataSet ds =
    readVTKDataSet(vtkm::cont::testing::Testing::DataPath("unstructured/pixel_cells.vtk"));

  vtkm::cont::CellSetSingleType<> cellSet;
  ds.GetCellSet().AsCellSet(cellSet);
  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> coords;
  ds.GetCoordinateSystem().GetData().AsArrayHandle(coords);

  for (vtkm::Id cellIndex = 0; cellIndex < cellSet.GetNumberOfCells(); ++cellIndex)
  {
    VTKM_TEST_ASSERT(cellSet.GetCellShape(cellIndex) == vtkm::CELL_SHAPE_QUAD);

    constexpr vtkm::IdComponent NUM_VERTS = 4;
    vtkm::Vec<vtkm::Id, NUM_VERTS> pointIndices;
    cellSet.GetIndices(cellIndex, pointIndices);
    vtkm::Vec<vtkm::Vec3f, NUM_VERTS> pointCoords;
    auto coordPortal = coords.ReadPortal();
    for (vtkm::IdComponent vertIndex = 0; vertIndex < NUM_VERTS; ++vertIndex)
    {
      pointCoords[vertIndex] = coordPortal.Get(pointIndices[vertIndex]);
    }

    VTKM_TEST_ASSERT(pointCoords[0][0] != pointCoords[1][0]);
    VTKM_TEST_ASSERT(pointCoords[0][1] == pointCoords[1][1]);
    VTKM_TEST_ASSERT(pointCoords[0][2] == pointCoords[1][2]);

    VTKM_TEST_ASSERT(pointCoords[1][0] == pointCoords[2][0]);
    VTKM_TEST_ASSERT(pointCoords[1][1] != pointCoords[2][1]);
    VTKM_TEST_ASSERT(pointCoords[1][2] == pointCoords[2][2]);

    VTKM_TEST_ASSERT(pointCoords[2][0] != pointCoords[3][0]);
    VTKM_TEST_ASSERT(pointCoords[2][1] == pointCoords[3][1]);
    VTKM_TEST_ASSERT(pointCoords[2][2] == pointCoords[3][2]);

    VTKM_TEST_ASSERT(pointCoords[3][0] == pointCoords[0][0]);
    VTKM_TEST_ASSERT(pointCoords[3][1] != pointCoords[0][1]);
    VTKM_TEST_ASSERT(pointCoords[3][2] == pointCoords[0][2]);
  }
}

void TestReadingUnstructuredVoxels()
{
  // VTK has a special voxel cell type that is the same as a hexahedron but with a different
  // vertex order. The reader must convert voxels to hexahedra. Make sure this is happening
  // correctly. This file has only axis-aligned voxels.
  vtkm::cont::DataSet ds =
    readVTKDataSet(vtkm::cont::testing::Testing::DataPath("unstructured/voxel_cells.vtk"));

  vtkm::cont::CellSetSingleType<> cellSet;
  ds.GetCellSet().AsCellSet(cellSet);
  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> coords;
  ds.GetCoordinateSystem().GetData().AsArrayHandle(coords);

  for (vtkm::Id cellIndex = 0; cellIndex < cellSet.GetNumberOfCells(); ++cellIndex)
  {
    VTKM_TEST_ASSERT(cellSet.GetCellShape(cellIndex) == vtkm::CELL_SHAPE_HEXAHEDRON);

    constexpr vtkm::IdComponent NUM_VERTS = 8;
    vtkm::Vec<vtkm::Id, NUM_VERTS> pointIndices;
    cellSet.GetIndices(cellIndex, pointIndices);
    vtkm::Vec<vtkm::Vec3f, NUM_VERTS> pointCoords;
    auto coordPortal = coords.ReadPortal();
    for (vtkm::IdComponent vertIndex = 0; vertIndex < NUM_VERTS; ++vertIndex)
    {
      pointCoords[vertIndex] = coordPortal.Get(pointIndices[vertIndex]);
    }

    VTKM_TEST_ASSERT(pointCoords[0][0] != pointCoords[1][0]);
    VTKM_TEST_ASSERT(pointCoords[0][1] == pointCoords[1][1]);
    VTKM_TEST_ASSERT(pointCoords[0][2] == pointCoords[1][2]);

    VTKM_TEST_ASSERT(pointCoords[1][0] == pointCoords[2][0]);
    VTKM_TEST_ASSERT(pointCoords[1][1] != pointCoords[2][1]);
    VTKM_TEST_ASSERT(pointCoords[1][2] == pointCoords[2][2]);

    VTKM_TEST_ASSERT(pointCoords[2][0] != pointCoords[3][0]);
    VTKM_TEST_ASSERT(pointCoords[2][1] == pointCoords[3][1]);
    VTKM_TEST_ASSERT(pointCoords[2][2] == pointCoords[3][2]);

    VTKM_TEST_ASSERT(pointCoords[3][0] == pointCoords[0][0]);
    VTKM_TEST_ASSERT(pointCoords[3][1] != pointCoords[0][1]);
    VTKM_TEST_ASSERT(pointCoords[3][2] == pointCoords[0][2]);

    VTKM_TEST_ASSERT(pointCoords[0][0] == pointCoords[4][0]);
    VTKM_TEST_ASSERT(pointCoords[0][1] == pointCoords[4][1]);
    VTKM_TEST_ASSERT(pointCoords[0][2] != pointCoords[4][2]);

    VTKM_TEST_ASSERT(pointCoords[1][0] == pointCoords[5][0]);
    VTKM_TEST_ASSERT(pointCoords[1][1] == pointCoords[5][1]);
    VTKM_TEST_ASSERT(pointCoords[1][2] != pointCoords[5][2]);

    VTKM_TEST_ASSERT(pointCoords[2][0] == pointCoords[6][0]);
    VTKM_TEST_ASSERT(pointCoords[2][1] == pointCoords[6][1]);
    VTKM_TEST_ASSERT(pointCoords[2][2] != pointCoords[6][2]);

    VTKM_TEST_ASSERT(pointCoords[3][0] == pointCoords[7][0]);
    VTKM_TEST_ASSERT(pointCoords[3][1] == pointCoords[7][1]);
    VTKM_TEST_ASSERT(pointCoords[3][2] != pointCoords[7][2]);
  }
}

void TestReadingUnstructuredGridVisIt(Format format)
{
  VTKM_TEST_ASSERT(format == FORMAT_ASCII);

  std::string testFileName =
    vtkm::cont::testing::Testing::DataPath("unstructured/simple_unstructured_visit_ascii.vtk");

  vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 26, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 26,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.GetNumberOfCells() == 15, "Incorrect number of cells");
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetExplicit<>>(),
                   "Incorrect cellset type");
}

void TestReadingRectilinearGrid1(Format format)
{
  VTKM_TEST_ASSERT(format == FORMAT_ASCII);

  std::string testFileName =
    vtkm::cont::testing::Testing::DataPath("rectilinear/simple_rectilinear1_ascii.vtk");

  vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 125, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 125,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.GetNumberOfCells() == 64, "Incorrect number of cells");
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetStructured<3>>(),
                   "Incorrect cellset type");
}

void TestReadingRectilinearGrid2(Format format)
{
  VTKM_TEST_ASSERT(format == FORMAT_ASCII);

  std::string testFileName =
    vtkm::cont::testing::Testing::DataPath("rectilinear/simple_rectilinear2_ascii.vtk");

  vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 24, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 24,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.GetNumberOfCells() == 6, "Incorrect number of cells");
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetStructured<3>>(),
                   "Incorrect cellset type");
}

void TestReadingStructuredGridASCII()
{
  std::string testFileName =
    vtkm::cont::testing::Testing::DataPath("curvilinear/simple_structured_ascii.vtk");

  vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 6, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 6,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.GetNumberOfCells() == 2, "Incorrect number of cells");
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetStructured<2>>(),
                   "Incorrect cellset type");
}

void TestReadingStructuredGridBin()
{
  std::string testFileName =
    vtkm::cont::testing::Testing::DataPath("curvilinear/simple_structured_bin.vtk");

  vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 18, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 18,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.GetNumberOfCells() == 4, "Incorrect number of cells");
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetStructured<3>>(),
                   "Incorrect cellset type");
}

void TestReadingFishTank()
{
  std::string fishtank = vtkm::cont::testing::Testing::DataPath("rectilinear/fishtank.vtk");
  vtkm::cont::DataSet ds = readVTKDataSet(fishtank.c_str());

  // This is information you can glean by running 'strings' on fishtank.vtk:
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetStructured<3>>(),
                   "Incorrect cellset type");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 50 * 50 * 50, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 50 * 50 * 50,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.HasField("vec"), "The vtk file has a field 'vec', but the dataset does not.");
  VTKM_TEST_ASSERT(ds.HasField("vec_magnitude"),
                   "The vtk file has a field 'vec_magnitude', but the dataset does not.");

  // I believe the coordinate system is implicitly given by the first element of X_COORDINATES:
  VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                   "Need one and only one coordinate system.");
  // In order to get the data from the coordinate system, I used the following workflow:
  // First, I deleted all ascii header lines just past 'X_COORDINATES 50 float'.
  // Once this is done, I can get the binary data from
  // $ od -tfF --endian=big fishtank_copy.vtk
  // The result is:
  // 0     0.020408163 ... 0.9591837  0.97959185   1
  // So monotone increasing, bound [0,1].
  const vtkm::cont::CoordinateSystem& coordinateSystem = ds.GetCoordinateSystem();
  vtkm::Vec<vtkm::Range, 3> ranges = coordinateSystem.GetRange();
  vtkm::Range xRange = ranges[0];
  VTKM_TEST_ASSERT(xRange.Min == 0);
  VTKM_TEST_ASSERT(xRange.Max == 1);
  // Do the same past 'Y_COORDINATES 50 float'.
  // You get exactly the same as the x data.
  vtkm::Range yRange = ranges[1];
  VTKM_TEST_ASSERT(yRange.Min == 0);
  VTKM_TEST_ASSERT(yRange.Max == 1);
  // And finally, do it past 'Z_COORDINATES 50 float':
  vtkm::Range zRange = ranges[2];
  VTKM_TEST_ASSERT(zRange.Min == 0);
  VTKM_TEST_ASSERT(zRange.Max == 1);

  // Now delete the text up to LOOKUP TABLE default.
  // I see:
  // 0 0 0 0 3.5267966 . . .
  // This is a vector magnitude, so all values must be >= 0.
  // A cursory glance shows that 124.95 is a large value, so we can sanity check the data with the bounds
  // [0, ~130].
  // And if we open the file in Paraview, we can observe the bounds [0, 156.905].
  const vtkm::cont::Field& vec_magnitude = ds.GetField("vec_magnitude");
  VTKM_TEST_ASSERT(vec_magnitude.GetName() == "vec_magnitude");
  VTKM_TEST_ASSERT(vec_magnitude.IsPointField());

  vtkm::Range mag_range;
  vec_magnitude.GetRange(&mag_range);
  VTKM_TEST_ASSERT(mag_range.Min == 0);
  VTKM_TEST_ASSERT(mag_range.Max <= 156.906);

  // This info was gleaned from the Paraview Information panel:
  const vtkm::cont::Field& vec = ds.GetField("vec");
  VTKM_TEST_ASSERT(vec.GetName() == "vec");
  VTKM_TEST_ASSERT(vec.IsPointField());
  // Bounds from Information panel:
  // [-65.3147, 86.267], [-88.0325, 78.7217], [-67.0969, 156.867]
  const vtkm::cont::ArrayHandle<vtkm::Range>& vecRanges = vec.GetRange();
  VTKM_TEST_ASSERT(vecRanges.GetNumberOfValues() == 3);
  auto vecRangesReadPortal = vecRanges.ReadPortal();

  auto xVecRange = vecRangesReadPortal.Get(0);
  VTKM_TEST_ASSERT(xVecRange.Min >= -65.3148 && xVecRange.Min <= -65.3146);
  VTKM_TEST_ASSERT(xVecRange.Max >= 86.26 && xVecRange.Min <= 86.268);

  auto yVecRange = vecRangesReadPortal.Get(1);
  VTKM_TEST_ASSERT(yVecRange.Min >= -88.0326 && yVecRange.Min <= -88.0324);
  VTKM_TEST_ASSERT(yVecRange.Max >= 78.721);
  VTKM_TEST_ASSERT(yVecRange.Max <= 78.7218);

  auto zVecRange = vecRangesReadPortal.Get(2);
  VTKM_TEST_ASSERT(zVecRange.Min >= -67.097 && zVecRange.Min <= -67.096);
  VTKM_TEST_ASSERT(zVecRange.Max >= 156.866 && zVecRange.Max <= 156.868);
}

void TestReadingDoublePrecisionFishTank()
{
  std::string fishtank =
    vtkm::cont::testing::Testing::DataPath("rectilinear/fishtank_double_big_endian.vtk");
  vtkm::cont::DataSet ds = readVTKDataSet(fishtank.c_str());

  // This is information you can glean by running 'strings' on fishtank.vtk:
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetStructured<3>>(),
                   "Incorrect cellset type");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 50 * 50 * 50, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 50 * 50 * 50,
                   "Incorrect number of points (from cell set)");

  VTKM_TEST_ASSERT(ds.HasField("vec"), "The vtk file has a field 'vec', but the dataset does not.");


  VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                   "fishtank has one and only one coordinate system.");
  // See the single precision version for info:
  const vtkm::cont::CoordinateSystem& coordinateSystem = ds.GetCoordinateSystem();
  vtkm::Vec<vtkm::Range, 3> ranges = coordinateSystem.GetRange();
  vtkm::Range xRange = ranges[0];
  VTKM_TEST_ASSERT(xRange.Min == 0);
  VTKM_TEST_ASSERT(xRange.Max == 1);
  vtkm::Range yRange = ranges[1];
  VTKM_TEST_ASSERT(yRange.Min == 0);
  VTKM_TEST_ASSERT(yRange.Max == 1);
  vtkm::Range zRange = ranges[2];
  VTKM_TEST_ASSERT(zRange.Min == 0);
  VTKM_TEST_ASSERT(zRange.Max == 1);

  // This info was gleaned from the Paraview Information panel:
  const vtkm::cont::Field& vec = ds.GetField("vec");
  VTKM_TEST_ASSERT(vec.GetName() == "vec");
  VTKM_TEST_ASSERT(vec.IsPointField());
  // Bounds from Information panel:
  // [-65.3147, 86.267], [-88.0325, 78.7217], [-67.0969, 156.867]
  const vtkm::cont::ArrayHandle<vtkm::Range>& vecRanges = vec.GetRange();
  VTKM_TEST_ASSERT(vecRanges.GetNumberOfValues() == 3);
  auto vecRangesReadPortal = vecRanges.ReadPortal();

  auto xVecRange = vecRangesReadPortal.Get(0);
  VTKM_TEST_ASSERT(xVecRange.Min >= -65.3148 && xVecRange.Min <= -65.3146);
  VTKM_TEST_ASSERT(xVecRange.Max >= 86.26 && xVecRange.Min <= 86.268);

  auto yVecRange = vecRangesReadPortal.Get(1);
  VTKM_TEST_ASSERT(yVecRange.Min >= -88.0326 && yVecRange.Min <= -88.0324);
  VTKM_TEST_ASSERT(yVecRange.Max >= 78.721);
  VTKM_TEST_ASSERT(yVecRange.Max <= 78.7218);

  auto zVecRange = vecRangesReadPortal.Get(2);
  VTKM_TEST_ASSERT(zVecRange.Min >= -67.097 && zVecRange.Min <= -67.096);
  VTKM_TEST_ASSERT(zVecRange.Max >= 156.866 && zVecRange.Max <= 156.868);
}

void TestReadingASCIIFishTank()
{
  std::string fishtank =
    vtkm::cont::testing::Testing::DataPath("rectilinear/fishtank_double_ascii.vtk");
  vtkm::cont::DataSet ds = readVTKDataSet(fishtank.c_str());
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetStructured<3>>(),
                   "Incorrect cellset type");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 50 * 50 * 50, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 50 * 50 * 50,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.HasField("vec"), "The vtk file has a field 'vec', but the dataset does not.");
  VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                   "fishtank has one and only one coordinate system.");
  const vtkm::cont::CoordinateSystem& coordinateSystem = ds.GetCoordinateSystem();
  vtkm::Vec<vtkm::Range, 3> ranges = coordinateSystem.GetRange();
  vtkm::Range xRange = ranges[0];
  VTKM_TEST_ASSERT(xRange.Min == 0);
  VTKM_TEST_ASSERT(xRange.Max == 1);
  vtkm::Range yRange = ranges[1];
  VTKM_TEST_ASSERT(yRange.Min == 0);
  VTKM_TEST_ASSERT(yRange.Max == 1);
  vtkm::Range zRange = ranges[2];
  VTKM_TEST_ASSERT(zRange.Min == 0);
  VTKM_TEST_ASSERT(zRange.Max == 1);

  const vtkm::cont::Field& vec = ds.GetField("vec");
  VTKM_TEST_ASSERT(vec.GetName() == "vec");
  VTKM_TEST_ASSERT(vec.IsPointField());
  // Bounds from Paraview information panel:
  // [-65.3147, 86.267], [-88.0325, 78.7217], [-67.0969, 156.867]
  const vtkm::cont::ArrayHandle<vtkm::Range>& vecRanges = vec.GetRange();
  VTKM_TEST_ASSERT(vecRanges.GetNumberOfValues() == 3);
  auto vecRangesReadPortal = vecRanges.ReadPortal();
  auto xVecRange = vecRangesReadPortal.Get(0);
  VTKM_TEST_ASSERT(xVecRange.Min >= -65.3148 && xVecRange.Min <= -65.3146);
  VTKM_TEST_ASSERT(xVecRange.Max >= 86.26 && xVecRange.Min <= 86.268);

  auto yVecRange = vecRangesReadPortal.Get(1);
  VTKM_TEST_ASSERT(yVecRange.Min >= -88.0326 && yVecRange.Min <= -88.0324);
  VTKM_TEST_ASSERT(yVecRange.Max >= 78.721);
  VTKM_TEST_ASSERT(yVecRange.Max <= 78.7218);

  auto zVecRange = vecRangesReadPortal.Get(2);
  VTKM_TEST_ASSERT(zVecRange.Min >= -67.097 && zVecRange.Min <= -67.096);
  VTKM_TEST_ASSERT(zVecRange.Max >= 156.866 && zVecRange.Max <= 156.868);
}

void TestReadingFusion()
{
  std::string fusion = vtkm::cont::testing::Testing::DataPath("rectilinear/fusion.vtk");
  vtkm::cont::DataSet ds = readVTKDataSet(fusion.c_str());

  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetStructured<3>>(),
                   "Incorrect cellset type");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 32 * 32 * 32, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 32 * 32 * 32,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.HasField("vec_magnitude"),
                   "The vtk file has a field 'vec_magnitude', but the dataset does not.");
  VTKM_TEST_ASSERT(ds.HasField("vec"), "The vtk file has a field 'vec', but the dataset does not.");
  VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                   "The vtk file has a field 'vec', but the dataset does not.");

  // Taken from Paraview + clicking Data Axes Grid:
  const vtkm::cont::CoordinateSystem& coordinateSystem = ds.GetCoordinateSystem();
  vtkm::Vec<vtkm::Range, 3> ranges = coordinateSystem.GetRange();
  vtkm::Range xRange = ranges[0];
  VTKM_TEST_ASSERT(xRange.Min == 0);
  VTKM_TEST_ASSERT(xRange.Max == 1);
  vtkm::Range yRange = ranges[1];
  VTKM_TEST_ASSERT(yRange.Min == 0);
  VTKM_TEST_ASSERT(yRange.Max == 1);
  vtkm::Range zRange = ranges[2];
  VTKM_TEST_ASSERT(zRange.Min == 0);
  VTKM_TEST_ASSERT(zRange.Max == 1);

  // Paraview Information Panel of this file:
  // vec_magnitude [0, 3.73778]
  vtkm::cont::Field vec_magnitude = ds.GetField("vec_magnitude");
  VTKM_TEST_ASSERT(vec_magnitude.GetName() == "vec_magnitude");
  VTKM_TEST_ASSERT(vec_magnitude.IsPointField());

  vtkm::Range mag_range;
  vec_magnitude.GetRange(&mag_range);
  VTKM_TEST_ASSERT(mag_range.Min == 0);
  VTKM_TEST_ASSERT(mag_range.Max <= 3.73779);
  VTKM_TEST_ASSERT(mag_range.Max >= 3.73777);

  vtkm::cont::Field vec = ds.GetField("vec");
  VTKM_TEST_ASSERT(vec.GetName() == "vec");
  VTKM_TEST_ASSERT(vec.IsPointField());
  const vtkm::cont::ArrayHandle<vtkm::Range>& vecRanges = vec.GetRange();
  VTKM_TEST_ASSERT(vecRanges.GetNumberOfValues() == 3);
  auto vecRangesReadPortal = vecRanges.ReadPortal();

  // vec float [-3.41054, 3.40824], [-3.41018, 3.41036], [-0.689022, 0.480726]
  auto xVecRange = vecRangesReadPortal.Get(0);
  VTKM_TEST_ASSERT(test_equal(xVecRange.Min, -3.41054));
  VTKM_TEST_ASSERT(test_equal(xVecRange.Max, 3.40824));

  auto yVecRange = vecRangesReadPortal.Get(1);

  VTKM_TEST_ASSERT(test_equal(yVecRange.Min, -3.41018));
  VTKM_TEST_ASSERT(test_equal(yVecRange.Max, 3.41036));

  auto zVecRange = vecRangesReadPortal.Get(2);
  VTKM_TEST_ASSERT(test_equal(zVecRange.Min, -0.689022));
  VTKM_TEST_ASSERT(test_equal(zVecRange.Max, 0.480726));
}

void TestSkppingStringFields(Format format)
{
  std::string testFileName = (format == FORMAT_ASCII)
    ? vtkm::cont::testing::Testing::DataPath("uniform/simple_structured_points_strings_ascii.vtk")
    : vtkm::cont::testing::Testing::DataPath("uniform/simple_structured_points_strings_bin.vtk");

  vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 72, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 72,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.GetNumberOfCells() == 30, "Incorrect number of cells");
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetStructured<3>>(),
                   "Incorrect cellset type");
}

void TestReadingVTKDataSet()
{
  std::cout << "Test reading VTK Polydata file in ASCII" << std::endl;
  TestReadingPolyData(FORMAT_ASCII);
  std::cout << "Test reading VTK Polydata file in BINARY" << std::endl;
  TestReadingPolyData(FORMAT_BINARY);
  std::cout << "Test reading VTK Polydata with no cells" << std::endl;
  TestReadingPolyDataEmpty();
  std::cout << "Test reading VTK StructuredPoints file in ASCII" << std::endl;
  TestReadingStructuredPoints(FORMAT_ASCII);

  std::cout << "Test reading VTK StructuredPoints file in BINARY" << std::endl;
  TestReadingStructuredPoints(FORMAT_BINARY);
  std::cout << "Test reading VTK UnstructuredGrid file in ASCII" << std::endl;
  TestReadingUnstructuredGrid(FORMAT_ASCII);
  std::cout << "Test reading VTK UnstructuredGrid file in BINARY" << std::endl;
  TestReadingUnstructuredGrid(FORMAT_BINARY);
  std::cout << "Test reading VTK UnstructuredGrid with no cells" << std::endl;
  TestReadingUnstructuredGridEmpty();
  std::cout << "Test reading VTK UnstructuredGrid with pixels" << std::endl;
  TestReadingUnstructuredPixels();
  std::cout << "Test reading VTK UnstructuredGrid with voxels" << std::endl;
  TestReadingUnstructuredVoxels();

  std::cout << "Test reading VTK RectilinearGrid file in ASCII" << std::endl;
  TestReadingRectilinearGrid1(FORMAT_ASCII);
  TestReadingRectilinearGrid2(FORMAT_ASCII);

  std::cout << "Test reading VTK/VisIt StructuredPoints file in ASCII" << std::endl;
  TestReadingStructuredPointsVisIt(FORMAT_ASCII);
  std::cout << "Test reading VTK/VisIt UnstructuredGrid file in ASCII" << std::endl;
  TestReadingUnstructuredGridVisIt(FORMAT_ASCII);

  std::cout << "Test reading VTK StructuredGrid file in ASCII" << std::endl;
  TestReadingStructuredGridASCII();
  std::cout << "Test reading VTK StructuredGrid file in BINARY" << std::endl;
  TestReadingStructuredGridBin();
  std::cout << "Test reading float precision fishtank" << std::endl;
  TestReadingFishTank();
  std::cout << "Test reading double precision fishtank" << std::endl;
  TestReadingDoublePrecisionFishTank();
  std::cout << "Test ASCII fishtank" << std::endl;
  TestReadingASCIIFishTank();
  std::cout << "Test reading fusion" << std::endl;
  TestReadingFusion();

  std::cout << "Test skipping string fields in ASCII files" << std::endl;
  TestSkppingStringFields(FORMAT_ASCII);
  std::cout << "Test skipping string fields in BINARY files" << std::endl;
  TestSkppingStringFields(FORMAT_BINARY);

  std::cout << "Test reading v5 file format in ASCII" << std::endl;
  TestReadingV5Format(FORMAT_ASCII);
  std::cout << "Test reading v5 file format in BINARY" << std::endl;
  TestReadingV5Format(FORMAT_BINARY);
}

int UnitTestVTKDataSetReader(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestReadingVTKDataSet, argc, argv);
}
