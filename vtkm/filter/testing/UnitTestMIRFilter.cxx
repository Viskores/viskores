//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/MIRFilter.h>

void ConnectionHelperHex(std::vector<vtkm::Id>& conn, int x, int y, int z, int mx, int my, int mz)
{
  (void)mz;
  conn.push_back(mx * (my * z + y) + x);
  conn.push_back(mx * (my * z + y) + x + 1);
  conn.push_back(mx * (my * z + y + 1) + x + 1);
  conn.push_back(mx * (my * z + y + 1) + x);
  conn.push_back(mx * (my * (z + 1) + y) + x);
  conn.push_back(mx * (my * (z + 1) + y) + x + 1);
  conn.push_back(mx * (my * (z + 1) + y + 1) + x + 1);
  conn.push_back(mx * (my * (z + 1) + y + 1) + x);
}

vtkm::cont::DataSet GetTestDataSet()
{
  vtkm::cont::DataSetBuilderExplicit dsb;

  int mx = 3, my = 3, mz = 3;


  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::Id> connections;
  std::vector<vtkm::IdComponent> numberofInd;
  std::vector<vtkm::Vec3f_32> points;

  for (int z = 0; z < mz - 1; z++)
  {
    for (int y = 0; y < my - 1; y++)
    {
      for (int x = 0; x < mx - 1; x++)
      {
        ConnectionHelperHex(connections, x, y, z, mx, my, mz);
      }
    }
  }

  std::vector<vtkm::Id> idAR{ 1, 2, 2, 1, 2, 1, 1, 2 };
  std::vector<vtkm::Id> lnAR{ 1, 1, 1, 1, 1, 1, 1, 1 };
  std::vector<vtkm::Id> ofAR{ 0, 1, 2, 3, 4, 5, 6, 7 };
  vtkm::cont::ArrayHandle<vtkm::Id> offsets =
    vtkm::cont::make_ArrayHandle(ofAR, vtkm::CopyFlag::On);
  vtkm::cont::ArrayHandle<vtkm::Id> lengths =
    vtkm::cont::make_ArrayHandle(lnAR, vtkm::CopyFlag::On);
  vtkm::cont::ArrayHandle<vtkm::Id> ids = vtkm::cont::make_ArrayHandle(idAR, vtkm::CopyFlag::On);
  std::vector<vtkm::Float32> vfAR{ 1, 1, 1, 1, 1, 1, 1, 1 };
  vtkm::cont::ArrayHandle<vtkm::Float32> vfs =
    vtkm::cont::make_ArrayHandle(vfAR, vtkm::CopyFlag::On);

  shapes.reserve((mx - 1) * (my - 1) * (mz - 1));
  numberofInd.reserve((mx - 1) * (my - 1) * (mz - 1));
  for (int i = 0; i < (mx - 1) * (my - 1) * (mz - 1); i++)
  {
    shapes.push_back(vtkm::CELL_SHAPE_HEXAHEDRON);
    numberofInd.push_back(8);
  }

  points.reserve(mz * my * mx);
  for (int z = 0; z < mz; z++)
  {
    for (int y = 0; y < my; y++)
    {
      for (int x = 0; x < mx; x++)
      {
        vtkm::Vec3f_32 point(static_cast<vtkm::Float32>(x),
                             static_cast<vtkm::Float32>(y),
                             static_cast<vtkm::Float32>(z));
        points.push_back(point);
      }
    }
  }
  vtkm::cont::DataSet ds = dsb.Create(points, shapes, numberofInd, connections);
  ds.AddField(vtkm::cont::Field("scatter_pos", vtkm::cont::Field::Association::CELL_SET, offsets));
  ds.AddField(vtkm::cont::Field("scatter_len", vtkm::cont::Field::Association::CELL_SET, lengths));
  ds.AddField(vtkm::cont::Field("scatter_ids", vtkm::cont::Field::Association::WHOLE_MESH, ids));
  ds.AddField(vtkm::cont::Field("scatter_vfs", vtkm::cont::Field::Association::WHOLE_MESH, vfs));

  return ds;
}

void TestMIR()
{
  vtkm::cont::DataSet ds = GetTestDataSet();

  vtkm::filter::MIRFilter mir;
  mir.SetIDWholeSetName("scatter_ids");
  mir.SetPositionCellSetName("scatter_pos");
  mir.SetLengthCellSetName("scatter_len");
  mir.SetVFWholeSetName("scatter_vfs");

  mir.SetErrorScaling(vtkm::Float64(0.2));
  mir.SetScalingDecay(vtkm::Float64(1.0));
  mir.SetMaxIterations(vtkm::IdComponent(0)); // =0 -> No iterations..
  mir.SetMaxPercentError(vtkm::Float64(
    0.00001)); // Only useful for iterations >= 1, will stop iterating if total % error for entire mesh is less than this value
  // Note it is mathematically impossible to obtain 0% error outside of VERY special cases (neglecting float error)
  VTKM_LOG_S(vtkm::cont::LogLevel::Warn, "Before executing filter");

  vtkm::cont::DataSet ds_from_mir = mir.Execute(ds);

  VTKM_LOG_S(vtkm::cont::LogLevel::Warn, "After executing filter");

  // Test if ds_from_mir has 40 cells
  VTKM_TEST_ASSERT(ds_from_mir.GetNumberOfCells() == 40, "Wrong number of output cells");
}

int UnitTestMIRFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestMIR, argc, argv);
}
