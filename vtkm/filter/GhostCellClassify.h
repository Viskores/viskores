//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_GhostCellClassify_h
#define vtk_m_filter_GhostCellClassify_h

#include <vtkm/filter/FilterDataSet.h>

namespace vtkm
{
namespace filter
{

struct GhostCellClassifyPolicy : vtkm::filter::PolicyBase<GhostCellClassifyPolicy>
{
  using FieldTypeList = vtkm::ListTagBase<vtkm::UInt8>;
};

class GhostCellClassify : public vtkm::filter::FilterDataSet<GhostCellClassify>
{
public:
  VTKM_CONT
  GhostCellClassify();

  template <typename Policy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData,
                                          vtkm::filter::PolicyBase<Policy> policy);

  template <typename ValueType, typename Storage, typename Policy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<ValueType, Storage>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<Policy>);

private:
};
}
} // namespace vtkm::filter

#include <vtkm/filter/GhostCellClassify.hxx>

#endif //vtk_m_filter_GhostCellClassify_h
