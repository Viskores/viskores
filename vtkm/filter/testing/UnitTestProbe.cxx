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
#include <vtkm/filter/Probe.h>

#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/worklet/CellDeepCopy.h>

namespace
{

vtkm::cont::DataSet MakeInputDataSet()
{
  std::vector<vtkm::Float32> pvec(16), cvec(9);
  for (std::size_t i = 0; i < 16; ++i)
  {
    pvec[i] = static_cast<vtkm::Float32>(i) * 0.3f;
  }
  for (std::size_t i = 0; i < 9; ++i)
  {
    cvec[i] = static_cast<vtkm::Float32>(i) * 0.7f;
  }

  auto input = vtkm::cont::DataSetBuilderUniform::Create(
    vtkm::Id2(4, 4), vtkm::make_Vec(0.0f, 0.0f), vtkm::make_Vec(1.0f, 1.0f));
  vtkm::cont::DataSetFieldAdd::AddPointField(input, "pointdata", pvec);
  vtkm::cont::DataSetFieldAdd::AddCellField(input, "celldata", cvec);
  return input;
}

vtkm::cont::DataSet MakeGeometryDataSet()
{
  auto geometry = vtkm::cont::DataSetBuilderUniform::Create(
    vtkm::Id2(9, 9), vtkm::make_Vec(0.7f, 0.7f), vtkm::make_Vec(0.35f, 0.35f));
  return geometry;
}

struct ConvertImpl
{
  template <typename DeviceAdapter>
  bool operator()(DeviceAdapter device,
                  const vtkm::cont::DataSet& uds,
                  vtkm::cont::DataSet& eds) const
  {
    vtkm::cont::CellSetExplicit<> cs(uds.GetCellSet().GetName());
    vtkm::worklet::CellDeepCopy::Run(uds.GetCellSet(), cs, device);
    eds.AddCellSet(cs);

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> points;
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy(uds.GetCoordinateSystem().GetData(),
                                                            points);
    eds.AddCoordinateSystem(
      vtkm::cont::CoordinateSystem(uds.GetCoordinateSystem().GetName(), points));

    for (vtkm::IdComponent i = 0; i < uds.GetNumberOfFields(); ++i)
    {
      eds.AddField(uds.GetField(i));
    }

    return true;
  }
};

vtkm::cont::DataSet ConvertDataSetUniformToExplicit(const vtkm::cont::DataSet& uds)
{
  vtkm::cont::DataSet eds;
  vtkm::cont::TryExecute(ConvertImpl(), uds, eds);
  return eds;
}

const std::vector<vtkm::Float32>& GetExpectedPointData()
{
  static std::vector<vtkm::Float32> expected = {
    1.05f,  1.155f, 1.26f,  1.365f, 1.47f,  1.575f, 1.68f,  0.0f,   0.0f,   1.47f,  1.575f, 1.68f,
    1.785f, 1.89f,  1.995f, 2.1f,   0.0f,   0.0f,   1.89f,  1.995f, 2.1f,   2.205f, 2.31f,  2.415f,
    2.52f,  0.0f,   0.0f,   2.31f,  2.415f, 2.52f,  2.625f, 2.73f,  2.835f, 2.94f,  0.0f,   0.0f,
    2.73f,  2.835f, 2.94f,  3.045f, 3.15f,  3.255f, 3.36f,  0.0f,   0.0f,   3.15f,  3.255f, 3.36f,
    3.465f, 3.57f,  3.675f, 3.78f,  0.0f,   0.0f,   3.57f,  3.675f, 3.78f,  3.885f, 3.99f,  4.095f,
    4.2f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,
    0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f
  };
  return expected;
}

const std::vector<vtkm::Float32>& GetExpectedCellData()
{
  static std::vector<vtkm::Float32> expected = {
    0.0f, 0.7f, 0.7f, 0.7f, 1.4f, 1.4f, 1.4f, 0.0f, 0.0f, 2.1f, 2.8f, 2.8f, 2.8f, 3.5f,
    3.5f, 3.5f, 0.0f, 0.0f, 2.1f, 2.8f, 2.8f, 2.8f, 3.5f, 3.5f, 3.5f, 0.0f, 0.0f, 2.1f,
    2.8f, 2.8f, 2.8f, 3.5f, 3.5f, 3.5f, 0.0f, 0.0f, 4.2f, 4.9f, 4.9f, 4.9f, 5.6f, 5.6f,
    5.6f, 0.0f, 0.0f, 4.2f, 4.9f, 4.9f, 4.9f, 5.6f, 5.6f, 5.6f, 0.0f, 0.0f, 4.2f, 4.9f,
    4.9f, 4.9f, 5.6f, 5.6f, 5.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
  };
  return expected;
}

const std::vector<vtkm::UInt8>& GetExpectedHiddenPoints()
{
  static std::vector<vtkm::UInt8> expected = { 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2,
                                               2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0,
                                               2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0,
                                               0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2,
                                               2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
  return expected;
}

const std::vector<vtkm::UInt8>& GetExpectedHiddenCells()
{
  static std::vector<vtkm::UInt8> expected = { 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2,
                                               0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2,
                                               0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2,
                                               2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
  return expected;
}

template <typename T>
void TestResultArray(const vtkm::cont::ArrayHandle<T>& result, const std::vector<T>& expected)
{
  VTKM_TEST_ASSERT(result.GetNumberOfValues() == static_cast<vtkm::Id>(expected.size()),
                   "Incorrect field size");

  auto portal = result.GetPortalConstControl();
  vtkm::Id size = portal.GetNumberOfValues();
  for (vtkm::Id i = 0; i < size; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(portal.Get(i), expected[static_cast<std::size_t>(i)]),
                     "Incorrect field value");
  }
}

class TestProbe
{
private:
  using FieldArrayType = vtkm::cont::ArrayHandle<vtkm::Float32>;
  using HiddenArrayType = vtkm::cont::ArrayHandle<vtkm::UInt8>;

  static void ExplicitToUnifrom()
  {
    std::cout << "Testing Probe Explicit to Uniform:\n";

    auto input = ConvertDataSetUniformToExplicit(MakeInputDataSet());
    auto geometry = MakeGeometryDataSet();

    vtkm::filter::Probe probe;
    probe.SetGeometry(geometry);
    probe.SetFieldsToPass({ "pointdata", "celldata" });
    auto output = probe.Execute(input);

    TestResultArray(output.GetField("pointdata").GetData().template Cast<FieldArrayType>(),
                    GetExpectedPointData());
    TestResultArray(output.GetField("celldata").GetData().template Cast<FieldArrayType>(),
                    GetExpectedCellData());
    TestResultArray(output.GetPointField("HIDDEN").GetData().template Cast<HiddenArrayType>(),
                    GetExpectedHiddenPoints());
    TestResultArray(output.GetCellField("HIDDEN").GetData().template Cast<HiddenArrayType>(),
                    GetExpectedHiddenCells());
  }

  static void UniformToExplict()
  {
    std::cout << "Testing Probe Uniform to Explicit:\n";

    auto input = MakeInputDataSet();
    auto geometry = ConvertDataSetUniformToExplicit(MakeGeometryDataSet());

    vtkm::filter::Probe probe;
    probe.SetGeometry(geometry);
    probe.SetFieldsToPass({ "pointdata", "celldata" });
    auto output = probe.Execute(input);

    TestResultArray(output.GetField("pointdata").GetData().template Cast<FieldArrayType>(),
                    GetExpectedPointData());
    TestResultArray(output.GetField("celldata").GetData().template Cast<FieldArrayType>(),
                    GetExpectedCellData());
    TestResultArray(output.GetPointField("HIDDEN").GetData().template Cast<HiddenArrayType>(),
                    GetExpectedHiddenPoints());
    TestResultArray(output.GetCellField("HIDDEN").GetData().template Cast<HiddenArrayType>(),
                    GetExpectedHiddenCells());
  }

  static void ExplicitToExplict()
  {
    std::cout << "Testing Probe Explicit to Explicit:\n";

    auto input = ConvertDataSetUniformToExplicit(MakeInputDataSet());
    auto geometry = ConvertDataSetUniformToExplicit(MakeGeometryDataSet());

    vtkm::filter::Probe probe;
    probe.SetGeometry(geometry);
    probe.SetFieldsToPass({ "pointdata", "celldata" });
    auto output = probe.Execute(input);

    TestResultArray(output.GetField("pointdata").GetData().template Cast<FieldArrayType>(),
                    GetExpectedPointData());
    TestResultArray(output.GetField("celldata").GetData().template Cast<FieldArrayType>(),
                    GetExpectedCellData());
    TestResultArray(output.GetPointField("HIDDEN").GetData().template Cast<HiddenArrayType>(),
                    GetExpectedHiddenPoints());
    TestResultArray(output.GetCellField("HIDDEN").GetData().template Cast<HiddenArrayType>(),
                    GetExpectedHiddenCells());
  }

public:
  static void Run()
  {
    ExplicitToUnifrom();
    UniformToExplict();
    ExplicitToExplict();
  }
};

} // anonymous namespace

int UnitTestProbe(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestProbe::Run);
}
