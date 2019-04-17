//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/ScatterCounting.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/ErrorBadValue.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <sstream>

namespace
{

VTKM_CONT
inline vtkm::cont::ArrayHandleConcatenate<
  vtkm::cont::ArrayHandleConstant<vtkm::Id>,
  vtkm::cont::ArrayHandleView<vtkm::cont::ArrayHandle<vtkm::Id>>>
ShiftArrayHandleByOne(const vtkm::cont::ArrayHandle<vtkm::Id>& array)
{
  return vtkm::cont::make_ArrayHandleConcatenate(
    vtkm::cont::make_ArrayHandleConstant<vtkm::Id>(0, 1),
    vtkm::cont::make_ArrayHandleView(array, 0, array.GetNumberOfValues() - 1));
}

struct ReverseInputToOutputMapWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn outputStartIndices,
                                FieldIn outputEndIndices,
                                WholeArrayOut outputToInputMap,
                                WholeArrayOut visit);
  using ExecutionSignature = void(_1, _2, _3, _4, InputIndex);
  using InputDomain = _2;

  template <typename OutputMapType, typename VisitType>
  VTKM_EXEC void operator()(vtkm::Id outputStartIndex,
                            vtkm::Id outputEndIndex,
                            const OutputMapType& outputToInputMap,
                            const VisitType& visit,
                            vtkm::Id inputIndex) const
  {
    vtkm::IdComponent visitIndex = 0;
    for (vtkm::Id outputIndex = outputStartIndex; outputIndex < outputEndIndex; outputIndex++)
    {
      outputToInputMap.Set(outputIndex, inputIndex);
      visit.Set(outputIndex, visitIndex);
      visitIndex++;
    }
  }

  VTKM_CONT
  static void Run(const vtkm::cont::ArrayHandle<vtkm::Id>& inputToOutputMap,
                  const vtkm::cont::ArrayHandle<vtkm::Id>& outputToInputMap,
                  const vtkm::cont::ArrayHandle<vtkm::IdComponent>& visit,
                  vtkm::cont::DeviceAdapterId device)
  {
    vtkm::worklet::DispatcherMapField<ReverseInputToOutputMapWorklet> dispatcher;
    dispatcher.SetDevice(device);
    dispatcher.Invoke(
      ShiftArrayHandleByOne(inputToOutputMap), inputToOutputMap, outputToInputMap, visit);
  }
};

struct SubtractToVisitIndexWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn startsOfGroup, WholeArrayOut visit);
  using ExecutionSignature = void(InputIndex, _1, _2);
  using InputDomain = _1;

  template <typename VisitType>
  VTKM_EXEC void operator()(vtkm::Id inputIndex,
                            vtkm::Id startOfGroup,
                            const VisitType& visit) const
  {
    vtkm::IdComponent visitIndex = static_cast<vtkm::IdComponent>(inputIndex - startOfGroup);
    visit.Set(inputIndex, visitIndex);
  }
};

} // anonymous namespace

namespace vtkm
{
namespace worklet
{
namespace detail
{

struct ScatterCountingBuilder
{
  template <typename CountArrayType>
  VTKM_CONT static void BuildArrays(vtkm::worklet::ScatterCounting* self,
                                    const CountArrayType& countArray,
                                    vtkm::cont::DeviceAdapterId device,
                                    bool saveInputToOutputMap)
  {
    VTKM_IS_ARRAY_HANDLE(CountArrayType);

    self->InputRange = countArray.GetNumberOfValues();

    // The input to output map is actually built off by one. The first entry
    // is actually for the second value. The last entry is the total number of
    // output. This off-by-one is so that an upper bound find will work when
    // building the output to input map. Later we will either correct the
    // map or delete it.
    vtkm::cont::ArrayHandle<vtkm::Id> inputToOutputMapOffByOne;
    vtkm::Id outputSize = vtkm::cont::Algorithm::ScanInclusive(
      device, vtkm::cont::make_ArrayHandleCast(countArray, vtkm::Id()), inputToOutputMapOffByOne);

    // We have implemented two different ways to compute the output to input
    // map. The first way is to use a binary search on each output index into
    // the input map. The second way is to schedule on each input and
    // iteratively fill all the output indices for that input. The first way is
    // faster for output sizes that are small relative to the input (typical in
    // Marching Cubes, for example) and also tends to be well load balanced.
    // The second way is faster for larger outputs (typical in triangulation,
    // for example). We will use the first method for small output sizes and
    // the second for large output sizes. Toying with this might be a good
    // place for optimization.
    if (outputSize < self->InputRange)
    {
      BuildOutputToInputMapWithFind(self, outputSize, device, inputToOutputMapOffByOne);
    }
    else
    {
      BuildOutputToInputMapWithIterate(self, outputSize, device, inputToOutputMapOffByOne);
    }

    if (saveInputToOutputMap)
    {
      // Since we are saving it, correct the input to output map.
      vtkm::cont::Algorithm::Copy(
        device, ShiftArrayHandleByOne(inputToOutputMapOffByOne), self->InputToOutputMap);
    }
  }

  VTKM_CONT static void BuildOutputToInputMapWithFind(
    vtkm::worklet::ScatterCounting* self,
    vtkm::Id outputSize,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::ArrayHandle<vtkm::Id> inputToOutputMapOffByOne)
  {
    vtkm::cont::ArrayHandleIndex outputIndices(outputSize);
    vtkm::cont::Algorithm::UpperBounds(
      device, inputToOutputMapOffByOne, outputIndices, self->OutputToInputMap);

    vtkm::cont::ArrayHandle<vtkm::Id> startsOfGroups;

    // This find gives the index of the start of a group.
    vtkm::cont::Algorithm::LowerBounds(
      device, self->OutputToInputMap, self->OutputToInputMap, startsOfGroups);

    self->VisitArray.Allocate(outputSize);
    vtkm::worklet::DispatcherMapField<SubtractToVisitIndexWorklet> dispatcher;
    dispatcher.SetDevice(device);
    dispatcher.Invoke(startsOfGroups, self->VisitArray);
  }

  VTKM_CONT static void BuildOutputToInputMapWithIterate(
    vtkm::worklet::ScatterCounting* self,
    vtkm::Id outputSize,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::ArrayHandle<vtkm::Id> inputToOutputMapOffByOne)
  {
    self->OutputToInputMap.Allocate(outputSize);
    self->VisitArray.Allocate(outputSize);

    ReverseInputToOutputMapWorklet::Run(
      inputToOutputMapOffByOne, self->OutputToInputMap, self->VisitArray, device);
  }

  template <typename ArrayType>
  void operator()(const ArrayType& countArray,
                  vtkm::cont::DeviceAdapterId device,
                  bool saveInputToOutputMap,
                  vtkm::worklet::ScatterCounting* self)
  {
    BuildArrays(self, countArray, device, saveInputToOutputMap);
  }
};
}
}
} // namespace vtkm::worklet::detail

void vtkm::worklet::ScatterCounting::BuildArrays(const VariantArrayHandleCount& countArray,
                                                 vtkm::cont::DeviceAdapterId device,
                                                 bool saveInputToOutputMap)
{
  countArray.CastAndCall(
    vtkm::worklet::detail::ScatterCountingBuilder(), device, saveInputToOutputMap, this);
}
