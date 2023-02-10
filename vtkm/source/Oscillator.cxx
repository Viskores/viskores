//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/source/Oscillator.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace
{

struct Oscillation
{
  vtkm::Vec3f Center;
  vtkm::FloatDefault Radius;
  vtkm::FloatDefault Omega;
  vtkm::FloatDefault Zeta;
};

class OscillatorWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef _2 ExecutionSignature(_1);

  VTKM_CONT
  void AddPeriodic(vtkm::FloatDefault x,
                   vtkm::FloatDefault y,
                   vtkm::FloatDefault z,
                   vtkm::FloatDefault radius,
                   vtkm::FloatDefault omega,
                   vtkm::FloatDefault zeta)
  {
    if (this->PeriodicOscillators.GetNumberOfComponents() < MAX_OSCILLATORS)
    {
      this->PeriodicOscillators.Append(Oscillation{ { x, y, z }, radius, omega, zeta });
    }
  }

  VTKM_CONT
  void AddDamped(vtkm::FloatDefault x,
                 vtkm::FloatDefault y,
                 vtkm::FloatDefault z,
                 vtkm::FloatDefault radius,
                 vtkm::FloatDefault omega,
                 vtkm::FloatDefault zeta)
  {
    if (this->DampedOscillators.GetNumberOfComponents() < MAX_OSCILLATORS)
    {
      this->DampedOscillators.Append(Oscillation{ { x, y, z }, radius, omega, zeta });
    }
  }

  VTKM_CONT
  void AddDecaying(vtkm::FloatDefault x,
                   vtkm::FloatDefault y,
                   vtkm::FloatDefault z,
                   vtkm::FloatDefault radius,
                   vtkm::FloatDefault omega,
                   vtkm::FloatDefault zeta)
  {
    if (this->DecayingOscillators.GetNumberOfComponents() < MAX_OSCILLATORS)
    {
      this->DecayingOscillators.Append(Oscillation{ { x, y, z }, radius, omega, zeta });
    }
  }

  VTKM_CONT
  void SetTime(vtkm::FloatDefault time) { this->Time = time; }

  VTKM_EXEC
  vtkm::FloatDefault operator()(const vtkm::Vec3f& vec) const
  {
    vtkm::UInt8 oIdx;
    vtkm::FloatDefault t0, t, result = 0;
    const Oscillation* oscillator;

    t0 = 0.0;
    t = vtkm::FloatDefault(this->Time * 2 * 3.14159265358979323846);

    // Compute damped
    for (oIdx = 0; oIdx < this->DampedOscillators.GetNumberOfComponents(); oIdx++)
    {
      oscillator = &this->DampedOscillators[oIdx];

      vtkm::Vec3f delta = oscillator->Center - vec;
      vtkm::FloatDefault dist2 = dot(delta, delta);
      vtkm::FloatDefault dist_damp =
        vtkm::Exp(-dist2 / (2 * oscillator->Radius * oscillator->Radius));
      vtkm::FloatDefault phi = vtkm::ACos(oscillator->Zeta);
      vtkm::FloatDefault val = vtkm::FloatDefault(
        1. -
        vtkm::Exp(-oscillator->Zeta * oscillator->Omega * t0) *
          (vtkm::Sin(vtkm::Sqrt(1 - oscillator->Zeta * oscillator->Zeta) * oscillator->Omega * t +
                     phi) /
           vtkm::Sin(phi)));
      result += val * dist_damp;
    }

    // Compute decaying
    for (oIdx = 0; oIdx < this->DecayingOscillators.GetNumberOfComponents(); oIdx++)
    {
      oscillator = &this->DecayingOscillators[oIdx];
      t = t0 + 1 / oscillator->Omega;
      vtkm::Vec3f delta = oscillator->Center - vec;
      vtkm::FloatDefault dist2 = dot(delta, delta);
      vtkm::FloatDefault dist_damp =
        vtkm::Exp(-dist2 / (2 * oscillator->Radius * oscillator->Radius));
      vtkm::FloatDefault val = vtkm::Sin(t / oscillator->Omega) / (oscillator->Omega * t);
      result += val * dist_damp;
    }

    // Compute periodic
    for (oIdx = 0; oIdx < this->PeriodicOscillators.GetNumberOfComponents(); oIdx++)
    {
      oscillator = &this->PeriodicOscillators[oIdx];
      t = t0 + 1 / oscillator->Omega;
      vtkm::Vec3f delta = oscillator->Center - vec;
      vtkm::FloatDefault dist2 = dot(delta, delta);
      vtkm::FloatDefault dist_damp =
        vtkm::Exp(-dist2 / (2 * oscillator->Radius * oscillator->Radius));
      vtkm::FloatDefault val = vtkm::Sin(t / oscillator->Omega);
      result += val * dist_damp;
    }

    // We are done...
    return result;
  }

private:
  static constexpr vtkm::IdComponent MAX_OSCILLATORS = 10;
  vtkm::VecVariable<Oscillation, MAX_OSCILLATORS> PeriodicOscillators;
  vtkm::VecVariable<Oscillation, MAX_OSCILLATORS> DampedOscillators;
  vtkm::VecVariable<Oscillation, MAX_OSCILLATORS> DecayingOscillators;
  vtkm::FloatDefault Time{};
}; // OscillatorWorklet

} // anonymous namespace

