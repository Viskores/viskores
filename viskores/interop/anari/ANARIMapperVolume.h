//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_interop_anari_ANARIMapperVolume_h
#define viskores_interop_anari_ANARIMapperVolume_h

#include <viskores/interop/anari/ANARIMapper.h>

namespace viskores
{
namespace interop
{
namespace anari
{

/// @brief Raw ANARI arrays and parameter values set on the `ANARISpatialField`.
///
struct StructuredVolumeParameters
{
  anari_cpp::Array3D Data{ nullptr };
  int Dims[3];
  float Origin[3];
  float Spacing[3];
};

struct UnstructuredVolumeParameters
{
  anari_cpp::Array1D VertexPosition{ nullptr };
  anari_cpp::Array1D VertexData{ nullptr };
  anari_cpp::Array1D Index{ nullptr };
  anari_cpp::Array1D CellIndex{ nullptr };
  anari_cpp::Array1D CellData{ nullptr };
  anari_cpp::Array1D CellType{ nullptr };
};

/// @brief Viskores data arrays underlying the `ANARIArray` handles created by the mapper.
///
struct StructuredVolumeArrays
{
  viskores::cont::ArrayHandle<viskores::Float32> Data;
  std::shared_ptr<viskores::cont::Token> Token{ new viskores::cont::Token };
};

struct UntructuredVolumeArrays
{
  viskores::cont::ArrayHandle<viskores::Vec3f_32> VertexPosition;
  viskores::cont::ArrayHandle<viskores::Float32> VertexData;
  viskores::cont::ArrayHandle<viskores::UInt32> Index;
  viskores::cont::ArrayHandle<viskores::UInt32> CellIndex;
  viskores::cont::ArrayHandle<viskores::Float32> CellData;
  viskores::cont::ArrayHandle<viskores::UInt8> CellType;
  std::shared_ptr<viskores::cont::Token> Token{ new viskores::cont::Token };
};

/// @brief Mapper which turns structured or unstructured data into an ANARI
/// `transferFunction1D` volume.
///
/// A `structuredRegular` field requires a 3D structured cell set, uniform point coordinates,
/// a non-empty point-associated scalar field with one value per point, and at least two samples
/// in each dimension. Inputs which do not meet these requirements are rejected with
/// `viskores::cont::ErrorBadValue`; they are never silently reinterpreted as regular grids.
///
/// An `unstructured` field requires an ANARI device advertising
/// `ANARI_KHR_SPATIAL_FIELD_UNSTRUCTURED`, a `CellSetSingleType` or `CellSetExplicit` containing
/// only tetrahedra, hexahedra, wedges, and pyramids, and a point- or cell-associated scalar field
/// with matching cardinality. Connectivity and cell offsets must fit in `viskores::UInt32`.
/// Invalid inputs are rejected before an ANARI spatial field is created. Actor replacements build
/// a complete spatial-field state before replacing the previous state.
///
/// A completely empty actor represents an empty mapper and does not create ANARI objects.
///
/// NOTE: This currently only supports Float32 scalar fields. In the future this
/// mapper will also support Uint8, Uint16, and Float64 scalar fields.
struct VISKORES_ANARI_EXPORT ANARIMapperVolume : public ANARIMapper
{
  /// @brief Constructor
  ///
  ANARIMapperVolume(
    anari_cpp::Device device,
    const ANARIActor& actor = {},
    const std::string& name = "<volume>",
    const viskores::cont::ColorTable& colorTable = viskores::cont::ColorTable::Preset::Default);

  ANARIMapperVolume(const ANARIMapperVolume&) = delete;
  ANARIMapperVolume(ANARIMapperVolume&&);
  ANARIMapperVolume& operator=(const ANARIMapperVolume&) = delete;
  ANARIMapperVolume& operator=(ANARIMapperVolume&&) = delete;

  /// @brief Destructor
  ///
  ~ANARIMapperVolume() override;

  anari_cpp::SpatialField GetANARISpatialField() override;
  anari_cpp::Volume GetANARIVolume() override;

private:
  /// @brief Construct a complete candidate spatial-field state and install it.
  void ConstructArrays();

  /// @brief Atomic state for a spatial field and its backing arrays.
  struct ANARISpatialFieldState
  {
    anari_cpp::Device Device{ nullptr };
    anari_cpp::SpatialField SpatialField{ nullptr };
    StructuredVolumeParameters StructuredParameters;
    UnstructuredVolumeParameters UnstructuredParameters;
    StructuredVolumeArrays StructuredArrays;
    UntructuredVolumeArrays UnstructuredArrays;
    ~ANARISpatialFieldState();
    void ReleaseArrays();
  };

  /// @brief Update an ANARISpatialField object with its prepared state.
  void UpdateSpatialField(ANARISpatialFieldState& state);
  void UpdateMaterializedObjects() override;

  /// @brief Container of all relevant ANARI scene object handles.
  struct ANARIHandles
  {
    anari_cpp::Device Device{ nullptr };
    anari_cpp::Volume Volume{ nullptr };
    std::unique_ptr<ANARISpatialFieldState> Field;
    ~ANARIHandles();
  };

  std::unique_ptr<ANARIHandles> Handles;
};

} // namespace anari
} // namespace interop
} // namespace viskores

#endif
