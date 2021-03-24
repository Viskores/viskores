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

#include <vtkm/Particle.h>
#include <vtkm/filter/FilterDataSetWithField.h>

namespace vtkm
{
namespace filter
{
/// \brief generate streamlines from a vector field.

/// Takes as input a vector field and seed locations and generates the
/// paths taken by the seeds through the vector field.
class Streamline : public vtkm::filter::FilterDataSetWithField<Streamline>
{
public:
  using SupportedTypes = vtkm::TypeListFieldVec3;

  VTKM_CONT
  Streamline();

  VTKM_CONT
  void SetStepSize(vtkm::FloatDefault s) { this->StepSize = s; }

  VTKM_CONT
  void SetNumberOfSteps(vtkm::Id n) { this->NumberOfSteps = n; }

  VTKM_CONT
  void SetSeeds(vtkm::cont::ArrayHandle<vtkm::Particle>& seeds);

  VTKM_CONT
  bool GetUseThreadedAlgorithm() { return this->UseThreadedAlgorithm; }

  VTKM_CONT
  void SetUseThreadedAlgorithm(bool val) { this->UseThreadedAlgorithm = val; }

  template <typename DerivedPolicy>
  vtkm::cont::PartitionedDataSet PrepareForExecution(
    const vtkm::cont::PartitionedDataSet& input,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::Id NumberOfSteps;
  vtkm::FloatDefault StepSize;
  vtkm::cont::ArrayHandle<vtkm::Particle> Seeds;
  bool UseThreadedAlgorithm;
};
}
} // namespace vtkm::filter

#ifndef vtk_m_filter_Streamline_hxx
#include <vtkm/filter/Streamline.hxx>
#endif

#endif // vtk_m_filter_Streamline_h
