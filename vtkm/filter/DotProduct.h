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

#ifndef vtk_m_filter_DotProduct_h
#define vtk_m_filter_DotProduct_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/worklet/DotProduct.h>

namespace vtkm
{
namespace filter
{

class DotProduct : public vtkm::filter::FilterField<DotProduct>
{
public:
  VTKM_CONT
  DotProduct();

  VTKM_CONT
  void SetSecondaryFieldName(const std::string& nm) { SecondaryFieldName = nm; }

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
  vtkm::worklet::DotProduct Worklet;
  std::string SecondaryFieldName;
};

template <>
class FilterTraits<DotProduct>
{ //currently the DotProduct filter only works on vector data.
public:
  typedef TypeListTagVecCommon InputFieldTypeList;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/DotProduct.hxx>

#endif // vtk_m_filter_DotProduct_h
