//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_interop_anari_ANARIActor_h
#define vtk_m_interop_anari_ANARIActor_h

// vtk-m
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/UnknownCellSet.h>
#include <vtkm/interop/anari/VtkmANARITypes.h>
// std
#include <array>
#include <memory>

#include <vtkm/interop/anari/vtkm_anari_export.h>

namespace vtkm
{
namespace interop
{
namespace anari
{

/// \brief Convenience type used to represent all the fields in an `ANARIActor`.
///
using FieldSet = std::array<vtkm::cont::Field, 4>;

/// \brief Returns the appropriate ANARI attribute string based on field index.
///
const char* AnariMaterialInputString(vtkm::IdComponent p);

/// \brief Collects cells, coords, and 0-4 fields for ANARI mappers to consume.
///
/// `ANARIActor` represents a selected set of cells, coordinates, and fields for
/// `ANARIMapper` based mappers to map onto ANARI objects. This class also
/// maintains which field is the "main" field, which almost always is the field
/// which is used to color the geometry or volume.
///
/// Mappers creating geometry will generally add all fields as attribute arrays
/// if possible, letting applications use more than one field as material inputs
/// or data to be color mapped by samplers.
///
struct VTKM_ANARI_EXPORT ANARIActor
{
  ANARIActor() = default;

  /// @brief Main constructor taking cells, coordinates, and up to 4 fields.
  ///
  ANARIActor(const vtkm::cont::UnknownCellSet& cells,
             const vtkm::cont::CoordinateSystem& coordinates,
             const vtkm::cont::Field& field0 = {},
             const vtkm::cont::Field& field1 = {},
             const vtkm::cont::Field& field2 = {},
             const vtkm::cont::Field& field3 = {});

  /// @brief Convenience constructor when an entire FieldSet already exists.
  ///
  ANARIActor(const vtkm::cont::UnknownCellSet& cells,
             const vtkm::cont::CoordinateSystem& coordinates,
             const FieldSet& fieldset);

  /// @brief Convenience constructor using a dataset + named fields.
  ///
  ANARIActor(const vtkm::cont::DataSet& dataset,
             const std::string& field0 = "",
             const std::string& field1 = "",
             const std::string& field2 = "",
             const std::string& field3 = "");

  const vtkm::cont::UnknownCellSet& GetCellSet() const;
  const vtkm::cont::CoordinateSystem& GetCoordinateSystem() const;
  const vtkm::cont::Field& GetField(vtkm::IdComponent idx = -1) const;

  FieldSet GetFieldSet() const;

  void SetPrimaryFieldIndex(vtkm::IdComponent idx);
  vtkm::IdComponent GetPrimaryFieldIndex() const;

  /// @brief Utility to reconstitute a DataSet from the items in the actor.
  ///
  vtkm::cont::DataSet MakeDataSet(bool includeFields = false) const;

private:
  struct ActorData
  {
    vtkm::cont::UnknownCellSet Cells;
    vtkm::cont::CoordinateSystem Coordinates;
    FieldSet Fields;
    vtkm::IdComponent PrimaryField{ 0 };
  };

  std::shared_ptr<ActorData> Data = std::make_shared<ActorData>();
};

} // namespace anari
} // namespace interop
} // namespace vtkm

#endif
