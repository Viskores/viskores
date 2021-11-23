//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Streamline_h
#define vtk_m_filter_Streamline_h

#include <vtkm/filter/FilterParticleAdvection.h>

namespace vtkm
{
namespace filter
{
/// \brief Generate streamlines from a vector field.

/// Takes as input a vector field and seed locations and generates the
/// paths taken by the seeds through the vector field.
template <typename ParticleType = vtkm::Particle>
class StreamlineBase
  : public vtkm::filter::FilterParticleAdvection<StreamlineBase<ParticleType>, ParticleType>
{
public:
  VTKM_CONT
  StreamlineBase();

  template <typename DerivedPolicy>
  vtkm::cont::PartitionedDataSet PrepareForExecution(
    const vtkm::cont::PartitionedDataSet& input,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

private:
};

using Streamline = StreamlineBase<vtkm::Particle>;
}
} // namespace vtkm::filter

#ifndef vtk_m_filter_Streamline_hxx
#include <vtkm/filter/Streamline.hxx>
#endif

#endif // vtk_m_filter_Streamline_h
