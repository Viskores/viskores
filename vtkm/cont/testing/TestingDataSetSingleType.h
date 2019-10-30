//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_testing_TestingDataSetSingleType_h
#define vtk_m_cont_testing_TestingDataSetSingleType_h

#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/worklet/CellAverage.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

namespace vtkm
{
namespace cont
{
namespace testing
{

/// This class has a single static member, Run, that tests DataSetSingleType
/// with the given DeviceAdapter
///
template <class DeviceAdapterTag>
class TestingDataSetSingleType
{
private:
  template <typename T, typename Storage>
  static bool TestArrayHandle(const vtkm::cont::ArrayHandle<T, Storage>& ah,
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

  static inline vtkm::cont::DataSet make_SingleTypeDataSet()
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

    vtkm::cont::DataSetFieldAdd fieldAdder;
    fieldAdder.AddPointField(ds, "pointvar", vars, nVerts);

    return ds;
  }

  static void TestDataSet_SingleType()
  {
    vtkm::cont::DataSet dataSet = make_SingleTypeDataSet();

    //verify that we can get a CellSetSingleType from a dataset
    vtkm::cont::CellSetSingleType<> cellset;
    dataSet.GetCellSet().CopyTo(cellset);

    //verify that the point to cell connectivity types are correct
    vtkm::cont::ArrayHandleConstant<vtkm::UInt8> shapesPointToCell =
      cellset.GetShapesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
    vtkm::cont::ArrayHandle<vtkm::Id> connPointToCell =
      cellset.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());

    VTKM_TEST_ASSERT(shapesPointToCell.GetNumberOfValues() == 3, "Wrong number of shapes");
    VTKM_TEST_ASSERT(connPointToCell.GetNumberOfValues() == 9, "Wrong connectivity length");

    //verify that the cell to point connectivity types are correct
    //note the handle storage types differ compared to point to cell
    vtkm::cont::ArrayHandleConstant<vtkm::UInt8> shapesCellToPoint =
      cellset.GetShapesArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
    vtkm::cont::ArrayHandle<vtkm::Id> connCellToPoint =
      cellset.GetConnectivityArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

    VTKM_TEST_ASSERT(shapesCellToPoint.GetNumberOfValues() == 5, "Wrong number of shapes");
    VTKM_TEST_ASSERT(connCellToPoint.GetNumberOfValues() == 9, "Wrong connectivity length");

    //run a basic for-each topology algorithm on this
    vtkm::cont::ArrayHandle<vtkm::Float32> result;
    vtkm::worklet::DispatcherMapTopology<vtkm::worklet::CellAverage> dispatcher;
    dispatcher.SetDevice(DeviceAdapterTag());
    dispatcher.Invoke(cellset, dataSet.GetField("pointvar"), result);

    vtkm::Float32 expected[3] = { 20.1333f, 30.1667f, 40.2333f };
    for (int i = 0; i < 3; ++i)
    {
      VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                       "Wrong result for CellAverage worklet on explicit single type cellset data");
    }
  }

  struct TestAll
  {
    VTKM_CONT void operator()() const { TestingDataSetSingleType::TestDataSet_SingleType(); }
  };

public:
  static VTKM_CONT int Run(int argc, char* argv[])
  {
    return vtkm::cont::testing::Testing::Run(TestAll(), argc, argv);
  }
};
}
}
} // namespace vtkm::cont::testing

#endif // vtk_m_cont_testing_TestingDataSetSingleType_h
