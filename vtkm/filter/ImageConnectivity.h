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

#ifndef vtk_m_filter_ImageConnectivity_h
#define vtk_m_filter_ImageConnectivity_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/worklet/connectivities/ImageConnectivity.h>

namespace vtkm
{
namespace filter
{
class ImageConnectivity : public vtkm::filter::FilterField<ImageConnectivity>
{
public:
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMetadata,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>&);
};

template <>
class FilterTraits<vtkm::filter::ImageConnectivity>
{
public:
  struct InputFieldTypeList : vtkm::TypeListTagScalarAll
  {
  };
};
}
} // namespace vtkm::filter

#include <vtkm/filter/ImageConnectivity.hxx>

#endif //vtk_m_filter_ImageConnectivity_h