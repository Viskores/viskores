//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_DataSetIntegratorSteadyState_h
#define vtk_m_filter_flow_internal_DataSetIntegratorSteadyState_h

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/filter/flow/internal/DataSetIntegrator.h>

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
class AdvectHelperSteadyState
{
public:
  using WorkletType = vtkm::worklet::flow::ParticleAdvection;
  using SteadyStateGridEvalType = vtkm::worklet::flow::GridEvaluator<FieldType>;

  template <template <typename> class SolverType>
  static void DoAdvect(vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                       const FieldType& field,
                       const vtkm::cont::DataSet& dataset,
                       const TerminationType& termination,
                       vtkm::FloatDefault stepSize,
                       AnalysisType& analysis)
  {
    using StepperType =
      vtkm::worklet::flow::Stepper<SolverType<SteadyStateGridEvalType>, SteadyStateGridEvalType>;
    SteadyStateGridEvalType eval(dataset, field);
    StepperType stepper(eval, stepSize);

    WorkletType worklet;
    worklet.Run(stepper, seedArray, termination, analysis);
  }

  static void Advect(vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                     const FieldType& field,
                     const vtkm::cont::DataSet& dataset,
                     const TerminationType& termination,
                     const IntegrationSolverType& solverType,
                     vtkm::FloatDefault stepSize,
                     AnalysisType& analysis)
  {
    if (solverType == IntegrationSolverType::RK4_TYPE)
    {
      DoAdvect<vtkm::worklet::flow::RK4Integrator>(
        seedArray, field, dataset, termination, stepSize, analysis);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::flow::EulerIntegrator>(
        seedArray, field, dataset, termination, stepSize, analysis);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }
};
} // namespace detail

template <typename ParticleType,
          typename FieldType,
          typename TerminationType,
          typename AnalysisType>
class DataSetIntegratorSteadyState
  : public vtkm::filter::flow::internal::DataSetIntegrator<
      DataSetIntegratorSteadyState<ParticleType, FieldType, TerminationType, AnalysisType>,
      ParticleType>
{
public:
  using BaseType = vtkm::filter::flow::internal::DataSetIntegrator<
    DataSetIntegratorSteadyState<ParticleType, FieldType, TerminationType, AnalysisType>,
    ParticleType>;
  using PType = ParticleType;
  using FType = FieldType;
  using TType = TerminationType;
  using AType = AnalysisType;

  DataSetIntegratorSteadyState(vtkm::Id id,
                               const FieldType& field,
                               const vtkm::cont::DataSet& dataset,
                               vtkm::filter::flow::IntegrationSolverType solverType,
                               const TerminationType& termination,
                               const AnalysisType& analysis)
    : BaseType(id, solverType)
    , Field(field)
    , Dataset(dataset)
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
      detail::AdvectHelperSteadyState<ParticleType, FieldType, TerminationType, AnalysisType>;
    AnalysisType analysis;
    analysis.UseAsTemplate(this->Analysis);

    AdvectionHelper::Advect(seedArray,
                            this->Field,
                            this->Dataset,
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
  FieldType Field;
  vtkm::cont::DataSet Dataset;
  TerminationType Termination;
  // Used as a template to initialize successive analysis objects.
  AnalysisType Analysis;
  std::vector<AnalysisType> Analyses;
};

}
}
}
} //vtkm::filter::flow::internal

#endif //vtk_m_filter_flow_internal_DataSetIntegratorSteadyState_h
