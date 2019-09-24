//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_source_OscillatorSource_h
#define vtk_m_source_OscillatorSource_h

#include <vtkm/source/Source.h>
#include <vtkm/worklet/OscillatorSource.h>

namespace vtkm
{
namespace source
{

/**\brief An analytical, time-varying uniform dataset with a point based array
 *
 * The Execute method creates a complete structured dataset that have a
 * point field names 'scalars'
 *
 * This array is based on the coordinates and evaluates to a sum of time-varying
 * Gaussian exponentials specified in its configuration.
 */
class VTKM_SOURCE_EXPORT Oscillator final : public vtkm::source::Source
{
public:
  ///Construct a Oscillator with Cell Dimensions
  VTKM_CONT
  Oscillator(vtkm::Id3 dims);

  VTKM_CONT
  void SetTime(vtkm::Float64 time);

  VTKM_CONT
  void AddPeriodic(vtkm::Float64 x,
                   vtkm::Float64 y,
                   vtkm::Float64 z,
                   vtkm::Float64 radius,
                   vtkm::Float64 omega,
                   vtkm::Float64 zeta);

  VTKM_CONT
  void AddDamped(vtkm::Float64 x,
                 vtkm::Float64 y,
                 vtkm::Float64 z,
                 vtkm::Float64 radius,
                 vtkm::Float64 omega,
                 vtkm::Float64 zeta);

  VTKM_CONT
  void AddDecaying(vtkm::Float64 x,
                   vtkm::Float64 y,
                   vtkm::Float64 z,
                   vtkm::Float64 radius,
                   vtkm::Float64 omega,
                   vtkm::Float64 zeta);

  VTKM_CONT vtkm::cont::DataSet Execute() const;

private:
  vtkm::Id3 Dims;
  vtkm::worklet::OscillatorSource Worklet;
};
}
}

#endif // vtk_m_source_Oscillator_h
