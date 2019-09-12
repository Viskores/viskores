//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/Entropy.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/source/Tangle.h>


namespace
{
void TestEntropy()
{
  ///// make a data set /////
  vtkm::Id3 dims(32, 32, 32);
  vtkm::source::Tangle tangle(dims);
  vtkm::cont::DataSet dataSet = tangle.Execute();

  vtkm::filter::Entropy entropyFilter;

  ///// calculate entropy of "nodevar" field of the data set /////
  entropyFilter.SetNumberOfBins(50); //set number of bins
  entropyFilter.SetActiveField("nodevar");
  vtkm::cont::DataSet resultEntropy = entropyFilter.Execute(dataSet);

  ///// get entropy from resultEntropy /////
  vtkm::cont::ArrayHandle<vtkm::Float64> entropy;
  resultEntropy.GetField("entropy").GetData().CopyTo(entropy);
  vtkm::cont::ArrayHandle<vtkm::Float64>::PortalConstControl portal =
    entropy.GetPortalConstControl();
  vtkm::Float64 entropyFromFilter = portal.Get(0);

  /////// check if calculating entopry is close enough to ground truth value /////
  VTKM_TEST_ASSERT(fabs(entropyFromFilter - 4.59093) < 0.001, "Entropy calculation is incorrect");
} // TestFieldEntropy
}

int UnitTestEntropyFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestEntropy, argc, argv);
}
