//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_filter_CrossProduct_h
#define vtk_m_filter_CrossProduct_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/worklet/CrossProduct.h>

namespace vtkm
{
namespace filter
{

class CrossProduct : public vtkm::filter::FilterField<CrossProduct>
{
public:
  VTKM_CONT
  CrossProduct();

  VTKM_CONT
  void SetPrimaryField(
    const std::string& name,
    vtkm::cont::Field::AssociationEnum association = vtkm::cont::Field::ASSOC_ANY)
  {
    this->SetActiveField(name, association);
  }

  VTKM_CONT
  void SetSecondaryField(
    const std::string& name,
    vtkm::cont::Field::AssociationEnum association = vtkm::cont::Field::ASSOC_ANY)
  {
    this->SecondaryFieldName = name;
    this->SecondaryFieldAssociation = association;
  }

  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT vtkm::filter::Result DoExecute(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
    const vtkm::filter::FieldMetadata& fieldMeta,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
    const DeviceAdapter& tag);

  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT bool DoMapField(vtkm::filter::Result& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                            DeviceAdapter tag);

private:
  std::string SecondaryFieldName;
  vtkm::cont::Field::AssociationEnum SecondaryFieldAssociation;
};

template <>
class FilterTraits<CrossProduct>
{ //currently the CrossProduct filter only works on vector data.
public:
  typedef TypeListTagVecCommon InputFieldTypeList;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/CrossProduct.hxx>

#endif // vtk_m_filter_CrossProduct_h
