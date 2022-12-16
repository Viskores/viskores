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

#include <vtkm/filter/FilterField.h>
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
class VTKM_FILTER_FIELD_CONVERSION_EXPORT PointAverage : public vtkm::filter::FilterField
{
private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
} // namespace field_conversion
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_field_conversion_PointAverage_h
