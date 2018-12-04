//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_worklet_ScatterCounting_h
#define vtk_m_worklet_ScatterCounting_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
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

namespace vtkm
{
namespace worklet
{

namespace detail
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
  using ControlSignature = void(FieldIn<IdType> outputStartIndices,
                                FieldIn<IdType> outputEndIndices,
                                WholeArrayOut<IdType> outputToInputMap,
                                WholeArrayOut<IdComponentType> visit);
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
  using ControlSignature = void(FieldIn<IdType> startsOfGroup,
                                WholeArrayOut<IdComponentType> visit);
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

} // namespace detail

/// \brief A scatter that maps input to some numbers of output.
///
/// The \c Scatter classes are responsible for defining how much output is
/// generated based on some sized input. \c ScatterCounting establishes a 1 to
/// N mapping from input to output. That is, every input element generates 0 or
/// more output elements associated with it. The output elements are grouped by
/// the input associated.
///
/// A counting scatter takes an array of counts for each input. The data is
/// taken in the constructor and the index arrays are derived from that. So
/// changing the counts after the scatter is created will have no effect.
///
struct ScatterCounting
{
  /// Construct a \c ScatterCounting object using an array of counts for the
  /// number of outputs for each input. Part of the construction requires
  /// generating an input to output map, but this map is not needed for the
  /// operations of \c ScatterCounting, so by default it is deleted. However,
  /// other users might make use of it, so you can instruct the constructor
  /// to save the input to output map.
  ///
  template <typename CountArrayType>
  VTKM_CONT ScatterCounting(const CountArrayType& countArray,
                            vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny(),
                            bool saveInputToOutputMap = false)
  {
    this->BuildArrays(countArray, device, saveInputToOutputMap);
  }
  template <typename CountArrayType>
  VTKM_CONT ScatterCounting(const CountArrayType& countArray, bool saveInputToOutputMap)
  {
    this->BuildArrays(countArray, vtkm::cont::DeviceAdapterTagAny(), saveInputToOutputMap);
  }

  using OutputToInputMapType = vtkm::cont::ArrayHandle<vtkm::Id>;

  template <typename RangeType>
  VTKM_CONT OutputToInputMapType GetOutputToInputMap(RangeType) const
  {
    return this->OutputToInputMap;
  }

  using VisitArrayType = vtkm::cont::ArrayHandle<vtkm::IdComponent>;
  template <typename RangeType>
  VTKM_CONT VisitArrayType GetVisitArray(RangeType) const
  {
    return this->VisitArray;
  }

  VTKM_CONT
  vtkm::Id GetOutputRange(vtkm::Id inputRange) const
  {
    if (inputRange != this->InputRange)
    {
      std::stringstream msg;
      msg << "ScatterCounting initialized with input domain of size " << this->InputRange
          << " but used with a worklet invoke of size " << inputRange << std::endl;
      throw vtkm::cont::ErrorBadValue(msg.str());
    }
    return this->VisitArray.GetNumberOfValues();
  }
  VTKM_CONT
  vtkm::Id GetOutputRange(vtkm::Id3 inputRange) const
  {
    return this->GetOutputRange(inputRange[0] * inputRange[1] * inputRange[2]);
  }

  VTKM_CONT
  OutputToInputMapType GetOutputToInputMap() const { return this->OutputToInputMap; }

  /// This array will not be valid unless explicitly instructed to be saved.
  /// (See documentation for the constructor.)
  ///
  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Id> GetInputToOutputMap() const { return this->InputToOutputMap; }

private:
  vtkm::Id InputRange;
  vtkm::cont::ArrayHandle<vtkm::Id> InputToOutputMap;
  OutputToInputMapType OutputToInputMap;
  VisitArrayType VisitArray;

  template <typename CountArrayType>
  VTKM_CONT void BuildArrays(const CountArrayType& count,
                             vtkm::cont::DeviceAdapterId device,
                             bool saveInputToOutputMap)
  {
    VTKM_IS_ARRAY_HANDLE(CountArrayType);

    this->InputRange = count.GetNumberOfValues();

    // The input to output map is actually built off by one. The first entry
    // is actually for the second value. The last entry is the total number of
    // output. This off-by-one is so that an upper bound find will work when
    // building the output to input map. Later we will either correct the
    // map or delete it.
    vtkm::cont::ArrayHandle<vtkm::Id> inputToOutputMapOffByOne;
    vtkm::Id outputSize = vtkm::cont::Algorithm::ScanInclusive(
      device, vtkm::cont::make_ArrayHandleCast(count, vtkm::Id()), inputToOutputMapOffByOne);

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
    if (outputSize < this->InputRange)
    {
      this->BuildOutputToInputMapWithFind(outputSize, device, inputToOutputMapOffByOne);
    }
    else
    {
      this->BuildOutputToInputMapWithIterate(outputSize, device, inputToOutputMapOffByOne);
    }

    if (saveInputToOutputMap)
    {
      // Since we are saving it, correct the input to output map.
      vtkm::cont::Algorithm::Copy(
        device, detail::ShiftArrayHandleByOne(inputToOutputMapOffByOne), this->InputToOutputMap);
    }
  }

  VTKM_CONT void BuildOutputToInputMapWithFind(
    vtkm::Id outputSize,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::ArrayHandle<vtkm::Id> inputToOutputMapOffByOne)
  {
    vtkm::cont::ArrayHandleIndex outputIndices(outputSize);
    vtkm::cont::Algorithm::UpperBounds(
      device, inputToOutputMapOffByOne, outputIndices, this->OutputToInputMap);

    vtkm::cont::ArrayHandle<vtkm::Id> startsOfGroups;

    // This find gives the index of the start of a group.
    vtkm::cont::Algorithm::LowerBounds(
      device, this->OutputToInputMap, this->OutputToInputMap, startsOfGroups);

    this->VisitArray.Allocate(outputSize);
    vtkm::worklet::DispatcherMapField<detail::SubtractToVisitIndexWorklet> dispatcher;
    dispatcher.SetDevice(device);
    dispatcher.Invoke(startsOfGroups, this->VisitArray);
  }

  VTKM_CONT void BuildOutputToInputMapWithIterate(
    vtkm::Id outputSize,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::ArrayHandle<vtkm::Id> inputToOutputMapOffByOne)
  {
    this->OutputToInputMap.Allocate(outputSize);
    this->VisitArray.Allocate(outputSize);

    detail::ReverseInputToOutputMapWorklet::Run(
      inputToOutputMapOffByOne, this->OutputToInputMap, this->VisitArray, device);
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_ScatterCounting_h
