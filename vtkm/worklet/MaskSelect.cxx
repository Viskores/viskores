//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/MaskSelect.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleView.h>

namespace
{

struct ReverseOutputToThreadMap : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn outputToThreadMap,
                                FieldIn maskArray,
                                WholeArrayOut threadToOutputMap);
  using ExecutionSignature = void(_1, InputIndex, _2, _3);

  template <typename MaskType, typename ThreadToOutputPortal>
  VTKM_EXEC void operator()(vtkm::Id threadIndex,
                            vtkm::Id outputIndex,
                            MaskType mask,
                            ThreadToOutputPortal threadToOutput) const
  {
    if (mask)
    {
      threadToOutput.Set(threadIndex, outputIndex);
    }
  }
};

VTKM_CONT static vtkm::worklet::MaskSelect::ThreadToOutputMapType BuildThreadToOutputMapWithFind(
  vtkm::Id numThreads,
  vtkm::cont::ArrayHandle<vtkm::Id> outputToThreadMap,
  vtkm::cont::DeviceAdapterId device)
{
  vtkm::worklet::MaskSelect::ThreadToOutputMapType threadToOutputMap;

  vtkm::Id outputSize = outputToThreadMap.GetNumberOfValues();

  vtkm::cont::ArrayHandleIndex threadIndices(numThreads);
  vtkm::cont::Algorithm::UpperBounds(
    device,
    vtkm::cont::make_ArrayHandleView(outputToThreadMap, 1, outputSize - 1),
    threadIndices,
    threadToOutputMap);

  return threadToOutputMap;
}

template <typename MaskArrayType>
VTKM_CONT static vtkm::worklet::MaskSelect::ThreadToOutputMapType BuildThreadToOutputMapWithCopy(
  vtkm::Id numThreads,
  const vtkm::cont::ArrayHandle<vtkm::Id>& outputToThreadMap,
  const MaskArrayType& maskArray,
  vtkm::cont::DeviceAdapterId device)
{
  vtkm::worklet::MaskSelect::ThreadToOutputMapType threadToOutputMap;
  threadToOutputMap.Allocate(numThreads);

  vtkm::worklet::DispatcherMapField<ReverseOutputToThreadMap> dispatcher;
  dispatcher.SetDevice(device);
  dispatcher.Invoke(outputToThreadMap, maskArray, threadToOutputMap);

  return threadToOutputMap;
}

VTKM_CONT static vtkm::worklet::MaskSelect::ThreadToOutputMapType BuildThreadToOutputMapAllOn(
  vtkm::Id numThreads,
  vtkm::cont::DeviceAdapterId device)
{
  vtkm::worklet::MaskSelect::ThreadToOutputMapType threadToOutputMap;
  threadToOutputMap.Allocate(numThreads);
  vtkm::cont::Algorithm::Copy(
    device, vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(0, 1, numThreads), threadToOutputMap);
  return threadToOutputMap;
}

struct MaskBuilder
{
  template <typename ArrayHandleType>
  void operator()(const ArrayHandleType& maskArray,
                  vtkm::worklet::MaskSelect::ThreadToOutputMapType& threadToOutputMap,
                  vtkm::cont::DeviceAdapterId device)
  {
    vtkm::cont::ArrayHandle<vtkm::Id> outputToThreadMap;
    vtkm::Id numThreads = vtkm::cont::Algorithm::ScanExclusive(
      device, vtkm::cont::make_ArrayHandleCast<vtkm::Id>(maskArray), outputToThreadMap);
    VTKM_ASSERT(numThreads <= maskArray.GetNumberOfValues());

    // We have implemented two different ways to compute the thread to output map. The first way is
    // to use a binary search on each thread index into the output map. The second way is to
    // schedule on each output and copy the the index to the thread map. The first way is faster
    // for output sizes that are small relative to the input and also tends to be well load
    // balanced. The second way is faster for larger outputs.
    //
    // The former is obviously faster for one thread and the latter is obviously faster when all
    // outputs have a thread. We have to guess for values in the middle. I'm using if the square of
    // the number of threads is less than the number of outputs because it is easy to compute.
    if (numThreads == maskArray.GetNumberOfValues())
    { //fast path when everything is on
      threadToOutputMap = BuildThreadToOutputMapAllOn(numThreads, device);
    }
    else if ((numThreads * numThreads) < maskArray.GetNumberOfValues())
    {
      threadToOutputMap = BuildThreadToOutputMapWithFind(numThreads, outputToThreadMap, device);
    }
    else
    {
      threadToOutputMap =
        BuildThreadToOutputMapWithCopy(numThreads, outputToThreadMap, maskArray, device);
    }
  }
};

} // anonymous namespace

vtkm::worklet::MaskSelect::ThreadToOutputMapType vtkm::worklet::MaskSelect::Build(
  const VariantArrayHandleMask& maskArray,
  vtkm::cont::DeviceAdapterId device)
{
  vtkm::worklet::MaskSelect::ThreadToOutputMapType threadToOutputMap;
  maskArray.CastAndCall(MaskBuilder(), threadToOutputMap, device);
  return threadToOutputMap;
}
