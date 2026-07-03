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

#ifndef viskores_interop_anari_ANARIMapper_h
#define viskores_interop_anari_ANARIMapper_h

// viskores
#include <viskores/cont/ColorTable.h>
#include <viskores/interop/anari/ANARIActor.h>
// std
#include <memory>

#include <viskores/interop/anari/viskores_anari_export.h>

namespace viskores
{
namespace interop
{
namespace anari
{

inline void NoopANARIDeleter(const void*, const void*) {}

/// @brief This is the base class used for all ANARI mappers.
///
/// This class implements shared functionality of all ANARI mappers. All ANARI
/// object handle lifetimes are tied to the lifetime of the mapper, including
/// the device.  Applications are not intended to ever release handles received
/// from the mapper, unless they manually retain the handle. Additionally,
/// ANARIMappers will update surface or volume objects if changes occur, such as
/// changes to the color map or ANARIActor.
///
/// Mappers have unique ownership of their ANARI objects and backing Viskores
/// arrays. They may be moved into a scene, but they cannot be copied.
struct VISKORES_ANARI_EXPORT ANARIMapper
{
  ANARIMapper(
    anari_cpp::Device device,
    const ANARIActor& actor = {},
    const std::string& name = "<noname>",
    const viskores::cont::ColorTable& colorTable = viskores::cont::ColorTable::Preset::Default);
  virtual ~ANARIMapper();

  ANARIMapper(const ANARIMapper&) = delete;
  ANARIMapper(ANARIMapper&&);
  ANARIMapper& operator=(const ANARIMapper&) = delete;
  ANARIMapper& operator=(ANARIMapper&&) = delete;

  anari_cpp::Device GetDevice() const;
  const ANARIActor& GetActor() const;
  const char* GetName() const;
  const viskores::cont::ColorTable& GetColorTable() const;

  /// @brief Change the name applied to this mapper and its materialized ANARI objects.
  void SetName(const char* name);

  /// @brief Use a sampled Viskores color table for subsequent and existing ANARI objects.
  ///
  /// Calling this switches away from any raw arrays supplied by `SetANARIColorMap`.
  void SetColorTable(const viskores::cont::ColorTable& colorTable);

  /// @brief Set the current actor on this mapper.
  ///
  /// This sets the actor used to create the geometry. When the actor is changed
  /// the mapper will update all the corresponding ANARI objects accordingly.
  /// This will not cause new ANARI geometry handles to be made, rather the
  /// existing handles will be updated to reflect the new actor's data.
  virtual void SetActor(const ANARIActor& actor);

  /// @brief Set whether fields from `ANARIActor` should end up as geometry attributes.
  ///
  /// When this is disabled, the mapper will skip creating the data arrays
  /// associated with fields for when applications only want the raw geometry.
  /// This defaults to being enabled.
  virtual void SetMapFieldAsAttribute(bool enabled);
  bool GetMapFieldAsAttribute() const;

  /// @brief Set color map arrays using raw ANARI array handles.
  /// @param color Color array used for color mapping.
  /// @param opacity Opacity array used by volume mappers and ignored by surface mappers.
  /// This is an alternative to the sampled `ColorTable` path. The mapper keeps the arrays alive
  /// so the setting also applies when called before the corresponding ANARI object is created.
  /// @param releaseArrays If true, ownership of the caller's references is transferred to the
  /// mapper. If false, the mapper retains its own references and the caller remains responsible
  /// for releasing its references.
  virtual void SetANARIColorMap(anari_cpp::Array1D color,
                                anari_cpp::Array1D opacity,
                                bool releaseArrays = true);

  /// @brief Set the value range (domain) for the color map.
  ///
  virtual void SetANARIColorMapValueRange(const viskores::Vec2f_32& valueRange);

  /// @brief Set a scale factor for opacity (typically used for volumes).
  ///
  virtual void SetANARIColorMapOpacityScale(viskores::Float32 opacityScale);

  /// @brief Get the corresponding ANARIGeometry handle from this mapper.
  ///
  /// NOTE: This handle is not retained, so applications should not release it.
  virtual anari_cpp::Geometry GetANARIGeometry();

  /// @brief Get the corresponding ANARISpatialField handle from this mapper.
  ///
  /// NOTE: This handle is not retained, so applications should not release it.
  virtual anari_cpp::SpatialField GetANARISpatialField();

  /// @brief Get the corresponding ANARISurface handle from this mapper.
  ///
  /// NOTE: This handle is not retained, so applications should not release it.
  virtual anari_cpp::Surface GetANARISurface();

  /// @brief Get the corresponding ANARIVolume handle from this mapper.
  ///
  /// NOTE: This handle is not retained, so applications should not release it.
  virtual anari_cpp::Volume GetANARIVolume();

  anari_cpp::Group GetANARIGroup();
  anari_cpp::Instance GetANARIInstance();

  bool GroupIsEmpty() const;

protected:
  enum class DirtyCategory : viskores::UInt8
  {
    Data = 1 << 0,
    Topology = 1 << 1,
    Attributes = 1 << 2,
    Appearance = 1 << 3,
    Names = 1 << 4
  };

  std::string MakeObjectName(const char* suffix) const;
  void RefreshGroup();

  bool IsDirty(DirtyCategory category) const;
  void MarkDirty(DirtyCategory category);
  void ClearDirty(DirtyCategory category);

  /// Apply pending state to ANARI objects which have already been materialized.
  virtual void UpdateMaterializedObjects();

  /// Shared transfer-function construction for surface and volume mappers.
  void UpdateANARISampler(anari_cpp::Sampler sampler, const char* filter);
  void UpdateANARIVolumeAppearance(anari_cpp::Volume volume);

  bool Valid{ false };

private:
  void EnsureSampledColorArrays();
  void UpdateDefaultValueRange();

  struct ANARIHandles
  {
    anari_cpp::Device Device{ nullptr };
    anari_cpp::Group Group{ nullptr };
    anari_cpp::Instance Instance{ nullptr };
    ~ANARIHandles();
  };

  struct AppearanceState;

  std::unique_ptr<ANARIHandles> Handles;
  ANARIActor Actor;
  viskores::cont::ColorTable ColorTable;
  std::string Name;
  bool MapFieldAsAttribute{ true };
  viskores::UInt8 DirtyCategories{ (1 << 5) - 1 };
  std::unique_ptr<AppearanceState> Appearance;
};

} // namespace anari
} // namespace interop
} // namespace viskores

#endif
