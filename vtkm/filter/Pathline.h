//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Pathline_h
#define vtk_m_filter_Pathline_h

#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/worklet/ParticleAdvection.h>
#include <vtkm/worklet/particleadvection/GridEvaluators.h>
#include <vtkm/worklet/particleadvection/Integrators.h>

namespace vtkm
{
namespace filter
{
/// \brief generate streamlines from a vector field.

/// Takes as input a vector field and seed locations and generates the
/// paths taken by the seeds through the vector field.
class Pathline : public vtkm::filter::FilterDataSetWithField<Pathline>
{
public:
  VTKM_CONT
  Pathline();

  VTKM_CONT
  void SetPreviousTime(vtkm::worklet::particleadvection::ScalarType t) { this->PreviousTime = t; }
  VTKM_CONT
  void SetNextTime(vtkm::worklet::particleadvection::ScalarType t) { this->NextTime = t; }

  VTKM_CONT
  void SetNextDataSet(const vtkm::cont::DataSet& ds) { this->NextDataSet = ds; }

  VTKM_CONT
  void SetStepSize(vtkm::worklet::particleadvection::ScalarType s) { this->StepSize = s; }

  VTKM_CONT
  void SetNumberOfSteps(vtkm::Id n) { this->NumberOfSteps = n; }

  VTKM_CONT
  void SetSeeds(vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& seeds);

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
    const vtkm::filter::FieldMetadata& fieldMeta,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::worklet::Streamline Worklet;
  vtkm::worklet::particleadvection::ScalarType StepSize;
  vtkm::worklet::particleadvection::ScalarType PreviousTime;
  vtkm::worklet::particleadvection::ScalarType NextTime;
  vtkm::cont::DataSet NextDataSet;
  vtkm::Id NumberOfSteps;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> Seeds;
};

template <>
class FilterTraits<Pathline>
{
public:
  struct TypeListTagPathline
    : vtkm::ListTagBase<vtkm::Vec<vtkm::Float32, 3>, vtkm::Vec<vtkm::Float64, 3>>
  {
  };
  using InputFieldTypeList = TypeListTagPathline;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/Pathline.hxx>

#endif // vtk_m_filter_Pathline_h
