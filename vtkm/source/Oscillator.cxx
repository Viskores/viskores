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

namespace vtkm
{
namespace source
{

//-----------------------------------------------------------------------------
Oscillator::Oscillator(vtkm::Id3 dims)
  : Dims(dims)
  , Worklet()
{
}

//-----------------------------------------------------------------------------
void Oscillator::SetTime(vtkm::Float64 time)
{
  this->Worklet.SetTime(time);
}

//-----------------------------------------------------------------------------
void Oscillator::AddPeriodic(vtkm::Float64 x,
                             vtkm::Float64 y,
                             vtkm::Float64 z,
                             vtkm::Float64 radius,
                             vtkm::Float64 omega,
                             vtkm::Float64 zeta)
{
  this->Worklet.AddPeriodic(x, y, z, radius, omega, zeta);
}

//-----------------------------------------------------------------------------
void Oscillator::AddDamped(vtkm::Float64 x,
                           vtkm::Float64 y,
                           vtkm::Float64 z,
                           vtkm::Float64 radius,
                           vtkm::Float64 omega,
                           vtkm::Float64 zeta)
{
  this->Worklet.AddDamped(x, y, z, radius, omega, zeta);
}

//-----------------------------------------------------------------------------
void Oscillator::AddDecaying(vtkm::Float64 x,
                             vtkm::Float64 y,
                             vtkm::Float64 z,
                             vtkm::Float64 radius,
                             vtkm::Float64 omega,
                             vtkm::Float64 zeta)
{
  this->Worklet.AddDecaying(x, y, z, radius, omega, zeta);
}


//-----------------------------------------------------------------------------
vtkm::cont::DataSet Oscillator::Execute() const
{
  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  vtkm::cont::DataSet dataSet;

  vtkm::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(this->Dims);
  dataSet.SetCellSet(cellSet);

  const vtkm::Vec3f origin(0.0f, 0.0f, 0.0f);
  const vtkm::Vec3f spacing(1.0f / static_cast<vtkm::FloatDefault>(this->Dims[0]),
                            1.0f / static_cast<vtkm::FloatDefault>(this->Dims[1]),
                            1.0f / static_cast<vtkm::FloatDefault>(this->Dims[2]));

  const vtkm::Id3 pdims{ this->Dims + vtkm::Id3{ 1, 1, 1 } };
  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(pdims, origin, spacing);
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));


  vtkm::cont::ArrayHandle<vtkm::Float64> outArray;
  //todo, we need to use the policy to determine the valid conversions
  //that the dispatcher should do
  this->Invoke(this->Worklet, coordinates, outArray);
  dataSet.AddField(vtkm::cont::make_FieldPoint("scalars", outArray));

  return dataSet;
}
}
} // namespace vtkm::filter
