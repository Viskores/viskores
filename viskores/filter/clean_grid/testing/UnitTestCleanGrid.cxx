//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/filter/clean_grid/CleanGrid.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/contour/ContourMarchingCells.h>

#include <vector>

namespace
{

// Keep the repeated cell-set cast in one place so the tests can focus on the
// expected topology.
viskores::cont::CellSetExplicit<> GetExplicitCellSet(const viskores::cont::DataSet& dataSet)
{
  viskores::cont::CellSetExplicit<> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);
  return cellSet;
}

// Create a small explicit data set that uses every point in both cells. Several
// merge tests vary only the coordinates while keeping the topology fixed.
viskores::cont::DataSet MakeTwoTriangleDataSet(const std::vector<viskores::Vec3f_32>& coords)
{
  std::vector<viskores::UInt8> shapes{ viskores::CELL_SHAPE_TRIANGLE,
                                       viskores::CELL_SHAPE_TRIANGLE };
  std::vector<viskores::IdComponent> numIndices{ 3, 3 };
  std::vector<viskores::Id> connectivity{ 0, 2, 3, 1, 2, 3 };

  viskores::cont::DataSetBuilderExplicit builder;
  return builder.Create(coords, shapes, numIndices, connectivity);
}

void TestUniformGrid(viskores::filter::clean_grid::CleanGrid clean)
{
  std::cout << "Testing 'clean' uniform grid." << std::endl;

  viskores::cont::testing::MakeTestDataSet makeData;

  viskores::cont::DataSet inData = makeData.Make2DUniformDataSet0();

  clean.SetFieldsToPass({ "pointvar", "cellvar" });
  viskores::cont::DataSet outData = clean.Execute(inData);
  VISKORES_TEST_ASSERT(outData.HasField("pointvar"), "Failed to map point field");
  VISKORES_TEST_ASSERT(outData.HasField("cellvar"), "Failed to map cell field");

  viskores::cont::CellSetExplicit<> outCellSet;
  outData.GetCellSet().AsCellSet(outCellSet);
  VISKORES_TEST_ASSERT(outCellSet.GetNumberOfPoints() == 6,
                       "Wrong number of points: ",
                       outCellSet.GetNumberOfPoints());
  VISKORES_TEST_ASSERT(
    outCellSet.GetNumberOfCells() == 2, "Wrong number of cells: ", outCellSet.GetNumberOfCells());
  viskores::Id4 cellIds;
  outCellSet.GetIndices(0, cellIds);
  VISKORES_TEST_ASSERT((cellIds == viskores::Id4(0, 1, 4, 3)), "Bad cell ids: ", cellIds);
  outCellSet.GetIndices(1, cellIds);
  VISKORES_TEST_ASSERT((cellIds == viskores::Id4(1, 2, 5, 4)), "Bad cell ids: ", cellIds);

  viskores::cont::ArrayHandle<viskores::Float32> outPointField;
  outData.GetField("pointvar").GetData().AsArrayHandle(outPointField);
  VISKORES_TEST_ASSERT(outPointField.GetNumberOfValues() == 6,
                       "Wrong point field size: ",
                       outPointField.GetNumberOfValues());
  VISKORES_TEST_ASSERT(test_equal(outPointField.ReadPortal().Get(1), 20.1),
                       "Bad point field value: ",
                       outPointField.ReadPortal().Get(1));
  VISKORES_TEST_ASSERT(test_equal(outPointField.ReadPortal().Get(4), 50.1),
                       "Bad point field value: ",
                       outPointField.ReadPortal().Get(4));

  viskores::cont::ArrayHandle<viskores::Float32> outCellField;
  outData.GetField("cellvar").GetData().AsArrayHandle(outCellField);
  VISKORES_TEST_ASSERT(outCellField.GetNumberOfValues() == 2, "Wrong cell field size.");
  VISKORES_TEST_ASSERT(test_equal(outCellField.ReadPortal().Get(0), 100.1),
                       "Bad cell field value",
                       outCellField.ReadPortal().Get(0));
  VISKORES_TEST_ASSERT(test_equal(outCellField.ReadPortal().Get(1), 200.1),
                       "Bad cell field value",
                       outCellField.ReadPortal().Get(1));
}

