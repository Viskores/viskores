//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtkm_m_filter_CellSetConnectivity_h
#define vtkm_m_filter_CellSetConnectivity_h


#include <vtkm/filter/FilterCell.h>
#include <vtkm/worklet/connectivities/CellSetConnectivity.h>

namespace vtkm
{
namespace filter
{
class CellSetConnectivity : public vtkm::filter::FilterCell<CellSetConnectivity>
{
public:
  VTKM_CONT CellSetConnectivity();

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMetadata,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>&);
};

template <>
class FilterTraits<vtkm::filter::CellSetConnectivity>
{
public:
  struct InputFieldTypeList : vtkm::TypeListTagScalarAll
  {
  };
};
}
}

#ifndef vtkm_m_filter_CellSetConnectivity_hxx
#include <vtkm/filter/CellSetConnectivity.hxx>
#endif

#endif //vtkm_m_filter_CellSetConnectivity_h
