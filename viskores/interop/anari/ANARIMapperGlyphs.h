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

#ifndef viskores_interop_anari_ANARIMapperGlyphs_h
#define viskores_interop_anari_ANARIMapperGlyphs_h

#include <viskores/interop/anari/ANARIMapper.h>

namespace viskores
{
namespace interop
{
namespace anari
{

/// @brief Raw ANARI arrays and parameter values set on the `ANARIGeometry`.
///
struct GlyphsParameters
{
  struct VertexData
  {
    anari_cpp::Array1D Position{ nullptr };
    anari_cpp::Array1D Radius{ nullptr };
  } Vertex{};

  unsigned int NumPrimitives{ 0 };
};

/// @brief Viskores data arrays underlying the `ANARIArray` handles created by the mapper.
///
struct GlyphArrays
{
  viskores::cont::ArrayHandle<viskores::Vec3f_32> Vertices;
  viskores::cont::ArrayHandle<viskores::Float32> Radii;
  std::shared_ptr<viskores::cont::Token> Token{ new viskores::cont::Token };
};

/// @brief Mapper which turns vector data into arrow glyphs.
///
/// This mapper creates ANARI `cone` geometry for the primary field in the
/// provided `ANARIActor`. The field must contain one three-component vector
/// per point or cell. Vector magnitude does not affect glyph length: nonzero
/// vectors are normalized and used only for direction. Zero and non-finite
/// vectors produce invisible, zero-radius glyphs at their sample positions.
///
/// Glyph size is based on the coordinate-bounds diagonal divided by 300. The
/// generated arrow length is twice that size. Degenerate or non-finite bounds
/// use a size of 0.01 instead. Materialization throws
/// `viskores::cont::ErrorBadValue` when the device does not support
/// `ANARI_KHR_GEOMETRY_CONE` and the `cone` geometry subtype.
struct VISKORES_ANARI_EXPORT ANARIMapperGlyphs : public ANARIMapper
{
  /// @brief Constructor
  ///
  ANARIMapperGlyphs(
    anari_cpp::Device device,
    const ANARIActor& actor = {},
    const char* name = "<glyphs>",
    const viskores::cont::ColorTable& colorTable = viskores::cont::ColorTable::Preset::Default);

  ANARIMapperGlyphs(const ANARIMapperGlyphs&) = delete;
  ANARIMapperGlyphs(ANARIMapperGlyphs&&);
  ANARIMapperGlyphs& operator=(const ANARIMapperGlyphs&) = delete;
  ANARIMapperGlyphs& operator=(ANARIMapperGlyphs&&) = delete;

  /// @brief Destructor
  ///
  ~ANARIMapperGlyphs() override;

  /// @brief Offset the glyph in the direction of the vector at each point.
  ///
  /// This will cause the mapper to offset the glyph, making the arrow appear to
  /// be coming out of the point instead of going through it. This is useful for
  /// visualizing things like surface normals on a mesh. Changes take effect on
  /// already-materialized geometry as well as geometry created later.
  void SetOffsetGlyphs(bool enabled);

  /// @brief Get the corresponding ANARIGeometry handle from this mapper.
  ///
  /// NOTE: This handle is not retained, so applications should not release it.
  anari_cpp::Geometry GetANARIGeometry() override;

  /// @brief Get the corresponding ANARISurface handle from this mapper.
  ///
  /// NOTE: This handle is not retained, so applications should not release it.
  anari_cpp::Surface GetANARISurface() override;

private:
  /// @brief Do the work to construct the basic ANARI arrays for the ANARIGeometry.
  ///
  void ConstructArrays();
  /// @brief Update ANARIGeometry object with the latest data from the actor.
  void UpdateGeometry();
  void UpdateMaterializedObjects() override;

  /// @brief Owner of ANARI scene handles and the Viskores arrays backing them.
  struct ANARIHandles
  {
    anari_cpp::Device Device{ nullptr };
    anari_cpp::Geometry Geometry{ nullptr };
    anari_cpp::Material Material{ nullptr };
    anari_cpp::Surface Surface{ nullptr };
    GlyphsParameters Parameters;
    GlyphArrays Arrays;
    ~ANARIHandles();
    void ReleaseArrays();
  };

  std::unique_ptr<ANARIHandles> Handles;

  bool Offset{ false };
};

} // namespace anari
} // namespace interop
} // namespace viskores

#endif
