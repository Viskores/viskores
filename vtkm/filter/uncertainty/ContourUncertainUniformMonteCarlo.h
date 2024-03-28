//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_uncertainty_ContourUncertainUniformMonteCarlo_h
#define vtk_m_filter_uncertainty_ContourUncertainUniformMonteCarlo_h
#include <vtkm/filter/Filter.h>
#include <vtkm/filter/uncertainty/vtkm_filter_uncertainty_export.h>

namespace vtkm
{
namespace filter
{
namespace uncertainty
{
/// \brief Visualize isosurface uncertainty using Monte Carlo approach for uniformly distributed data.
///
/// This filter is implemented to validate the correctness of the ContourUncertainUniform filter.
/// We encourage usage of the ContourUncertainUniform filter because the Monte Carlo approach implemented
/// in this filter is computationally inefficient.
///
class VTKM_FILTER_UNCERTAINTY_EXPORT ContourUncertainUniformMonteCarlo : public vtkm::filter::Filter
{
  std::string NumberNonzeroProbabilityName = "num_nonzero_probability";
  std::string EntropyName = "entropy";
  vtkm::Float64 IsoValue = 0.0;
  vtkm::IdComponent IterValue = 1;

public:
  VTKM_CONT ContourUncertainUniformMonteCarlo();

  VTKM_CONT void SetMinField(const std::string& fieldName)
  {
    this->SetActiveField(0, fieldName, vtkm::cont::Field::Association::Points);
  }
  VTKM_CONT void SetMaxField(const std::string& fieldName)
  {
    this->SetActiveField(1, fieldName, vtkm::cont::Field::Association::Points);
  }
  VTKM_CONT void SetIsoValue(vtkm::Float64 value) { this->IsoValue = value; }
  VTKM_CONT vtkm::Float64 GetIsoValue() const { return this->IsoValue; }

  VTKM_CONT void SetNumSample(vtkm::IdComponent value) { this->IterValue = value; }
  VTKM_CONT vtkm::IdComponent GetNumSample() const { return this->IterValue; }

  VTKM_CONT void SetCrossProbabilityName(const std::string& name)
  {
    this->SetOutputFieldName(name);
  }
  VTKM_CONT const std::string& GetCrossProbabilityName() const
  {
    return this->GetOutputFieldName();
  }

  VTKM_CONT void SetNumberNonzeroProbabilityName(const std::string& name)
  {
    this->NumberNonzeroProbabilityName = name;
  }
  VTKM_CONT const std::string& GetNumberNonzeroProbabilityName() const
  {
    return this->NumberNonzeroProbabilityName;
  }
  VTKM_CONT void SetEntropyName(const std::string& name) { this->EntropyName = name; }
  VTKM_CONT const std::string& GetEntropyName() const { return this->EntropyName; }

protected:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
}
}
}
#endif