// Verify that CompactPointFields removes points that are not referenced by the
// topology and that disabling it preserves point coordinates and fields.
void TestCompactPointFields()
{
  std::cout << "Testing compact point fields with unused points." << std::endl;

  std::vector<viskores::Vec3f_32> coords{ viskores::Vec3f_32(0.0f, 0.0f, 0.0f),
                                          viskores::Vec3f_32(1.0f, 0.0f, 0.0f),
                                          viskores::Vec3f_32(0.0f, 1.0f, 0.0f),
                                          viskores::Vec3f_32(1.0f, 1.0f, 0.0f),
                                          viskores::Vec3f_32(9.0f, 9.0f, 0.0f) };
  std::vector<viskores::UInt8> shapes{ viskores::CELL_SHAPE_QUAD };
  std::vector<viskores::IdComponent> numIndices{ 4 };
  std::vector<viskores::Id> connectivity{ 0, 1, 3, 2 };

  viskores::cont::DataSetBuilderExplicit builder;
  viskores::cont::DataSet inData = builder.Create(coords, shapes, numIndices, connectivity);

  const viskores::Float32 pointvar[5] = { 10.0f, 20.0f, 30.0f, 40.0f, 50.0f };
  const viskores::Float32 cellvar[1] = { 100.0f };
  inData.AddPointField("pointvar", pointvar, 5);
  inData.AddCellField("cellvar", cellvar, 1);

  viskores::filter::clean_grid::CleanGrid clean;
  clean.SetFieldsToPass({ "pointvar", "cellvar" });
  clean.SetMergePoints(false);
  clean.SetRemoveDegenerateCells(false);

  clean.SetCompactPointFields(true);
  viskores::cont::DataSet compacted = clean.Execute(inData);
  viskores::cont::CellSetExplicit<> compactedCells = GetExplicitCellSet(compacted);
  VISKORES_TEST_ASSERT(compacted.GetNumberOfPoints() == 4, "Unused point was not removed.");
  VISKORES_TEST_ASSERT(compactedCells.GetNumberOfPoints() == 4, "Cell set was not compacted.");
  viskores::Id4 compactedIds;
  compactedCells.GetIndices(0, compactedIds);
  VISKORES_TEST_ASSERT(
    (compactedIds == viskores::Id4(0, 1, 3, 2)), "Unexpected compacted cell ids: ", compactedIds);

  viskores::cont::ArrayHandle<viskores::Float32> compactedPointField;
  compacted.GetField("pointvar").GetData().AsArrayHandle(compactedPointField);
  VISKORES_TEST_ASSERT(compactedPointField.GetNumberOfValues() == 4,
                       "Compacted point field has wrong size.");
  VISKORES_TEST_ASSERT(test_equal(compactedPointField.ReadPortal().Get(3), 40.0f),
                       "Wrong compacted point field value.");

  clean.SetCompactPointFields(false);
  viskores::cont::DataSet preserved = clean.Execute(inData);
  viskores::cont::CellSetExplicit<> preservedCells = GetExplicitCellSet(preserved);
  VISKORES_TEST_ASSERT(preserved.GetNumberOfPoints() == 5, "Unused point should be preserved.");
  VISKORES_TEST_ASSERT(preservedCells.GetNumberOfPoints() == 5,
                       "Cell set should retain the original point count.");

  viskores::cont::ArrayHandle<viskores::Float32> preservedPointField;
  preserved.GetField("pointvar").GetData().AsArrayHandle(preservedPointField);
  VISKORES_TEST_ASSERT(preservedPointField.GetNumberOfValues() == 5,
                       "Preserved point field has wrong size.");
  VISKORES_TEST_ASSERT(test_equal(preservedPointField.ReadPortal().Get(4), 50.0f),
                       "Unused point field value was not preserved.");
}

