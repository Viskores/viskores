//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_interop_anari_ANARIMapperVolume_h
#define vtk_m_interop_anari_ANARIMapperVolume_h

#include <vtkm/interop/anari/ANARIMapper.h>

namespace vtkm
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
  bool IndexPrefixed{ false };
};

/// @brief VTK-m data arrays underlying the `ANARIArray` handles created by the mapper.
///
struct StructuredVolumeArrays
{
  vtkm::cont::ArrayHandle<vtkm::Float32> Data;
  std::shared_ptr<vtkm::cont::Token> Token{ new vtkm::cont::Token };
};

struct UntructuredVolumeArrays
{
  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> VertexPosition;
  vtkm::cont::ArrayHandle<vtkm::Float32> VertexData;
  vtkm::cont::ArrayHandle<vtkm::UInt64> Index;
  vtkm::cont::ArrayHandle<vtkm::UInt64> CellIndex;
  vtkm::cont::ArrayHandle<vtkm::Float32> CellData;
  vtkm::cont::ArrayHandle<vtkm::UInt8> CellType;
  std::shared_ptr<vtkm::cont::Token> Token{ new vtkm::cont::Token };
};

/// @brief Mapper which turns structured volumes into a single ANARI `transferFunction1D` volume.
///
/// NOTE: This currently only supports Float32 scalar fields. In the future this
/// mapper will also support Uint8, Uint16, and Float64 scalar fields.
struct VTKM_ANARI_EXPORT ANARIMapperVolume : public ANARIMapper
{
  /// @brief Constructor
  ///
  ANARIMapperVolume(
    anari_cpp::Device device,
    const ANARIActor& actor = {},
    const std::string& name = "<volume>",
    const vtkm::cont::ColorTable& colorTable = vtkm::cont::ColorTable::Preset::Default);

  /// @brief Destructor
  ///
  ~ANARIMapperVolume() override;

  /// @brief Set the current actor on this mapper.
  ///
  /// See `ANARIMapper` for more detail.
  void SetActor(const ANARIActor& actor) override;

  /// @brief Set color map arrays using raw ANARI array handles.
  ///
  /// See `ANARIMapper` for more detail.
  void SetANARIColorMap(anari_cpp::Array1D color,
                        anari_cpp::Array1D opacity,
                        bool releaseArrays = true) override;
  /// @brief Set the value range (domain) for the color map.
  ///
  void SetANARIColorMapValueRange(const vtkm::Vec2f_32& valueRange) override;

  /// @brief Set a scale factor for opacity.
  ///
  void SetANARIColorMapOpacityScale(vtkm::Float32 opacityScale) override;

  anari_cpp::SpatialField GetANARISpatialField() override;
  anari_cpp::Volume GetANARIVolume() override;

private:
  /// @brief Do the work to construct the basic ANARI arrays for the ANARIGeometry.
  /// @param regenerate Force the position/radius arrays are regenerated.
  ///
  void ConstructArrays(bool regenerate = false);
  /// @brief Update ANARISpatialField object with the latest data from the actor.
  void UpdateSpatialField();

  /// @brief Container of all relevant ANARI scene object handles.
  struct ANARIHandles
  {
    anari_cpp::Device Device{ nullptr };
    anari_cpp::SpatialField SpatialField{ nullptr };
    anari_cpp::Volume Volume{ nullptr };
    StructuredVolumeParameters StructuredParameters;
    UnstructuredVolumeParameters UnstructuredParameters;
    ~ANARIHandles();
    void ReleaseArrays();
  };

  std::shared_ptr<ANARIHandles> Handles;
  StructuredVolumeArrays StructuredArrays;
  UntructuredVolumeArrays UnstructuredArrays;
};

} // namespace anari
} // namespace interop
} // namespace vtkm

#endif
