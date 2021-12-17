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
  ///Construct a PerlinNoise with Cell Dimensions
  VTKM_CONT PerlinNoise(vtkm::Id3 dims);
  VTKM_CONT PerlinNoise(vtkm::Id3 dims, vtkm::IdComponent seed);
  VTKM_CONT PerlinNoise(vtkm::Id3 dims, vtkm::Vec3f origin);
  VTKM_CONT PerlinNoise(vtkm::Id3 dims, vtkm::Vec3f origin, vtkm::IdComponent seed);

  vtkm::IdComponent GetSeed() const { return this->Seed; }

  void SetSeed(vtkm::IdComponent seed) { this->Seed = seed; }

  vtkm::cont::DataSet Execute() const override;

private:
  vtkm::Id3 Dims;
  vtkm::Vec3f Origin;
  vtkm::IdComponent Seed;
};
} //namespace source
} //namespace vtkm

#endif //vtk_m_source_PerlinNoise_h
