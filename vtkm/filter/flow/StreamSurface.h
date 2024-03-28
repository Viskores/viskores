//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_StreamSurface_h
#define vtk_m_filter_flow_StreamSurface_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/flow/FlowTypes.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

/// \brief Generate stream surfaces from a vector field.
///
/// This filter takes as input a velocity vector field and seed locations. The seed locations
/// should be arranged in a line or curve. The filter then traces the path each seed point
/// would take if moving at the velocity specified by the field and connects all the lines
/// together into a surface. Mathematically, this is the surface that is tangent to the
/// velocity field everywhere.
///
/// The output of this filter is a `vtkm::cont::DataSet` containing a mesh for the created
/// surface.
class VTKM_FILTER_FLOW_EXPORT StreamSurface : public vtkm::filter::Filter
{
public:
  /// @brief Specifies the step size used for the numerical integrator.
  ///
  /// The numerical integrators operate by advancing each particle by a finite amount.
  /// This parameter defines the distance to advance each time. Smaller values are
  /// more accurate but take longer to integrate. An appropriate step size is usually
  /// around the size of each cell.
  VTKM_CONT void SetStepSize(vtkm::FloatDefault s) { this->StepSize = s; }

  /// @brief Specifies the maximum number of integration steps for each particle.
  ///
  /// Some particle paths may loop and continue indefinitely. This parameter sets an upper
  /// limit on the total length of advection.
  VTKM_CONT void SetNumberOfSteps(vtkm::Id n) { this->NumberOfSteps = n; }

  /// @brief Specify the seed locations for the particle advection.
  ///
  /// Each seed represents one particle that is advected by the vector field.
  /// The particles are represented by a `vtkm::Particle` object.
  template <typename ParticleType>
  VTKM_CONT void SetSeeds(vtkm::cont::ArrayHandle<ParticleType>& seeds)
  {
    this->Seeds = seeds;
  }

  /// @copydoc SetSeeds
  template <typename ParticleType>
  VTKM_CONT void SetSeeds(const std::vector<ParticleType>& seeds,
                          vtkm::CopyFlag copyFlag = vtkm::CopyFlag::On)
  {
    this->Seeds = vtkm::cont::make_ArrayHandle(seeds, copyFlag);
  }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override;

  vtkm::Id NumberOfSteps = 0;
  vtkm::cont::UnknownArrayHandle Seeds;
  vtkm::FloatDefault StepSize = 0;
};

}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_StreamSurface_h
