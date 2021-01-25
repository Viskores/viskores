//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_DataSetIntegrator_h
#define vtk_m_filter_DataSetIntegrator_h

#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/worklet/ParticleAdvection.h>
#include <vtkm/worklet/particleadvection/RK4Integrator.h>

#include <memory>
#include <vector>

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

class VTKM_ALWAYS_EXPORT DataSetIntegrator
{
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::particleadvection::VelocityField<FieldHandle>;
  using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;

public:
  DataSetIntegrator(const vtkm::cont::DataSet& ds, vtkm::Id id, std::string fieldNm)
    : ActiveField(ds.GetField(fieldNm))
    , CopySeedArray(false)
    , Eval(nullptr)
    , ID(id)
  {
    auto fieldData = this->ActiveField.GetData();
    FieldHandle fieldArray;

    if (fieldData.IsType<FieldHandle>())
      fieldArray = fieldData.AsArrayHandle<FieldHandle>();
    else
      vtkm::cont::ArrayCopy(
        fieldData.ResetTypes<vtkm::TypeListFieldVec3, VTKM_DEFAULT_STORAGE_LIST>(), fieldArray);

    this->Eval = std::shared_ptr<GridEvalType>(new GridEvalType(ds, fieldArray));
  }

  vtkm::Id GetID() const { return this->ID; }
  void SetCopySeedFlag(bool val) { this->CopySeedArray = val; }

  template <typename ResultType>
  void Advect(std::vector<vtkm::Particle>& v,
              vtkm::FloatDefault stepSize,
              vtkm::Id maxSteps,
              ResultType& result) const
  {
    auto copyFlag = (this->CopySeedArray ? vtkm::CopyFlag::On : vtkm::CopyFlag::Off);
    auto seedArray = vtkm::cont::make_ArrayHandle(v, copyFlag);
    RK4Type rk4(*this->Eval, stepSize);
    this->DoAdvect(seedArray, rk4, maxSteps, result);
  }

private:
  template <typename ResultType>
  inline void DoAdvect(vtkm::cont::ArrayHandle<vtkm::Particle>& seeds,
                       const RK4Type& rk4,
                       vtkm::Id maxSteps,
                       ResultType& result) const;

  vtkm::cont::Field ActiveField;
  bool CopySeedArray;
  std::shared_ptr<GridEvalType> Eval;
  vtkm::Id ID;
};

//-----
// Specialization for ParticleAdvection worklet
template <>
inline void DataSetIntegrator::DoAdvect(
  vtkm::cont::ArrayHandle<vtkm::Particle>& seeds,
  const RK4Type& rk4,
  vtkm::Id maxSteps,
  vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>& result) const
{
  vtkm::worklet::ParticleAdvection Worklet;
  result = Worklet.Run(rk4, seeds, maxSteps);
}

//-----
// Specialization for Streamline worklet
template <>
inline void DataSetIntegrator::DoAdvect(
  vtkm::cont::ArrayHandle<vtkm::Particle>& seeds,
  const RK4Type& rk4,
  vtkm::Id maxSteps,
  vtkm::worklet::StreamlineResult<vtkm::Particle>& result) const
{
  vtkm::worklet::Streamline Worklet;
  result = Worklet.Run(rk4, seeds, maxSteps);
}
}
}
} // namespace vtkm::filter::particleadvection

#endif
