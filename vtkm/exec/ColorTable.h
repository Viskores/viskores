//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_exec_ColorTable_h
#define vtk_m_exec_ColorTable_h

#include <vtkm/VirtualObjectBase.h>

namespace vtkm
{
namespace exec
{

class VTKM_ALWAYS_EXPORT ColorTableBase : public vtkm::VirtualObjectBase
{
public:
  inline VTKM_EXEC vtkm::Vec<float, 3> MapThroughColorSpace(double) const;

  VTKM_EXEC virtual vtkm::Vec<float, 3> MapThroughColorSpace(const vtkm::Vec<float, 3>& rgb1,
                                                             const vtkm::Vec<float, 3>& rgb2,
                                                             float weight) const = 0;

  inline VTKM_EXEC float MapThroughOpacitySpace(double value) const;

  double const* ColorNodes = nullptr;
  vtkm::Vec<float, 3> const* RGB = nullptr;

  double const* ONodes = nullptr;
  float const* Alpha = nullptr;
  vtkm::Vec<float, 2> const* MidSharp = nullptr;

  vtkm::Int32 ColorSize = 0;
  vtkm::Int32 OpacitySize = 0;

  vtkm::Vec<float, 3> NaNColor = { 0.5f, 0.0f, 0.0f };
  vtkm::Vec<float, 3> BelowRangeColor = { 0.0f, 0.0f, 0.0f };
  vtkm::Vec<float, 3> AboveRangeColor = { 0.0f, 0.0f, 0.0f };

  bool UseClamping = true;

private:
  inline VTKM_EXEC void FindColors(double value,
                                   vtkm::Vec<float, 3>& first,
                                   vtkm::Vec<float, 3>& second,
                                   float& weight) const;
};

class VTKM_ALWAYS_EXPORT ColorTableRGB final : public ColorTableBase
{
public:
  inline VTKM_EXEC vtkm::Vec<float, 3> MapThroughColorSpace(const vtkm::Vec<float, 3>& rgb1,
                                                            const vtkm::Vec<float, 3>& rgb2,
                                                            float weight) const;
};

class VTKM_ALWAYS_EXPORT ColorTableHSV final : public ColorTableBase
{
public:
  inline VTKM_EXEC vtkm::Vec<float, 3> MapThroughColorSpace(const vtkm::Vec<float, 3>& rgb1,
                                                            const vtkm::Vec<float, 3>& rgb2,
                                                            float weight) const;
};

class VTKM_ALWAYS_EXPORT ColorTableHSVWrap final : public ColorTableBase
{
public:
  inline VTKM_EXEC vtkm::Vec<float, 3> MapThroughColorSpace(const vtkm::Vec<float, 3>& rgb1,
                                                            const vtkm::Vec<float, 3>& rgb2,
                                                            float weight) const;
};

class VTKM_ALWAYS_EXPORT ColorTableLab final : public ColorTableBase
{
public:
  inline VTKM_EXEC vtkm::Vec<float, 3> MapThroughColorSpace(const vtkm::Vec<float, 3>& rgb1,
                                                            const vtkm::Vec<float, 3>& rgb2,
                                                            float weight) const;
};

class VTKM_ALWAYS_EXPORT ColorTableDiverging final : public ColorTableBase
{
public:
  inline VTKM_EXEC vtkm::Vec<float, 3> MapThroughColorSpace(const vtkm::Vec<float, 3>& rgb1,
                                                            const vtkm::Vec<float, 3>& rgb2,
                                                            float weight) const;
};
}
}

#include <vtkm/exec/ColorTable.hxx>

#endif
