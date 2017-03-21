//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/ExtractPoints.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace {


class TestingExtractPoints
{
public:
  void TestUniformById() const
  {
    std::cout << "Testing extract points structured by id:" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    vtkm::filter::ResultDataSet result;

    // Points to extract
    const int nPoints = 13;
    vtkm::Id pointids[nPoints] = {0, 1, 2, 3, 4, 5, 10, 15, 20, 25, 50, 75, 100};
    vtkm::cont::ArrayHandle<vtkm::Id> pointIds =
                            vtkm::cont::make_ArrayHandle(pointids, nPoints);

    // Setup and run filter to extract by point ids
    vtkm::filter::ExtractPoints extractPoints;
    extractPoints.SetPointIds(pointIds);
    extractPoints.SetCompactPoints(true);

    result = extractPoints.Execute(dataset);

    // Only point data can be transferred to result
    for (vtkm::IdComponent i = 0; i < dataset.GetNumberOfFields(); ++i)
    {
      extractPoints.MapFieldOntoOutput(result, dataset.GetField(i));
    }

    vtkm::cont::DataSet output = result.GetDataSet();
    VTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfCells(), nPoints), 
                     "Wrong result for ExtractPoints");
  }

  void operator()() const
  {
    this->TestUniformById();
  }
};

}

int UnitTestExtractPointsFilter(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestingExtractPoints());
}
