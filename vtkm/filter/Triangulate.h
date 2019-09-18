//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Triangulate_h
#define vtk_m_filter_Triangulate_h

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/Triangulate.h>

namespace vtkm
{
namespace filter
{

class Triangulate : public vtkm::filter::FilterDataSet<Triangulate>
{
public:
  VTKM_CONT
  Triangulate();

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

  // Map new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::worklet::Triangulate Worklet;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/Triangulate.hxx>

#endif // vtk_m_filter_Triangulate_h
