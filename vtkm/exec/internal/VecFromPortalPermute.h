//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
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
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_internal_VecFromPortalPermute_h
#define vtk_m_exec_internal_VecFromPortalPermute_h

#include <vtkm/Math.h>
#include <vtkm/Types.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/VecTraits.h>

namespace vtkm {
namespace exec {
namespace internal {

/// \brief A short vector from an ArrayPortal and a vector of indices.
///
/// The \c VecFromPortalPermute class is a Vec-like class that holds an array
/// portal and a second Vec-like containing indices into the array. Each value
/// of this vector is the value from the array with the respective index.
///
template<typename IndexVecType, typename PortalType>
class VecFromPortalPermute
{
public:
  typedef typename PortalType::ValueType ComponentType;

  VTKM_EXEC_EXPORT
  VecFromPortalPermute() {  }

  VTKM_EXEC_EXPORT
  VecFromPortalPermute(const IndexVecType *indices, const PortalType &portal)
    : Indices(indices), Portal(portal) {  }

  VTKM_EXEC_EXPORT
  vtkm::IdComponent GetNumberOfComponents() const {
    return this->Indices->GetNumberOfComponents();
  }

  template<vtkm::IdComponent DestSize>
  VTKM_EXEC_EXPORT
  void CopyInto(vtkm::Vec<ComponentType,DestSize> &dest) const
  {
    vtkm::IdComponent numComponents =
        vtkm::Min(DestSize, this->GetNumberOfComponents());
    for (vtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = (*this)[index];
    }
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_EXPORT
  ComponentType operator[](vtkm::IdComponent index) const
  {
    return this->Portal.Get((*this->Indices)[index]);
  }

private:
  const IndexVecType *Indices;
  PortalType Portal;
};

}
}
} // namespace vtkm::exec::internal

// Implementations of traits classes, which by definition are in the vtkm
// namespace.

namespace vtkm {

template<typename IndexVecType, typename PortalType>
struct TypeTraits<
    vtkm::exec::internal::VecFromPortalPermute<IndexVecType,PortalType> >
{
private:
  typedef vtkm::exec::internal::VecFromPortalPermute<IndexVecType,PortalType>
      VecType;
  typedef typename PortalType::ValueType ComponentType;

public:
  typedef typename vtkm::TypeTraits<ComponentType>::NumericTag NumericTag;
  typedef TypeTraitsVectorTag DimensionalityTag;

  VTKM_EXEC_EXPORT
  static VecType ZeroInitialization()
  {
    return VecType();
  }
};

template<typename IndexVecType, typename PortalType>
struct VecTraits<
    vtkm::exec::internal::VecFromPortalPermute<IndexVecType,PortalType> >
{
  typedef vtkm::exec::internal::VecFromPortalPermute<IndexVecType,PortalType>
      VecType;

  typedef typename VecType::ComponentType ComponentType;
  typedef vtkm::VecTraitsTagMultipleComponents HasMultipleComponents;
  typedef vtkm::VecTraitsTagSizeVariable IsSizeStatic;

  VTKM_EXEC_EXPORT
  static vtkm::IdComponent GetNumberOfComponents(const VecType &vector) {
    return vector.GetNumberOfComponents();
  }

  VTKM_EXEC_EXPORT
  static ComponentType GetComponent(const VecType &vector,
                                    vtkm::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }

  template<vtkm::IdComponent destSize>
  VTKM_EXEC_EXPORT
  static void CopyInto(const VecType &src,
                       vtkm::Vec<ComponentType,destSize> &dest)
  {
    src.CopyInto(dest);
  }
};

} // namespace vtkm

#endif //vtk_m_exec_internal_VecFromPortalPermute_h
