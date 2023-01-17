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

#include <vtkm/Math.h>

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
 * The Execute method creates a complete structured dataset that has a
 * point field named `RTData`
 *
 * The RTData scalars are computed as:
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
 *
 *  If the extent has zero length in the z-direction, a 2D dataset is generated.
 */
class VTKM_SOURCE_EXPORT Wavelet final : public vtkm::source::Source
{
public:
  VTKM_CONT Wavelet() = default;
  VTKM_CONT ~Wavelet() = default;

  VTKM_DEPRECATED(2.0, "Use SetExtent.")
  VTKM_CONT Wavelet(vtkm::Id3 minExtent, vtkm::Id3 maxExtent = { 10 });

  ///@{
  /// \brief Specifies the center of the wavelet function.
  ///
  /// Note that the center of the function can be anywhere in space including
  /// outside the domain of the data created (as specified by the origin,
  /// spacing and extent).
  VTKM_CONT void SetCenter(const vtkm::Vec3f& center) { this->Center = center; }
  VTKM_CONT vtkm::Vec3f GetCenter() const { return this->Center; }
  ///@}

  ///@{
  /// \brief Specifies the origin (lower left corner) of the dataset created.
  ///
  /// If the origin is not specified, it will be placed such that extent
  /// index (0, 0, 0) is at the coordinate system origin.
  VTKM_CONT void SetOrigin(const vtkm::Vec3f& origin) { this->Origin = origin; }
  VTKM_CONT vtkm::Vec3f GetOrigin() const
  {
    if (!vtkm::IsNan(this->Origin[0]))
    {
      return this->Origin;
    }
    else
    {
      return this->MinimumExtent * this->Spacing;
    }
  }

  VTKM_CONT void SetSpacing(const vtkm::Vec3f& spacing) { this->Spacing = spacing; }
  VTKM_CONT vtkm::Vec3f GetSpacing() const { return this->Spacing; }

  VTKM_CONT void SetFrequency(const vtkm::Vec3f& frequency) { this->Frequency = frequency; }
  VTKM_CONT vtkm::Vec3f GetFrequency() const { return this->Frequency; }

  VTKM_CONT void SetMagnitude(const vtkm::Vec3f& magnitude) { this->Magnitude = magnitude; }
  VTKM_CONT vtkm::Vec3f GetMagnitude() const { return this->Magnitude; }

  VTKM_CONT void SetMinimumExtent(const vtkm::Id3& minExtent) { this->MinimumExtent = minExtent; }
  VTKM_CONT vtkm::Id3 GetMinimumExtent() const { return this->MinimumExtent; }

  VTKM_CONT void SetMaximumExtent(const vtkm::Id3& maxExtent) { this->MaximumExtent = maxExtent; }
  VTKM_CONT vtkm::Id3 GetMaximumExtent() const { return this->MaximumExtent; }

  VTKM_CONT void SetExtent(const vtkm::Id3& minExtent, const vtkm::Id3& maxExtent)
  {
    this->MinimumExtent = minExtent;
    this->MaximumExtent = maxExtent;
  }

  VTKM_CONT void SetMaximumValue(const vtkm::FloatDefault& maxVal) { this->MaximumValue = maxVal; }
  VTKM_CONT vtkm::FloatDefault GetMaximumValue() const { return this->MaximumValue; }

  VTKM_CONT void SetStandardDeviation(const vtkm::FloatDefault& stdev)
  {
    this->StandardDeviation = stdev;
  }
  VTKM_CONT vtkm::FloatDefault GetStandardDeviation() const { return this->StandardDeviation; }

private:
  vtkm::cont::DataSet DoExecute() const override;

  template <vtkm::IdComponent Dim>
  vtkm::cont::Field GeneratePointField(const vtkm::cont::CellSetStructured<Dim>& cellset,
                                       const std::string& name) const;

  template <vtkm::IdComponent Dim>
  vtkm::cont::DataSet GenerateDataSet(vtkm::cont::CoordinateSystem coords) const;

  vtkm::Vec3f Center = { 0, 0, 0 };
  vtkm::Vec3f Origin = { vtkm::Nan<vtkm::FloatDefault>() };
  vtkm::Vec3f Spacing = { 1, 1, 1 };
  vtkm::Vec3f Frequency = { 60.0f, 30.0f, 40.0f };
  vtkm::Vec3f Magnitude = { 10.0f, 18.0f, 5.0f };
  vtkm::Id3 MinimumExtent = { -10, -10, -10 };
  vtkm::Id3 MaximumExtent = { 10, 10, 10 };
  vtkm::FloatDefault MaximumValue = 255.0f;
  vtkm::FloatDefault StandardDeviation = 0.5f;
};
} //namespace source
} //namespace vtkm

#endif //vtk_m_source_Wavelet_h
