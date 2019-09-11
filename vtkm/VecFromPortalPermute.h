//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_VecFromPortalPermute_h
#define vtk_m_VecFromPortalPermute_h

#include <vtkm/Math.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

namespace vtkm
{

/// \brief A short vector from an ArrayPortal and a vector of indices.
///
/// The \c VecFromPortalPermute class is a Vec-like class that holds an array
/// portal and a second Vec-like containing indices into the array. Each value
/// of this vector is the value from the array with the respective index.
///
template <typename IndexVecType, typename PortalType>
class VecFromPortalPermute
{
public:
  using ComponentType = typename std::remove_const<typename PortalType::ValueType>::type;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  VecFromPortalPermute() {}

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  VecFromPortalPermute(const IndexVecType* indices, const PortalType& portal)
    : Indices(indices)
    , Portal(portal)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfComponents() const { return this->Indices->GetNumberOfComponents(); }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <vtkm::IdComponent DestSize>
  VTKM_EXEC_CONT void CopyInto(vtkm::Vec<ComponentType, DestSize>& dest) const
  {
    vtkm::IdComponent numComponents = vtkm::Min(DestSize, this->GetNumberOfComponents());
    for (vtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = (*this)[index];
    }
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ComponentType operator[](vtkm::IdComponent index) const
  {
    return this->Portal.Get((*this->Indices)[index]);
  }

private:
  const IndexVecType* const Indices;
  PortalType Portal;
};

template <typename IndexVecType, typename PortalType>
class VecFromPortalPermute<IndexVecType, const PortalType*>
{
public:
  using ComponentType = typename std::remove_const<typename PortalType::ValueType>::type;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  VecFromPortalPermute() {}

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  VecFromPortalPermute(const IndexVecType* indices, const PortalType* const portal)
    : Indices(indices)
    , Portal(portal)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfComponents() const { return this->Indices->GetNumberOfComponents(); }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <vtkm::IdComponent DestSize>
  VTKM_EXEC_CONT void CopyInto(vtkm::Vec<ComponentType, DestSize>& dest) const
  {
    vtkm::IdComponent numComponents = vtkm::Min(DestSize, this->GetNumberOfComponents());
    for (vtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = (*this)[index];
    }
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ComponentType operator[](vtkm::IdComponent index) const
  {
    return this->Portal->Get((*this->Indices)[index]);
  }

private:
  const IndexVecType* const Indices;
  const PortalType* const Portal;
};

template <typename IndexVecType, typename PortalType>
struct TypeTraits<vtkm::VecFromPortalPermute<IndexVecType, PortalType>>
{
private:
  using VecType = vtkm::VecFromPortalPermute<IndexVecType, PortalType>;
  using ComponentType = typename PortalType::ValueType;

public:
  using NumericTag = typename vtkm::TypeTraits<ComponentType>::NumericTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  static VecType ZeroInitialization() { return VecType(); }
};

template <typename IndexVecType, typename PortalType>
struct VecTraits<vtkm::VecFromPortalPermute<IndexVecType, PortalType>>
{
  using VecType = vtkm::VecFromPortalPermute<IndexVecType, PortalType>;

  using ComponentType = typename VecType::ComponentType;
  using BaseComponentType = typename vtkm::VecTraits<ComponentType>::BaseComponentType;
  using HasMultipleComponents = vtkm::VecTraitsTagMultipleComponents;
  using IsSizeStatic = vtkm::VecTraitsTagSizeVariable;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  static vtkm::IdComponent GetNumberOfComponents(const VecType& vector)
  {
    return vector.GetNumberOfComponents();
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  static ComponentType GetComponent(const VecType& vector, vtkm::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <vtkm::IdComponent destSize>
  VTKM_EXEC_CONT static void CopyInto(const VecType& src, vtkm::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

template <typename IndexVecType, typename PortalType>
inline VTKM_EXEC VecFromPortalPermute<IndexVecType, PortalType> make_VecFromPortalPermute(
  const IndexVecType* index,
  const PortalType& portal)
{
  return VecFromPortalPermute<IndexVecType, PortalType>(index, portal);
}

template <typename IndexVecType, typename PortalType>
inline VTKM_EXEC VecFromPortalPermute<IndexVecType, const PortalType*> make_VecFromPortalPermute(
  const IndexVecType* index,
  const PortalType* const portal)
{
  return VecFromPortalPermute<IndexVecType, const PortalType*>(index, portal);
}

} // namespace vtkm

#endif //vtk_m_VecFromPortalPermute_h
