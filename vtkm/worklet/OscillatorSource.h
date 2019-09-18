//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_OscillatorSource_h
#define vtk_m_worklet_OscillatorSource_h

#include <vtkm/Math.h>
#include <vtkm/worklet/WorkletMapField.h>

#define MAX_OSCILLATORS 10

namespace vtkm
{
namespace worklet
{
namespace internal
{

struct Oscillator
{
  void Set(vtkm::Float64 x,
           vtkm::Float64 y,
           vtkm::Float64 z,
           vtkm::Float64 radius,
           vtkm::Float64 omega,
           vtkm::Float64 zeta)
  {
    this->Center[0] = x;
    this->Center[1] = y;
    this->Center[2] = z;
    this->Radius = radius;
    this->Omega = omega;
    this->Zeta = zeta;
  }

  vtkm::Vec3f_64 Center;
  vtkm::Float64 Radius;
  vtkm::Float64 Omega;
  vtkm::Float64 Zeta;
};
}

class OscillatorSource : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef _2 ExecutionSignature(_1);

  VTKM_CONT
  OscillatorSource()
    : NumberOfPeriodics(0)
    , NumberOfDamped(0)
    , NumberOfDecaying(0)
  {
  }

  VTKM_CONT
  void AddPeriodic(vtkm::Float64 x,
                   vtkm::Float64 y,
                   vtkm::Float64 z,
                   vtkm::Float64 radius,
                   vtkm::Float64 omega,
                   vtkm::Float64 zeta)
  {
    if (this->NumberOfPeriodics < MAX_OSCILLATORS)
    {
      this->PeriodicOscillators[this->NumberOfPeriodics].Set(x, y, z, radius, omega, zeta);
      this->NumberOfPeriodics++;
    }
  }

  VTKM_CONT
  void AddDamped(vtkm::Float64 x,
                 vtkm::Float64 y,
                 vtkm::Float64 z,
                 vtkm::Float64 radius,
                 vtkm::Float64 omega,
                 vtkm::Float64 zeta)
  {
    if (this->NumberOfDamped < MAX_OSCILLATORS)
    {
      this->DampedOscillators[this->NumberOfDamped * 6].Set(x, y, z, radius, omega, zeta);
      this->NumberOfDamped++;
    }
  }

  VTKM_CONT
  void AddDecaying(vtkm::Float64 x,
                   vtkm::Float64 y,
                   vtkm::Float64 z,
                   vtkm::Float64 radius,
                   vtkm::Float64 omega,
                   vtkm::Float64 zeta)
  {
    if (this->NumberOfDecaying < MAX_OSCILLATORS)
    {
      this->DecayingOscillators[this->NumberOfDecaying * 6].Set(x, y, z, radius, omega, zeta);
      this->NumberOfDecaying++;
    }
  }

  VTKM_CONT
  void SetTime(vtkm::Float64 time) { this->Time = time; }

  VTKM_EXEC
  vtkm::Float64 operator()(const vtkm::Vec3f_64& vec) const
  {
    vtkm::UInt8 oIdx;
    vtkm::Float64 t0, t, result = 0;
    const internal::Oscillator* oscillator;

    t0 = 0.0;
    t = this->Time * 2 * 3.14159265358979323846;

    // Compute damped
    for (oIdx = 0; oIdx < this->NumberOfDamped; oIdx++)
    {
      oscillator = &this->DampedOscillators[oIdx];

      vtkm::Vec3f_64 delta = oscillator->Center - vec;
      vtkm::Float64 dist2 = dot(delta, delta);
      vtkm::Float64 dist_damp = vtkm::Exp(-dist2 / (2 * oscillator->Radius * oscillator->Radius));
      vtkm::Float64 phi = vtkm::ACos(oscillator->Zeta);
      vtkm::Float64 val = 1. -
        vtkm::Exp(-oscillator->Zeta * oscillator->Omega * t0) *
          (vtkm::Sin(vtkm::Sqrt(1 - oscillator->Zeta * oscillator->Zeta) * oscillator->Omega * t +
                     phi) /
           vtkm::Sin(phi));
      result += val * dist_damp;
    }

    // Compute decaying
    for (oIdx = 0; oIdx < this->NumberOfDecaying; oIdx++)
    {
      oscillator = &this->DecayingOscillators[oIdx];
      t = t0 + 1 / oscillator->Omega;
      vtkm::Vec3f_64 delta = oscillator->Center - vec;
      vtkm::Float64 dist2 = dot(delta, delta);
      vtkm::Float64 dist_damp = vtkm::Exp(-dist2 / (2 * oscillator->Radius * oscillator->Radius));
      vtkm::Float64 val = vtkm::Sin(t / oscillator->Omega) / (oscillator->Omega * t);
      result += val * dist_damp;
    }

    // Compute periodic
    for (oIdx = 0; oIdx < this->NumberOfPeriodics; oIdx++)
    {
      oscillator = &this->PeriodicOscillators[oIdx];
      t = t0 + 1 / oscillator->Omega;
      vtkm::Vec3f_64 delta = oscillator->Center - vec;
      vtkm::Float64 dist2 = dot(delta, delta);
      vtkm::Float64 dist_damp = vtkm::Exp(-dist2 / (2 * oscillator->Radius * oscillator->Radius));
      vtkm::Float64 val = vtkm::Sin(t / oscillator->Omega);
      result += val * dist_damp;
    }

    // We are done...
    return result;
  }

  template <typename T>
  VTKM_EXEC vtkm::Float64 operator()(const vtkm::Vec<T, 3>& vec) const
  {
    return (*this)(vtkm::make_Vec(static_cast<vtkm::Float64>(vec[0]),
                                  static_cast<vtkm::Float64>(vec[1]),
                                  static_cast<vtkm::Float64>(vec[2])));
  }

private:
  vtkm::Vec<internal::Oscillator, MAX_OSCILLATORS> PeriodicOscillators;
  vtkm::Vec<internal::Oscillator, MAX_OSCILLATORS> DampedOscillators;
  vtkm::Vec<internal::Oscillator, MAX_OSCILLATORS> DecayingOscillators;
  vtkm::UInt8 NumberOfPeriodics;
  vtkm::UInt8 NumberOfDamped;
  vtkm::UInt8 NumberOfDecaying;
  vtkm::Float64 Time;
};
}

} // namespace vtkm

#endif // vtk_m_worklet_PointElevation_h