// Exercise degenerate cell removal directly for both 2D and 3D cell shapes.
// The 3D cases specifically cover cells whose faces collapse after point ids
// repeat.
void TestDegenerateCellRemoval()
{
  std::cout << "Testing degenerate 2D cell removal." << std::endl;

  std::vector<viskores::Vec3f_32> coords2D{ viskores::Vec3f_32(0.0f, 0.0f, 0.0f),
                                            viskores::Vec3f_32(1.0f, 0.0f, 0.0f),
                                            viskores::Vec3f_32(0.0f, 1.0f, 0.0f),
                                            viskores::Vec3f_32(1.0f, 1.0f, 0.0f) };
  std::vector<viskores::UInt8> shapes2D{ viskores::CELL_SHAPE_TRIANGLE,
                                         viskores::CELL_SHAPE_TRIANGLE,
                                         viskores::CELL_SHAPE_QUAD,
                                         viskores::CELL_SHAPE_QUAD };
  std::vector<viskores::IdComponent> numIndices2D{ 3, 3, 4, 4 };
  std::vector<viskores::Id> connectivity2D{ 0, 1, 2, 0, 1, 1, 0, 1, 3, 2, 0, 1, 1, 0 };

  viskores::cont::DataSetBuilderExplicit builder;
  viskores::cont::DataSet data2D = builder.Create(coords2D, shapes2D, numIndices2D, connectivity2D);
  const viskores::Float32 cellvar2D[4] = { 10.0f, 20.0f, 30.0f, 40.0f };
  data2D.AddCellField("cellvar", cellvar2D, 4);

  viskores::filter::clean_grid::CleanGrid clean;
  clean.SetFieldsToPass({ "cellvar" });
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(false);
  clean.SetRemoveDegenerateCells(true);

  viskores::cont::DataSet out2D = clean.Execute(data2D);
  viskores::cont::CellSetExplicit<> out2DCells = GetExplicitCellSet(out2D);
  VISKORES_TEST_ASSERT(out2D.GetNumberOfCells() == 2, "Wrong number of 2D cells.");
  VISKORES_TEST_ASSERT(out2DCells.GetCellShape(0) == viskores::CELL_SHAPE_TRIANGLE,
                       "First 2D cell should be the valid triangle.");
  VISKORES_TEST_ASSERT(out2DCells.GetCellShape(1) == viskores::CELL_SHAPE_QUAD,
                       "Second 2D cell should be the valid quad.");

  viskores::cont::ArrayHandle<viskores::Float32> out2DCellField;
  out2D.GetField("cellvar").GetData().AsArrayHandle(out2DCellField);
  VISKORES_TEST_ASSERT(test_equal(out2DCellField.ReadPortal().Get(0), 10.0f),
                       "Wrong 2D cell field value.");
  VISKORES_TEST_ASSERT(test_equal(out2DCellField.ReadPortal().Get(1), 30.0f),
                       "Wrong 2D cell field value.");

  std::cout << "Testing degenerate 3D cell removal." << std::endl;

  std::vector<viskores::Vec3f_32> coords3D{
    viskores::Vec3f_32(0.0f, 0.0f, 0.0f), viskores::Vec3f_32(1.0f, 0.0f, 0.0f),
    viskores::Vec3f_32(1.0f, 1.0f, 0.0f), viskores::Vec3f_32(0.0f, 1.0f, 0.0f),
    viskores::Vec3f_32(0.0f, 0.0f, 1.0f), viskores::Vec3f_32(1.0f, 0.0f, 1.0f),
    viskores::Vec3f_32(1.0f, 1.0f, 1.0f), viskores::Vec3f_32(0.0f, 1.0f, 1.0f)
  };
  std::vector<viskores::UInt8> shapes3D{ viskores::CELL_SHAPE_TETRA,
                                         viskores::CELL_SHAPE_TETRA,
                                         viskores::CELL_SHAPE_HEXAHEDRON,
                                         viskores::CELL_SHAPE_HEXAHEDRON };
  std::vector<viskores::IdComponent> numIndices3D{ 4, 4, 8, 8 };
  std::vector<viskores::Id> connectivity3D{ 0, 1, 2, 4, 0, 1, 2, 2, 0, 1, 2, 3,
                                            4, 5, 6, 7, 0, 1, 2, 3, 0, 1, 2, 3 };

  viskores::cont::DataSet data3D = builder.Create(coords3D, shapes3D, numIndices3D, connectivity3D);
  const viskores::Float32 cellvar3D[4] = { 11.0f, 22.0f, 33.0f, 44.0f };
  data3D.AddCellField("cellvar", cellvar3D, 4);

  viskores::cont::DataSet out3D = clean.Execute(data3D);
  viskores::cont::CellSetExplicit<> out3DCells = GetExplicitCellSet(out3D);
  VISKORES_TEST_ASSERT(out3D.GetNumberOfCells() == 2, "Wrong number of 3D cells.");
  VISKORES_TEST_ASSERT(out3DCells.GetCellShape(0) == viskores::CELL_SHAPE_TETRA,
                       "First 3D cell should be the valid tetra.");
  VISKORES_TEST_ASSERT(out3DCells.GetCellShape(1) == viskores::CELL_SHAPE_HEXAHEDRON,
                       "Second 3D cell should be the valid hexahedron.");

  viskores::cont::ArrayHandle<viskores::Float32> out3DCellField;
  out3D.GetField("cellvar").GetData().AsArrayHandle(out3DCellField);
  VISKORES_TEST_ASSERT(test_equal(out3DCellField.ReadPortal().Get(0), 11.0f),
                       "Wrong 3D cell field value.");
  VISKORES_TEST_ASSERT(test_equal(out3DCellField.ReadPortal().Get(1), 33.0f),
                       "Wrong 3D cell field value.");
}

