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
#ifndef vtk_m_VecFromPortalPermute_h
#define vtk_m_VecFromPortalPermute_h

#include <vtkm/Math.h>
#include <vtkm/Types.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/VecTraits.h>

namespace vtkm {

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
  using ComponentType =
      typename std::remove_const<typename PortalType::ValueType>::type;


  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  VecFromPortalPermute() {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  VecFromPortalPermute(const IndexVecType *indices, const PortalType &portal)
    : Indices(indices), Portal(portal) {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfComponents() const {
    return this->Indices->GetNumberOfComponents();
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template<vtkm::IdComponent DestSize>
  VTKM_EXEC_CONT
  //DRP
  /*inline*/ void CopyInto(vtkm::Vec<ComponentType,DestSize> &dest) const
  {
    vtkm::IdComponent numComponents =
        vtkm::Min(DestSize, this->GetNumberOfComponents());
    for (vtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = (*this)[index];
    }
  }

  //DRP
  VTKM_SUPPRESS_EXEC_WARNINGS
  template<vtkm::IdComponent DestSize>
  VTKM_EXEC_CONT
  inline void CopyRangeInto(vtkm::Vec<ComponentType,DestSize> &dest) const
  {
    this->Portal.CopyRangeInto((*this->Indices), dest);
  }    

  //DRP
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  inline void Copy8(vtkm::Vec<ComponentType,8> &dest) const
  {
      this->Portal.Get8((*this->Indices), dest);
      
      //dest[0] = this->Portal.Get_1((*this->Indices)); //(*this->Indices));
      /*
      dest[0] = this->Portal.Get((*this->Indices)[0]);      
      dest[1] = this->Portal.Get((*this->Indices)[1]);
      dest[2] = this->Portal.Get((*this->Indices)[2]);
      dest[3] = this->Portal.Get((*this->Indices)[3]);
      dest[4] = this->Portal.Get((*this->Indices)[4]);
      dest[5] = this->Portal.Get((*this->Indices)[5]);
      dest[6] = this->Portal.Get((*this->Indices)[6]);
      dest[7] = this->Portal.Get((*this->Indices)[7]);
      */

  }    

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  //DRP
  /*  inline*/ ComponentType operator[](vtkm::IdComponent index) const
  {
    return this->Portal.Get((*this->Indices)[index]);
  }

private:
  const IndexVecType *Indices;
  PortalType Portal;
};

template<typename IndexVecType, typename PortalType>
struct TypeTraits<
    vtkm::VecFromPortalPermute<IndexVecType,PortalType> >
{
private:
  typedef vtkm::VecFromPortalPermute<IndexVecType,PortalType>
      VecType;
  typedef typename PortalType::ValueType ComponentType;

public:
  typedef typename vtkm::TypeTraits<ComponentType>::NumericTag NumericTag;
  typedef TypeTraitsVectorTag DimensionalityTag;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  static VecType ZeroInitialization()
  {
    return VecType();
  }
};

template<typename IndexVecType, typename PortalType>
struct VecTraits<
    vtkm::VecFromPortalPermute<IndexVecType,PortalType> >
{
  typedef vtkm::VecFromPortalPermute<IndexVecType,PortalType>
      VecType;

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

#endif //vtk_m_VecFromPortalPermute_h
