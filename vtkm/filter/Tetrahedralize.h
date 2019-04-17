//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Tetrahedralize_h
#define vtk_m_filter_Tetrahedralize_h

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/Tetrahedralize.h>

namespace vtkm
{
namespace filter
{

class Tetrahedralize : public vtkm::filter::FilterDataSet<Tetrahedralize>
{
public:
  VTKM_CONT
  Tetrahedralize();

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  // Map new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::worklet::Tetrahedralize Worklet;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/Tetrahedralize.hxx>

#endif // vtk_m_filter_Tetrahedralize_h