// Check more than output sizes when points merge: merged topology, averaged
// point fields, and merged coordinates all need to remain consistent.
void TestPointMergeDetails()
{
  std::cout << "Testing point merge coordinates, connectivity, and fields." << std::endl;

  std::vector<viskores::Vec3f_32> coords{ viskores::Vec3f_32(0.0f, 0.0f, 0.0f),
                                          viskores::Vec3f_32(0.0f, 0.0f, 0.0f),
                                          viskores::Vec3f_32(1.0f, 0.0f, 0.0f),
                                          viskores::Vec3f_32(0.0f, 1.0f, 0.0f) };
  viskores::cont::DataSet inData = MakeTwoTriangleDataSet(coords);
  const viskores::Float32 pointvar[4] = { 10.0f, 30.0f, 100.0f, 200.0f };
  const viskores::Float32 cellvar[2] = { 7.0f, 8.0f };
  inData.AddPointField("pointvar", pointvar, 4);
  inData.AddCellField("cellvar", cellvar, 2);

  viskores::filter::clean_grid::CleanGrid clean;
  clean.SetFieldsToPass({ "pointvar", "cellvar" });
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(true);
  clean.SetFastMerge(false);
  clean.SetToleranceIsAbsolute(true);
  clean.SetTolerance(0.01);
  clean.SetRemoveDegenerateCells(false);

  viskores::cont::DataSet outData = clean.Execute(inData);
  VISKORES_TEST_ASSERT(outData.GetNumberOfPoints() == 3, "Duplicate points were not merged.");

  viskores::cont::CellSetExplicit<> outCellSet = GetExplicitCellSet(outData);
  VISKORES_TEST_ASSERT(outCellSet.GetNumberOfCells() == 2, "Wrong number of cells after merge.");
  viskores::Id3 cellIds;
  outCellSet.GetIndices(0, cellIds);
  VISKORES_TEST_ASSERT((cellIds == viskores::Id3(0, 1, 2)), "Bad merged cell ids: ", cellIds);
  outCellSet.GetIndices(1, cellIds);
  VISKORES_TEST_ASSERT((cellIds == viskores::Id3(0, 1, 2)), "Bad merged cell ids: ", cellIds);

  viskores::cont::ArrayHandle<viskores::Float32> outPointField;
  outData.GetField("pointvar").GetData().AsArrayHandle(outPointField);
  VISKORES_TEST_ASSERT(test_equal(outPointField.ReadPortal().Get(0), 20.0f),
                       "Merged point field should be averaged.");
  VISKORES_TEST_ASSERT(test_equal(outPointField.ReadPortal().Get(1), 100.0f),
                       "Wrong point field value after merge.");
  VISKORES_TEST_ASSERT(test_equal(outPointField.ReadPortal().Get(2), 200.0f),
                       "Wrong point field value after merge.");

  viskores::cont::ArrayHandle<viskores::Vec3f> outCoords;
  outData.GetCoordinateSystem().GetData().AsArrayHandle(outCoords);
  VISKORES_TEST_ASSERT(test_equal(outCoords.ReadPortal().Get(0), viskores::Vec3f(0.0f)),
                       "Wrong merged coordinate.");
}