namespace vtkm
{
namespace source
{

//-----------------------------------------------------------------------------
struct Oscillator::InternalStruct
{
  vtkm::Id3 PointDimensions = { 3, 3, 3 };
  OscillatorWorklet Worklet;
};

//-----------------------------------------------------------------------------
Oscillator::Oscillator()
  : Internals(new InternalStruct)
{
}

//-----------------------------------------------------------------------------
Oscillator::Oscillator(vtkm::Id3 dims)
  : Internals(new InternalStruct)
{
  this->SetCellDimensions(dims);
}

Oscillator::~Oscillator() = default;

//-----------------------------------------------------------------------------
void Oscillator::SetPointDimensions(vtkm::Id3 pointDimensions)
{
  this->Internals->PointDimensions = pointDimensions;
}
vtkm::Id3 Oscillator::GetPointDimensions() const
{
  return this->Internals->PointDimensions;
}

void Oscillator::SetCellDimensions(vtkm::Id3 cellDimensions)
{
  this->SetPointDimensions(cellDimensions + vtkm::Id3(1));
}
vtkm::Id3 Oscillator::GetCellDimensions() const
{
  return this->GetPointDimensions() - vtkm::Id3(1);
}

//-----------------------------------------------------------------------------
void Oscillator::SetTime(vtkm::FloatDefault time)
{
  this->Internals->Worklet.SetTime(time);
}

//-----------------------------------------------------------------------------
void Oscillator::AddPeriodic(vtkm::FloatDefault x,
                             vtkm::FloatDefault y,
                             vtkm::FloatDefault z,
                             vtkm::FloatDefault radius,
                             vtkm::FloatDefault omega,
                             vtkm::FloatDefault zeta)
{
  this->Internals->Worklet.AddPeriodic(x, y, z, radius, omega, zeta);
}

//-----------------------------------------------------------------------------
void Oscillator::AddDamped(vtkm::FloatDefault x,
                           vtkm::FloatDefault y,
                           vtkm::FloatDefault z,
                           vtkm::FloatDefault radius,
                           vtkm::FloatDefault omega,
                           vtkm::FloatDefault zeta)
{
  this->Internals->Worklet.AddDamped(x, y, z, radius, omega, zeta);
}

//-----------------------------------------------------------------------------
void Oscillator::AddDecaying(vtkm::FloatDefault x,
                             vtkm::FloatDefault y,
                             vtkm::FloatDefault z,
                             vtkm::FloatDefault radius,
                             vtkm::FloatDefault omega,
                             vtkm::FloatDefault zeta)
{
  this->Internals->Worklet.AddDecaying(x, y, z, radius, omega, zeta);
}


//-----------------------------------------------------------------------------
vtkm::cont::DataSet Oscillator::DoExecute() const
{
  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  vtkm::cont::DataSet dataSet;

  vtkm::cont::CellSetStructured<3> cellSet;
  vtkm::Id3 pointDims = this->GetPointDimensions();
  cellSet.SetPointDimensions(pointDims);
  dataSet.SetCellSet(cellSet);

  vtkm::Id3 cellDims = this->GetCellDimensions();
  const vtkm::Vec3f origin(0.0f, 0.0f, 0.0f);
  const vtkm::Vec3f spacing(1.0f / static_cast<vtkm::FloatDefault>(cellDims[0]),
                            1.0f / static_cast<vtkm::FloatDefault>(cellDims[1]),
                            1.0f / static_cast<vtkm::FloatDefault>(cellDims[2]));

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(pointDims, origin, spacing);
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));


  vtkm::cont::ArrayHandle<vtkm::FloatDefault> outArray;
  this->Invoke(this->Internals->Worklet, coordinates, outArray);
  dataSet.AddField(vtkm::cont::make_FieldPoint("oscillating", outArray));

  return dataSet;
}
}
} // namespace vtkm::filter
