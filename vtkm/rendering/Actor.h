//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_Actor_h
#define vtk_m_rendering_Actor_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Mapper.h>

#include <memory>

namespace vtkm
{
namespace rendering
{

/// @brief An item to be rendered.
///
/// The `Actor` holds the geometry from a `vtkm::cont::DataSet` as well as other visual
/// properties that define how the geometry should look when it is rendered.
class VTKM_RENDERING_EXPORT Actor
{
public:
  Actor(const vtkm::cont::DataSet dataSet,
        const std::string coordinateName,
        const std::string fieldName);

  Actor(const vtkm::cont::DataSet dataSet,
        const std::string coordinateName,
        const std::string fieldName,
        const vtkm::cont::ColorTable& colorTable);

  Actor(const vtkm::cont::DataSet dataSet,
        const std::string coordinateName,
        const std::string fieldName,
        const vtkm::rendering::Color& color);

  Actor(const vtkm::cont::PartitionedDataSet dataSet,
        const std::string coordinateName,
        const std::string fieldName);

  Actor(const vtkm::cont::PartitionedDataSet dataSet,
        const std::string coordinateName,
        const std::string fieldName,
        const vtkm::cont::ColorTable& colorTable);

  Actor(const vtkm::cont::PartitionedDataSet dataSet,
        const std::string coordinateName,
        const std::string fieldName,
        const vtkm::rendering::Color& color);

  /// Create an `Actor` object that renders a set of cells positioned by a given coordiante
  /// system. A field to apply psudocoloring is also provided. The default colormap is applied.
  /// The cells, coordinates, and field are typically pulled from a `vtkm::cont::DataSet` object.
  Actor(const vtkm::cont::UnknownCellSet& cells,
        const vtkm::cont::CoordinateSystem& coordinates,
        const vtkm::cont::Field& scalarField);

  /// Create an `Actor` object that renders a set of cells positioned by a given coordiante
  /// system. A field to apply psudocoloring is also provided. A color table providing the map
  /// from scalar values to colors is also provided.
  /// The cells, coordinates, and field are typically pulled from a `vtkm::cont::DataSet` object.
  Actor(const vtkm::cont::UnknownCellSet& cells,
        const vtkm::cont::CoordinateSystem& coordinates,
        const vtkm::cont::Field& scalarField,
        const vtkm::cont::ColorTable& colorTable);

  /// Create an `Actor` object that renders a set of cells positioned by a given coordiante
  /// system. A constant color to apply to the object is also provided.
  /// The cells and coordinates are typically pulled from a `vtkm::cont::DataSet` object.
  // Why do you have to provide a `Field` if a constant color is provided?
  Actor(const vtkm::cont::UnknownCellSet& cells,
        const vtkm::cont::CoordinateSystem& coordinates,
        const vtkm::cont::Field& scalarField,
        const vtkm::rendering::Color& color);

  Actor(const Actor&);
  Actor& operator=(const Actor&);

  Actor(Actor&&) noexcept;
  Actor& operator=(Actor&&) noexcept;
  ~Actor();

  void Render(vtkm::rendering::Mapper& mapper,
              vtkm::rendering::Canvas& canvas,
              const vtkm::rendering::Camera& camera) const;

  const vtkm::cont::UnknownCellSet& GetCells() const;

  vtkm::cont::CoordinateSystem GetCoordinates() const;

  const vtkm::cont::Field& GetScalarField() const;

  const vtkm::cont::ColorTable& GetColorTable() const;

  const vtkm::Range& GetScalarRange() const;

  const vtkm::Bounds& GetSpatialBounds() const;

  /// @brief Specifies the range for psudocoloring.
  ///
  /// When coloring data by mapping a scalar field to colors, this is the range used for
  /// the colors provided by the table. If a range is not provided, the range of data in the
  /// field is used.
  void SetScalarRange(const vtkm::Range& scalarRange);

private:
  struct InternalsType;
  std::unique_ptr<InternalsType> Internals;

  void Init(const vtkm::cont::CoordinateSystem& coordinates, const vtkm::cont::Field& scalarField);

  void Init();
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_Actor_h
