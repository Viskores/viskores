//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_ColorTable_h
#define vtk_m_exec_ColorTable_h

#include <vtkm/Deprecated.h>
#include <vtkm/Types.h>

namespace vtkm
{

enum struct ColorSpace
{
  RGB,
  HSV,
  HSVWrap,
  Lab,
  Diverging
};

} // namespace vtkm

namespace vtkm
{
namespace exec
{

class VTKM_ALWAYS_EXPORT ColorTable
{
public:
  inline VTKM_EXEC vtkm::Vec3f_32 MapThroughColorSpace(vtkm::Float64) const;

  inline VTKM_EXEC vtkm::Vec3f_32 MapThroughColorSpace(const vtkm::Vec3f_32& rgb1,
                                                       const vtkm::Vec3f_32& rgb2,
                                                       vtkm::Float32 weight) const;

  inline VTKM_EXEC vtkm::Float32 MapThroughOpacitySpace(vtkm::Float64 value) const;

  vtkm::ColorSpace Space;

  vtkm::Float64 const* ColorNodes = nullptr;
  vtkm::Vec3f_32 const* RGB = nullptr;

  vtkm::Float64 const* ONodes = nullptr;
  vtkm::Float32 const* Alpha = nullptr;
  vtkm::Vec2f_32 const* MidSharp = nullptr;

  vtkm::Int32 ColorSize = 0;
  vtkm::Int32 OpacitySize = 0;

  vtkm::Vec3f_32 NaNColor = { 0.5f, 0.0f, 0.0f };
  vtkm::Vec3f_32 BelowRangeColor = { 0.0f, 0.0f, 0.0f };
  vtkm::Vec3f_32 AboveRangeColor = { 0.0f, 0.0f, 0.0f };

  bool UseClamping = true;

private:
  inline VTKM_EXEC void FindColors(vtkm::Float64 value,
                                   vtkm::Vec3f_32& first,
                                   vtkm::Vec3f_32& second,
                                   vtkm::Float32& weight) const;

  inline VTKM_EXEC vtkm::Vec3f_32 MapThroughColorSpaceRGB(const vtkm::Vec3f_32& rgb1,
                                                          const vtkm::Vec3f_32& rgb2,
                                                          vtkm::Float32 weight) const;

  inline VTKM_EXEC vtkm::Vec3f_32 MapThroughColorSpaceHSV(const vtkm::Vec3f_32& rgb1,
                                                          const vtkm::Vec3f_32& rgb2,
                                                          vtkm::Float32 weight) const;

  inline VTKM_EXEC vtkm::Vec3f_32 MapThroughColorSpaceHSVWrap(const vtkm::Vec3f_32& rgb1,
                                                              const vtkm::Vec3f_32& rgb2,
                                                              vtkm::Float32 weight) const;

  inline VTKM_EXEC vtkm::Vec3f_32 MapThroughColorSpaceLab(const vtkm::Vec3f_32& rgb1,
                                                          const vtkm::Vec3f_32& rgb2,
                                                          vtkm::Float32 weight) const;

  inline VTKM_EXEC vtkm::Vec3f_32 MapThroughColorSpaceDiverging(const vtkm::Vec3f_32& rgb1,
                                                                const vtkm::Vec3f_32& rgb2,
                                                                vtkm::Float32 weight) const;
};

class VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(1.6, "Use vtkm::exec::ColorTable.") ColorTableBase
  : public vtkm::exec::ColorTable
{
};

class VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(1.6, "Use vtkm::exec::ColorTable.") ColorTableRGB final
  : public ColorTable
{
public:
  ColorTableRGB() { this->Space = vtkm::ColorSpace::RGB; }
};

class VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(1.6, "Use vtkm::exec::ColorTable.") ColorTableHSV final
  : public ColorTable
{
public:
  ColorTableHSV() { this->Space = vtkm::ColorSpace::HSV; }
};

class VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(1.6, "Use vtkm::exec::ColorTable.") ColorTableHSVWrap final
  : public ColorTable
{
public:
  ColorTableHSVWrap() { this->Space = vtkm::ColorSpace::HSVWrap; }
};

class VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(1.6, "Use vtkm::exec::ColorTable.") ColorTableLab final
  : public ColorTable
{
public:
  ColorTableLab() { this->Space = vtkm::ColorSpace::Lab; }
};

class VTKM_ALWAYS_EXPORT ColorTableDiverging final : public ColorTable
{
public:
  ColorTableDiverging() { this->Space = vtkm::ColorSpace::Diverging; }
};
}
}

#include <vtkm/exec/ColorTable.hxx>

#endif
