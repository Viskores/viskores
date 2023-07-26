//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#ifndef vtkm_worklet_particleadvection_analysis
#define vtkm_worklet_particleadvection_analysis

#include <vtkm/Particle.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

namespace vtkm
{
namespace worklet
{
namespace flow
{

template <typename ParticleType>
class VTKM_FILTER_FLOW_EXPORT NoAnalysisExec
{
public:
  VTKM_EXEC_CONT
  NoAnalysisExec() {}

  VTKM_EXEC void PreStepAnalyze(const vtkm::Id index, const ParticleType& particle)
  {
    (void)index;
    (void)particle;
  }

  //template <typename ParticleType>
  VTKM_EXEC void Analyze(const vtkm::Id index,
                         const ParticleType& oldParticle,
                         const ParticleType& newParticle)
  {
    // Do nothing
    (void)index;
    (void)oldParticle;
    (void)newParticle;
  }
};

template <typename ParticleType>
class NoAnalysis : public vtkm::cont::ExecutionObjectBase
{
public:
  // Intended to store advected particles after Finalize
  vtkm::cont::ArrayHandle<ParticleType> Particles;

  VTKM_CONT
  NoAnalysis()
    : Particles()
  {
  }

  VTKM_CONT
  void UseAsTemplate(const NoAnalysis& other) { (void)other; }

  VTKM_CONT
  //template <typename ParticleType>
  void InitializeAnalysis(const vtkm::cont::ArrayHandle<ParticleType>& particles)
  {
    (void)particles;
  }

  VTKM_CONT
  //template <typename ParticleType>
  void FinalizeAnalysis(vtkm::cont::ArrayHandle<ParticleType>& particles)
  {
    this->Particles = particles; //, vtkm::CopyFlag::Off);
  }

  VTKM_CONT NoAnalysisExec<ParticleType> PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                             vtkm::cont::Token& token) const
  {
    (void)device;
    (void)token;
    return NoAnalysisExec<ParticleType>();
  }

  VTKM_CONT bool SupportPushOutOfBounds() const { return true; }

  VTKM_CONT static bool MakeDataSet(vtkm::cont::DataSet& dataset,
                                    const std::vector<NoAnalysis>& results);
};

template <typename ParticleType>
class VTKM_FILTER_FLOW_EXPORT StreamlineAnalysisExec
{
public:
  VTKM_EXEC_CONT
  StreamlineAnalysisExec()
    : NumParticles(0)
    , MaxSteps(0)
    , Streams()
    , StreamLengths()
    , Validity()
  {
  }

  VTKM_CONT
  StreamlineAnalysisExec(vtkm::Id numParticles,
                         vtkm::Id maxSteps,
                         const vtkm::cont::ArrayHandle<vtkm::Vec3f>& streams,
                         const vtkm::cont::ArrayHandle<vtkm::Id>& streamLengths,
                         const vtkm::cont::ArrayHandle<vtkm::Id>& validity,
                         vtkm::cont::DeviceAdapterId device,
                         vtkm::cont::Token& token)
    : NumParticles(numParticles)
    , MaxSteps(maxSteps + 1)
  {
    Streams = streams.PrepareForOutput(this->NumParticles * this->MaxSteps, device, token);
    StreamLengths = streamLengths.PrepareForInPlace(device, token);
    Validity = validity.PrepareForInPlace(device, token);
  }

  VTKM_EXEC void PreStepAnalyze(const vtkm::Id index, const ParticleType& particle)
  {
    vtkm::Id streamLength = this->StreamLengths.Get(index);
    if (streamLength == 0)
    {
      this->StreamLengths.Set(index, 1);
      vtkm::Id loc = index * MaxSteps;
      this->Streams.Set(loc, particle.GetPosition());
      this->Validity.Set(loc, 1);
    }
  }

  //template <typename ParticleType>
  VTKM_EXEC void Analyze(const vtkm::Id index,
                         const ParticleType& oldParticle,
                         const ParticleType& newParticle)
  {
    (void)oldParticle;
    vtkm::Id streamLength = this->StreamLengths.Get(index);
    vtkm::Id loc = index * MaxSteps + streamLength;
    this->StreamLengths.Set(index, ++streamLength);
    this->Streams.Set(loc, newParticle.GetPosition());
    this->Validity.Set(loc, 1);
  }

private:
  using IdPortal = typename vtkm::cont::ArrayHandle<vtkm::Id>::WritePortalType;
  using VecPortal = typename vtkm::cont::ArrayHandle<vtkm::Vec3f>::WritePortalType;

  vtkm::Id NumParticles;
  vtkm::Id MaxSteps;
  VecPortal Streams;
  IdPortal StreamLengths;
  IdPortal Validity;
};

template <typename ParticleType>
class StreamlineAnalysis : public vtkm::cont::ExecutionObjectBase
{
public:
  // Intended to store advected particles after Finalize
  vtkm::cont::ArrayHandle<ParticleType> Particles;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> Streams;
  vtkm::cont::CellSetExplicit<> PolyLines;

  //Helper functor for compacting history
  struct IsOne
  {
    template <typename T>
    VTKM_EXEC_CONT bool operator()(const T& x) const
    {
      return x == T(1);
    }
  };

  VTKM_CONT
  StreamlineAnalysis()
    : Particles()
    , MaxSteps(0)
  {
  }

  VTKM_CONT
  StreamlineAnalysis(vtkm::Id maxSteps)
    : Particles()
    , MaxSteps(maxSteps)
  {
  }

  VTKM_CONT
  void UseAsTemplate(const StreamlineAnalysis& other) { this->MaxSteps = other.MaxSteps; }

  VTKM_CONT StreamlineAnalysisExec<ParticleType> PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const
  {
    return StreamlineAnalysisExec<ParticleType>(this->NumParticles,
                                                this->MaxSteps,
                                                this->Streams,
                                                this->StreamLengths,
                                                this->Validity,
                                                device,
                                                token);
  }

  VTKM_CONT bool SupportPushOutOfBounds() const { return true; }

  VTKM_CONT
  void InitializeAnalysis(const vtkm::cont::ArrayHandle<ParticleType>& particles);

  VTKM_CONT
  //template <typename ParticleType>
  void FinalizeAnalysis(vtkm::cont::ArrayHandle<ParticleType>& particles);


  VTKM_CONT static bool MakeDataSet(vtkm::cont::DataSet& dataset,
                                    const std::vector<StreamlineAnalysis>& results);

private:
  vtkm::Id NumParticles;
  vtkm::Id MaxSteps;

  vtkm::cont::ArrayHandle<vtkm::Id> StreamLengths;
  vtkm::cont::ArrayHandle<vtkm::Id> InitialLengths;
  vtkm::cont::ArrayHandle<vtkm::Id> Validity;
};

#ifndef vtk_m_filter_flow_worklet_Analysis_cxx
extern template class VTKM_FILTER_FLOW_TEMPLATE_EXPORT NoAnalysis<vtkm::Particle>;
extern template class VTKM_FILTER_FLOW_TEMPLATE_EXPORT NoAnalysis<vtkm::ChargedParticle>;
extern template class VTKM_FILTER_FLOW_TEMPLATE_EXPORT StreamlineAnalysis<vtkm::Particle>;
extern template class VTKM_FILTER_FLOW_TEMPLATE_EXPORT StreamlineAnalysis<vtkm::ChargedParticle>;
#endif //!vtk_m_filter_flow_worklet_Analysis_cxx

} // namespace flow
} // namespace worklet
} // namespace vtkm

#endif //vtkm_worklet_particleadvection_analysis
