//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/NDimsHistogram.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{
// Make testing dataset with three fields(variables), each one has 100 values
vtkm::cont::DataSet MakeTestDataSet()
{
  vtkm::cont::DataSet dataSet;

  const int nVerts = 100;

  vtkm::Float32 fieldA[nVerts] = { 8,  10, 9,  8,  14, 11, 12, 9,  19, 7, 8,  11, 7,  10, 11,
                                   11, 11, 6,  8,  8,  7,  15, 9,  7,  8, 10, 9,  10, 10, 12,
                                   7,  6,  14, 10, 14, 10, 7,  11, 13, 9, 13, 11, 10, 10, 12,
                                   12, 7,  12, 10, 11, 12, 8,  13, 9,  5, 12, 11, 9,  5,  9,
                                   12, 9,  6,  10, 11, 9,  9,  11, 9,  7, 7,  18, 16, 13, 12,
                                   8,  10, 11, 9,  8,  17, 3,  15, 15, 9, 10, 10, 8,  10, 9,
                                   7,  9,  8,  10, 13, 9,  7,  11, 7,  10 };

  vtkm::Float32 fieldB[nVerts] = { 24, 19, 28, 19, 25, 28, 25, 22, 27, 26, 35, 26, 30, 28, 24,
                                   23, 21, 31, 20, 11, 21, 22, 14, 25, 20, 24, 24, 21, 24, 29,
                                   26, 21, 32, 29, 23, 28, 31, 25, 23, 30, 18, 24, 22, 25, 33,
                                   24, 22, 23, 21, 17, 20, 28, 30, 18, 20, 32, 25, 24, 32, 15,
                                   27, 24, 27, 19, 30, 27, 17, 24, 29, 23, 22, 19, 24, 19, 28,
                                   24, 25, 24, 25, 30, 24, 31, 30, 27, 25, 25, 25, 15, 29, 23,
                                   29, 29, 21, 25, 35, 24, 28, 10, 31, 23 };

  vtkm::Float32 fieldC[nVerts] = {
    3, 1, 4, 6,  5,  4,  8, 7, 2, 9, 2, 0, 0, 4, 3, 2, 5, 2, 3,  6, 3, 8, 3, 4,  3,
    3, 2, 7, 2,  10, 9,  6, 1, 1, 4, 7, 3, 3, 1, 4, 4, 3, 9, 4,  4, 7, 3, 2, 4,  7,
    3, 3, 2, 10, 1,  6,  2, 2, 3, 8, 3, 3, 6, 9, 4, 1, 4, 3, 16, 7, 0, 1, 8, 7,  13,
    3, 5, 0, 3,  8,  10, 3, 5, 5, 1, 5, 2, 1, 3, 2, 5, 3, 4, 3,  3, 3, 3, 1, 13, 2
  };

  // Set point scalars
  dataSet.AddField(vtkm::cont::make_Field(
    "fieldA", vtkm::cont::Field::Association::POINTS, fieldA, nVerts, vtkm::CopyFlag::On));
  dataSet.AddField(vtkm::cont::make_Field(
    "fieldB", vtkm::cont::Field::Association::POINTS, fieldB, nVerts, vtkm::CopyFlag::On));
  dataSet.AddField(vtkm::cont::make_Field(
    "fieldC", vtkm::cont::Field::Association::POINTS, fieldC, nVerts, vtkm::CopyFlag::On));

  return dataSet;
}

void TestNDimsHistogram()
{
  // Create a dataset
  vtkm::cont::DataSet ds = MakeTestDataSet();

  vtkm::worklet::NDimsHistogram ndHistogram;

  // Set the number of data points
  ndHistogram.SetNumOfDataPoints(ds.GetField(0).GetNumberOfValues());

  // Add field one by one
  vtkm::Range rangeFieldA;
  vtkm::Float64 deltaFieldA;
  ndHistogram.AddField(ds.GetField("fieldA").GetData(), 4, rangeFieldA, deltaFieldA);

  vtkm::Range rangeFieldB;
  vtkm::Float64 deltaFieldB;
  ndHistogram.AddField(ds.GetField("fieldB").GetData(), 4, rangeFieldB, deltaFieldB);

  vtkm::Range rangeFieldC;
  vtkm::Float64 deltaFieldC;
  ndHistogram.AddField(ds.GetField("fieldC").GetData(), 4, rangeFieldC, deltaFieldC);

  // the return binIds and freqs is sparse distribution representation
  // (we do not keep the 0 frequency entities)
  // e.g. we have three variable(data arrays) in this example
  // binIds[0, 1, 2][j] is a combination of bin ID of three variable,
  // freqs[j] is the frequency of this bin IDs combination
  std::vector<vtkm::cont::ArrayHandle<vtkm::Id>> binIds;
  vtkm::cont::ArrayHandle<vtkm::Id> freqs;
  ndHistogram.Run(binIds, freqs);

  // Ground truth ND histogram
  vtkm::Id gtNonSparseBins = 33;
  vtkm::Id gtIdx0[33] = { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                          2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3 };
  vtkm::Id gtIdx1[33] = { 1, 1, 2, 3, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3,
                          0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 1, 1, 2, 2, 2, 3 };
  vtkm::Id gtIdx2[33] = { 0, 1, 1, 0, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 3,
                          0, 0, 1, 0, 1, 2, 3, 0, 1, 2, 0, 2, 0, 1, 2, 1 };
  vtkm::Id gtFreq[33] = { 1, 1, 1, 3,  2, 1, 1, 6, 6, 3, 17, 8, 2, 6, 2, 1, 2,
                          1, 1, 4, 11, 4, 1, 1, 3, 3, 1, 1,  1, 1, 1, 2, 1 };

  // Check result
  vtkm::Id nonSparseBins = binIds[0].GetPortalControl().GetNumberOfValues();
  VTKM_TEST_ASSERT(nonSparseBins == gtNonSparseBins, "Incorrect ND-histogram results");

  for (int i = 0; i < nonSparseBins; i++)
  {
    vtkm::Id idx0 = binIds[0].GetPortalControl().Get(i);
    vtkm::Id idx1 = binIds[1].GetPortalControl().Get(i);
    vtkm::Id idx2 = binIds[2].GetPortalControl().Get(i);
    vtkm::Id f = freqs.GetPortalControl().Get(i);
    VTKM_TEST_ASSERT(idx0 == gtIdx0[i] && idx1 == gtIdx1[i] && idx2 == gtIdx2[i] && f == gtFreq[i],
                     "Incorrect ND-histogram results");
  }
} // TestNDHistogram
}

int UnitTestNDimsHistogram(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestNDimsHistogram, argc, argv);
}