// Use the same coordinates with relative and absolute tolerance settings. The
// large bounds diagonal makes the default relative tolerance merge points that
// the same absolute tolerance should keep separate.
void TestToleranceModes()
{
  std::cout << "Testing relative and absolute tolerances." << std::endl;

  std::vector<viskores::Vec3f_32> coords{ viskores::Vec3f_32(0.0f, 0.0f, 0.0f),
                                          viskores::Vec3f_32(0.5f, 0.0f, 0.0f),
                                          viskores::Vec3f_32(1000000.0f, 0.0f, 0.0f),
                                          viskores::Vec3f_32(0.0f, 10.0f, 0.0f) };

  viskores::filter::clean_grid::CleanGrid relativeTolerance;
  relativeTolerance.SetCompactPointFields(false);
  relativeTolerance.SetMergePoints(true);
  relativeTolerance.SetFastMerge(false);
  relativeTolerance.SetRemoveDegenerateCells(false);
  relativeTolerance.SetTolerance(1.0e-6);
  relativeTolerance.SetToleranceIsAbsolute(false);

  viskores::cont::DataSet relativeOut = relativeTolerance.Execute(MakeTwoTriangleDataSet(coords));
  VISKORES_TEST_ASSERT(relativeOut.GetNumberOfPoints() == 3,
                       "Relative tolerance should scale with the bounds diagonal.");

  viskores::filter::clean_grid::CleanGrid absoluteTolerance = relativeTolerance;
  absoluteTolerance.SetToleranceIsAbsolute(true);
  viskores::cont::DataSet absoluteOut = absoluteTolerance.Execute(MakeTwoTriangleDataSet(coords));
  VISKORES_TEST_ASSERT(absoluteOut.GetNumberOfPoints() == 4,
                       "Absolute tolerance should not scale with the bounds diagonal.");
}

// Place two close points on opposite sides of a merge bin boundary. Non-fast
// merge should find them with shifted bins; fast merge should not do that extra
// pass.
void TestFastMergeBoundary()
{
  std::cout << "Testing non-fast merge across a bin boundary." << std::endl;

  std::vector<viskores::Vec3f_32> coords{ viskores::Vec3f_32(0.19f, 0.0f, 0.0f),
                                          viskores::Vec3f_32(0.21f, 0.0f, 0.0f),
                                          viskores::Vec3f_32(1.0f, 0.0f, 0.0f),
                                          viskores::Vec3f_32(0.0f, 1.0f, 0.0f) };

  viskores::filter::clean_grid::CleanGrid clean;
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(true);
  clean.SetRemoveDegenerateCells(false);
  clean.SetToleranceIsAbsolute(true);
  clean.SetTolerance(0.1);

  clean.SetFastMerge(false);
  viskores::cont::DataSet nonFastOut = clean.Execute(MakeTwoTriangleDataSet(coords));
  VISKORES_TEST_ASSERT(nonFastOut.GetNumberOfPoints() == 3,
                       "Non-fast merge should find points straddling a bin boundary.");

  clean.SetFastMerge(true);
  viskores::cont::DataSet fastOut = clean.Execute(MakeTwoTriangleDataSet(coords));
  VISKORES_TEST_ASSERT(fastOut.GetNumberOfPoints() == 4,
                       "Fast merge should not run shifted-bin passes.");
}

// Use two coordinate systems with different duplicate-point structure to verify
// that CleanGrid merges according to the selected active coordinate system.
void TestActiveCoordinateSystem()
{
  std::cout << "Testing active coordinate system for point merging." << std::endl;

  std::vector<viskores::Vec3f_32> coords{ viskores::Vec3f_32(0.0f, 0.0f, 0.0f),
                                          viskores::Vec3f_32(1.0f, 0.0f, 0.0f),
                                          viskores::Vec3f_32(2.0f, 0.0f, 0.0f),
                                          viskores::Vec3f_32(0.0f, 1.0f, 0.0f) };
  viskores::cont::DataSet inData = MakeTwoTriangleDataSet(coords);

  std::vector<viskores::Vec3f_32> mergeCoords{ viskores::Vec3f_32(0.0f, 0.0f, 0.0f),
                                               viskores::Vec3f_32(0.0f, 0.0f, 0.0f),
                                               viskores::Vec3f_32(2.0f, 0.0f, 0.0f),
                                               viskores::Vec3f_32(0.0f, 1.0f, 0.0f) };
  inData.AddCoordinateSystem(viskores::cont::CoordinateSystem(
    "mergecoords", viskores::cont::make_ArrayHandle(mergeCoords, viskores::CopyFlag::On)));

  viskores::filter::clean_grid::CleanGrid clean;
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(true);
  clean.SetFastMerge(false);
  clean.SetRemoveDegenerateCells(false);
  clean.SetToleranceIsAbsolute(true);
  clean.SetTolerance(0.01);

  viskores::cont::DataSet defaultCoordsOut = clean.Execute(inData);
  VISKORES_TEST_ASSERT(defaultCoordsOut.GetNumberOfPoints() == 4,
                       "Default coordinate system should not merge points.");

  clean.SetActiveCoordinateSystem(1);
  viskores::cont::DataSet mergeCoordsOut = clean.Execute(inData);
  VISKORES_TEST_ASSERT(mergeCoordsOut.GetNumberOfPoints() == 3,
                       "Active coordinate system was not used for point merging.");
}

