//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_density_estimate_ParticleDensityBase_h
#define vtk_m_filter_density_estimate_ParticleDensityBase_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/density_estimate/vtkm_filter_density_estimate_export.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
class VTKM_FILTER_DENSITY_ESTIMATE_EXPORT ParticleDensityBase : public vtkm::filter::Filter
{
protected:
  ParticleDensityBase() = default;

public:
  /// @brief Toggles between summing mass and computing instances.
  ///
  /// When this flag is false (the default), the active field of the input is accumulated
  /// in each bin of the output. When this flag is set to true, the active field is ignored
  /// and the associated particles are simply counted.
  VTKM_CONT void SetComputeNumberDensity(bool flag) { this->ComputeNumberDensity = flag; }
  /// @copydoc SetComputeNumberDensity
  VTKM_CONT bool GetComputeNumberDensity() const { return this->ComputeNumberDensity; }

  /// @brief Specifies whether the accumulated mass (or count) is divided by the volume of the cell.
  ///
  /// When this flag is on (the default), the computed mass will be divided by the volume of the
  /// bin to give a density value. Turning off this flag provides an accumulated mass or count.
  ///
  VTKM_CONT void SetDivideByVolume(bool flag) { this->DivideByVolume = flag; }
  /// @copydoc SetDivideByVolume
  VTKM_CONT bool GetDivideByVolume() const { return this->DivideByVolume; }

  /// @brief The number of bins in the grid used as regions to estimate density.
  ///
  /// To estimate particle density, this filter defines a uniform grid in space.
  ///
  /// The numbers specify the number of *bins* (i.e. cells in the output mesh) in each
  /// dimension, not the number of points in the output mesh.
  ///
  VTKM_CONT void SetDimension(const vtkm::Id3& dimension) { this->Dimension = dimension; }
  /// @copydoc SetDimension
  VTKM_CONT vtkm::Id3 GetDimension() const { return this->Dimension; }

  /// @brief The lower-left (minimum) corner of the domain of density estimation.
  ///
  VTKM_CONT void SetOrigin(const vtkm::Vec3f& origin) { this->Origin = origin; }
  /// @copydoc SetOrigin
  VTKM_CONT vtkm::Vec3f GetOrigin() const { return this->Origin; }

  /// @brief The spacing of the grid points used to form the grid for density estimation.
  ///
  VTKM_CONT void SetSpacing(const vtkm::Vec3f& spacing) { this->Spacing = spacing; }
  /// @copydoc SetSpacing
  VTKM_CONT vtkm::Vec3f GetSpacing() const { return this->Spacing; }

  /// @brief The bounds of the region where density estimation occurs.
  ///
  /// This method can be used in place of `SetOrigin` and `SetSpacing`. It is often
  /// easiest to compute the bounds of the input coordinate system (or other spatial
  /// region) to use as the input.
  ///
  /// The dimensions must be set before the bounds are set. Calling `SetDimension`
  /// will change the ranges of the bounds.
  ///
  VTKM_CONT void SetBounds(const vtkm::Bounds& bounds)
  {
    this->Origin = { static_cast<vtkm::FloatDefault>(bounds.X.Min),
                     static_cast<vtkm::FloatDefault>(bounds.Y.Min),
                     static_cast<vtkm::FloatDefault>(bounds.Z.Min) };
    this->Spacing = (vtkm::Vec3f{ static_cast<vtkm::FloatDefault>(bounds.X.Length()),
                                  static_cast<vtkm::FloatDefault>(bounds.Y.Length()),
                                  static_cast<vtkm::FloatDefault>(bounds.Z.Length()) } /
                     Dimension);
  }
  VTKM_CONT vtkm::Bounds GetBounds() const
  {
    return { { this->Origin[0], this->Origin[0] + (this->Spacing[0] * this->Dimension[0]) },
             { this->Origin[1], this->Origin[1] + (this->Spacing[1] * this->Dimension[1]) },
             { this->Origin[2], this->Origin[2] + (this->Spacing[2] * this->Dimension[2]) } };
  }

protected:
  // Note: we are using the paradoxical "const ArrayHandle&" parameter whose content can actually
  // be change by the function.
  VTKM_CONT void DoDivideByVolume(const vtkm::cont::UnknownArrayHandle& array) const;

  vtkm::Id3 Dimension = { 100, 100, 100 }; // Cell dimension
  vtkm::Vec3f Origin = { 0.0f, 0.0f, 0.0f };
  vtkm::Vec3f Spacing = { 1.0f, 1.0f, 1.0f };
  bool ComputeNumberDensity = false;
  bool DivideByVolume = true;
};
} // namespace density_estimate
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_density_estimate_ParticleDensityBase_h
