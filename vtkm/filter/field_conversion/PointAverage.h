//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_field_conversion_PointAverage_h
#define vtk_m_filter_field_conversion_PointAverage_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/field_conversion/vtkm_filter_field_conversion_export.h>

namespace vtkm
{
namespace filter
{
namespace field_conversion
{
/// \brief Cell to Point interpolation filter.
///
/// PointAverage is a filter that transforms cell data (i.e., data
/// specified per cell) into point data (i.e., data specified at cell
/// points). The method of transformation is based on averaging the data
/// values of all cells using a particular point.
///
/// The cell field to convert comes from the active scalars.
/// The default name for the output cell field is the same name as the input
/// point field. The name can be overridden as always using the
/// `SetOutputFieldName()` method.
///
class VTKM_FILTER_FIELD_CONVERSION_EXPORT PointAverage : public vtkm::filter::Filter
{
private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
} // namespace field_conversion
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_field_conversion_PointAverage_h
