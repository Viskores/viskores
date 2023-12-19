//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_MapperVolume_h
#define vtk_m_rendering_MapperVolume_h

#include <vtkm/rendering/Mapper.h>

#include <memory>

namespace vtkm
{
namespace rendering
{

/// @brief Mapper that renders a volume as a translucent cloud.
class VTKM_RENDERING_EXPORT MapperVolume : public Mapper
{
public:
  MapperVolume();

  ~MapperVolume();

  void SetCanvas(vtkm::rendering::Canvas* canvas) override;
  virtual vtkm::rendering::Canvas* GetCanvas() const override;

  vtkm::rendering::Mapper* NewCopy() const override;
  /// @brief Specify how much space is between samples of rays that traverse the volume.
  ///
  /// The volume rendering ray caster finds the entry point of the ray through the volume
  /// and then samples the volume along the direction of the ray at regular intervals.
  /// This parameter specifies how far these samples occur.
  void SetSampleDistance(const vtkm::Float32 distance);
  void SetCompositeBackground(const bool compositeBackground);

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;

  virtual void RenderCellsImpl(const vtkm::cont::UnknownCellSet& cellset,
                               const vtkm::cont::CoordinateSystem& coords,
                               const vtkm::cont::Field& scalarField,
                               const vtkm::cont::ColorTable&, //colorTable
                               const vtkm::rendering::Camera& camera,
                               const vtkm::Range& scalarRange,
                               const vtkm::cont::Field& ghostField) override;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_MapperVolume_h
