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

#ifndef vtk_m_cont_testing_TestingDataSetExplicit_h
#define vtk_m_cont_testing_TestingDataSetExplicit_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <set>

namespace vtkm
{
namespace cont
{
namespace testing
{

/// This class has a single static member, Run, that tests DataSetExplicit
/// with the given DeviceAdapter
///
template <class DeviceAdapterTag>
class TestingDataSetExplicit
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

  static void TestDataSet_Explicit()
  {
    vtkm::cont::testing::MakeTestDataSet tds;
    vtkm::cont::DataSet ds = tds.Make3DExplicitDataSet0();

    VTKM_TEST_ASSERT(ds.GetNumberOfCellSets() == 1, "Incorrect number of cell sets");

    VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Incorrect number of fields");

    // test various field-getting methods and associations
    const vtkm::cont::Field& f1 = ds.GetField("pointvar");
    VTKM_TEST_ASSERT(f1.GetAssociation() == vtkm::cont::Field::Association::POINTS,
                     "Association of 'pointvar' was not Association::POINTS");
    try
    {
      //const vtkm::cont::Field &f2 =
      ds.GetField("cellvar", vtkm::cont::Field::Association::CELL_SET);
    }
    catch (...)
    {
      VTKM_TEST_FAIL("Failed to get field 'cellvar' with Association::CELL_SET.");
    }

    try
    {
      //const vtkm::cont::Field &f3 =
      ds.GetField("cellvar", vtkm::cont::Field::Association::POINTS);
      VTKM_TEST_FAIL("Failed to get expected error for association mismatch.");
    }
    catch (vtkm::cont::ErrorBadValue& error)
    {
      std::cout << "Caught expected error for association mismatch: " << std::endl
                << "    " << error.GetMessage() << std::endl;
    }

    VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                     "Incorrect number of coordinate systems");

    // test cell-to-point connectivity
    vtkm::cont::CellSetExplicit<> cellset;
    ds.GetCellSet(0).CopyTo(cellset);

    vtkm::Id connectivitySize = 7;
    vtkm::Id numPoints = 5;

    vtkm::UInt8 correctShapes[] = { 1, 1, 1, 1, 1 };
    vtkm::IdComponent correctNumIndices[] = { 1, 2, 2, 1, 1 };
    vtkm::Id correctConnectivity[] = { 0, 0, 1, 0, 1, 1, 1 };

    vtkm::cont::ArrayHandleConstant<vtkm::UInt8> shapes =
      cellset.GetShapesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices =
      cellset.GetNumIndicesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
    vtkm::cont::ArrayHandle<vtkm::Id> conn =
      cellset.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());

    VTKM_TEST_ASSERT(TestArrayHandle(shapes, correctShapes, numPoints), "Got incorrect shapes");
    VTKM_TEST_ASSERT(TestArrayHandle(numIndices, correctNumIndices, numPoints),
                     "Got incorrect shapes");

    // Some device adapters have unstable sorts, which may cause the order of
    // the indices for each point to be different but still correct. Iterate
    // over all the points and check the connectivity for each one.
    VTKM_TEST_ASSERT(conn.GetNumberOfValues() == connectivitySize,
                     "Connectivity array wrong size.");
    vtkm::Id connectivityIndex = 0;
    for (vtkm::Id pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      vtkm::IdComponent numIncidentCells = correctNumIndices[pointIndex];
      std::set<vtkm::Id> correctIncidentCells;
      for (vtkm::IdComponent cellIndex = 0; cellIndex < numIncidentCells; cellIndex++)
      {
        correctIncidentCells.insert(correctConnectivity[connectivityIndex + cellIndex]);
      }
      for (vtkm::IdComponent cellIndex = 0; cellIndex < numIncidentCells; cellIndex++)
      {
        vtkm::Id expectedCell = conn.GetPortalConstControl().Get(connectivityIndex + cellIndex);
        std::set<vtkm::Id>::iterator foundCell = correctIncidentCells.find(expectedCell);
        VTKM_TEST_ASSERT(foundCell != correctIncidentCells.end(),
                         "An incident cell in the connectivity list is wrong or repeated.");
        correctIncidentCells.erase(foundCell);
      }
      connectivityIndex += numIncidentCells;
    }

    //verify that GetIndices works properly
    vtkm::Id expectedPointIds[4] = { 2, 1, 3, 4 };
    vtkm::Vec<vtkm::Id, 4> retrievedPointIds;
    cellset.GetIndices(1, retrievedPointIds);
    for (vtkm::IdComponent i = 0; i < 4; i++)
    {
      VTKM_TEST_ASSERT(retrievedPointIds[i] == expectedPointIds[i],
                       "Incorrect point ID for quad cell");
    }
  }

  struct TestAll
  {
    VTKM_CONT void operator()() const { TestingDataSetExplicit::TestDataSet_Explicit(); }
  };

public:
  static VTKM_CONT int Run() { return vtkm::cont::testing::Testing::Run(TestAll()); }
};
}
}
} // namespace vtkm::cont::testing

#endif // vtk_m_cont_testing_TestingDataSetExplicit_h
