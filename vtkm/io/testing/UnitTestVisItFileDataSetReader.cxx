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
#include <vtkm/io/ErrorIO.h>
#include <vtkm/io/VTKVisItFileReader.h>

namespace
{

inline vtkm::cont::PartitionedDataSet readVisItFileDataSet(const std::string& fname)
{
  vtkm::cont::PartitionedDataSet pds;
  vtkm::io::VTKVisItFileReader reader(fname);
  try
  {
    pds = reader.ReadPartitionedDataSet();
  }
  catch (vtkm::io::ErrorIO& e)
  {
    std::string message("Error reading ");
    message += fname;
    message += ", ";
    message += e.GetMessage();

    VTKM_TEST_FAIL(message.c_str());
  }

  return pds;
}

} // anonymous namespace

void TestReadingVisItFileDataSet()
{
  std::string visItFile = vtkm::cont::testing::Testing::DataPath("uniform/venn250.visit");

  auto const& pds = readVisItFileDataSet(visItFile);
  VTKM_TEST_ASSERT(pds.GetNumberOfPartitions() == 2, "Incorrect number of partitions");

  for (const auto& ds : pds)
  {
    VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == 63001, "Wrong number of points in partition");
    VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 5, "Wrong number of fields in partition");
  }
}


int UnitTestVisItFileDataSetReader(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestReadingVisItFileDataSet, argc, argv);
}
