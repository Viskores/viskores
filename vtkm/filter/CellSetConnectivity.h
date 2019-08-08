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

namespace vtkm
{
namespace filter
{
class CellSetConnectivity : public vtkm::filter::FilterCell<CellSetConnectivity>
{
public:
  using SupportedTypes = vtkm::TypeListTagScalarAll;
  VTKM_CONT CellSetConnectivity();

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMetadata,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>&);
};
}
}

#include <vtkm/filter/CellSetConnectivity.hxx>

#endif //vtkm_m_filter_CellSetConnectivity_h
