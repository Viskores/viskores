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
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
#include <vtkm/cont/ErrorControlBadValue.h>

#include <vtkm/exec/FunctorBase.h>

#include <sstream>

namespace vtkm {
namespace worklet {

namespace detail {

template<typename Device>
struct ReverseInputToOutputMapKernel : vtkm::exec::FunctorBase
{
  typedef typename
    vtkm::cont::ArrayHandle<vtkm::Id>::ExecutionTypes<Device>::PortalConst
    InputMapType;
  typedef typename
    vtkm::cont::ArrayHandle<vtkm::Id>::ExecutionTypes<Device>::Portal
    OutputMapType;
  typedef typename
    vtkm::cont::ArrayHandle<vtkm::IdComponent>::ExecutionTypes<Device>::Portal
    VisitType;

  InputMapType InputToOutputMap;
  OutputMapType OutputToInputMap;
  VisitType Visit;
  vtkm::Id OutputSize;

  VTKM_CONT_EXPORT
  ReverseInputToOutputMapKernel(const InputMapType &inputToOutputMap,
                                const OutputMapType &outputToInputMap,
                                const VisitType &visit,
                                vtkm::Id outputSize)
    : InputToOutputMap(inputToOutputMap),
      OutputToInputMap(outputToInputMap),
      Visit(visit),
      OutputSize(outputSize)
  {  }

  VTKM_EXEC_EXPORT
  void operator()(vtkm::Id inputIndex) const
  {
    vtkm::Id outputStartIndex;
    if (inputIndex > 0)
    {
      outputStartIndex = this->InputToOutputMap.Get(inputIndex-1);
    }
    else
    {
      outputStartIndex = 0;
    }
    vtkm::Id outputEndIndex = this->InputToOutputMap.Get(inputIndex);

    vtkm::IdComponent visitIndex = 0;
    for (vtkm::Id outputIndex = outputStartIndex;
         outputIndex < outputEndIndex;
         outputIndex++)
    {
      this->OutputToInputMap.Set(outputIndex, inputIndex);
      this->Visit.Set(outputIndex, visitIndex);
      visitIndex++;
    }
  }
};

template<typename Device>
struct SubtractToVisitIndexKernel : vtkm::exec::FunctorBase
{
  typedef typename
    vtkm::cont::ArrayHandle<vtkm::Id>::ExecutionTypes<Device>::PortalConst
    StartsOfGroupsType;
  typedef typename
    vtkm::cont::ArrayHandle<vtkm::IdComponent>::ExecutionTypes<Device>::Portal
    VisitType;

  StartsOfGroupsType StartsOfGroups;
  VisitType Visit;

  VTKM_CONT_EXPORT
  SubtractToVisitIndexKernel(const StartsOfGroupsType &startsOfGroups,
                             const VisitType &visit)
    : StartsOfGroups(startsOfGroups), Visit(visit)
  {  }

  VTKM_EXEC_EXPORT
  void operator()(vtkm::Id inputIndex) const
  {
    vtkm::Id startOfGroup = this->StartsOfGroups.Get(inputIndex);
    vtkm::IdComponent visitIndex =
        static_cast<vtkm::IdComponent>(inputIndex - startOfGroup);
    this->Visit.Set(inputIndex, visitIndex);
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
  VTKM_CONT_EXPORT
  ScatterCounting()
  {
  }

  template<typename CountArrayType, typename Device>
  VTKM_CONT_EXPORT
  ScatterCounting(const CountArrayType &countArray, Device)
  {
    this->BuildArrays(countArray, Device());
  }

  typedef vtkm::cont::ArrayHandle<vtkm::Id> OutputToInputMapType;
  template<typename RangeType>
  VTKM_CONT_EXPORT
  OutputToInputMapType GetOutputToInputMap(RangeType) const
  {
    return this->OutputToInputMap;
  }

  typedef vtkm::cont::ArrayHandle<vtkm::IdComponent> VisitArrayType;
  template<typename RangeType>
  VTKM_CONT_EXPORT
  VisitArrayType GetVisitArray(RangeType) const
  {
    return this->VisitArray;
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetOutputRange(vtkm::Id inputRange) const
  {
    if (inputRange != this->InputRange)
    {
      std::stringstream msg;
      msg << "ScatterCounting initialized with input domain of size "
          << this->InputRange
          << " but used with a worklet invoke of size "
          << inputRange << std::endl;
      throw vtkm::cont::ErrorControlBadValue(msg.str());
    }
    return this->VisitArray.GetNumberOfValues();
  }
  VTKM_CONT_EXPORT
  vtkm::Id GetOutputRange(vtkm::Id3 inputRange) const
  {
    return this->GetOutputRange(inputRange[0]*inputRange[1]*inputRange[2]);
  }

private:
  vtkm::Id InputRange;
  OutputToInputMapType OutputToInputMap;
  VisitArrayType VisitArray;

  template<typename CountArrayType, typename Device>
  VTKM_CONT_EXPORT
  void BuildArrays(const CountArrayType &count, Device)
  {
    VTKM_IS_ARRAY_HANDLE(CountArrayType);
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    this->InputRange = count.GetNumberOfValues();

    // Currently we are treating the input to output map as a temporary
    // variable. However, it is possible that this could, be useful elsewhere,
    // so we may want to save this and make it available.
    //
    // The input to output map is actually built off by one. The first entry
    // is actually for the second value. The last entry is the total number of
    // output. This off-by-one is so that an upper bound find will work when
    // building the output to input map.
    vtkm::cont::ArrayHandle<vtkm::Id> inputToOutputMap;
    vtkm::Id outputSize =
        vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanInclusive(
          vtkm::cont::make_ArrayHandleCast(count, vtkm::Id()),
          inputToOutputMap);

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
      this->BuildOutputToInputMapWithFind(
            outputSize, inputToOutputMap, Device());
    }
    else
    {
      this->BuildOutputToInputMapWithIterate(
            outputSize, inputToOutputMap, Device());
    }
  }

  template<typename Device>
  VTKM_CONT_EXPORT
  void BuildOutputToInputMapWithFind(
      vtkm::Id outputSize,
      vtkm::cont::ArrayHandle<vtkm::Id> inputToOutputMap,
      Device)
  {
    vtkm::cont::ArrayHandleIndex outputIndices(outputSize);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::UpperBounds(
          inputToOutputMap, outputIndices, this->OutputToInputMap);

    // Do not need this anymore.
    inputToOutputMap.ReleaseResources();

    vtkm::cont::ArrayHandle<vtkm::Id> startsOfGroups;

    // This find gives the index of the start of a group.
    vtkm::cont::DeviceAdapterAlgorithm<Device>::LowerBounds(
          this->OutputToInputMap, this->OutputToInputMap, startsOfGroups);

    detail::SubtractToVisitIndexKernel<Device>
        kernel(startsOfGroups.PrepareForInput(Device()),
               this->VisitArray.PrepareForOutput(outputSize, Device()));
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, outputSize);
  }

  template<typename Device>
  VTKM_CONT_EXPORT
  void BuildOutputToInputMapWithIterate(
      vtkm::Id outputSize,
      vtkm::cont::ArrayHandle<vtkm::Id> inputToOutputMap,
      Device)
  {
    detail::ReverseInputToOutputMapKernel<Device>
        kernel(inputToOutputMap.PrepareForInput(Device()),
               this->OutputToInputMap.PrepareForOutput(outputSize, Device()),
               this->VisitArray.PrepareForOutput(outputSize, Device()),
               outputSize);

    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(
          kernel, inputToOutputMap.GetNumberOfValues());
  }
};

}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_ScatterCounting_h
