//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <string>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/BOVDataSetReader.h>
#include <vtkm/io/ErrorIO.h>

namespace
{

inline vtkm::cont::DataSet readBOVDataSet(const char* fname)
{
  vtkm::cont::DataSet ds;
  vtkm::io::BOVDataSetReader reader(fname);
  try
  {
    ds = reader.ReadDataSet();
  }
  catch (vtkm::io::ErrorIO& e)
  {
    std::string message("Error reading ");
    message += fname;
    message += ", ";
    message += e.GetMessage();

    VTKM_TEST_FAIL(message.c_str());
  }

  return ds;
}

} // anonymous namespace

void TestReadingBOVDataSet()
{
  std::string bovFile =
    vtkm::cont::testing::Testing::DataPath("third_party/visit/example_temp.bov");

  auto const& ds = readBOVDataSet(bovFile.data());

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Incorrect number of fields");
  // See the .bov file: DATA SIZE: 50 50 50
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 50 * 50 * 50, "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 50 * 50 * 50,
                   "Incorrect number of points (from cell set)");
  VTKM_TEST_ASSERT(ds.GetNumberOfCells() == 49 * 49 * 49, "Incorrect number of cells");
  // See the .bov file: VARIABLE: "var"
  VTKM_TEST_ASSERT(ds.HasField("var"), "Should have field 'var', but does not.");
  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2, "There is only one field in noise.bov");
  VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                   "There is only one coordinate system in noise.bov");

  auto const& field = ds.GetField("var");
  // I'm pretty sure that all .bov files have their fields associated with points . . .
  VTKM_TEST_ASSERT(field.GetAssociation() == vtkm::cont::Field::Association::Points,
                   "The field should be associated with points.");
}


int UnitTestBOVDataSetReader(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestReadingBOVDataSet, argc, argv);
}
