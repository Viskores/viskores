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

namespace vtkm
{
namespace source
{
namespace internal
{
class OscillatorSource;
}

/**\brief An analytical, time-varying uniform dataset with a point based array
 *
 * The Execute method creates a complete structured dataset that have a
 * point field names 'oscillating'
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

  // We can not declare default destructor here since compiler does not know how
  // to create one for the Worklet at this point yet. However, the implementation
  // in Oscillator.cxx does have ~Oscillator() = default;
  VTKM_CONT
  ~Oscillator();

  VTKM_CONT
  void SetTime(vtkm::FloatDefault time);

  VTKM_CONT
  void AddPeriodic(vtkm::FloatDefault x,
                   vtkm::FloatDefault y,
                   vtkm::FloatDefault z,
                   vtkm::FloatDefault radius,
                   vtkm::FloatDefault omega,
                   vtkm::FloatDefault zeta);

  VTKM_CONT
  void AddDamped(vtkm::FloatDefault x,
                 vtkm::FloatDefault y,
                 vtkm::FloatDefault z,
                 vtkm::FloatDefault radius,
                 vtkm::FloatDefault omega,
                 vtkm::FloatDefault zeta);

  VTKM_CONT
  void AddDecaying(vtkm::FloatDefault x,
                   vtkm::FloatDefault y,
                   vtkm::FloatDefault z,
                   vtkm::FloatDefault radius,
                   vtkm::FloatDefault omega,
                   vtkm::FloatDefault zeta);

  VTKM_CONT vtkm::cont::DataSet Execute() const;

private:
  vtkm::Id3 Dims;
  std::unique_ptr<internal::OscillatorSource> Worklet;
};
}
}

#endif // vtk_m_source_Oscillator_h
