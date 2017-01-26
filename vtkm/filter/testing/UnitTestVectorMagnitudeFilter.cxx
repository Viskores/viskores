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

#define VTKM_DEFAULT_TYPE_LIST_TAG ::vtkm::TypeListTagAll

#include <vtkm/filter/VectorMagnitude.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace {

void TestVectorMagnitude()
{
  std::cout << "Testing VectorMagnitude Filter" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  const int nVerts = 18;
  vtkm::Float64 fvars[nVerts] = {10.1, 20.1, 30.1, 40.1, 50.2,
                                 60.2, 70.2, 80.2, 90.3, 100.3,
                                 110.3, 120.3, 130.4, 140.4,
                                 150.4, 160.4, 170.5, 180.5};

  vtkm::Int32 ivars[nVerts] = {10, 20, 30, 40, 50,
                               60, 70, 80, 90, 100,
                               110, 120, 130, 140,
                               150, 160, 170, 180};

  std::vector< vtkm::Vec<vtkm::Float64,3> > fvec(nVerts);
  std::vector< vtkm::Vec<vtkm::Int32,3> > ivec(nVerts);
  for(std::size_t i=0; i < fvec.size(); ++i)
  {
    fvec[i] = vtkm::make_Vec(fvars[i],fvars[i],fvars[i]);
    ivec[i] = vtkm::make_Vec(ivars[i],ivars[i],ivars[i]);
  }
  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float64,3> > finput =
    vtkm::cont::make_ArrayHandle(fvec);
  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Int32,3> > iinput =
    vtkm::cont::make_ArrayHandle(ivec);

  vtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "double_vec_pointvar", finput);
  vtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "integer_vec_pointvar", iinput);

  vtkm::filter::ResultField result;
  vtkm::filter::VectorMagnitude vm;

  result = vm.Execute(dataSet, dataSet.GetField("double_vec_pointvar"));

  VTKM_TEST_ASSERT( result.IsValid(), "result should be valid" );
  VTKM_TEST_ASSERT(result.GetField().GetName() == "magnitude",
                   "Output field has wrong name.");
  VTKM_TEST_ASSERT(result.GetField().GetAssociation() ==
                   vtkm::cont::Field::ASSOC_POINTS,
                   "Output field has wrong association");
  
  vtkm::cont::ArrayHandle<vtkm::Float64> resultArrayHandle;
  bool valid = result.FieldAs(resultArrayHandle);
  if(valid)
  { 
    for (vtkm::Id i = 0; i < resultArrayHandle.GetNumberOfValues(); ++i)
    {
      VTKM_TEST_ASSERT(test_equal(std::sqrt(3*fvars[i]*fvars[i]),
                                  resultArrayHandle.GetPortalConstControl().Get(i)),
         "Wrong result for Magnitude worklet");
    }
  }

  result = vm.Execute(dataSet, dataSet.GetField("integer_vec_pointvar"));

  VTKM_TEST_ASSERT( result.IsValid(), "result should be valid" );
  VTKM_TEST_ASSERT(result.GetField().GetName() == "magnitude",
                   "Output field has wrong name.");
  VTKM_TEST_ASSERT(result.GetField().GetAssociation() ==
                   vtkm::cont::Field::ASSOC_POINTS,
                   "Output field has wrong association");

  valid = result.FieldAs(resultArrayHandle);
  if(valid)
  {
    for (vtkm::Id i = 0; i < resultArrayHandle.GetNumberOfValues(); ++i)
    {
      VTKM_TEST_ASSERT(test_equal(std::sqrt(3*ivars[i]*ivars[i]),
                                  resultArrayHandle.GetPortalConstControl().Get(i)),
         "Wrong result for Magnitude worklet");
    }
  }
}

}

int UnitTestVectorMagnitudeFilter(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestVectorMagnitude);
}
