//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_source_Wavelet_h
#define vtk_m_source_Wavelet_h

#include <vtkm/source/Source.h>

namespace vtkm
{
namespace source
{
/**
 * @brief The Wavelet source creates a dataset similar to VTK's
 * vtkRTAnalyticSource.
 *
 * This class generates a predictable structured dataset with a smooth yet
 * interesting set of scalars, which is useful for testing and benchmarking.
 *
 * The Execute method creates a complete structured dataset that have a
 * point field names 'scalars'
 *
 * The scalars are computed as:
 *
 * ```
 * MaxVal * Gauss + MagX * sin(FrqX*x) + MagY * sin(FrqY*y) + MagZ * cos(FrqZ*z)
 * ```
 *
 * The dataset properties are determined by:
 * - `Minimum/MaximumExtent`: The logical point extents of the dataset.
 * - `Spacing`: The distance between points of the dataset.
 * - `Center`: The center of the dataset.
 *
 * The scalar functions is control via:
 * - `Center`: The center of a Gaussian contribution to the scalars.
 * - `StandardDeviation`: The unscaled width of a Gaussian contribution.
 * - `MaximumValue`: Upper limit of the scalar range.
 * - `Frequency`: The Frq[XYZ] parameters of the periodic contributions.
 * - `Magnitude`: The Mag[XYZ] parameters of the periodic contributions.
 *
 * By default, the following parameters are used:
 * - `Extents`: { -10, -10, -10 } `-->` { 10, 10, 10 }
 * - `Spacing`: { 1, 1, 1 }
 * - `Center`: { 0, 0, 0 }
 * - `StandardDeviation`: 0.5
 * - `MaximumValue`: 255
 * - `Frequency`: { 60, 30, 40 }
 * - `Magnitude`: { 10, 18, 5 }
 */
class VTKM_SOURCE_EXPORT Wavelet final : public vtkm::source::Source
{
public:
  VTKM_CONT
  Wavelet(vtkm::Id3 minExtent = { -10 }, vtkm::Id3 maxExtent = { 10 });

  VTKM_CONT void SetCenter(const vtkm::Vec<FloatDefault, 3>& center) { this->Center = center; }

  VTKM_CONT void SetSpacing(const vtkm::Vec<FloatDefault, 3>& spacing) { this->Spacing = spacing; }

  VTKM_CONT void SetFrequency(const vtkm::Vec<FloatDefault, 3>& frequency)
  {
    this->Frequency = frequency;
  }

  VTKM_CONT void SetMagnitude(const vtkm::Vec<FloatDefault, 3>& magnitude)
  {
    this->Magnitude = magnitude;
  }

  VTKM_CONT void SetMinimumExtent(const vtkm::Id3& minExtent) { this->MinimumExtent = minExtent; }

  VTKM_CONT void SetMaximumExtent(const vtkm::Id3& maxExtent) { this->MaximumExtent = maxExtent; }

  VTKM_CONT void SetExtent(const vtkm::Id3& minExtent, const vtkm::Id3& maxExtent)
  {
    this->MinimumExtent = minExtent;
    this->MaximumExtent = maxExtent;
  }

  VTKM_CONT void SetMaximumValue(const vtkm::FloatDefault& maxVal) { this->MaximumValue = maxVal; }

  VTKM_CONT void SetStandardDeviation(const vtkm::FloatDefault& stdev)
  {
    this->StandardDeviation = stdev;
  }

  vtkm::cont::DataSet Execute() const;

private:
  vtkm::cont::Field GeneratePointField(const vtkm::cont::CellSetStructured<3>& cellset,
                                       const std::string& name) const;

  vtkm::Vec3f Center;
  vtkm::Vec3f Spacing;
  vtkm::Vec3f Frequency;
  vtkm::Vec3f Magnitude;
  vtkm::Id3 MinimumExtent;
  vtkm::Id3 MaximumExtent;
  vtkm::FloatDefault MaximumValue;
  vtkm::FloatDefault StandardDeviation;
};
} //namespace source
} //namespace vtkm

#endif //vtk_m_source_Wavelet_h
