//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_density_estimate_NDEntropy_h
#define vtk_m_filter_density_estimate_NDEntropy_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/density_estimate/vtkm_filter_density_estimate_export.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
/// \brief Calculate the entropy of input N-Dims fields
///
/// This filter calculate the entropy of input N-Dims fields.
///
class VTKM_FILTER_DENSITY_ESTIMATE_EXPORT NDEntropy : public vtkm::filter::FilterField
{
public:
  VTKM_CONT
  void AddFieldAndBin(const std::string& fieldName, vtkm::Id numOfBins);

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  std::vector<vtkm::Id> NumOfBins;
  std::vector<std::string> FieldNames;
};
} // namespace density_estimate
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_density_estimate_NDEntropy_h
