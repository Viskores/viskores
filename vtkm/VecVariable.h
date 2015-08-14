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
#ifndef vtk_m_VecVariable_h
#define vtk_m_VecVariable_h

#include <vtkm/Math.h>
#include <vtkm/Types.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/VecTraits.h>

namespace vtkm {

/// \brief A short variable-length array with maximum length.
///
/// The \c VecVariable class is a Vec-like class that holds a short array of
/// some maximum length. To avoid dynamic allocations, the maximum length is
/// specified at compile time. Internally, \c VecVariable holds a \c Vec of
/// the maximum length and exposes a subsection of it.
///
template<typename T, vtkm::IdComponent MaxSize>
class VecVariable
{
public:
  typedef T ComponentType;

  VTKM_EXEC_CONT_EXPORT
  VecVariable() : NumComponents(0) {  }

  template<vtkm::IdComponent SrcSize>
  VTKM_EXEC_CONT_EXPORT
  VecVariable(const vtkm::VecVariable<ComponentType,SrcSize> &src)
    : NumComponents(src.GetNumberOfComponents())
  {
    for (vtkm::IdComponent index = 0; index < this->NumComponents; index++)
    {
      this->Data[index] = src[index];
    }
  }

  template<vtkm::IdComponent SrcSize>
  VTKM_EXEC_CONT_EXPORT
  VecVariable(const vtkm::Vec<ComponentType,SrcSize> &src)
    : NumComponents(SrcSize)
  {
    for (vtkm::IdComponent index = 0; index < this->NumComponents; index++)
    {
      this->Data[index] = src[index];
    }
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetNumberOfComponents() const {
    return this->NumComponents;
  }

  template<vtkm::IdComponent DestSize>
  VTKM_EXEC_CONT_EXPORT
  void CopyInto(vtkm::Vec<ComponentType,DestSize> &dest) const
  {
    vtkm::IdComponent numComponents = vtkm::Min(DestSize, this->NumComponents);
    for (vtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = this->Data[index];
    }
  }

  VTKM_EXEC_CONT_EXPORT
  const ComponentType &operator[](vtkm::IdComponent index) const
  {
    return this->Data[index];
  }

  VTKM_EXEC_CONT_EXPORT
  ComponentType &operator[](vtkm::IdComponent index)
  {
    return this->Data[index];
  }

  VTKM_EXEC_CONT_EXPORT
  void Append(ComponentType value)
  {
    this->Data[this->NumComponents] = value;
    this->NumComponents++;
  }

private:
  vtkm::Vec<T,MaxSize> Data;
  vtkm::IdComponent NumComponents;
};

template<typename T, vtkm::IdComponent MaxSize>
struct TypeTraits<vtkm::VecVariable<T,MaxSize> >
{
  typedef typename vtkm::TypeTraits<T>::NumericTag NumericTag;
  typedef TypeTraitsVectorTag DimensionalityTag;

  VTKM_EXEC_CONT_EXPORT
  static vtkm::VecVariable<T,MaxSize> ZeroInitialization()
  {
    return vtkm::VecVariable<T,MaxSize>();
  }
};

template<typename T, vtkm::IdComponent MaxSize>
struct VecTraits<vtkm::VecVariable<T,MaxSize> >
{
  typedef vtkm::VecVariable<T,MaxSize> VecType;

  typedef typename VecType::ComponentType ComponentType;
  typedef vtkm::VecTraitsTagMultipleComponents HasMultipleComponents;
  typedef vtkm::VecTraitsTagSizeVariable IsSizeStatic;

  VTKM_EXEC_CONT_EXPORT
  static vtkm::IdComponent GetNumberOfComponents(const VecType &vector) {
    return vector.GetNumberOfComponents();
  }

  VTKM_EXEC_CONT_EXPORT
  static const ComponentType &GetComponent(const VecType &vector,
                                           vtkm::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }
  VTKM_EXEC_CONT_EXPORT
  static ComponentType &GetComponent(VecType &vector,
                                     vtkm::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }

  VTKM_EXEC_CONT_EXPORT
  static void SetComponent(VecType &vector,
                           vtkm::IdComponent componentIndex,
                           const ComponentType &value)
  {
    vector[componentIndex] = value;
  }

  template<vtkm::IdComponent destSize>
  VTKM_EXEC_CONT_EXPORT
  static void CopyInto(const VecType &src,
                       vtkm::Vec<ComponentType,destSize> &dest)
  {
    src.CopyInto(dest);
  }
};

} // namespace vtkm

#endif //vtk_m_VecVariable_h
