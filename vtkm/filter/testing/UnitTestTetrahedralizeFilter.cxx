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

#include <vtkm/filter/Tetrahedralize.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace {

class TestingTetrahedralize
{
public:
  void TestStructured() const
  {
    std::cout << "Testing tetrahedralize structured" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet0();
    vtkm::filter::ResultDataSet result;

    vtkm::filter::Tetrahedralize tetrahedralize;

    result = tetrahedralize.Execute(dataset);

    tetrahedralize.MapFieldOntoOutput(result, dataset.GetField("pointvar") );
    tetrahedralize.MapFieldOntoOutput(result, dataset.GetField("cellvar") );

    vtkm::cont::DataSet output = result.GetDataSet();
    VTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfCells(), 20), 
                     "Wrong result for Tetrahedralize");
    VTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetData().GetNumberOfValues(), 18),
                     "Wrong number of points for Tetrahedralize");
  }

  void TestExplicit() const
  {
    std::cout << "Testing tetrahedralize explicit" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();
    vtkm::filter::ResultDataSet result;

    vtkm::filter::Tetrahedralize tetrahedralize;

    result = tetrahedralize.Execute(dataset);

    tetrahedralize.MapFieldOntoOutput(result, dataset.GetField("pointvar") );
    tetrahedralize.MapFieldOntoOutput(result, dataset.GetField("cellvar") );

    vtkm::cont::DataSet output = result.GetDataSet();
    VTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfCells(), 11), 
                     "Wrong result for Tetrahedralize");
    VTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetData().GetNumberOfValues(), 11),
                     "Wrong number of points for Tetrahedralize");
  }

  void operator()() const
  {
    this->TestStructured();
    this->TestExplicit();
  }
};

}

int UnitTestTetrahedralizeFilter(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestingTetrahedralize());
}
