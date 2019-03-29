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

#ifndef vtkm_m_filter_CellSetConnectivity_hxx
#define vtkm_m_filter_CellSetConnectivity_hxx

#include <vtkm/filter/CellSetConnectivity.h>
#include <vtkm/filter/internal/CreateResult.h>
#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm
{
namespace filter
{

VTKM_CONT CellSetConnectivity::CellSetConnectivity()
{
  this->SetOutputFieldName("component");
}

template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet CellSetConnectivity::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>&,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  vtkm::cont::ArrayHandle<vtkm::Id> component;

  vtkm::worklet::connectivity::CellSetConnectivity().Run(
    vtkm::filter::ApplyPolicy(input.GetCellSet(this->GetActiveCellSetIndex()), policy), component);

  auto result = internal::CreateResult(input,
                                       component,
                                       this->GetOutputFieldName(),
                                       vtkm::cont::Field::Association::CELL_SET,
                                       fieldMetadata.GetCellSetName());
  return result;
}
}
}

#endif //vtkm_m_filter_CellSetConnectivity_hxx
