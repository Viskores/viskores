//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_PathParticle_h
#define vtk_m_filter_PathParticle_h

#include <vtkm/filter/FilterTemporalParticleAdvection.h>

namespace vtkm
{
namespace filter
{
/// \brief Advect particles in a time varying vector field.

/// Takes as input a vector field and seed locations and generates the
/// paths taken by the seeds through the vector field.
class PathParticle : public vtkm::filter::FilterTemporalParticleAdvection<PathParticle>
{
public:
  VTKM_CONT
  PathParticle();

  template <typename DerivedPolicy>
  vtkm::cont::PartitionedDataSet PrepareForExecution(
    const vtkm::cont::PartitionedDataSet& input,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

protected:
private:
};

}
} // namespace vtkm::filter

#ifndef vtk_m_filter_PathParticle_hxx
#include <vtkm/filter/PathParticle.hxx>
#endif

#endif // vtk_m_filter_PathParticle_h
