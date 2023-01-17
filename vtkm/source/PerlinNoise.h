//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_source_PerlinNoise_h
#define vtk_m_source_PerlinNoise_h

#include <vtkm/source/Source.h>

namespace vtkm
{
namespace source
{
/**
 * @brief The PerlinNoise source creates a uniform dataset.
 *
 * This class generates a uniform grid dataset with a tileable perlin
 * noise scalar point field.
 *
 * The Execute method creates a complete structured dataset that have a
 * scalar point field named 'perlinnoise'.
**/
class VTKM_SOURCE_EXPORT PerlinNoise final : public vtkm::source::Source
{
public:
  VTKM_CONT PerlinNoise() = default;
  VTKM_CONT ~PerlinNoise() = default;

  VTKM_CONT PerlinNoise(const PerlinNoise&) = default;
  VTKM_CONT PerlinNoise(PerlinNoise&&) = default;
  VTKM_CONT PerlinNoise& operator=(const PerlinNoise&) = default;
  VTKM_CONT PerlinNoise& operator=(PerlinNoise&&) = default;

  VTKM_DEPRECATED(2.0, "Use SetCellDimensions or SetPointDimensions.")
  VTKM_CONT PerlinNoise(vtkm::Id3 dims);
  VTKM_DEPRECATED(2.0, "Use Set*Dimensions and SetSeed.")
  VTKM_CONT PerlinNoise(vtkm::Id3 dims, vtkm::IdComponent seed);
  VTKM_DEPRECATED(2.0, "Use Set*Dimensions and SetOrigin.")
  VTKM_CONT PerlinNoise(vtkm::Id3 dims, vtkm::Vec3f origin);
  VTKM_DEPRECATED(2.0, "Use Set*Dimensions, SetOrigin, and SetSeed.")
  VTKM_CONT PerlinNoise(vtkm::Id3 dims, vtkm::Vec3f origin, vtkm::IdComponent seed);

  VTKM_CONT vtkm::Id3 GetPointDimensions() const { return this->PointDimensions; }
  VTKM_CONT void SetPointDimensions(vtkm::Id3 dims) { this->PointDimensions = dims; }

  VTKM_CONT vtkm::Id3 GetCellDimensions() const { return this->PointDimensions - vtkm::Id3(1); }
  VTKM_CONT void SetCellDimensions(vtkm::Id3 dims) { this->PointDimensions = dims + vtkm::Id3(1); }

  VTKM_CONT vtkm::Vec3f GetOrigin() const { return this->Origin; }
  VTKM_CONT void SetOrigin(const vtkm::Vec3f& origin) { this->Origin = origin; }

  /// \brief The seed used for the pseudorandom number generation of the noise.
  ///
  /// If the seed is not set, then a new, unique seed is picked each time `Execute` is run.
  VTKM_CONT vtkm::IdComponent GetSeed() const { return this->Seed; }
  VTKM_CONT void SetSeed(vtkm::IdComponent seed)
  {
    this->Seed = seed;
    this->SeedSet = true;
  }

private:
  vtkm::cont::DataSet DoExecute() const override;

  vtkm::Id3 PointDimensions = { 16, 16, 16 };
  vtkm::Vec3f Origin = { 0, 0, 0 };
  vtkm::IdComponent Seed = 0;
  bool SeedSet = false;
};
} //namespace source
} //namespace vtkm

#endif //vtk_m_source_PerlinNoise_h
