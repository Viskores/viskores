//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/DispatcherCellNeighborhood.h>
#include <vtkm/worklet/WorkletCellNeighborhood.h>

#include <vtkm/worklet/ScatterIdentity.h>
#include <vtkm/worklet/ScatterUniform.h>

#include <vtkm/Math.h>

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace test_cellneighborhood
{

struct MaxNeighborValue : public vtkm::worklet::WorkletCellNeighborhood
{

  using ControlSignature = void(FieldInNeighborhood neighbors, CellSetIn, FieldOut maxV);

  using ExecutionSignature = void(Boundary, _1, _3);
  //verify input domain can be something other than first parameter
  using InputDomain = _2;

  template <typename FieldIn, typename FieldOut>
  VTKM_EXEC void operator()(const vtkm::exec::BoundaryState& boundary,
                            const vtkm::exec::FieldNeighborhood<FieldIn>& inputField,
                            FieldOut& output) const
  {
    using ValueType = typename FieldIn::ValueType;

    auto* nboundary = inputField.Boundary;

    if (!(nboundary->IsRadiusInXBoundary(1) == boundary.IsRadiusInXBoundary(1)))
    {
      this->RaiseError("Got invalid XPos boundary state");
    }

    if (!(nboundary->IsRadiusInYBoundary(1) == boundary.IsRadiusInYBoundary(1)))
    {
      this->RaiseError("Got invalid YPos boundary state");
    }

    if (!(nboundary->IsRadiusInZBoundary(1) == boundary.IsRadiusInZBoundary(1)))
    {
      this->RaiseError("Got invalid ZPos boundary state");
    }

    if (!(nboundary->IsRadiusInBoundary(1) == boundary.IsRadiusInBoundary(1)))
    {
      this->RaiseError("Got invalid boundary state");
    }

    if (nboundary->IsRadiusInXBoundary(1) !=
        (boundary.IsNeighborInXBoundary(-1) && boundary.IsNeighborInXBoundary(1)))
    {
      this->RaiseError("Neighbor/Radius boundary mismatch in X dimension.");
    }

    if (nboundary->IsRadiusInYBoundary(1) !=
        (boundary.IsNeighborInYBoundary(-1) && boundary.IsNeighborInYBoundary(1)))
    {
      this->RaiseError("Neighbor/Radius boundary mismatch in Y dimension.");
    }

    if (nboundary->IsRadiusInZBoundary(1) !=
        (boundary.IsNeighborInZBoundary(-1) && boundary.IsNeighborInZBoundary(1)))
    {
      this->RaiseError("Neighbor/Radius boundary mismatch in Z dimension.");
    }

    if (nboundary->IsRadiusInBoundary(1) !=
        (boundary.IsNeighborInBoundary({ -1 }) && boundary.IsNeighborInBoundary({ 1 })))
    {
      this->RaiseError("Neighbor/Radius boundary mismatch.");
    }


    auto minNeighbors = boundary.MinNeighborIndices(1);
    auto maxNeighbors = boundary.MaxNeighborIndices(1);

    ValueType maxV = inputField.Get(0, 0, 0); //our value
    for (vtkm::IdComponent k = minNeighbors[2]; k <= maxNeighbors[2]; ++k)
    {
      for (vtkm::IdComponent j = minNeighbors[1]; j <= maxNeighbors[1]; ++j)
      {
        for (vtkm::IdComponent i = minNeighbors[0]; i <= maxNeighbors[0]; ++i)
        {
          maxV = vtkm::Max(maxV, inputField.Get(i, j, k));
        }
      }
    }
    output = static_cast<FieldOut>(maxV);
  }
};

struct ScatterIdentityNeighbor : public vtkm::worklet::WorkletCellNeighborhood
{
  using ControlSignature = void(CellSetIn topology);
  using ExecutionSignature = void(WorkIndex, InputIndex, OutputIndex, ThreadIndices, VisitIndex);

  VTKM_CONT
  ScatterIdentityNeighbor() {}

  VTKM_EXEC void operator()(
    const vtkm::Id& workIndex,
    const vtkm::Id& inputIndex,
    const vtkm::Id& outputIndex,
    const vtkm::exec::arg::ThreadIndicesCellNeighborhood& vtkmNotUsed(threadIndices),
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
      this->RaiseError("Got wrong visit value1.");
    }
  }


  using ScatterType = vtkm::worklet::ScatterIdentity;
};

