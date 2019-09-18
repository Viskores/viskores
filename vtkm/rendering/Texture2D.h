//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_rendering_Texture2D_h
#define vtk_m_rendering_Texture2D_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ExecutionObjectBase.h>

namespace vtkm
{
namespace rendering
{

enum class TextureFilterMode
{
  NearestNeighbour,
  Linear,
}; // enum TextureFilterMode

enum class TextureWrapMode
{
  Clamp,
  Repeat,
}; // enum TextureWrapMode

template <vtkm::IdComponent NumComponents>
class Texture2D
{
public:
  using TextureDataHandle = typename vtkm::cont::ArrayHandle<vtkm::UInt8>;
  using ColorType = vtkm::Vec<vtkm::Float32, NumComponents>;

  class Texture2DSampler;

#define UV_BOUNDS_CHECK(u, v, NoneType)                                                            \
  if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)                                                \
  {                                                                                                \
    return NoneType();                                                                             \
  }

  VTKM_CONT
  Texture2D()
    : Width(0)
    , Height(0)
  {
  }

  VTKM_CONT
  Texture2D(vtkm::Id width, vtkm::Id height, const TextureDataHandle& data)
    : Width(width)
    , Height(height)
    , FilterMode(TextureFilterMode::Linear)
    , WrapMode(TextureWrapMode::Clamp)
  {
    VTKM_ASSERT(data.GetNumberOfValues() == (Width * Height * NumComponents));
    // We do not know the lifetime of the underlying data source of input `data`. Since it might
    // be from a shallow copy of the data source, we make a deep copy of the input data and keep
    // it's portal. The copy operation is very fast.
    vtkm::cont::Algorithm::Copy(data, Data);
  }

  VTKM_CONT
  bool IsValid() const { return Width > 0 && Height > 0; }

  VTKM_CONT
  TextureFilterMode GetFilterMode() const { return this->FilterMode; }

  VTKM_CONT
  void SetFilterMode(TextureFilterMode filterMode) { this->FilterMode = filterMode; }

  VTKM_CONT
  TextureWrapMode GetWrapMode() const { return this->WrapMode; }

  VTKM_CONT
  void SetWrapMode(TextureWrapMode wrapMode) { this->WrapMode = wrapMode; }

  VTKM_CONT Texture2DSampler GetExecObjectFactory() const
  {
    return Texture2DSampler(Width, Height, Data, FilterMode, WrapMode);
  }

  template <typename Device>
  class Texture2DSamplerExecutionObject
  {
  public:
    using TextureExecPortal =
      typename TextureDataHandle::template ExecutionTypes<Device>::PortalConst;

    VTKM_CONT
    Texture2DSamplerExecutionObject()
      : Width(0)
      , Height(0)
    {
    }

    VTKM_CONT
    Texture2DSamplerExecutionObject(vtkm::Id width,
                                    vtkm::Id height,
                                    const TextureDataHandle& data,
                                    TextureFilterMode filterMode,
                                    TextureWrapMode wrapMode)
      : Width(width)
      , Height(height)
      , Data(data.PrepareForInput(Device()))
      , FilterMode(filterMode)
      , WrapMode(wrapMode)
    {
    }

    VTKM_EXEC
    inline ColorType GetColor(vtkm::Float32 u, vtkm::Float32 v) const
    {
      v = 1.0f - v;
      UV_BOUNDS_CHECK(u, v, ColorType);
      switch (FilterMode)
      {
        case TextureFilterMode::NearestNeighbour:
          return GetNearestNeighbourFilteredColor(u, v);

        case TextureFilterMode::Linear:
          return GetLinearFilteredColor(u, v);

        default:
          return ColorType();
      }
    }

  private:
    VTKM_EXEC
    inline ColorType GetNearestNeighbourFilteredColor(vtkm::Float32 u, vtkm::Float32 v) const
    {
      vtkm::Id x = static_cast<vtkm::Id>(vtkm::Round(u * static_cast<vtkm::Float32>(Width - 1)));
      vtkm::Id y = static_cast<vtkm::Id>(vtkm::Round(v * static_cast<vtkm::Float32>(Height - 1)));
      return GetColorAtCoords(x, y);
    }

