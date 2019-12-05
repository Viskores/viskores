//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_ScatterCounting_h
#define vtk_m_worklet_ScatterCounting_h

#include <vtkm/worklet/internal/ScatterBase.h>
#include <vtkm/worklet/vtkm_worklet_export.h>

#include <vtkm/cont/VariantArrayHandle.h>

#include <sstream>

namespace vtkm
{
namespace worklet
{

namespace detail
{

struct ScatterCountingBuilder;

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
struct VTKM_WORKLET_EXPORT ScatterCounting : internal::ScatterBase
{
  using CountTypes = vtkm::List<vtkm::Int64,
                                vtkm::Int32,
                                vtkm::Int16,
                                vtkm::Int8,
                                vtkm::UInt64,
                                vtkm::UInt32,
                                vtkm::UInt16,
                                vtkm::UInt8>;
  using VariantArrayHandleCount = vtkm::cont::VariantArrayHandleBase<CountTypes>;

  /// Construct a \c ScatterCounting object using an array of counts for the
  /// number of outputs for each input. Part of the construction requires
  /// generating an input to output map, but this map is not needed for the
  /// operations of \c ScatterCounting, so by default it is deleted. However,
  /// other users might make use of it, so you can instruct the constructor
  /// to save the input to output map.
  ///
  template <typename TypeList>
  VTKM_CONT ScatterCounting(const vtkm::cont::VariantArrayHandleBase<TypeList>& countArray,
                            vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny(),
                            bool saveInputToOutputMap = false)
  {
    this->BuildArrays(VariantArrayHandleCount(countArray), device, saveInputToOutputMap);
  }
  VTKM_CONT ScatterCounting(const VariantArrayHandleCount& countArray,
                            vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny(),
                            bool saveInputToOutputMap = false)
  {
    this->BuildArrays(countArray, device, saveInputToOutputMap);
  }
  template <typename TypeList>
  VTKM_CONT ScatterCounting(const vtkm::cont::VariantArrayHandleBase<TypeList>& countArray,
                            bool saveInputToOutputMap)
  {
    this->BuildArrays(
      VariantArrayHandleCount(countArray), vtkm::cont::DeviceAdapterTagAny(), saveInputToOutputMap);
  }
  VTKM_CONT ScatterCounting(const VariantArrayHandleCount& countArray, bool saveInputToOutputMap)
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

  friend struct detail::ScatterCountingBuilder;

  VTKM_CONT void BuildArrays(const VariantArrayHandleCount& countArray,
                             vtkm::cont::DeviceAdapterId device,
                             bool saveInputToOutputMap);
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_ScatterCounting_h
