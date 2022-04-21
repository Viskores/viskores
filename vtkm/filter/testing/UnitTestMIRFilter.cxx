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
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/MIRFilter.h>

#include <vtkm/io/VTKDataSetReader.h>

#include <stdio.h>

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
  std::vector<vtkm::Vec3f> points;

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
  std::vector<vtkm::FloatDefault> vfAR{ 1, 1, 1, 1, 1, 1, 1, 1 };
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> vfs =
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
  ds.AddField(vtkm::cont::Field("scatter_pos", vtkm::cont::Field::Association::Cells, offsets));
  ds.AddField(vtkm::cont::Field("scatter_len", vtkm::cont::Field::Association::Cells, lengths));
  ds.AddField(vtkm::cont::Field("scatter_ids", vtkm::cont::Field::Association::WholeMesh, ids));
  ds.AddField(vtkm::cont::Field("scatter_vfs", vtkm::cont::Field::Association::WholeMesh, vfs));

  return ds;
}

class MetaDataLength : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);

  VTKM_EXEC
  void operator()(const vtkm::FloatDefault& background,
                  const vtkm::FloatDefault& circle_a,
                  const vtkm::FloatDefault& circle_b,
                  const vtkm::FloatDefault& circle_c,
                  vtkm::Id& length) const
  {
    length = 0;
    if (background > vtkm::FloatDefault(0.0))
      length++;
    if (circle_a > vtkm::FloatDefault(0.0))
      length++;
    if (circle_b > vtkm::FloatDefault(0.0))
      length++;
    if (circle_c > vtkm::FloatDefault(0.0))
      length++;
  }
};

class MetaDataPopulate : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature =
    void(FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayOut, WholeArrayOut);

  template <typename IdArray, typename DataArray>
  VTKM_EXEC void operator()(const vtkm::Id& offset,
                            const vtkm::FloatDefault& background,
                            const vtkm::FloatDefault& circle_a,
                            const vtkm::FloatDefault& circle_b,
                            const vtkm::FloatDefault& circle_c,
                            IdArray& matIds,
                            DataArray& matVFs) const
  {
    vtkm::Id index = offset;
    if (background > vtkm::FloatDefault(0.0))
    {
      matIds.Set(index, 1);
      matVFs.Set(index, background);
      index++;
    }
    if (circle_a > vtkm::FloatDefault(0.0))
    {
      matIds.Set(index, 2);
      matVFs.Set(index, circle_a);
      index++;
    }
    if (circle_b > vtkm::FloatDefault(0.0))
    {
      matIds.Set(index, 3);
      matVFs.Set(index, circle_b);
      index++;
    }
    if (circle_c > vtkm::FloatDefault(0.0))
    {
      matIds.Set(index, 4);
      matVFs.Set(index, circle_c);
      index++;
    }
  }
};

void TestMIRVenn250()
{
  using IdArray = vtkm::cont::ArrayHandle<vtkm::Id>;
  using DataArray = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  vtkm::cont::Invoker invoker;

  std::string vennFile = vtkm::cont::testing::Testing::DataPath("uniform/venn250.vtk");
  vtkm::io::VTKDataSetReader reader(vennFile);
  vtkm::cont::DataSet data = reader.ReadDataSet();

  DataArray backArr;
  data.GetField("mesh_topo/background").GetDataAsDefaultFloat().AsArrayHandle(backArr);
  DataArray cirAArr;
  data.GetField("mesh_topo/circle_a").GetDataAsDefaultFloat().AsArrayHandle(cirAArr);
  DataArray cirBArr;
  data.GetField("mesh_topo/circle_b").GetDataAsDefaultFloat().AsArrayHandle(cirBArr);
  DataArray cirCArr;
  data.GetField("mesh_topo/circle_c").GetDataAsDefaultFloat().AsArrayHandle(cirCArr);

  IdArray length;
  IdArray offset;
  IdArray matIds;
  DataArray matVFs;
  invoker(MetaDataLength{}, backArr, cirAArr, cirBArr, cirCArr, length);
  vtkm::cont::Algorithm::ScanExclusive(length, offset);

  vtkm::Id total = vtkm::cont::Algorithm::Reduce(length, vtkm::Id(0));
  matIds.Allocate(total);
  matVFs.Allocate(total);

  invoker(MetaDataPopulate{}, offset, backArr, cirAArr, cirBArr, cirCArr, matIds, matVFs);

  data.AddField(vtkm::cont::Field("scatter_pos", vtkm::cont::Field::Association::Cells, offset));
  data.AddField(vtkm::cont::Field("scatter_len", vtkm::cont::Field::Association::Cells, length));
  data.AddField(
    vtkm::cont::Field("scatter_ids", vtkm::cont::Field::Association::WholeMesh, matIds));
  data.AddField(
    vtkm::cont::Field("scatter_vfs", vtkm::cont::Field::Association::WholeMesh, matVFs));

  vtkm::filter::MIRFilter mir;
  mir.SetIDWholeSetName("scatter_ids");
  mir.SetPositionCellSetName("scatter_pos");
  mir.SetLengthCellSetName("scatter_len");
  mir.SetVFWholeSetName("scatter_vfs");
  mir.SetErrorScaling(vtkm::Float64(0.2));
  mir.SetScalingDecay(vtkm::Float64(1.0));
  mir.SetMaxIterations(vtkm::IdComponent(0)); // =0 -> No iterations..
  // Only useful for iterations >= 1, will stop iterating if total % error for entire mesh is less than this value
  // Note it is mathematically impossible to obtain 0% error outside of VERY special cases (neglecting float error)
  mir.SetMaxPercentError(vtkm::Float64(0.00001));

  VTKM_LOG_S(vtkm::cont::LogLevel::Warn, "Before executing filter w/ Venn data");

  vtkm::cont::DataSet fromMIR = mir.Execute(data);

  VTKM_LOG_S(vtkm::cont::LogLevel::Warn, "After executing filter w/ Venn data");

  VTKM_TEST_ASSERT(fromMIR.GetNumberOfCells() == 66086, "Wrong number of output cells");
}

void TestMIRSynthetic()
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

void TestMIR()
{
  TestMIRSynthetic();
  TestMIRVenn250();
}

int UnitTestMIRFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestMIR, argc, argv);
}