    VTKM_EXEC
    inline ColorType GetLinearFilteredColor(vtkm::Float32 u, vtkm::Float32 v) const
    {
      u = u * static_cast<vtkm::Float32>(Width) - 0.5f;
      v = v * static_cast<vtkm::Float32>(Height) - 0.5f;
      vtkm::Id x = static_cast<vtkm::Id>(vtkm::Floor(u));
      vtkm::Id y = static_cast<vtkm::Id>(vtkm::Floor(v));
      vtkm::Float32 uRatio = u - static_cast<vtkm::Float32>(x);
      vtkm::Float32 vRatio = v - static_cast<vtkm::Float32>(y);
      vtkm::Float32 uOpposite = 1.0f - uRatio;
      vtkm::Float32 vOpposite = 1.0f - vRatio;
      vtkm::Id xn, yn;
      GetNextCoords(x, y, xn, yn);
      ColorType c1 = GetColorAtCoords(x, y);
      ColorType c2 = GetColorAtCoords(xn, y);
      ColorType c3 = GetColorAtCoords(x, yn);
      ColorType c4 = GetColorAtCoords(xn, yn);
      return (c1 * uOpposite + c2 * uRatio) * vOpposite + (c3 * uOpposite + c4 * uRatio) * vRatio;
    }

    VTKM_EXEC
    inline ColorType GetColorAtCoords(vtkm::Id x, vtkm::Id y) const
    {
      vtkm::Id idx = (y * Width + x) * NumComponents;
      ColorType color;
      for (vtkm::IdComponent i = 0; i < NumComponents; ++i)
      {
        color[i] = Data.Get(idx + i) / 255.0f;
      }
      return color;
    }

    VTKM_EXEC
    inline void GetNextCoords(vtkm::Id x, vtkm::Id y, vtkm::Id& xn, vtkm::Id& yn) const
    {
      switch (WrapMode)
      {
        case TextureWrapMode::Clamp:
          xn = (x + 1) < Width ? (x + 1) : x;
          yn = (y + 1) < Height ? (y + 1) : y;
          break;
        case TextureWrapMode::Repeat:
        default:
          xn = (x + 1) % Width;
          yn = (y + 1) % Height;
          break;
      }
    }

    vtkm::Id Width;
    vtkm::Id Height;
    TextureExecPortal Data;
    TextureFilterMode FilterMode;
    TextureWrapMode WrapMode;
  };

  class Texture2DSampler : public vtkm::cont::ExecutionObjectBase
  {
  public:
    VTKM_CONT
    Texture2DSampler()
      : Width(0)
      , Height(0)
    {
    }

    VTKM_CONT
    Texture2DSampler(vtkm::Id width,
                     vtkm::Id height,
                     const TextureDataHandle& data,
                     TextureFilterMode filterMode,
                     TextureWrapMode wrapMode)
      : Width(width)
      , Height(height)
      , Data(data)
      , FilterMode(filterMode)
      , WrapMode(wrapMode)
    {
    }

    template <typename Device>
    VTKM_CONT Texture2DSamplerExecutionObject<Device> PrepareForExecution(Device) const
    {
      return Texture2DSamplerExecutionObject<Device>(
        this->Width, this->Height, this->Data, this->FilterMode, this->WrapMode);
    }

  private:
    vtkm::Id Width;
    vtkm::Id Height;
    TextureDataHandle Data;
    TextureFilterMode FilterMode;
    TextureWrapMode WrapMode;
  }; // class Texture2DSampler

private:
  vtkm::Id Width;
  vtkm::Id Height;
  TextureDataHandle Data;
  TextureFilterMode FilterMode;
  TextureWrapMode WrapMode;
}; // class Texture2D
}
} // namespace vtkm::rendering

#endif // vtk_m_rendering_Texture2D_h
