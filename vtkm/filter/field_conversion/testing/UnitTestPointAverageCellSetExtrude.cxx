//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleXGCCoordinates.h>
#include <vtkm/cont/CellSetExtrude.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/field_conversion/PointAverage.h>

namespace
{
std::vector<float> points_rz = { 1.72485139f, 0.020562f,   1.73493571f,
                                 0.02052826f, 1.73478011f, 0.02299051f }; //really a vec<float,2>
std::vector<int> topology = { 0, 2, 1 };
std::vector<int> nextNode = { 0, 1, 2 };

int TestCellSetExtrude()
{
  const std::size_t numPlanes = 8;

  auto coords = vtkm::cont::make_ArrayHandleXGCCoordinates(points_rz, numPlanes, false);
  auto cells = vtkm::cont::make_CellSetExtrude(topology, coords, nextNode);
  VTKM_TEST_ASSERT(cells.GetNumberOfPoints() == coords.GetNumberOfValues(),
                   "number of points don't match between cells and coordinates");

  //test a filter
  vtkm::cont::DataSet dataset;

  dataset.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coords", coords));
  dataset.SetCellSet(cells);

  // verify that a constant value point field can be accessed
  std::vector<float> pvalues(static_cast<size_t>(coords.GetNumberOfValues()), 42.0f);
  vtkm::cont::Field pfield = vtkm::cont::make_Field(
    "pfield", vtkm::cont::Field::Association::Points, pvalues, vtkm::CopyFlag::Off);
  dataset.AddField(pfield);

  // verify that a constant cell value can be accessed
  std::vector<float> cvalues(static_cast<size_t>(cells.GetNumberOfCells()), 42.0f);
  vtkm::cont::Field cfield = vtkm::cont::make_Field(
    "cfield", vtkm::cont::Field::Association::Cells, cvalues, vtkm::CopyFlag::Off);
  dataset.AddField(cfield);

  vtkm::filter::field_conversion::PointAverage avg;
  try
  {
    avg.SetActiveField("cfield");
    auto result = avg.Execute(dataset);
    VTKM_TEST_ASSERT(result.HasPointField("cfield"), "filter resulting dataset should be valid");
  }
  catch (const vtkm::cont::Error& err)
  {
    std::cout << err.GetMessage() << std::endl;
    VTKM_TEST_ASSERT(false, "Filter execution threw an exception");
  }


  return 0;
}
}

int UnitTestPointAverageCellSetExtrude(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellSetExtrude, argc, argv);
}
