//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_Particle_h
#define vtk_m_Particle_h

#include <vtkm/Bitset.h>
#include <vtkm/VecVariable.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/Serialization.h>

namespace vtkm
{

//Bit field describing the status:
class ParticleStatus : public vtkm::Bitset<vtkm::UInt8>
{
public:
  VTKM_EXEC_CONT ParticleStatus()
  {
    this->SetOk();
    this->ClearTerminate();
  }

  VTKM_EXEC_CONT void SetOk() { this->set(this->SUCCESS_BIT); }
  VTKM_EXEC_CONT bool CheckOk() const { return this->test(this->SUCCESS_BIT); }

  VTKM_EXEC_CONT void SetFail() { this->reset(this->SUCCESS_BIT); }
  VTKM_EXEC_CONT bool CheckFail() const { return !this->test(this->SUCCESS_BIT); }

  VTKM_EXEC_CONT void SetTerminate() { this->set(this->TERMINATE_BIT); }
  VTKM_EXEC_CONT void ClearTerminate() { this->reset(this->TERMINATE_BIT); }
  VTKM_EXEC_CONT bool CheckTerminate() const { return this->test(this->TERMINATE_BIT); }

  VTKM_EXEC_CONT void SetSpatialBounds() { this->set(this->SPATIAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT void ClearSpatialBounds() { this->reset(this->SPATIAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT bool CheckSpatialBounds() const { return this->test(this->SPATIAL_BOUNDS_BIT); }

  VTKM_EXEC_CONT void SetTemporalBounds() { this->set(this->TEMPORAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT void ClearTemporalBounds() { this->reset(this->TEMPORAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT bool CheckTemporalBounds() const { return this->test(this->TEMPORAL_BOUNDS_BIT); }

  VTKM_EXEC_CONT void SetTookAnySteps() { this->set(this->TOOK_ANY_STEPS_BIT); }
  VTKM_EXEC_CONT void ClearTookAnySteps() { this->reset(this->TOOK_ANY_STEPS_BIT); }
  VTKM_EXEC_CONT bool CheckTookAnySteps() const { return this->test(this->TOOK_ANY_STEPS_BIT); }

private:
  static constexpr vtkm::Id SUCCESS_BIT = 0;
  static constexpr vtkm::Id TERMINATE_BIT = 1;
  static constexpr vtkm::Id SPATIAL_BOUNDS_BIT = 2;
  static constexpr vtkm::Id TEMPORAL_BOUNDS_BIT = 3;
  static constexpr vtkm::Id TOOK_ANY_STEPS_BIT = 4;
};

inline VTKM_CONT std::ostream& operator<<(std::ostream& s, const vtkm::ParticleStatus& status)
{
  s << "[" << status.CheckOk() << " " << status.CheckTerminate() << " "
    << status.CheckSpatialBounds() << " " << status.CheckTemporalBounds() << "]";
  return s;
}

class ParticleBase
{
public:
  VTKM_EXEC_CONT
  ParticleBase() {}

  VTKM_EXEC_CONT virtual ~ParticleBase() noexcept
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VTKM_EXEC_CONT
  ParticleBase(const vtkm::Vec3f& p,
               const vtkm::Id& id,
               const vtkm::Id& numSteps = 0,
               const vtkm::ParticleStatus& status = vtkm::ParticleStatus(),
               const vtkm::FloatDefault& time = 0)
    : Pos(p)
    , ID(id)
    , NumSteps(numSteps)
    , Status(status)
    , Time(time)
  {
  }

  VTKM_EXEC_CONT
  ParticleBase(const vtkm::ParticleBase& p)
    : Pos(p.Pos)
    , ID(p.ID)
    , NumSteps(p.NumSteps)
    , Status(p.Status)
    , Time(p.Time)
  {
  }

  vtkm::ParticleBase& operator=(const vtkm::ParticleBase&) = default;

  // The basic particle is only meant to be advected in a velocity
  // field. In that case it is safe to assume that the velocity value
  // will always be stored in the first location of vectors
  VTKM_EXEC_CONT
  virtual vtkm::Vec3f Next(const vtkm::VecVariable<vtkm::Vec3f, 2>&, const vtkm::FloatDefault&) = 0;

  // The basic particle is only meant to be advected in a velocity
  // field. In that case it is safe to assume that the velocity value
  // will always be stored in the first location of vectors
  VTKM_EXEC_CONT
  virtual vtkm::Vec3f Velocity(const vtkm::VecVariable<vtkm::Vec3f, 2>&,
                               const vtkm::FloatDefault&) = 0;

  vtkm::Vec3f Pos;
  vtkm::Id ID = -1;
  vtkm::Id NumSteps = 0;
  vtkm::ParticleStatus Status;
  vtkm::FloatDefault Time = 0;
};

class Particle : public vtkm::ParticleBase
{
public:
  VTKM_EXEC_CONT
  Particle() {}

  VTKM_EXEC_CONT Particle(const vtkm::Particle& rhs)
    : ParticleBase(rhs)
  {
    // This must not be defaulted, since defaulted copy constructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VTKM_EXEC_CONT ~Particle() noexcept override
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }


  VTKM_EXEC_CONT
  Particle(const vtkm::Vec3f& p,
           const vtkm::Id& id,
           const vtkm::Id& numSteps = 0,
           const vtkm::ParticleStatus& status = vtkm::ParticleStatus(),
           const vtkm::FloatDefault& time = 0)
    : ParticleBase(p, id, numSteps, status, time)
  {
  }

  VTKM_EXEC_CONT Particle& operator=(const vtkm::Particle& rhs)
  {
    // This must not be defaulted, since defaulted assignment operators are
    // troublesome with CUDA __host__ __device__ markup.

    if (&rhs == this)
    {
      return *this;
    }
    vtkm::ParticleBase::operator=(rhs);
    return *this;
  }

  VTKM_EXEC_CONT
  vtkm::Vec3f Next(const vtkm::VecVariable<vtkm::Vec3f, 2>& vectors,
                   const vtkm::FloatDefault& length) override
  {
    VTKM_ASSERT(vectors.GetNumberOfComponents() > 0);
    return this->Pos + length * vectors[0];
  }

  VTKM_EXEC_CONT
  vtkm::Vec3f Velocity(const vtkm::VecVariable<vtkm::Vec3f, 2>& vectors,
                       const vtkm::FloatDefault& vtkmNotUsed(length)) override
  {
    // Velocity is evaluated from the Velocity field
    // and is not influenced by the particle
    VTKM_ASSERT(vectors.GetNumberOfComponents() > 0);
    return vectors[0];
  }
};

class Electron : public vtkm::ParticleBase
{
public:
  VTKM_EXEC_CONT
  Electron() {}

  VTKM_EXEC_CONT
  Electron(const vtkm::Vec3f& position,
           const vtkm::Id& id,
           const vtkm::FloatDefault& mass,
           const vtkm::FloatDefault& charge,
           const vtkm::FloatDefault& weighting,
           const vtkm::Vec3f& momentum,
           const vtkm::Id& numSteps = 0,
           const vtkm::ParticleStatus& status = vtkm::ParticleStatus(),
           const vtkm::FloatDefault& time = 0)
    : ParticleBase(position, id, numSteps, status, time)
    , Mass(mass)
    , Charge(charge)
    , Weighting(weighting)
    , Momentum(momentum)
  {
  }

  VTKM_EXEC_CONT
  vtkm::FloatDefault Beta(vtkm::Vec3f momentum) const
  {
    return static_cast<vtkm::FloatDefault>(vtkm::Magnitude(momentum / this->Mass) /
                                           vtkm::Pow(SPEED_OF_LIGHT, 2));
  }

  VTKM_EXEC_CONT
  vtkm::Vec3f Next(const vtkm::VecVariable<vtkm::Vec3f, 2>& vectors,
                   const vtkm::FloatDefault& length) override
  {
    // TODO: implement Lorentz force calculation
    return this->Pos + length * this->Velocity(vectors, length);
  }

  VTKM_EXEC_CONT
  vtkm::Vec3f Velocity(const vtkm::VecVariable<vtkm::Vec3f, 2>& vectors,
                       const vtkm::FloatDefault& length) override
  {
    VTKM_ASSERT(vectors.GetNumberOfComponents() == 2);

    // Suppress unused warning
    (void)this->Weighting;

    vtkm::Vec3f velocity;
    // Particle has a charge and a mass
    // Velocity updated using Lorentz force
    // Return velocity of the particle
    vtkm::Vec3f eField = vectors[0];
    vtkm::Vec3f bField = vectors[1];
    const vtkm::FloatDefault QoM = this->Charge / this->Mass;
    const vtkm::FloatDefault half = 0.5;
    const vtkm::Vec3f mom_ = this->Momentum + (half * this->Charge * eField * length);

    //TODO : Calculate Gamma
    vtkm::Vec3f gamma_reci = vtkm::Sqrt(1 - this->Beta(mom_));
    // gamma(mom_, mass) -> needs velocity calculation
    const vtkm::Vec3f t = half * QoM * bField * gamma_reci * length;
    const vtkm::Vec3f s = 2.0f * t * (1 / 1 + vtkm::Magnitude(t));

    const vtkm::Vec3f mom_pr = mom_ + vtkm::Cross(mom_, t);
    const vtkm::Vec3f mom_pl = mom_ + vtkm::Cross(mom_pr, s);

    const vtkm::Vec3f mom_new = mom_pl + 0.5 * this->Charge * eField * length;

    //TODO : this is a const method, figure a better way to update momentum

    this->Momentum = mom_new;
    velocity = mom_new / this->Mass;

    return velocity;
  }

private:
  vtkm::FloatDefault Mass;
  vtkm::FloatDefault Charge;
  vtkm::FloatDefault Weighting;
  vtkm::Vec3f Momentum;
  constexpr static vtkm::FloatDefault SPEED_OF_LIGHT =
    static_cast<vtkm::FloatDefault>(2.99792458e8);

  friend struct mangled_diy_namespace::Serialization<vtkm::Electron>;
};

} //namespace vtkm


namespace mangled_diy_namespace
{
template <>
struct Serialization<vtkm::Particle>
{
public:
  static VTKM_CONT void save(BinaryBuffer& bb, const vtkm::Particle& p)
  {
    vtkmdiy::save(bb, p.Pos);
    vtkmdiy::save(bb, p.ID);
    vtkmdiy::save(bb, p.NumSteps);
    vtkmdiy::save(bb, p.Status);
    vtkmdiy::save(bb, p.Time);
  }

  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::Particle& p)
  {
    vtkmdiy::load(bb, p.Pos);
    vtkmdiy::load(bb, p.ID);
    vtkmdiy::load(bb, p.NumSteps);
    vtkmdiy::load(bb, p.Status);
    vtkmdiy::load(bb, p.Time);
  }
};

template <>
struct Serialization<vtkm::Electron>
{
public:
  static VTKM_CONT void save(BinaryBuffer& bb, const vtkm::Electron& e)
  {
    vtkmdiy::save(bb, e.Pos);
    vtkmdiy::save(bb, e.ID);
    vtkmdiy::save(bb, e.NumSteps);
    vtkmdiy::save(bb, e.Status);
    vtkmdiy::save(bb, e.Time);
    vtkmdiy::save(bb, e.Mass);
    vtkmdiy::save(bb, e.Charge);
    vtkmdiy::save(bb, e.Weighting);
    vtkmdiy::save(bb, e.Momentum);
  }

  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::Electron& e)
  {
    vtkmdiy::load(bb, e.Pos);
    vtkmdiy::load(bb, e.ID);
    vtkmdiy::load(bb, e.NumSteps);
    vtkmdiy::load(bb, e.Status);
    vtkmdiy::load(bb, e.Time);
    vtkmdiy::load(bb, e.Mass);
    vtkmdiy::load(bb, e.Charge);
    vtkmdiy::load(bb, e.Weighting);
    vtkmdiy::load(bb, e.Momentum);
  }
};
}

#endif // vtk_m_Particle_h
