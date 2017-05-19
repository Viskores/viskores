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
#ifndef vtk_m_VecFromPortal_h
#define vtk_m_VecFromPortal_h

#include <vtkm/Math.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

#include <vtkm/internal/ArrayPortalValueReference.h>

namespace vtkm {

/// \brief A short variable-length array from a window in an ArrayPortal.
///
/// The \c VecFromPortal class is a Vec-like class that holds an array portal
/// and exposes a small window of that portal as if it were a \c Vec.
///
template<typename PortalType>
class VecFromPortal
{
public:
  using ComponentType =
      typename std::remove_const<typename PortalType::ValueType>::type;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  VecFromPortal() : NumComponents(0), Offset(0) {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  VecFromPortal(const PortalType &portal,
                vtkm::IdComponent numComponents = 0,
                vtkm::Id offset = 0)
    : Portal(portal), NumComponents(numComponents), Offset(offset) {  }

  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfComponents() const {
    return this->NumComponents;
  }

  template<typename T, vtkm::IdComponent DestSize>
  VTKM_EXEC_CONT
  void CopyInto(vtkm::Vec<T,DestSize> &dest) const
  {
    vtkm::IdComponent numComponents = vtkm::Min(DestSize, this->NumComponents);
    for (vtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = this->Portal.Get(index + this->Offset);
    }
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::internal::ArrayPortalValueReference<PortalType>
  operator[](vtkm::IdComponent index) const
  {
    return vtkm::internal::ArrayPortalValueReference<PortalType>(
          this->Portal, index + this->Offset);
  }

private:
  PortalType Portal;
  vtkm::IdComponent NumComponents;
  vtkm::Id Offset;
};

template<typename PortalType>
struct TypeTraits<vtkm::VecFromPortal<PortalType> >
{
private:
  typedef typename PortalType::ValueType ComponentType;

public:
  typedef typename vtkm::TypeTraits<ComponentType>::NumericTag NumericTag;
  typedef TypeTraitsVectorTag DimensionalityTag;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  static vtkm::VecFromPortal<PortalType> ZeroInitialization()
  {
    return vtkm::VecFromPortal<PortalType>();
  }
};

template<typename PortalType>
struct VecTraits<vtkm::VecFromPortal<PortalType> >
{
  typedef vtkm::VecFromPortal<PortalType> VecType;

  typedef typename VecType::ComponentType ComponentType;
  typedef vtkm::VecTraitsTagMultipleComponents HasMultipleComponents;
  typedef vtkm::VecTraitsTagSizeVariable IsSizeStatic;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  static vtkm::IdComponent GetNumberOfComponents(const VecType &vector) {
    return vector.GetNumberOfComponents();
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  static ComponentType GetComponent(const VecType &vector,
                                    vtkm::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template<vtkm::IdComponent destSize>
  VTKM_EXEC_CONT
  static void CopyInto(const VecType &src,
                       vtkm::Vec<ComponentType,destSize> &dest)
  {
    src.CopyInto(dest);
  }
};

} // namespace vtkm

#endif //vtk_m_VecFromPortal_h
