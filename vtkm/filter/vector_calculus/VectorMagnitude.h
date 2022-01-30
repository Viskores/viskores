//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_vector_calculus_VectorMagnitude_h
#define vtk_m_filter_vector_calculus_VectorMagnitude_h

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/vector_calculus/vtkm_filter_vector_calculus_export.h>

namespace vtkm
{
namespace filter
{
namespace vector_calculus
{
class VTKM_FILTER_VECTOR_CALCULUS_EXPORT VectorMagnitude : public vtkm::filter::NewFilterField
{
public:
  VectorMagnitude();

private:
  //currently, the VectorMagnitude filter only works on vector data.
  using SupportedTypes = vtkm::TypeListVecCommon;

  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
} // namespace vector_calculus
} // namespace filter
} // namespace vtkm::filter

#endif // vtk_m_filter_vector_calculus_VectorMagnitude_h
