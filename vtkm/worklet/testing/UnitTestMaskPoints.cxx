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

#include <vtkm/worklet/MaskPoints.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/CellSet.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace {

using vtkm::cont::testing::MakeTestDataSet;

template <typename DeviceAdapter>
class TestingMaskPoints
{
public:
  void TestUniform2D() const
  {
    std::cout << "Testing mask points stride on 2D uniform dataset" << std::endl;

    typedef vtkm::cont::CellSetStructured<2> CellSetType;
    typedef vtkm::cont::CellSetSingleType<> OutCellSetType;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();

    CellSetType cellset;
    dataset.GetCellSet(0).CopyTo(cellset);

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));
    for (vtkm::Id indx = 0; indx < dataset.GetNumberOfFields(); indx++)
    {
      vtkm::cont::Field field = dataset.GetField(indx);
      if (field.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS)
      {
        outDataSet.AddField(field);
      }
    }

    // Output dataset gets new cell set of points that pass subsampling
    vtkm::worklet::MaskPoints maskPoints;
    OutCellSetType outCellSet;
    outCellSet = maskPoints.Run(cellset,
                                2,
                                DeviceAdapter());
    outDataSet.AddCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 12), "Wrong result for MaskPoints");
  }

  void TestUniform3D() const
  {
    std::cout << "Testing mask points stride on 3D uniform dataset" << std::endl;

    typedef vtkm::cont::CellSetStructured<3> CellSetType;
    typedef vtkm::cont::CellSetSingleType<> OutCellSetType;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    CellSetType cellset;
    dataset.GetCellSet(0).CopyTo(cellset);

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));
    for (vtkm::Id indx = 0; indx < dataset.GetNumberOfFields(); indx++)
    {
      vtkm::cont::Field field = dataset.GetField(indx);
      if (field.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS)
      {
        outDataSet.AddField(field);
      }
    }

    // Output dataset gets new cell set of points that meet threshold predicate
    vtkm::worklet::MaskPoints maskPoints;
    OutCellSetType outCellSet;
    outCellSet = maskPoints.Run(cellset,
                                5,
                                DeviceAdapter());
    outDataSet.AddCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 25), "Wrong result for MaskPoints");
  }

  void TestExplicit3D() const
  {
    std::cout << "Testing mask points stride on 3D explicit dataset" << std::endl;

    typedef vtkm::cont::CellSetExplicit<> CellSetType;
    typedef vtkm::cont::CellSetSingleType<> OutCellSetType;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    CellSetType cellset;
    dataset.GetCellSet(0).CopyTo(cellset);

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));
    for (vtkm::Id indx = 0; indx < dataset.GetNumberOfFields(); indx++)
    {
      vtkm::cont::Field field = dataset.GetField(indx);
      if (field.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS)
      {
        outDataSet.AddField(field);
      }
    }

    // Output dataset gets new cell set of points that meet threshold predicate
    vtkm::worklet::MaskPoints maskPoints;
    OutCellSetType outCellSet;
    outCellSet = maskPoints.Run(cellset,
                                3,
                                DeviceAdapter());
    outDataSet.AddCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 3), "Wrong result for MaskPoints");
  }

  void operator()() const
  {
    this->TestUniform2D();
    this->TestUniform3D();
    this->TestExplicit3D();
  }
};

}

int UnitTestMaskPoints(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(
    TestingMaskPoints<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>());
}
