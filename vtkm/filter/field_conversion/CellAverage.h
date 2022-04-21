//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_field_conversion_CellAverage_h
#define vtk_m_filter_field_conversion_CellAverage_h

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/field_conversion/vtkm_filter_field_conversion_export.h>

namespace vtkm
{
namespace filter
{
namespace field_conversion
{
/// \brief  Point to cell interpolation filter.
///
/// CellAverage is a filter that transforms point data (i.e., data
/// specified at cell points) into cell data (i.e., data specified per cell).
/// The method of transformation is based on averaging the data
/// values of all points used by particular cell.
///
class VTKM_FILTER_FIELD_CONVERSION_EXPORT CellAverage : public vtkm::filter::NewFilterField
{
private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
} // namespace field_conversion
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::field_conversion::CellAverage.") CellAverage
  : public vtkm::filter::field_conversion::CellAverage
{
  using field_conversion::CellAverage::CellAverage;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_field_conversion_CellAverage_h
