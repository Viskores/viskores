//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayCopy.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/MaskSelect.h>
#include <vtkm/worklet/ScatterUniform.h>

namespace
{

class TestWorkletMapTopo : public vtkm::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn topology, FieldInVisit pointCoords);
  using ExecutionSignature = void(_2, WorkIndex, InputIndex, OutputIndex, VisitIndex);
};

class TestWorkletMapTopoIdentity : public TestWorkletMapTopo
{
public:
  using ScatterType = vtkm::worklet::ScatterIdentity;

  VTKM_EXEC void operator()(const vtkm::Vec<int, 3>& vtkmNotUsed(coords),
                            const vtkm::Id& workIndex,
                            const vtkm::Id& inputIndex,
                            const vtkm::Id& outputIndex,
                            const vtkm::Id& visitIndex) const
  {
    if (workIndex != inputIndex)
    {
      this->RaiseError("Got wrong input value.");
    }
    if (outputIndex != workIndex)
    {
      this->RaiseError("Got work and output index don't match.");
    }
    if (visitIndex != 0)
    {
      this->RaiseError("Got wrong visit value.");
    }
  }
};

class TestWorkletMapTopoUniform : public TestWorkletMapTopo
{
public:
  using ScatterType = vtkm::worklet::ScatterUniform<2>;

  VTKM_EXEC void operator()(const vtkm::Vec<int, 3>& vtkmNotUsed(coords),
                            const vtkm::Id& workIndex,
                            const vtkm::Id& inputIndex,
                            const vtkm::Id& outputIndex,
                            const vtkm::Id& visitIndex) const
  {
    if ((workIndex / 2) != inputIndex)
    {
      this->RaiseError("Got wrong input value.");
    }
    if (outputIndex != workIndex)
    {
      this->RaiseError("Got work and output index don't match.");
    }
    if ((workIndex % 2) != visitIndex)
    {
      this->RaiseError("Got wrong visit value.");
    }
  }
};

class TestWorkletMapTopoNone : public TestWorkletMapTopo
{
public:
  using MaskType = vtkm::worklet::MaskNone;

  VTKM_EXEC void operator()(const vtkm::Vec<int, 3>& vtkmNotUsed(coords),
                            const vtkm::Id& workIndex,
                            const vtkm::Id& inputIndex,
                            const vtkm::Id& outputIndex,
                            const vtkm::Id& visitIndex) const
  {
    if (workIndex != inputIndex)
    {
      this->RaiseError("Got wrong input value.");
    }
    if (outputIndex != workIndex)
    {
      this->RaiseError("Got work and output index don't match.");
    }
    if (visitIndex != 0)
    {
      this->RaiseError("Got wrong visit value.");
    }
  }
};

class TestWorkletMapTopoSelect : public TestWorkletMapTopo
{
public:
  using MaskType = vtkm::worklet::MaskSelect;

  VTKM_EXEC void operator()(const vtkm::Vec<int, 3>& vtkmNotUsed(coords),
                            const vtkm::Id& vtkmNotUsed(workIndex),
                            const vtkm::Id& vtkmNotUsed(inputIndex),
                            const vtkm::Id& vtkmNotUsed(outputIndex),
                            const vtkm::Id& vtkmNotUsed(visitIndex)) const
  {
    // This method should never be called
    this->RaiseError("An element was selected, this test selects none.");
  }
};

template <typename WorkletType>
struct DoTestWorklet
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    vtkm::cont::testing::MakeTestDataSet testDataSet;
    vtkm::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();

    vtkm::cont::CellSetStructured<3> cellSet =
      dataSet3D.GetCellSet().Cast<vtkm::cont::CellSetStructured<3>>();

    vtkm::cont::Invoker invoker;
    invoker(WorkletType{}, cellSet, dataSet3D.GetCoordinateSystem());
  }
};

template <>
struct DoTestWorklet<TestWorkletMapTopoSelect>
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    vtkm::cont::testing::MakeTestDataSet testDataSet;
    vtkm::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();

    // Start select array with an array of zeros
    auto selectArrayHandle = vtkm::cont::make_ArrayHandleMove(
      std::vector<vtkm::IdComponent>(static_cast<std::size_t>(dataSet3D.GetNumberOfPoints()), 0));

    vtkm::cont::CellSetStructured<3> cellSet =
      dataSet3D.GetCellSet().Cast<vtkm::cont::CellSetStructured<3>>();

    vtkm::cont::Invoker invoker;
    invoker(TestWorkletMapTopoSelect{},
            vtkm::worklet::MaskSelect(selectArrayHandle),
            cellSet,
            dataSet3D.GetCoordinateSystem());
  }
};

void TestWorkletMapField3d(vtkm::cont::DeviceAdapterId id)
{
  using HandleTypesToTest3D =
    vtkm::List<vtkm::Id, vtkm::Vec2i_32, vtkm::FloatDefault, vtkm::Vec3f_64>;

  using HandleTypesToTest1D =
    vtkm::List<vtkm::Int32, vtkm::Int64, vtkm::UInt32, vtkm::UInt64, vtkm::Int8, vtkm::UInt8, char>;

  std::cout << "Testing WorkletMapTopology with ScatterIdentity on device adapter: " << id.GetName()
            << std::endl;

  vtkm::testing::Testing::TryTypes(DoTestWorklet<TestWorkletMapTopoIdentity>(),
                                   HandleTypesToTest3D());

  std::cout << "Testing WorkletMapTopology with ScatterUniform on device adapter: " << id.GetName()
            << std::endl;

  vtkm::testing::Testing::TryTypes(DoTestWorklet<TestWorkletMapTopoUniform>(),
                                   HandleTypesToTest3D());

  std::cout << "Testing WorkletMapTopology with MaskNone on device adapter: " << id.GetName()
            << std::endl;

  vtkm::testing::Testing::TryTypes(DoTestWorklet<TestWorkletMapTopoNone>(), HandleTypesToTest3D());

  std::cout << "Testing WorkletMapTopology with MaskSelect on device adapter: " << id.GetName()
            << std::endl;

  vtkm::testing::Testing::TryTypes(DoTestWorklet<TestWorkletMapTopoSelect>(),
                                   HandleTypesToTest1D());
}

} //  namespace

int UnitTestScatterAndMaskWithTopology(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::RunOnDevice(TestWorkletMapField3d, argc, argv);
}
