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
#include <vtkm/worklet/particleadvection/Stepper.h>
#include <vtkm/worklet/particleadvection/TemporalGridEvaluators.h>

#include <memory>
#include <vector>

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

template <typename GridEvalType>
class VTKM_ALWAYS_EXPORT DataSetIntegratorBase
{
public:
  DataSetIntegratorBase(bool copySeeds = false, vtkm::Id id = -1)
    : CopySeedArray(copySeeds)
    , Eval(nullptr)
    , ID(id)
  {
  }

  ~DataSetIntegratorBase() = default;

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
    Stepper rk4(*this->Eval, stepSize);
    this->DoAdvect(seedArray, rk4, maxSteps, result);
  }

protected:
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;
  using Stepper = vtkm::worklet::particleadvection::Stepper<RK4Type, GridEvalType>;
  using FieldHandleType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

  template <typename ResultType>
  inline void DoAdvect(vtkm::cont::ArrayHandle<vtkm::Particle>& seeds,
                       const Stepper& rk4,
                       vtkm::Id maxSteps,
                       ResultType& result) const;

  FieldHandleType GetFieldHandle(const vtkm::cont::DataSet& ds, const std::string& fieldNm)
  {
    if (!ds.HasField(fieldNm))
      throw vtkm::cont::ErrorFilterExecution("Field " + fieldNm + " not found on dataset.");

    FieldHandleType fieldArray;
    auto fieldData = ds.GetField(fieldNm).GetData();
    vtkm::cont::ArrayCopyShallowIfPossible(fieldData, fieldArray);

    return fieldArray;
  }

  bool CopySeedArray;
  std::shared_ptr<GridEvalType> Eval;
  vtkm::Id ID;
};

class VTKM_ALWAYS_EXPORT DataSetIntegrator
  : public DataSetIntegratorBase<vtkm::worklet::particleadvection::GridEvaluator<
      vtkm::worklet::particleadvection::VelocityField<vtkm::cont::ArrayHandle<vtkm::Vec3f>>>>
{
public:
  DataSetIntegrator(const vtkm::cont::DataSet& ds, vtkm::Id id, const std::string& fieldNm)
    : DataSetIntegratorBase<vtkm::worklet::particleadvection::GridEvaluator<
        vtkm::worklet::particleadvection::VelocityField<FieldHandleType>>>(false, id)
  {
    auto fieldArray = this->GetFieldHandle(ds, fieldNm);

    using EvalType = vtkm::worklet::particleadvection::GridEvaluator<
      vtkm::worklet::particleadvection::VelocityField<FieldHandleType>>;

    this->Eval = std::shared_ptr<EvalType>(new EvalType(ds, fieldArray));
  }
};

class VTKM_ALWAYS_EXPORT TemporalDataSetIntegrator
  : public DataSetIntegratorBase<vtkm::worklet::particleadvection::TemporalGridEvaluator<
      vtkm::worklet::particleadvection::VelocityField<vtkm::cont::ArrayHandle<vtkm::Vec3f>>>>
{
  using FieldHandleType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

public:
  TemporalDataSetIntegrator(const vtkm::cont::DataSet& ds1,
                            vtkm::FloatDefault t1,
                            const vtkm::cont::DataSet& ds2,
                            vtkm::FloatDefault t2,
                            vtkm::Id id,
                            const std::string& fieldNm)
    : DataSetIntegratorBase<vtkm::worklet::particleadvection::TemporalGridEvaluator<
        vtkm::worklet::particleadvection::VelocityField<FieldHandleType>>>(false, id)
  {
    auto fieldArray1 = this->GetFieldHandle(ds1, fieldNm);
    auto fieldArray2 = this->GetFieldHandle(ds2, fieldNm);

    using EvalType = vtkm::worklet::particleadvection::TemporalGridEvaluator<
      vtkm::worklet::particleadvection::VelocityField<FieldHandleType>>;

    this->Eval =
      std::shared_ptr<EvalType>(new EvalType(ds1, t1, fieldArray1, ds2, t2, fieldArray2));
  }
};

}
}
} // namespace vtkm::filter::particleadvection

#ifndef vtk_m_filter_particleadvection_DataSetIntegrator_hxx
#include <vtkm/filter/particleadvection/DataSetIntegrator.hxx>
#endif

#endif //vtk_m_filter_DataSetIntegrator_h
