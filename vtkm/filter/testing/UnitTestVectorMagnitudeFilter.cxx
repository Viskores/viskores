//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/VectorMagnitude.h>

#include <vector>

namespace
{

void TestVectorMagnitude()
{
  std::cout << "Testing VectorMagnitude Filter" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  const int nVerts = 18;
  vtkm::Float64 fvars[nVerts] = { 10.1,  20.1,  30.1,  40.1,  50.2,  60.2,  70.2,  80.2,  90.3,
                                  100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5 };

  std::vector<vtkm::Vec<vtkm::Float64, 3>> fvec(nVerts);
  for (std::size_t i = 0; i < fvec.size(); ++i)
  {
    fvec[i] = vtkm::make_Vec(fvars[i], fvars[i], fvars[i]);
  }
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>> finput = vtkm::cont::make_ArrayHandle(fvec);

  vtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "double_vec_pointvar", finput);

  vtkm::filter::VectorMagnitude vm;
  vm.SetActiveField("double_vec_pointvar");
  auto result = vm.Execute(dataSet);

  VTKM_TEST_ASSERT(result.HasField("magnitude", vtkm::cont::Field::Association::POINTS),
                   "Output field missing.");

  vtkm::cont::ArrayHandle<vtkm::Float64> resultArrayHandle;
  result.GetField("magnitude", vtkm::cont::Field::Association::POINTS)
    .GetData()
    .CopyTo(resultArrayHandle);
  for (vtkm::Id i = 0; i < resultArrayHandle.GetNumberOfValues(); ++i)
  {
    VTKM_TEST_ASSERT(test_equal(std::sqrt(3 * fvars[i] * fvars[i]),
                                resultArrayHandle.GetPortalConstControl().Get(i)),
                     "Wrong result for Magnitude worklet");
  }
}
}

int UnitTestVectorMagnitudeFilter(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestVectorMagnitude);
}
