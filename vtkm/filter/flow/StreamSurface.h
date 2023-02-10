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

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/flow/FlowTypes.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

/// \brief generate streamlines from a vector field.

/// Takes as input a vector field and seed locations and generates the
/// paths taken by the seeds through the vector field.

class VTKM_FILTER_FLOW_EXPORT StreamSurface : public vtkm::filter::FilterField
{
public:
  VTKM_CONT
  void SetStepSize(vtkm::FloatDefault s) { this->StepSize = s; }

  VTKM_CONT
  void SetNumberOfSteps(vtkm::Id n) { this->NumberOfSteps = n; }

  template <typename ParticleType>
  VTKM_CONT void SetSeeds(vtkm::cont::ArrayHandle<ParticleType>& seeds)
  {
    this->Seeds = seeds;
  }

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
