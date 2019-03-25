//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

#ifndef vtkm_m_filter_CellSetConnectivity_h
#define vtkm_m_filter_CellSetConnectivity_h


#include <vtkm/filter/FilterField.h>
#include <vtkm/worklet/connectivities/CellSetConnectivity.h>

namespace vtkm
{
namespace filter
{
class CellSetConnectivity : public vtkm::filter::FilterField<CellSetConnectivity>
{
public:
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

#include <vtkm/filter/CellSetConnectivity.hxx>

#endif //vtkm_m_filter_CellSetConnectivity_h
