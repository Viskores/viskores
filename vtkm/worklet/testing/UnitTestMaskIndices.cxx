//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/worklet/MaskIndices.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <ctime>
#include <random>

namespace
{

class Worklet : public vtkm::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldInOutPoint outPointId);
  using ExecutionSignature = void(InputIndex, _2);
  using InputDomain = _1;

  using MaskType = vtkm::worklet::MaskIndices;

  VTKM_EXEC void operator()(vtkm::Id pointId, vtkm::Id& outPointId) const { outPointId = pointId; }
};

template <typename CellSetType>
void RunTest(const CellSetType& cellset, const vtkm::cont::ArrayHandle<vtkm::Id>& indices)
{
  vtkm::Id numPoints = cellset.GetNumberOfPoints();
  vtkm::cont::ArrayHandle<vtkm::Id> outPointId;
  vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleConstant<vtkm::Id>(-1, numPoints), outPointId);

  vtkm::worklet::DispatcherMapTopology<Worklet> dispatcher(vtkm::worklet::MaskIndices{ indices });
  dispatcher.Invoke(cellset, outPointId);

  vtkm::cont::ArrayHandle<vtkm::Int8> stencil;
  vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleConstant<vtkm::Int8>(0, numPoints), stencil);

  // Check that output that should be written was.
  for (vtkm::Id i = 0; i < indices.GetNumberOfValues(); ++i)
  {
    // All unmasked indices should have been copied to the output.
    vtkm::Id unmaskedIndex = indices.GetPortalConstControl().Get(i);
    vtkm::Id writtenValue = outPointId.GetPortalConstControl().Get(unmaskedIndex);
    VTKM_TEST_ASSERT(unmaskedIndex == writtenValue,
                     "Did not pass unmasked index. Expected ",
                     unmaskedIndex,
                     ". Got ",
                     writtenValue);

    // Mark index as passed.
    stencil.GetPortalControl().Set(unmaskedIndex, 1);
  }

  // Check that output that should not be written was not.
  for (vtkm::Id i = 0; i < numPoints; ++i)
  {
    if (stencil.GetPortalConstControl().Get(i) == 0)
    {
      vtkm::Id foundValue = outPointId.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(foundValue == -1,
                       "Expected index ",
                       i,
                       " to be unwritten but was filled with ",
                       foundValue);
    }
  }
}

void TestMaskIndices()
{
  vtkm::cont::DataSet dataset = vtkm::cont::testing::MakeTestDataSet().Make2DUniformDataSet0();
  auto cellset = dataset.GetCellSet();
  vtkm::Id numberOfPoints = cellset.GetNumberOfPoints();

  vtkm::UInt32 seed = static_cast<vtkm::UInt32>(std::time(nullptr));

  std::default_random_engine generator;
  generator.seed(seed);
  std::uniform_int_distribution<vtkm::Id> countDistribution(1, 2 * numberOfPoints);
  std::uniform_int_distribution<vtkm::Id> ptidDistribution(0, numberOfPoints - 1);

  const int iterations = 5;
  std::cout << "Testing with random indices " << iterations << " times\n";
  std::cout << "Seed: " << seed << std::endl;
  for (int i = 1; i <= iterations; ++i)
  {
    std::cout << "iteration: " << i << "\n";

    vtkm::Id count = countDistribution(generator);
    vtkm::cont::ArrayHandle<vtkm::Id> indices;
    indices.Allocate(count);

    // Note that it is possible that the same index will be written twice, which is generally
    // a bad idea with MaskIndices. However, the worklet will write the same value for each
    // instance, so we should still get the correct result.
    auto portal = indices.GetPortalControl();
    std::cout << "using indices:";
    for (vtkm::Id j = 0; j < count; ++j)
    {
      auto val = ptidDistribution(generator);
      std::cout << " " << val;
      portal.Set(j, val);
    }
    std::cout << "\n";

    RunTest(cellset, indices);
  }
}

} // anonymous namespace

int UnitTestMaskIndices(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestMaskIndices, argc, argv);
}
