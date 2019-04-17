//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ImageConnectivity_h
#define vtk_m_filter_ImageConnectivity_h

#include <vtkm/filter/FilterCell.h>
#include <vtkm/worklet/connectivities/ImageConnectivity.h>

namespace vtkm
{
namespace filter
{
class ImageConnectivity : public vtkm::filter::FilterCell<ImageConnectivity>
{
public:
  VTKM_CONT ImageConnectivity();

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

#ifndef vtk_m_filter_ImageConnectivity_hxx
#include <vtkm/filter/ImageConnectivity.hxx>
#endif

#endif //vtk_m_filter_ImageConnectivity_h
