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

#ifndef vtk_m_filter_ImageConnectivity_hxx
#define vtk_m_filter_ImageConnectivity_hxx

#include <vtkm/filter/ImageConnectivity.h>
#include <vtkm/filter/internal/CreateResult.h>
#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm
{
namespace filter
{

VTKM_CONT ImageConnectivity::ImageConnectivity()
{
  this->SetOutputFieldName("component");
}

template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ImageConnectivity::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  if (fieldMetadata.GetAssociation() != vtkm::cont::Field::Association::POINTS)
  {
    throw vtkm::cont::ErrorBadValue("Active field for ImageConnectivity must be a cell field.");
  }

  vtkm::cont::ArrayHandle<vtkm::Id> component;

  vtkm::worklet::connectivity::ImageConnectivity().Run(
    vtkm::filter::ApplyPolicy(input.GetCellSet(this->GetActiveCellSetIndex()), policy),
    field,
    component);

  auto result = internal::CreateResult(input,
                                       component,
                                       this->GetOutputFieldName(),
                                       vtkm::cont::Field::Association::POINTS,
                                       fieldMetadata.GetCellSetName());
  return result;
}
}
}

#endif //vtk_m_filter_ImageConnectivity_hxx
