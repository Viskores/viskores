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

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorBadValue.h>

#include <vtkm/exec/FunctorBase.h>

#include <sstream>

namespace vtkm
{
namespace worklet
{

namespace detail
{

template <typename Device>
struct ReverseInputToOutputMapKernel : vtkm::exec::FunctorBase
{
  using InputMapType =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::ExecutionTypes<Device>::PortalConst;
  using OutputMapType = typename vtkm::cont::ArrayHandle<vtkm::Id>::ExecutionTypes<Device>::Portal;
  using VisitType =
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::ExecutionTypes<Device>::Portal;

  InputMapType InputToOutputMap;
  OutputMapType OutputToInputMap;
  VisitType Visit;
  vtkm::Id OutputSize;

  VTKM_CONT
  ReverseInputToOutputMapKernel(const InputMapType& inputToOutputMap,
                                const OutputMapType& outputToInputMap,
                                const VisitType& visit,
                                vtkm::Id outputSize)
    : InputToOutputMap(inputToOutputMap)
    , OutputToInputMap(outputToInputMap)
    , Visit(visit)
    , OutputSize(outputSize)
  {
  }

  VTKM_EXEC
  void operator()(vtkm::Id inputIndex) const
  {
    vtkm::Id outputStartIndex;
    if (inputIndex > 0)
    {
      outputStartIndex = this->InputToOutputMap.Get(inputIndex - 1);
    }
    else
    {
      outputStartIndex = 0;
    }
    vtkm::Id outputEndIndex = this->InputToOutputMap.Get(inputIndex);

    vtkm::IdComponent visitIndex = 0;
    for (vtkm::Id outputIndex = outputStartIndex; outputIndex < outputEndIndex; outputIndex++)
    {
      this->OutputToInputMap.Set(outputIndex, inputIndex);
      this->Visit.Set(outputIndex, visitIndex);
      visitIndex++;
    }
  }
};

template <typename Device>
struct SubtractToVisitIndexKernel : vtkm::exec::FunctorBase
{
  using StartsOfGroupsType =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::ExecutionTypes<Device>::PortalConst;
  using VisitType =
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::ExecutionTypes<Device>::Portal;

  StartsOfGroupsType StartsOfGroups;
  VisitType Visit;

  VTKM_CONT
  SubtractToVisitIndexKernel(const StartsOfGroupsType& startsOfGroups, const VisitType& visit)
    : StartsOfGroups(startsOfGroups)
    , Visit(visit)
  {
  }

  VTKM_EXEC
  void operator()(vtkm::Id inputIndex) const
  {
    vtkm::Id startOfGroup = this->StartsOfGroups.Get(inputIndex);
    vtkm::IdComponent visitIndex = static_cast<vtkm::IdComponent>(inputIndex - startOfGroup);
    this->Visit.Set(inputIndex, visitIndex);
  }
};

template <typename Device>
struct AdjustMapByOne : vtkm::exec::FunctorBase
{
  using OffByOnePortalType =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::ExecutionTypes<Device>::PortalConst;
  using CorrectedPortalType =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::ExecutionTypes<Device>::Portal;

  OffByOnePortalType MapOffByOne;
  CorrectedPortalType MapCorrected;

  VTKM_CONT
  AdjustMapByOne(const OffByOnePortalType& mapOffByOne, const CorrectedPortalType& mapCorrected)
    : MapOffByOne(mapOffByOne)
    , MapCorrected(mapCorrected)
  {
  }

  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    if (index != 0)
    {
      this->MapCorrected.Set(index, this->MapOffByOne.Get(index - 1));
    }
    else
    {
      this->MapCorrected.Set(0, 0);
    }
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
  template <typename CountArrayType, typename Device>
  VTKM_CONT ScatterCounting(const CountArrayType& countArray,
                            Device,
                            bool saveInputToOutputMap = false)
  {
    this->BuildArrays(countArray, Device(), saveInputToOutputMap);
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

  template <typename CountArrayType, typename Device>
  VTKM_CONT void BuildArrays(const CountArrayType& count, Device, bool saveInputToOutputMap)
  {
    VTKM_IS_ARRAY_HANDLE(CountArrayType);
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    this->InputRange = count.GetNumberOfValues();

    // The input to output map is actually built off by one. The first entry
    // is actually for the second value. The last entry is the total number of
    // output. This off-by-one is so that an upper bound find will work when
    // building the output to input map. Later we will either correct the
    // map or delete it.
    vtkm::cont::ArrayHandle<vtkm::Id> inputToOutputMapOffByOne;
    vtkm::Id outputSize = vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanInclusive(
      vtkm::cont::make_ArrayHandleCast(count, vtkm::Id()), inputToOutputMapOffByOne);

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
      this->BuildOutputToInputMapWithFind(outputSize, inputToOutputMapOffByOne, Device());
    }
    else
    {
      this->BuildOutputToInputMapWithIterate(outputSize, inputToOutputMapOffByOne, Device());
    }

    if (saveInputToOutputMap)
    {
      // Since we are saving it, correct the input to output map.
      detail::AdjustMapByOne<Device> kernel(
        inputToOutputMapOffByOne.PrepareForInput(Device()),
        this->InputToOutputMap.PrepareForOutput(this->InputRange, Device()));

      vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, this->InputRange);
    }
  }

  template <typename Device>
  VTKM_CONT void BuildOutputToInputMapWithFind(
    vtkm::Id outputSize,
    vtkm::cont::ArrayHandle<vtkm::Id> inputToOutputMapOffByOne,
    Device)
  {
    vtkm::cont::ArrayHandleIndex outputIndices(outputSize);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::UpperBounds(
      inputToOutputMapOffByOne, outputIndices, this->OutputToInputMap);

    vtkm::cont::ArrayHandle<vtkm::Id> startsOfGroups;

    // This find gives the index of the start of a group.
    vtkm::cont::DeviceAdapterAlgorithm<Device>::LowerBounds(
      this->OutputToInputMap, this->OutputToInputMap, startsOfGroups);

    detail::SubtractToVisitIndexKernel<Device> kernel(
      startsOfGroups.PrepareForInput(Device()),
      this->VisitArray.PrepareForOutput(outputSize, Device()));
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, outputSize);
  }

  template <typename Device>
  VTKM_CONT void BuildOutputToInputMapWithIterate(
    vtkm::Id outputSize,
    vtkm::cont::ArrayHandle<vtkm::Id> inputToOutputMapOffByOne,
    Device)
  {
    detail::ReverseInputToOutputMapKernel<Device> kernel(
      inputToOutputMapOffByOne.PrepareForInput(Device()),
      this->OutputToInputMap.PrepareForOutput(outputSize, Device()),
      this->VisitArray.PrepareForOutput(outputSize, Device()),
      outputSize);

    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(
      kernel, inputToOutputMapOffByOne.GetNumberOfValues());
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_ScatterCounting_h