struct ScatterUniformNeighbor : public vtkm::worklet::WorkletCellNeighborhood
{
  using ControlSignature = void(CellSetIn topology);
  using ExecutionSignature = void(WorkIndex, InputIndex, OutputIndex, ThreadIndices, VisitIndex);

  VTKM_CONT
  ScatterUniformNeighbor() {}

  VTKM_EXEC void operator()(
    const vtkm::Id& workIndex,
    const vtkm::Id& inputIndex,
    const vtkm::Id& outputIndex,
    const vtkm::exec::arg::ThreadIndicesCellNeighborhood& vtkmNotUsed(threadIndices),
    const vtkm::Id& visitIndex) const
  {
    if ((workIndex / 3) != inputIndex)
    {
      this->RaiseError("Got wrong input value.");
    }
    if (outputIndex != workIndex)
    {
      this->RaiseError("Got work and output index don't match.");
    }
    if ((workIndex % 3) != visitIndex)
    {
      this->RaiseError("Got wrong visit value2.");
    }
  }


  using ScatterType = vtkm::worklet::ScatterUniform<3>;
};

// An example of using WorkletCellNeighborhood to iterate over a structured 3D cell
// domain rather than look at an actual neighborhood. It reduces a domain by subsampling
// every other item in the input field.
struct Subsample : public vtkm::worklet::WorkletCellNeighborhood
{
  using ControlSignature =
    void(WholeCellSetIn<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint> inputTopology,
         WholeArrayIn inputField,
         CellSetIn outputTopology,
         FieldOut sampledField);
  using ExecutionSignature = void(_1, _2, Boundary, _4);
  using InputDomain = _3;

  template <typename InFieldPortal, typename T>
  VTKM_EXEC void operator()(const vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell,
                                                                     vtkm::TopologyElementTagPoint,
                                                                     3>& inputTopology,
                            const InFieldPortal& inFieldPortal,
                            const vtkm::exec::BoundaryState& boundary,
                            T& sample) const
  {
    sample =
      inFieldPortal.Get(inputTopology.LogicalToFlatVisitIndex(2 * boundary.GetCenterIndex()));
  }
};

} // namespace test_cellneighborhood

namespace
{

static void TestMaxNeighborValue();
static void TestScatterIdentityNeighbor();
static void TestScatterUnfiormNeighbor();
static void TestIndexing();

void TestWorkletCellNeighborhood(vtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Cell Neighborhood Worklet on device adapter: " << id.GetName() << std::endl;
  vtkm::cont::ScopedRuntimeDeviceTracker deviceScope(id);

  TestMaxNeighborValue();
  TestScatterIdentityNeighbor();
  TestScatterUnfiormNeighbor();
  TestIndexing();
}

static void TestMaxNeighborValue()
{
  std::cout << "Testing MaxNeighborValue worklet" << std::endl;


  vtkm::cont::testing::MakeTestDataSet testDataSet;

  vtkm::worklet::DispatcherCellNeighborhood<::test_cellneighborhood::MaxNeighborValue> dispatcher;

  vtkm::cont::ArrayHandle<vtkm::Float32> output;

  vtkm::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();
  dispatcher.Invoke(dataSet3D.GetField("cellvar")
                      .GetData()
                      .ResetTypes<vtkm::TypeListFieldScalar, VTKM_DEFAULT_STORAGE_LIST>(),
                    dataSet3D.GetCellSet(),
                    output);

  vtkm::Float32 expected3D[4] = { 100.4f, 100.4f, 100.4f, 100.4f };
  for (int i = 0; i < 4; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(output.ReadPortal().Get(i), expected3D[i]),
                     "Wrong result for MaxNeighborValue worklet");
  }

