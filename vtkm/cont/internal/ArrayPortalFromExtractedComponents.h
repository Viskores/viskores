//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_ArrayPortalFromExtractedComponents_h
#define vtk_m_cont_internal_ArrayPortalFromExtractedComponents_h

#include <vtkm/cont/ArrayHandleStride.h>

#include <vtkm/internal/ArrayPortalHelpers.h>

#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

#include <vector>

namespace vtkm
{
namespace cont
{

class UnknownArrayHandle;

namespace internal
{

/// `ArrayPortalFromExtractedComponents` is a convenience class that allows you to treat
/// a group of arrays that were extracted from the components of an array and treat them
/// like a portal to the array itself. It is used internally by `UnknownArrayHandle` to
/// get read and write portals to the array
///
/// Note that this portal only works on the control environment.
///
template <typename PortalType>
class ArrayPortalFromExtractedComponents
{
private:
  using T = typename PortalType::ValueType;
  std::vector<vtkm::cont::ArrayHandleStride<T>> Arrays;
  std::vector<PortalType> Portals;
  mutable std::vector<T> Values;

  friend UnknownArrayHandle;

  void AddArray(const vtkm::cont::ArrayHandleStride<T>& array, const PortalType& portal)
  {
    this->Arrays.push_back(array);
    this->Portals.push_back(portal);
  }

public:
  using ValueType = vtkm::VecCConst<T>;

  ArrayPortalFromExtractedComponents(vtkm::IdComponent expectedArrays = 0)
  {
    this->Arrays.reserve(static_cast<std::size_t>(expectedArrays));
    this->Portals.reserve(static_cast<std::size_t>(expectedArrays));
  }

  VTKM_CONT vtkm::Id GetNumberOfValues() const { return this->Portals[0].GetNumberOfValues(); }

  VTKM_CONT ValueType Get(vtkm::Id index) const
  {
    // Note: this is not thread-safe
    this->Values.clear();
    for (auto&& portal : this->Portals)
    {
      this->Values.push_back(portal.Get(index));
    }
    return ValueType(this->Values.data(), static_cast<vtkm::IdComponent>(this->Values.size()));
  }

  template <typename VecType,
            typename Writable = vtkm::internal::PortalSupportsSets<PortalType>,
            typename = typename std::enable_if<Writable::value>::type>
  VTKM_CONT void Set(vtkm::Id index, const VecType& value) const
  {
    using Traits = vtkm::VecTraits<VecType>;
    for (vtkm::IdComponent cIndex = 0; cIndex < Traits::GetNumberOfComponents(value); ++cIndex)
    {
      this->Portals[static_cast<std::size_t>(cIndex)].Set(index,
                                                          Traits::GetComponent(value, index));
    }
  }
};

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ArrayPortalFromExtractedComponents_h
