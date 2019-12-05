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
  using FieldTypeList = vtkm::List<vtkm::UInt8>;
};

class GhostCellClassify : public vtkm::filter::FilterDataSet<GhostCellClassify>
{
public:
  using SupportedTypes = vtkm::List<vtkm::UInt8>;

  VTKM_CONT
  GhostCellClassify();

  template <typename Policy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData,
                                          vtkm::filter::PolicyBase<Policy> policy);

  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    const vtkm::filter::PolicyBase<DerivedPolicy>&)
  {
    result.AddField(field);
    return true;
  }

private:
};
}
} // namespace vtkm::filter

#include <vtkm/filter/GhostCellClassify.hxx>

#endif //vtk_m_filter_GhostCellClassify_h
