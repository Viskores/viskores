//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_resampling_HistSampling_h
#define vtk_m_filter_resampling_HistSampling_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/resampling/vtkm_filter_resampling_export.h>

namespace vtkm
{
namespace filter
{
namespace resampling
{
// This filter can sample particles according to its importance level
// The source code of this filter comes from
// https://github.com/Alpine-DAV/ascent/blob/develop/src/libs/vtkh/filters/HistSampling.cpp
// More details can be found in the following paper:
// "In Situ Data-Driven Adaptive Sampling for Large-scale Simulation Data Summarization",
// Ayan Biswas, Soumya Dutta, Jesus Pulido, and James Ahrens, In Situ Infrastructures for Enabling Extreme-scale Analysis and Visualization (ISAV 2018), co-located with Supercomputing 2018
class VTKM_FILTER_RESAMPLING_EXPORT HistSampling : public vtkm::filter::FilterField
{
public:
  VTKM_CONT void SetNumberOfBins(vtkm::Id numberOfBins) { this->NumberOfBins = numberOfBins; }
  VTKM_CONT vtkm::Id GetNumberOfBins() { return this->NumberOfBins; }
  VTKM_CONT void SetSamplePercent(vtkm::FloatDefault samplePercent)
  {
    this->SamplePercent = samplePercent;
  }
  VTKM_CONT vtkm::FloatDefault GetSamplePercent() { return this->SamplePercent; }
  VTKM_CONT vtkm::UInt32 GetSeed() { return this->Seed; }
  VTKM_CONT void SetSeed(vtkm::UInt32 seed) { this->Seed = seed; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
  vtkm::Id NumberOfBins;
  vtkm::FloatDefault SamplePercent = static_cast<vtkm::FloatDefault>(0.1);
  vtkm::UInt32 Seed = 0;
};
} // namespace resampling
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_resampling_HistSampling_h
