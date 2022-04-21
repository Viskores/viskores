//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_density_estimate_Entropy_h
#define vtk_m_filter_density_estimate_Entropy_h

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/density_estimate/vtkm_filter_density_estimate_export.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
/// \brief Construct the entropy histogram of a given Field
///
/// Construct a histogram which is used to compute the entropy with a default of 10 bins
///
class VTKM_FILTER_DENSITY_ESTIMATE_EXPORT Entropy : public vtkm::filter::NewFilterField
{
public:
  //currently the Entropy filter only works on scalar data.
  using SupportedTypes = TypeListScalarAll;

  //Construct a histogram which is used to compute the entropy with a default of 10 bins
  VTKM_CONT
  Entropy();

  VTKM_CONT
  void SetNumberOfBins(vtkm::Id count) { this->NumberOfBins = count; }
  VTKM_CONT
  vtkm::Id GetNumberOfBins() const { return this->NumberOfBins; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::Id NumberOfBins = 10;
};
} // namespace density_estimate
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::density_estimate::Entropy.") Entropy
  : public vtkm::filter::density_estimate::Entropy
{
  using density_estimate::Entropy::Entropy;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_density_estimate_Entropy_h
