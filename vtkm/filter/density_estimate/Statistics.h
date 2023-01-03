//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_density_estimate_Statistics_h
#define vtk_m_filter_density_estimate_Statistics_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/density_estimate/vtkm_filter_density_estimate_export.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
/// \brief Calculating the statistics of input fields
///
/// This filter calculates the statistics of input fields.
///
class VTKM_FILTER_DENSITY_ESTIMATE_EXPORT Statistics : public vtkm::filter::FilterField
{
public:
  enum struct Stats
  {
    N = 0,
    Min,
    Max,
    Sum,
    Mean,
    SampleStdDev,
    PopulationStdDev,
    SampleVariance,
    PopulationVariance,
    Skewness,
    Kurtosis
  };


  /// \{
  /// \brief The output statistical variables for executing the statistics filter.
  ///
  void SetRequiredStats(const std::vector<Stats> StatsList) { RequiredStatsList = StatsList; }
  const std::vector<Stats>& GetRequiredStats() const { return this->RequiredStatsList; }
  /// \}
private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
  std::vector<Stats> RequiredStatsList{ Stats::N,
                                        Stats::Min,
                                        Stats::Max,
                                        Stats::Sum,
                                        Stats::Mean,
                                        Stats::SampleStdDev,
                                        Stats::PopulationStdDev,
                                        Stats::SampleVariance,
                                        Stats::PopulationVariance,
                                        Stats::Skewness,
                                        Stats::Kurtosis };
  // This string vector stores variables names stored in the output dataset
  std::vector<std::string> StatsName{ "N",
                                      "Min",
                                      "Max",
                                      "Sum",
                                      "Mean",
                                      "SampleStddev",
                                      "PopulationStdDev",
                                      "SampleVariance",
                                      "PopulationVariance",
                                      "Skewness",
                                      "Kurtosis" };
};
} // namespace density_estimate
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_density_estimate_Statistics_h