void TestPointMerging()
{
  viskores::cont::testing::MakeTestDataSet makeDataSet;
  viskores::cont::DataSet baseData = makeDataSet.Make3DUniformDataSet3(viskores::Id3(4, 4, 4));

  viskores::filter::contour::ContourMarchingCells marchingCubes;
  marchingCubes.SetIsoValue(0.05);
  marchingCubes.SetMergeDuplicatePoints(false);
  marchingCubes.SetActiveField("pointvar");
  viskores::cont::DataSet inData = marchingCubes.Execute(baseData);
  constexpr viskores::Id originalNumPoints = 228;
  constexpr viskores::Id originalNumCells = 76;
  VISKORES_TEST_ASSERT(inData.GetCellSet().GetNumberOfPoints() == originalNumPoints);
  VISKORES_TEST_ASSERT(inData.GetNumberOfCells() == originalNumCells);

  viskores::filter::clean_grid::CleanGrid cleanGrid;

  std::cout << "Clean grid without any merging" << std::endl;
  cleanGrid.SetCompactPointFields(false);
  cleanGrid.SetMergePoints(false);
  cleanGrid.SetRemoveDegenerateCells(false);
  viskores::cont::DataSet noMerging = cleanGrid.Execute(inData);
  VISKORES_TEST_ASSERT(noMerging.GetNumberOfCells() == originalNumCells);
  VISKORES_TEST_ASSERT(noMerging.GetCellSet().GetNumberOfPoints() == originalNumPoints);
  VISKORES_TEST_ASSERT(noMerging.GetNumberOfPoints() == originalNumPoints);
  VISKORES_TEST_ASSERT(noMerging.GetField("pointvar").GetNumberOfValues() == originalNumPoints);
  VISKORES_TEST_ASSERT(noMerging.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid by merging very close points" << std::endl;
  cleanGrid.SetMergePoints(true);
  cleanGrid.SetFastMerge(false);
  viskores::cont::DataSet closeMerge = cleanGrid.Execute(inData);
  constexpr viskores::Id closeMergeNumPoints = 62;
  VISKORES_TEST_ASSERT(closeMerge.GetNumberOfCells() == originalNumCells);
  VISKORES_TEST_ASSERT(closeMerge.GetCellSet().GetNumberOfPoints() == closeMergeNumPoints);
  VISKORES_TEST_ASSERT(closeMerge.GetNumberOfPoints() == closeMergeNumPoints);
  VISKORES_TEST_ASSERT(closeMerge.GetField("pointvar").GetNumberOfValues() == closeMergeNumPoints);
  VISKORES_TEST_ASSERT(closeMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid by merging very close points with fast merge" << std::endl;
  cleanGrid.SetFastMerge(true);
  viskores::cont::DataSet closeFastMerge = cleanGrid.Execute(inData);
  VISKORES_TEST_ASSERT(closeFastMerge.GetNumberOfCells() == originalNumCells);
  VISKORES_TEST_ASSERT(closeFastMerge.GetCellSet().GetNumberOfPoints() == closeMergeNumPoints);
  VISKORES_TEST_ASSERT(closeFastMerge.GetNumberOfPoints() == closeMergeNumPoints);
  VISKORES_TEST_ASSERT(closeFastMerge.GetField("pointvar").GetNumberOfValues() ==
                       closeMergeNumPoints);
  VISKORES_TEST_ASSERT(closeFastMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid with largely separated points" << std::endl;
  cleanGrid.SetFastMerge(false);
  cleanGrid.SetTolerance(0.1);
  viskores::cont::DataSet farMerge = cleanGrid.Execute(inData);
  constexpr viskores::Id farMergeNumPoints = 36;
  VISKORES_TEST_ASSERT(farMerge.GetNumberOfCells() == originalNumCells);
  VISKORES_TEST_ASSERT(farMerge.GetCellSet().GetNumberOfPoints() == farMergeNumPoints);
  VISKORES_TEST_ASSERT(farMerge.GetNumberOfPoints() == farMergeNumPoints);
  VISKORES_TEST_ASSERT(farMerge.GetField("pointvar").GetNumberOfValues() == farMergeNumPoints);
  VISKORES_TEST_ASSERT(farMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid with largely separated points quickly" << std::endl;
  cleanGrid.SetFastMerge(true);
  viskores::cont::DataSet farFastMerge = cleanGrid.Execute(inData);
  constexpr viskores::Id farFastMergeNumPoints = 19;
  VISKORES_TEST_ASSERT(farFastMerge.GetNumberOfCells() == originalNumCells);
  VISKORES_TEST_ASSERT(farFastMerge.GetCellSet().GetNumberOfPoints() == farFastMergeNumPoints);
  VISKORES_TEST_ASSERT(farFastMerge.GetNumberOfPoints() == farFastMergeNumPoints);
  VISKORES_TEST_ASSERT(farFastMerge.GetField("pointvar").GetNumberOfValues() ==
                       farFastMergeNumPoints);
  VISKORES_TEST_ASSERT(farFastMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid with largely separated points quickly with degenerate cells"
            << std::endl;
  cleanGrid.SetRemoveDegenerateCells(true);
  viskores::cont::DataSet noDegenerateCells = cleanGrid.Execute(inData);
  constexpr viskores::Id numNonDegenerateCells = 18;
  VISKORES_TEST_ASSERT(noDegenerateCells.GetNumberOfCells() == numNonDegenerateCells);
  VISKORES_TEST_ASSERT(noDegenerateCells.GetCellSet().GetNumberOfPoints() == farFastMergeNumPoints);
  VISKORES_TEST_ASSERT(noDegenerateCells.GetNumberOfPoints() == farFastMergeNumPoints);
  VISKORES_TEST_ASSERT(noDegenerateCells.GetField("pointvar").GetNumberOfValues() ==
                       farFastMergeNumPoints);
  VISKORES_TEST_ASSERT(noDegenerateCells.GetField("cellvar").GetNumberOfValues() ==
                       numNonDegenerateCells);
}

void RunTest()
{
  viskores::filter::clean_grid::CleanGrid clean;

  std::cout << "*** Test with compact point fields on merge points off" << std::endl;
  clean.SetCompactPointFields(true);
  clean.SetMergePoints(false);
  TestUniformGrid(clean);

  std::cout << "*** Test with compact point fields off merge points off" << std::endl;
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(false);
  TestUniformGrid(clean);

  std::cout << "*** Test with compact point fields on merge points on" << std::endl;
  clean.SetCompactPointFields(true);
  clean.SetMergePoints(true);
  TestUniformGrid(clean);

  std::cout << "*** Test with compact point fields off merge points on" << std::endl;
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(true);
  TestUniformGrid(clean);

  std::cout << "*** Test point merging" << std::endl;
  TestPointMerging();

  std::cout << "*** Test compact point fields" << std::endl;
  TestCompactPointFields();

  std::cout << "*** Test degenerate cell removal" << std::endl;
  TestDegenerateCellRemoval();

  std::cout << "*** Test detailed point merging" << std::endl;
  TestPointMergeDetails();

  std::cout << "*** Test tolerance modes" << std::endl;
  TestToleranceModes();

  std::cout << "*** Test fast merge boundary" << std::endl;
  TestFastMergeBoundary();

  std::cout << "*** Test active coordinate system" << std::endl;
  TestActiveCoordinateSystem();
}

} // anonymous namespace

int UnitTestCleanGrid(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTest, argc, argv);
}
