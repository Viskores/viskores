//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

//  This code is based on the algorithm presented in the following papers:
//  Wang, J., Athawale, T., Moreland, K., Chen, J., Johnson, C., & Pugmire,
//  D. (2023). FunMC^ 2: A Filter for Uncertainty Visualization of Marching
//  Cubes on Multi-Core Devices. Oak Ridge National Laboratory (ORNL),
//  Oak Ridge, TN (United States).
//
//  Athawale, T. M., Sane, S., & Johnson, C. R. (2021, October). Uncertainty
//  Visualization of the Marching Squares and Marching Cubes Topology Cases.
//  In 2021 IEEE Visualization Conference (VIS) (pp. 106-110). IEEE.

#ifndef vtk_m_filter_uncertainty_ContourUncertainUniform_h
#define vtk_m_filter_uncertainty_ContourUncertainUniform_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/uncertainty/vtkm_filter_uncertainty_export.h>

namespace vtkm
{
namespace filter
{
namespace uncertainty
{
/// @brief Visualize isosurface uncertainty for uniform distributed data.
///
/// This filter computes the positional uncertainty of isosurfaces as a
/// function of uncertainty in input data, where the data are assumed to
/// be uniformly distributed and sampled on a regular grid. The uniform
/// distribution range is given through the input datasets via the minimum
/// and maximum fields. Given the uniform distribution range, the computed
/// isosurface uncertainty corresponds to uncertainty in topology cases in
/// the marching cubes algorithm.
///
class VTKM_FILTER_UNCERTAINTY_EXPORT ContourUncertainUniform : public vtkm::filter::Filter
{

  std::string NumberNonzeroProbabilityName = "num_nonzero_probability";
  std::string EntropyName = "entropy";
  vtkm::Float64 IsoValue = 0.0;

public:
  /// @brief Constructor
  VTKM_CONT ContourUncertainUniform();

  /// @brief Sets minimum field.
  /// Sets minimum value of uniform distribution at each grid point.
  VTKM_CONT void SetMinField(const std::string& fieldName)
  {
    this->SetActiveField(0, fieldName, vtkm::cont::Field::Association::Points);
  }

  /// @brief Sets maximum field.
  /// Sets maximum value of uniform distribution at each grid point.
  VTKM_CONT void SetMaxField(const std::string& fieldName)
  {
    this->SetActiveField(1, fieldName, vtkm::cont::Field::Association::Points);
  }

  /// @brief Sets isovalue.
  /// Sets isovalue for extracting isosurfaces.
  VTKM_CONT void SetIsoValue(vtkm::Float64 value) { this->IsoValue = value; }

  /// @brief Gets isovalue
  /// Gets isovalue used for visualizing isosurfaces
  VTKM_CONT vtkm::Float64 GetIsoValue() const { return this->IsoValue; }

  /// @brief Sets crossing probability field (uncertainty field type 1).
  /// Sets the output field name that stores isosurface crossing probabiliy for each grid cell.
  VTKM_CONT void SetCrossProbabilityName(const std::string& name)
  {
    this->SetOutputFieldName(name);
  }

  /// @brief Gets crossing probability field (uncertainty field type 1).
  /// Gets the output field name that stores isosurface crossing probability for each grid cell.
  VTKM_CONT const std::string& GetCrossProbabilityName() const
  {
    return this->GetOutputFieldName();
  }

  /// @brief Sets toplogy case count field (uncertainty field type 2).
  /// Sets the output field name that stores the number of marching cubes toplogy cases for each grid cell.
  VTKM_CONT void SetNumberNonzeroProbabilityName(const std::string& name)
  {
    this->NumberNonzeroProbabilityName = name;
  }

  /// @brief Gets toplogy case count field (uncertainty field type 2.
  /// Gets the output field name that stores the number of marching cubes toplogy cases for each grid cell.
  VTKM_CONT const std::string& GetNumberNonzeroProbabilityName() const
  {
    return this->NumberNonzeroProbabilityName;
  }

  /// @brief Sets entropy field. (uncertainty field type 3)
  /// Sets the output field name that stores the entropy of a histogram of marching cubes toplogy cases.
  VTKM_CONT void SetEntropyName(const std::string& name) { this->EntropyName = name; }

  /// @brief Gets entropy field. (uncertainty field type 3)
  /// Gets the output field name that stores the entropy of a histogram of marching cubes toplogy cases.
  VTKM_CONT const std::string& GetEntropyName() const { return this->EntropyName; }

protected:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
}
}
}
#endif
