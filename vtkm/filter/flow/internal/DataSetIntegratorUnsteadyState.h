//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_DataSetIntegratorUnsteadyState_h
#define vtk_m_filter_flow_internal_DataSetIntegratorUnsteadyState_h

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/filter/flow/internal/DataSetIntegrator.h>
#include <vtkm/filter/flow/worklet/TemporalGridEvaluators.h>

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{
namespace detail
{
template <typename ParticleType,
          typename FieldType,
          typename TerminationType,
          typename AnalysisType>
class AdvectHelperUnsteadyState
{
public:
  using WorkletType = vtkm::worklet::flow::ParticleAdvection;
  using UnsteadyStateGridEvalType = vtkm::worklet::flow::TemporalGridEvaluator<FieldType>;

  template <template <typename> class SolverType>
  static void DoAdvect(vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                       const FieldType& field1,
                       const vtkm::cont::DataSet& ds1,
                       vtkm::FloatDefault t1,
                       const FieldType& field2,
                       const vtkm::cont::DataSet& ds2,
                       vtkm::FloatDefault t2,
                       const TerminationType& termination,
                       vtkm::FloatDefault stepSize,
                       AnalysisType& analysis)

  {
    using StepperType = vtkm::worklet::flow::Stepper<SolverType<UnsteadyStateGridEvalType>,
                                                     UnsteadyStateGridEvalType>;
    WorkletType worklet;
    UnsteadyStateGridEvalType eval(ds1, t1, field1, ds2, t2, field2);
    StepperType stepper(eval, stepSize);
    worklet.Run(stepper, seedArray, termination, analysis);
  }

  static void Advect(vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                     const FieldType& field1,
                     const vtkm::cont::DataSet& ds1,
                     vtkm::FloatDefault t1,
                     const FieldType& field2,
                     const vtkm::cont::DataSet& ds2,
                     vtkm::FloatDefault t2,
                     const TerminationType& termination,
                     const IntegrationSolverType& solverType,
                     vtkm::FloatDefault stepSize,
                     AnalysisType& analysis)
  {
    if (solverType == IntegrationSolverType::RK4_TYPE)
    {
      DoAdvect<vtkm::worklet::flow::RK4Integrator>(
        seedArray, field1, ds1, t1, field2, ds2, t2, termination, stepSize, analysis);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::flow::EulerIntegrator>(
        seedArray, field1, ds1, t1, field2, ds2, t2, termination, stepSize, analysis);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }
};
} //namespace detail

template <typename ParticleType,
          typename FieldType,
          typename TerminationType,
          typename AnalysisType>
class DataSetIntegratorUnsteadyState
  : public vtkm::filter::flow::internal::DataSetIntegrator<
      DataSetIntegratorUnsteadyState<ParticleType, FieldType, TerminationType, AnalysisType>,
      ParticleType>
{
public:
  using BaseType = vtkm::filter::flow::internal::DataSetIntegrator<
    DataSetIntegratorUnsteadyState<ParticleType, FieldType, TerminationType, AnalysisType>,
    ParticleType>;
  using PType = ParticleType;
  using FType = FieldType;
  using TType = TerminationType;
  using AType = AnalysisType;

  DataSetIntegratorUnsteadyState(vtkm::Id id,
                                 const FieldType& field1,
                                 const FieldType& field2,
                                 const vtkm::cont::DataSet& ds1,
                                 const vtkm::cont::DataSet& ds2,
                                 vtkm::FloatDefault t1,
                                 vtkm::FloatDefault t2,
                                 vtkm::filter::flow::IntegrationSolverType solverType,
                                 const TerminationType& termination,
                                 const AnalysisType& analysis)
    : BaseType(id, solverType)
    , Field1(field1)
    , Field2(field2)
    , DataSet1(ds1)
    , DataSet2(ds2)
    , Time1(t1)
    , Time2(t2)
    , Termination(termination)
    , Analysis(analysis)
  {
  }

  VTKM_CONT inline void DoAdvect(vtkm::filter::flow::internal::DSIHelperInfo<ParticleType>& block,
                                 vtkm::FloatDefault stepSize)
  {
    auto copyFlag = (this->CopySeedArray ? vtkm::CopyFlag::On : vtkm::CopyFlag::Off);
    auto seedArray = vtkm::cont::make_ArrayHandle(block.Particles, copyFlag);

    using AdvectionHelper =
      detail::AdvectHelperUnsteadyState<ParticleType, FieldType, TerminationType, AnalysisType>;
    AnalysisType analysis;
    analysis.UseAsTemplate(this->Analysis);

    AdvectionHelper::Advect(seedArray,
                            this->Field1,
                            this->DataSet1,
                            this->Time1,
                            this->Field2,
                            this->DataSet2,
                            this->Time2,
                            this->Termination,
                            this->SolverType,
                            stepSize,
                            analysis);
    this->UpdateResult(analysis, block);
  }

  VTKM_CONT void UpdateResult(AnalysisType& analysis,
                              vtkm::filter::flow::internal::DSIHelperInfo<ParticleType>& dsiInfo)
  {
    this->ClassifyParticles(analysis.Particles, dsiInfo);
    if (std::is_same<AnalysisType, vtkm::worklet::flow::NoAnalysis<ParticleType>>::value)
    {
      if (dsiInfo.TermIdx.empty())
        return;
      auto indicesAH = vtkm::cont::make_ArrayHandle(dsiInfo.TermIdx, vtkm::CopyFlag::Off);
      auto termPerm = vtkm::cont::make_ArrayHandlePermutation(indicesAH, analysis.Particles);
      vtkm::cont::ArrayHandle<ParticleType> termParticles;
      vtkm::cont::Algorithm::Copy(termPerm, termParticles);
      analysis.FinalizeAnalysis(termParticles);
      this->Analyses.emplace_back(analysis);
    }
    else
    {
      this->Analyses.emplace_back(analysis);
    }
  }

  VTKM_CONT bool GetOutput(vtkm::cont::DataSet& ds) const
  {
    std::size_t nAnalyses = this->Analyses.size();
    if (nAnalyses == 0)
      return false;
    return AnalysisType::MakeDataSet(ds, this->Analyses);
  }

private:
  FieldType Field1;
  FieldType Field2;
  vtkm::cont::DataSet DataSet1;
  vtkm::cont::DataSet DataSet2;
  vtkm::FloatDefault Time1;
  vtkm::FloatDefault Time2;
  TerminationType Termination;
  AnalysisType Analysis;
  std::vector<AnalysisType> Analyses;
};

}
}
}
} //vtkm::filter::flow::internal

#endif //vtk_m_filter_flow_internal_DataSetIntegratorUnsteadyState_h
