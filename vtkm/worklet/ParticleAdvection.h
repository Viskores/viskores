//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_ParticleAdvection_h
#define vtk_m_worklet_ParticleAdvection_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/worklet/particleadvection/ParticleAdvectionWorklets.h>

namespace vtkm
{
namespace worklet
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

  VTKM_EXEC void operator()(const vtkm::Vec3f& pt,
                            const vtkm::Id& id,
                            const vtkm::FloatDefault& time,
                            const vtkm::Id& step,
                            vtkm::Particle& particle) const
  {
    particle.Pos = pt;
    particle.ID = id;
    particle.Time = time;
    particle.NumSteps = step;
    particle.Status.SetOk();
  }
};

} //detail

struct ParticleAdvectionResult
{
  ParticleAdvectionResult()
    : Particles()
  {
  }

  ParticleAdvectionResult(const vtkm::cont::ArrayHandle<vtkm::Particle>& p)
    : Particles(p)
  {
  }

  vtkm::cont::ArrayHandle<vtkm::Particle> Particles;
};

class ParticleAdvection
{
public:
  ParticleAdvection() {}

  template <typename IntegratorType, typename ParticleStorage>
  ParticleAdvectionResult Run(const IntegratorType& it,
                              vtkm::cont::ArrayHandle<vtkm::Particle, ParticleStorage>& particles,
                              vtkm::Id MaxSteps)
  {
    vtkm::worklet::particleadvection::ParticleAdvectionWorklet<IntegratorType> worklet;

    worklet.Run(it, particles, MaxSteps);
    return ParticleAdvectionResult(particles);
  }

  template <typename IntegratorType, typename PointStorage>
  ParticleAdvectionResult Run(const IntegratorType& it,
                              const vtkm::cont::ArrayHandle<vtkm::Vec3f, PointStorage>& points,
                              vtkm::Id MaxSteps)
  {
    vtkm::worklet::particleadvection::ParticleAdvectionWorklet<IntegratorType> worklet;

    vtkm::cont::ArrayHandle<vtkm::Particle> particles;
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

    worklet.Run(it, particles, MaxSteps);
    return ParticleAdvectionResult(particles);
  }
};

struct StreamlineResult
{
  StreamlineResult()
    : Particles()
    , Positions()
    , PolyLines()
  {
  }

  StreamlineResult(const vtkm::cont::ArrayHandle<vtkm::Particle>& part,
                   const vtkm::cont::ArrayHandle<vtkm::Vec3f>& pos,
                   const vtkm::cont::CellSetExplicit<>& lines)
    : Particles(part)
    , Positions(pos)
    , PolyLines(lines)
  {
  }

  vtkm::cont::ArrayHandle<vtkm::Particle> Particles;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> Positions;
  vtkm::cont::CellSetExplicit<> PolyLines;
};

class Streamline
{
public:
  Streamline() {}

  template <typename IntegratorType, typename ParticleStorage>
  StreamlineResult Run(const IntegratorType& it,
                       vtkm::cont::ArrayHandle<vtkm::Particle, ParticleStorage>& particles,
                       vtkm::Id MaxSteps)
  {
    vtkm::worklet::particleadvection::StreamlineWorklet<IntegratorType> worklet;

    vtkm::cont::ArrayHandle<vtkm::Vec3f> positions;
    vtkm::cont::CellSetExplicit<> polyLines;

    worklet.Run(it, particles, MaxSteps, positions, polyLines);

    return StreamlineResult(particles, positions, polyLines);
  }
};
}
}

#endif // vtk_m_worklet_ParticleAdvection_h
