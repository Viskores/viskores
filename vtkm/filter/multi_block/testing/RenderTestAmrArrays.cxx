//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/MergePartitionedDataSet.h>
#include <vtkm/filter/entity_extraction/ExternalFaces.h>
#include <vtkm/filter/entity_extraction/Threshold.h>
#include <vtkm/source/Amr.h>

#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{

void TestAmrArraysExecute(int dim, int numberOfLevels, int cellsPerDimension)
{
  std::cout << "Generate Image for AMR" << std::endl;

  // Generate AMR
  vtkm::source::Amr source;
  source.SetDimension(dim);
  source.SetNumberOfLevels(numberOfLevels);
  source.SetCellsPerDimension(cellsPerDimension);
  vtkm::cont::PartitionedDataSet amrDataSet = source.Execute();
  //  std::cout << "amr " << std::endl;
  //  amrDataSet.PrintSummary(std::cout);

  // Remove blanked cells
  vtkm::filter::entity_extraction::Threshold threshold;
  threshold.SetLowerThreshold(0);
  threshold.SetUpperThreshold(1);
  threshold.SetActiveField("vtkGhostType");
  vtkm::cont::PartitionedDataSet derivedDataSet = threshold.Execute(amrDataSet);
  //  std::cout << "derived " << std::endl;
  //  derivedDataSet.PrintSummary(std::cout);

  // Extract surface for efficient 3D pipeline
  vtkm::filter::entity_extraction::ExternalFaces surface;
  surface.SetFieldsToPass("RTDataCells");
  derivedDataSet = surface.Execute(derivedDataSet);

  // Merge dataset
  vtkm::cont::DataSet result = vtkm::cont::MergePartitionedDataSet(derivedDataSet);
  //  std::cout << "merged " << std::endl;
  //  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderTestOptions testOptions;
  testOptions.AllowedPixelErrorRatio = 0.001f;
  testOptions.ColorTable = vtkm::cont::ColorTable("inferno");
  testOptions.EnableAnnotations = false;
  vtkm::rendering::testing::RenderTest(
    result, "RTDataCells", "filter/amrArrays" + std::to_string(dim) + "D.png", testOptions);
}

void TestAmrArrays()
{
  int numberOfLevels = 5;
  int cellsPerDimension = 6;
  TestAmrArraysExecute(2, numberOfLevels, cellsPerDimension);
  TestAmrArraysExecute(3, numberOfLevels, cellsPerDimension);
}
} // namespace

int RenderTestAmrArrays(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestAmrArrays, argc, argv);
}
