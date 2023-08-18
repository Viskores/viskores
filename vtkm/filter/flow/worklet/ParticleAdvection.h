//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_worklet_ParticleAdvection_h
#define vtk_m_filter_flow_worklet_ParticleAdvection_h

#include <vtkm/cont/Invoker.h>
#include <vtkm/filter/flow/worklet/ParticleAdvectionWorklets.h>

namespace vtkm
{
namespace worklet
{
namespace flow
{

namespace detail
{
class CopyToParticle : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature =
    void(FieldIn pt, FieldIn id, FieldIn time, FieldIn step, FieldOut particle);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename ParticleType>
  VTKM_EXEC void operator()(const vtkm::Vec3f& pt,
                            const vtkm::Id& id,
                            const vtkm::FloatDefault& time,
                            const vtkm::Id& step,
                            ParticleType& particle) const
  {
    particle.SetPosition(pt);
    particle.SetID(id);
    particle.SetTime(time);
    particle.SetNumberOfSteps(step);
    particle.GetStatus().SetOk();
  }
};

} //detail

class ParticleAdvection
{
public:
  ParticleAdvection() {}

  template <typename IntegratorType,
            typename ParticleType,
            typename ParticleStorage,
            typename TerminationType,
            typename AnalysisType>
  void Run(const IntegratorType& it,
           vtkm::cont::ArrayHandle<ParticleType, ParticleStorage>& particles,
           const TerminationType& termination,
           AnalysisType& analysis)
  {
    vtkm::worklet::flow::
      ParticleAdvectionWorklet<IntegratorType, ParticleType, TerminationType, AnalysisType>
        worklet;
    worklet.Run(it, particles, termination, analysis);
  }

  template <typename IntegratorType,
            typename ParticleType,
            typename PointStorage,
            typename TerminationType,
            typename AnalysisType>
  void Run(const IntegratorType& it,
           const vtkm::cont::ArrayHandle<vtkm::Vec3f, PointStorage>& points,
           const TerminationType& termination,
           AnalysisType& analysis)
  {
    vtkm::worklet::flow::
      ParticleAdvectionWorklet<IntegratorType, ParticleType, TerminationType, AnalysisType>
        worklet;

    vtkm::cont::ArrayHandle<ParticleType> particles;
    vtkm::cont::ArrayHandle<vtkm::Id> step, ids;
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> time;
    vtkm::cont::Invoker invoke;

    vtkm::Id numPts = points.GetNumberOfValues();
    vtkm::cont::ArrayHandleConstant<vtkm::Id> s(0, numPts);
    vtkm::cont::ArrayHandleConstant<vtkm::FloatDefault> t(0, numPts);
    vtkm::cont::ArrayHandleCounting<vtkm::Id> id(0, 1, numPts);

    //Copy input to vtkm::Particle
    vtkm::cont::ArrayCopy(s, step);
    vtkm::cont::ArrayCopy(t, time);
    vtkm::cont::ArrayCopy(id, ids);
    invoke(detail::CopyToParticle{}, points, ids, time, step, particles);

    worklet.Run(it, particles, termination, analysis);
  }
};

}
}
} // vtkm::worklet::flow

#endif // vtk_m_filter_flow_worklet_ParticleAdvection_h
