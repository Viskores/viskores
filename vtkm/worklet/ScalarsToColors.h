//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_ScalarsToColors_h
#define vtk_m_worklet_ScalarsToColors_h

#include <vtkm/Range.h>
#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace worklet
{

namespace colorconversion
{
inline void ComputeShiftScale(const vtkm::Range& range, vtkm::Float32& shift, vtkm::Float32& scale)
{
  //This scale logic seems to be unduly complicated
  shift = static_cast<vtkm::Float32>(-range.Min);
  scale = static_cast<vtkm::Float32>(range.Length());

  if (range.Length() <= 0)
  {
    scale = -1e17f;
  }
  if (scale * scale > 1e-30f)
  {
    scale = 1.0f / scale;
  }
  scale *= 255.0f;
}
}

class ScalarsToColors
{
  vtkm::Range ValueRange = { 0.0f, 255.0f };
  vtkm::Float32 Alpha = 1.0f;
  vtkm::Float32 Shift = 0.0f;
  vtkm::Float32 Scale = 1.0f;

public:
  ScalarsToColors() {}

  ScalarsToColors(const vtkm::Range& range, vtkm::Float32 alpha)
    : ValueRange(range)
    , Alpha(vtkm::Min(vtkm::Max(alpha, 0.0f), 1.0f))
  {
    colorconversion::ComputeShiftScale(range, this->Shift, this->Scale);
  }

  ScalarsToColors(const vtkm::Range& range)
    : ValueRange(range)
  {
    colorconversion::ComputeShiftScale(range, this->Shift, this->Scale);
  }

  ScalarsToColors(vtkm::Float32 alpha)
    : ValueRange(0.0f, 255.0f)
    , Alpha(vtkm::Min(vtkm::Max(alpha, 0.0f), 1.0f))
  {
  }

  void SetRange(const vtkm::Range& range)
  {
    this->ValueRange = range;
    colorconversion::ComputeShiftScale(range, this->Shift, this->Scale);
  }

  vtkm::Range GetRange() const { return this->ValueRange; }

  void SetAlpha(vtkm::Float32 alpha) { this->Alpha = vtkm::Min(vtkm::Max(alpha, 0.0f), 1.0f); }

  vtkm::Float32 GetAlpha() const { return this->Alpha; }

  /// \brief Use each component to generate RGBA colors
  ///
  template <typename T, typename S>
  void Run(const vtkm::cont::ArrayHandle<T, S>& values,
           vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Use each component to generate RGB colors
  ///
  template <typename T, typename S>
  void Run(const vtkm::cont::ArrayHandle<T, S>& values,
           vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const;


  /// \brief Use magnitude of a vector to generate RGBA colors
  ///
  template <typename T, int N, typename S>
  void RunMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                    vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Use magnitude of a vector to generate RGB colors
  ///
  template <typename T, int N, typename S>
  void RunMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                    vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const;

  /// \brief Use a single component of a vector to generate RGBA colors
  ///
  template <typename T, int N, typename S>
  void RunComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                    vtkm::IdComponent comp,
                    vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Use a single component of a vector to generate RGB colors
  ///
  template <typename T, int N, typename S>
  void RunComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                    vtkm::IdComponent comp,
                    vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const;
};
}
}

#include <vtkm/worklet/ScalarsToColors.hxx>

#endif