  vtkm::cont::DataSet dataSet2D = testDataSet.Make2DUniformDataSet1();
  dispatcher.Invoke(dataSet2D.GetField("cellvar")
                      .GetData()
                      .ResetTypes<vtkm::TypeListFieldScalar, VTKM_DEFAULT_STORAGE_LIST>(),
                    dataSet2D.GetCellSet(),
                    output);

  vtkm::Float32 expected2D[16] = { 5.0f,  6.0f,  7.0f,  7.0f,  9.0f,  10.0f, 11.0f, 11.0f,
                                   13.0f, 14.0f, 15.0f, 15.0f, 13.0f, 14.0f, 15.0f, 15.0f };

  for (int i = 0; i < 16; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(output.ReadPortal().Get(i), expected2D[i]),
                     "Wrong result for MaxNeighborValue worklet");
  }
}

static void TestScatterIdentityNeighbor()
{
  std::cout << "Testing identity scatter with CellNeighborhood" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;

  vtkm::worklet::DispatcherCellNeighborhood<::test_cellneighborhood::ScatterIdentityNeighbor>
    dispatcher;

  vtkm::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();
  dispatcher.Invoke(dataSet3D.GetCellSet());

  vtkm::cont::DataSet dataSet2D = testDataSet.Make2DUniformDataSet0();
  dispatcher.Invoke(dataSet2D.GetCellSet());
}


static void TestScatterUnfiormNeighbor()
{
  std::cout << "Testing uniform scatter with CellNeighborhood" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;

  vtkm::worklet::DispatcherCellNeighborhood<::test_cellneighborhood::ScatterUniformNeighbor>
    dispatcher;

  vtkm::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();
  dispatcher.Invoke(dataSet3D.GetCellSet());

  vtkm::cont::DataSet dataSet2D = testDataSet.Make2DUniformDataSet0();
  dispatcher.Invoke(dataSet2D.GetCellSet());
}

static void TestIndexing()
{
  std::cout << "Testing using CellNeighborhood for 3D indexing." << std::endl;

  constexpr vtkm::Id outDim = 4;
  constexpr vtkm::Id inDim = outDim * 2;

  vtkm::cont::CellSetStructured<3> inCellSet;
  inCellSet.SetPointDimensions({ inDim + 1 });
  vtkm::cont::CellSetStructured<3> outCellSet;
  outCellSet.SetPointDimensions({ outDim + 1 });

  vtkm::cont::ArrayHandleUniformPointCoordinates inField(vtkm::Id3{ inDim });

  vtkm::cont::ArrayHandle<vtkm::Vec3f> outField;

  vtkm::cont::Invoker invoke;
  invoke(::test_cellneighborhood::Subsample{}, inCellSet, inField, outCellSet, outField);

  VTKM_TEST_ASSERT(outField.GetNumberOfValues() == (outDim * outDim * outDim));

  vtkm::Id flatIndex = 0;
  vtkm::Id3 IJK;
  auto outFieldPortal = outField.WritePortal();
  for (IJK[2] = 0; IJK[2] < outDim; ++IJK[2])
  {
    for (IJK[1] = 0; IJK[1] < outDim; ++IJK[1])
    {
      for (IJK[0] = 0; IJK[0] < outDim; ++IJK[0])
      {
        vtkm::Vec3f computed = outFieldPortal.Get(flatIndex);
        VTKM_TEST_ASSERT(test_equal(computed, 2 * IJK));
        ++flatIndex;
      }
    }
  }
}

} // anonymous namespace

int UnitTestWorkletCellNeighborhood(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::RunOnDevice(TestWorkletCellNeighborhood, argc, argv);
}
