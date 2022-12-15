//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_vector_analysis_VectorMagnitude_h
#define vtk_m_filter_vector_analysis_VectorMagnitude_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/vector_analysis/vtkm_filter_vector_analysis_export.h>

namespace vtkm
{
namespace filter
{
namespace vector_analysis
{
class VTKM_FILTER_VECTOR_ANALYSIS_EXPORT VectorMagnitude : public vtkm::filter::FilterField
{
public:
  VectorMagnitude();

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
} // namespace vector_analysis
} // namespace filter
} // namespace vtkm::filter

#endif // vtk_m_filter_vector_analysis_VectorMagnitude_h
