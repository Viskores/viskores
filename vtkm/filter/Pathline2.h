//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Pathline2_h
#define vtk_m_filter_Pathline2_h

#include <vtkm/Particle.h>
#include <vtkm/filter/FilterParticleAdvection.h>
#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/particleadvection/ParticleAdvectionTypes.h>

namespace vtkm
{
namespace filter
{
/// \brief Advect particles in a vector field.

/// Takes as input a vector field and seed locations and generates the
/// end points for each seed through the vector field.

class Pathline2 : public vtkm::filter::FilterTemporalParticleAdvection<Pathline2, vtkm::Particle>
{
public:
  VTKM_CONT Pathline2();

  template <typename DerivedPolicy>
  vtkm::cont::PartitionedDataSet PrepareForExecution(
    const vtkm::cont::PartitionedDataSet& input,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  vtkm::cont::UnknownArrayHandle SeedArray;

  void SetPreviousTime(vtkm::FloatDefault t1) { this->Time1 = t1; }
  void SetNextTime(vtkm::FloatDefault t2) { this->Time2 = t2; }
  void SetNextDataSet(const vtkm::cont::DataSet& ds) { this->DataSet2 = { ds }; }
  void SetNextDataSet(const vtkm::cont::PartitionedDataSet& pds) { this->DataSet2 = pds; }

  vtkm::cont::PartitionedDataSet DataSet2;
  vtkm::FloatDefault Time1;
  vtkm::FloatDefault Time2;
};

class Pathline3 : public vtkm::filter::NewFilterField
{
public:
  //  VTKM_CONT Pathline3() {}

protected:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override;
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& inData) override;

  vtkm::cont::UnknownArrayHandle SeedArray;
};

}
} // namespace vtkm::filter

#ifndef vtk_m_filter_Pathline2_hxx
#include <vtkm/filter/Pathline2.hxx>
#endif

#endif // vtk_m_filter_Pathline2_h
