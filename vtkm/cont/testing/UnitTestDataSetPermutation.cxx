//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/worklet/CellAverage.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

namespace
{

template <typename T, typename Storage>
bool TestArrayHandle(const vtkm::cont::ArrayHandle<T, Storage>& ah,
                     const T* expected,
                     vtkm::Id size)
{
  if (size != ah.GetNumberOfValues())
  {
    return false;
  }

  for (vtkm::Id i = 0; i < size; ++i)
  {
    if (ah.GetPortalConstControl().Get(i) != expected[i])
    {
      return false;
    }
  }

  return true;
}

inline vtkm::cont::DataSet make_SingleTypeDataSet()
{
  using CoordType = vtkm::Vec3f_32;
  std::vector<CoordType> coordinates;
  coordinates.push_back(CoordType(0, 0, 0));
  coordinates.push_back(CoordType(1, 0, 0));
  coordinates.push_back(CoordType(1, 1, 0));
  coordinates.push_back(CoordType(2, 1, 0));
  coordinates.push_back(CoordType(2, 2, 0));

  std::vector<vtkm::Id> conn;
  // First Cell
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(2);
  // Second Cell
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(3);
  // Third Cell
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(4);

  vtkm::cont::DataSet ds;
  vtkm::cont::DataSetBuilderExplicit builder;
  ds = builder.Create(coordinates, vtkm::CellShapeTagTriangle(), 3, conn);

  //Set point scalar
  const int nVerts = 5;
  vtkm::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f };

  vtkm::cont::DataSetFieldAdd::AddPointField(ds, "pointvar", vars, nVerts);

  return ds;
}

void TestDataSet_Explicit()
{

  vtkm::cont::DataSet dataSet = make_SingleTypeDataSet();

  std::vector<vtkm::Id> validIds;
  validIds.push_back(1); //iterate the 2nd cell 4 times
  validIds.push_back(1);
  validIds.push_back(1);
  validIds.push_back(1);
  vtkm::cont::ArrayHandle<vtkm::Id> validCellIds = vtkm::cont::make_ArrayHandle(validIds);

  //get the cellset single type from the dataset
  vtkm::cont::CellSetSingleType<> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  //verify that we can create a subset of a singlset
  using SubsetType = vtkm::cont::CellSetPermutation<vtkm::cont::CellSetSingleType<>>;
  SubsetType subset;
  subset.Fill(validCellIds, cellSet);

  subset.PrintSummary(std::cout);

  using ExecObjectType = SubsetType::ExecutionTypes<vtkm::cont::DeviceAdapterTagSerial,
                                                    vtkm::TopologyElementTagCell,
                                                    vtkm::TopologyElementTagPoint>::ExecObjectType;

  ExecObjectType execConnectivity;
  execConnectivity = subset.PrepareForInput(vtkm::cont::DeviceAdapterTagSerial(),
                                            vtkm::TopologyElementTagCell(),
                                            vtkm::TopologyElementTagPoint());

  //run a basic for-each topology algorithm on this
  vtkm::cont::ArrayHandle<vtkm::Float32> result;
  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(subset, dataSet.GetField("pointvar"), result);

  //iterate same cell 4 times
  vtkm::Float32 expected[4] = { 30.1667f, 30.1667f, 30.1667f, 30.1667f };
  for (int i = 0; i < 4; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellAverage worklet on explicit subset data");
  }
}

void TestDataSet_Structured2D()
{

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  std::vector<vtkm::Id> validIds;
  validIds.push_back(1); //iterate the 2nd cell 4 times
  validIds.push_back(1);
  validIds.push_back(1);
  validIds.push_back(1);
  vtkm::cont::ArrayHandle<vtkm::Id> validCellIds = vtkm::cont::make_ArrayHandle(validIds);

  vtkm::cont::CellSetStructured<2> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  //verify that we can create a subset of a 2d UniformDataSet
  vtkm::cont::CellSetPermutation<vtkm::cont::CellSetStructured<2>> subset;
  subset.Fill(validCellIds, cellSet);

  subset.PrintSummary(std::cout);

  //verify that we can call PrepareForInput on CellSetSingleType
  using DeviceAdapterTag = vtkm::cont::DeviceAdapterTagSerial;

  //verify that PrepareForInput exists
  subset.PrepareForInput(
    DeviceAdapterTag(), vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());

  //run a basic for-each topology algorithm on this
  vtkm::cont::ArrayHandle<vtkm::Float32> result;
  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(subset, dataSet.GetField("pointvar"), result);

  vtkm::Float32 expected[4] = { 40.1f, 40.1f, 40.1f, 40.1f };
  for (int i = 0; i < 4; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellAverage worklet on 2d structured subset data");
  }
}

void TestDataSet_Structured3D()
{

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  std::vector<vtkm::Id> validIds;
  validIds.push_back(1); //iterate the 2nd cell 4 times
  validIds.push_back(1);
  validIds.push_back(1);
  validIds.push_back(1);
  vtkm::cont::ArrayHandle<vtkm::Id> validCellIds = vtkm::cont::make_ArrayHandle(validIds);

  vtkm::cont::CellSetStructured<3> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  //verify that we can create a subset of a 2d UniformDataSet
  vtkm::cont::CellSetPermutation<vtkm::cont::CellSetStructured<3>> subset;
  subset.Fill(validCellIds, cellSet);

  subset.PrintSummary(std::cout);

  //verify that PrepareForInput exists
  subset.PrepareForInput(vtkm::cont::DeviceAdapterTagSerial(),
                         vtkm::TopologyElementTagCell(),
                         vtkm::TopologyElementTagPoint());

  //run a basic for-each topology algorithm on this
  vtkm::cont::ArrayHandle<vtkm::Float32> result;
  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(subset, dataSet.GetField("pointvar"), result);

  vtkm::Float32 expected[4] = { 70.2125f, 70.2125f, 70.2125f, 70.2125f };
  for (int i = 0; i < 4; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellAverage worklet on 2d structured subset data");
  }
}

void TestDataSet_Permutation()
{
  std::cout << std::endl;
  std::cout << "--TestDataSet_Permutation--" << std::endl << std::endl;

  TestDataSet_Explicit();
  TestDataSet_Structured2D();
  TestDataSet_Structured3D();
}
}

int UnitTestDataSetPermutation(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestDataSet_Permutation, argc, argv);
}
