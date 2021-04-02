//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_PointAverage_h
#define vtk_m_filter_PointAverage_h

#include <vtkm/filter/vtkm_filter_extra_export.h>

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetExtrude.h>
#include <vtkm/filter/FilterField.h>
#include <vtkm/worklet/PointAverage.h>

namespace vtkm
{
namespace filter
{
/// \brief Cell to Point interpolation filter.
///
/// PointAverage is a filter that transforms cell data (i.e., data
/// specified per cell) into point data (i.e., data specified at cell
/// points). The method of transformation is based on averaging the data
/// values of all cells using a particular point.
class VTKM_FILTER_COMMON_EXPORT PointAverage : public vtkm::filter::FilterField<PointAverage>
{
public:
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

  // PointAverage is a simple filter that is used to test custom filter types.
  using AdditionalFieldStorage = vtkm::List<vtkm::cont::StorageTagXGCCoordinates>;
  using SupportedCellSets =
    vtkm::ListAppend<vtkm::List<vtkm::cont::CellSetExtrude>, VTKM_DEFAULT_CELL_SET_LIST>;

private:
  vtkm::worklet::PointAverage Worklet;
};

#ifndef vtkm_filter_PointAverage_cxx
VTKM_FILTER_COMMON_EXPORT_EXECUTE_METHOD(PointAverage);
#endif
}
} // namespace vtkm::filter

#include <vtkm/filter/PointAverage.hxx>

#endif // vtk_m_filter_PointAverage_h
