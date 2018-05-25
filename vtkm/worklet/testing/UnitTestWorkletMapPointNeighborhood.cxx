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

#include <vtkm/worklet/DispatcherPointNeighborhood.h>
#include <vtkm/worklet/WorkletPointNeighborhood.h>

#include <vtkm/worklet/ScatterIdentity.h>
#include <vtkm/worklet/ScatterUniform.h>

#include <vtkm/Math.h>
#include <vtkm/VecAxisAlignedPointCoordinates.h>

#include <vtkm/cont/DataSet.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace test_pointneighborhood
{

struct MaxNeighborValue : public vtkm::worklet::WorkletPointNeighborhood3x3x3
{

  using ControlSignature = void(FieldInNeighborhood<Scalar> neighbors,
                                CellSetIn,
                                FieldOut<Scalar> maxV);

  using ExecutionSignature = void(OnBoundary, _1, _3);
  //verify input domain can be something other than first parameter
  using InputDomain = _2;

  template <typename FieldIn, typename FieldOut>
  VTKM_EXEC void operator()(const vtkm::exec::arg::BoundaryState& boundary,
                            const vtkm::exec::arg::Neighborhood<1, FieldIn>& inputField,
                            FieldOut& output) const
  {
    using ValueType = typename FieldIn::ValueType;

    auto* nboundary = inputField.Boundary;

    if (!(nboundary->OnXPositive() == boundary.OnXPositive()))
    {
      this->RaiseError("Got invalid XPos boundary state");
    }

    if (!(nboundary->OnXNegative() == boundary.OnXNegative()))
    {
      this->RaiseError("Got invalid XNeg boundary state");
    }

    if (!(nboundary->OnYPositive() == boundary.OnYPositive()))
    {
      this->RaiseError("Got invalid YPos boundary state");
    }

    if (!(nboundary->OnYNegative() == boundary.OnYNegative()))
    {
      this->RaiseError("Got invalid YNeg boundary state");
    }

    if (!(nboundary->OnZPositive() == boundary.OnZPositive()))
    {
      this->RaiseError("Got invalid ZPos boundary state");
    }

    if (!(nboundary->OnZNegative() == boundary.OnZNegative()))
    {
      this->RaiseError("Got invalid ZNeg boundary state");
    }


    if (!(nboundary->OnX() == boundary.OnX()))
    {
      this->RaiseError("Got invalid X boundary state");
    }
    if (!(nboundary->OnY() == boundary.OnY()))
    {
      this->RaiseError("Got invalid Y boundary state");
    }
    if (!(nboundary->OnZ() == boundary.OnZ()))
    {
      this->RaiseError("Got invalid Z boundary state");
    }


    ValueType maxV = inputField.Get(0, 0, 0); //our value
    for (vtkm::IdComponent k = 0; k < 3; ++k)
    {
      for (vtkm::IdComponent j = 0; j < 3; ++j)
      {
        maxV = vtkm::Max(maxV, inputField.Get(-1, j - 1, k - 1));
        maxV = vtkm::Max(maxV, inputField.Get(0, j - 1, k - 1));
        maxV = vtkm::Max(maxV, inputField.Get(1, j - 1, k - 1));
      }
    }
    output = static_cast<FieldOut>(maxV);
  }
};

struct ScatterIdentityNeighbor : public vtkm::worklet::WorkletPointNeighborhood5x5x5
{
  using ControlSignature = void(CellSetIn topology, FieldIn<Vec3> pointCoords);
  using ExecutionSignature =
    void(_2, WorkIndex, InputIndex, OutputIndex, ThreadIndices, VisitIndex);

  VTKM_CONT
  ScatterIdentityNeighbor() {}

  template <typename T>
  VTKM_EXEC void operator()(
    const vtkm::Vec<T, 3>& vtkmNotUsed(coords),
    const vtkm::Id& workIndex,
    const vtkm::Id& inputIndex,
    const vtkm::Id& outputIndex,
    const vtkm::exec::arg::ThreadIndicesPointNeighborhood<2>& vtkmNotUsed(threadIndices),
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

struct ScatterUniformNeighbor : public vtkm::worklet::WorkletPointNeighborhood5x5x5
{
  using ControlSignature = void(CellSetIn topology, FieldIn<Vec3> pointCoords);
  using ExecutionSignature =
    void(_2, WorkIndex, InputIndex, OutputIndex, ThreadIndices, VisitIndex);

  VTKM_CONT
  ScatterUniformNeighbor() {}

  template <typename T>
  VTKM_EXEC void operator()(
    const vtkm::Vec<T, 3>& vtkmNotUsed(coords),
    const vtkm::Id& workIndex,
    const vtkm::Id& inputIndex,
    const vtkm::Id& outputIndex,
    const vtkm::exec::arg::ThreadIndicesPointNeighborhood<2>& vtkmNotUsed(threadIndices),
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
}

namespace
{

static void TestMaxNeighborValue();
static void TestScatterIdentityNeighbor();
static void TestScatterUnfiormNeighbor();

void TestWorkletPointNeighborhood()
{
  using DeviceAdapterTraits = vtkm::cont::DeviceAdapterTraits<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>;
  std::cout << "Testing Point Neighborhood Worklet on device adapter: "
            << DeviceAdapterTraits::GetName() << std::endl;

  TestMaxNeighborValue();
  TestScatterIdentityNeighbor();
  TestScatterUnfiormNeighbor();
}

static void TestMaxNeighborValue()
{
  std::cout << "Testing MaxPointOfCell worklet" << std::endl;


  vtkm::cont::testing::MakeTestDataSet testDataSet;

  vtkm::worklet::DispatcherPointNeighborhood<::test_pointneighborhood::MaxNeighborValue> dispatcher;

  vtkm::cont::ArrayHandle<vtkm::Float32> output;

  vtkm::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();
  dispatcher.Invoke(dataSet3D.GetField("pointvar"), dataSet3D.GetCellSet(), output);

  vtkm::Float32 expected3D[18] = { 110.3f, 120.3f, 120.3f, 110.3f, 120.3f, 120.3f,
                                   170.5f, 180.5f, 180.5f, 170.5f, 180.5f, 180.5f,
                                   170.5f, 180.5f, 180.5f, 170.5f, 180.5f, 180.5f };
  for (int i = 0; i < 18; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(output.GetPortalConstControl().Get(i), expected3D[i]),
                     "Wrong result for MaxNeighborValue worklet");
  }

  vtkm::cont::DataSet dataSet2D = testDataSet.Make2DUniformDataSet1();
  dispatcher.Invoke(dataSet2D.GetField("pointvar"), dataSet2D.GetCellSet(), output);

  vtkm::Float32 expected2D[25] = { 100.0f, 100.0f, 78.0f, 49.0f, 33.0f, 100.0f, 100.0f,
                                   78.0f,  50.0f,  48.0f, 94.0f, 94.0f, 91.0f,  91.0f,
                                   91.0f,  52.0f,  52.0f, 91.0f, 91.0f, 91.0f,  12.0f,
                                   51.0f,  91.0f,  91.0f, 91.0f };

  for (int i = 0; i < 25; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(output.GetPortalConstControl().Get(i), expected2D[i]),
                     "Wrong result for MaxNeighborValue worklet");
  }
}

static void TestScatterIdentityNeighbor()
{
  std::cout << "Testing identity scatter with PointNeighborhood" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;

  vtkm::worklet::DispatcherPointNeighborhood<::test_pointneighborhood::ScatterIdentityNeighbor>
    dispatcher;

  vtkm::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();
  dispatcher.Invoke(dataSet3D.GetCellSet(), dataSet3D.GetCoordinateSystem());

  vtkm::cont::DataSet dataSet2D = testDataSet.Make2DUniformDataSet0();
  dispatcher.Invoke(dataSet2D.GetCellSet(), dataSet2D.GetCoordinateSystem());
}


static void TestScatterUnfiormNeighbor()
{
  std::cout << "Testing uniform scatter with PointNeighborhood" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;

  vtkm::worklet::DispatcherPointNeighborhood<::test_pointneighborhood::ScatterUniformNeighbor>
    dispatcher;

  vtkm::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();
  dispatcher.Invoke(dataSet3D.GetCellSet(), dataSet3D.GetCoordinateSystem());

  vtkm::cont::DataSet dataSet2D = testDataSet.Make2DUniformDataSet0();
  dispatcher.Invoke(dataSet2D.GetCellSet(), dataSet2D.GetCoordinateSystem());
}

} // anonymous namespace

int UnitTestWorkletMapPointNeighborhood(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestWorkletPointNeighborhood);
}
