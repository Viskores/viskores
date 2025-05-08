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

#ifndef viskores_exec_CubicHermiteSpline_h
#define viskores_exec_CubicHermiteSpline_h

#include <viskores/ErrorCode.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>

namespace viskores
{
namespace exec
{

class VISKORES_ALWAYS_EXPORT CubicHermiteSpline
{
private:
  using DataArrayHandleType = viskores::cont::ArrayHandle<viskores::Vec3f>;
  using KnotsArrayHandleType = viskores::cont::ArrayHandle<viskores::FloatDefault>;
  using TangentArrayHandleType = viskores::cont::ArrayHandle<viskores::Vec3f>;
  using DataPortalType = typename DataArrayHandleType::ReadPortalType;
  using KnotsPortalType = typename KnotsArrayHandleType::ReadPortalType;
  using TangentPortalType = typename TangentArrayHandleType::ReadPortalType;

public:
  VISKORES_CONT CubicHermiteSpline(const viskores::cont::ArrayHandle<viskores::Vec3f>& data,
                                   const viskores::cont::ArrayHandle<viskores::FloatDefault>& knots,
                                   const viskores::cont::ArrayHandle<viskores::Vec3f>& tangents,
                                   viskores::cont::DeviceAdapterId device,
                                   viskores::cont::Token& token)
  {
    this->Data = data.PrepareForInput(device, token);
    this->Knots = knots.PrepareForInput(device, token);
    this->Tangents = tangents.PrepareForInput(device, token);
  }

  VISKORES_EXEC viskores::ErrorCode Evaluate(const viskores::FloatDefault& tVal,
                                             viskores::Vec3f& val) const
  {
    viskores::Vec3f p0, p1, m0, m1;
    viskores::FloatDefault t0, t1;

    if (!this->GetInterval(tVal, p0, p1, t0, t1, m0, m1))
      return viskores::ErrorCode::ValueOutOfRange;

    val = this->EvaluatePosition(tVal, p0, p1, t0, t1, m0, m1);
    return viskores::ErrorCode::Success;
  }

private:
  VISKORES_EXEC bool GetInterval(const viskores::FloatDefault& tVal,
                                 viskores::Vec3f& p0,
                                 viskores::Vec3f& p1,
                                 viskores::FloatDefault& t0,
                                 viskores::FloatDefault& t1,
                                 viskores::Vec3f& m0,
                                 viskores::Vec3f& m1) const
  {
    viskores::Id idx = this->FindParametricInterval(tVal);
    if (idx < 0)
      return false;

    t0 = this->Knots.Get(idx);
    t1 = this->Knots.Get(idx + 1);
    p0 = this->Data.Get(idx);
    p1 = this->Data.Get(idx + 1);
    m0 = this->Tangents.Get(idx);
    m1 = this->Tangents.Get(idx + 1);

    return true;
  }

  VISKORES_EXEC viskores::Vec3f EvaluatePosition(const viskores::FloatDefault& tVal,
                                                 const viskores::Vec3f& p0,
                                                 const viskores::Vec3f& p1,
                                                 const viskores::FloatDefault& t0,
                                                 const viskores::FloatDefault& t1,
                                                 const viskores::Vec3f& m0,
                                                 const viskores::Vec3f& m1) const
  {
    viskores::Vec3f result;

    // Hermite basis functions.
    viskores::FloatDefault dt = t1 - t0;
    viskores::FloatDefault tNorm = (tVal - t0) / dt;
    viskores::FloatDefault t2 = tNorm * tNorm, t3 = t2 * tNorm;
    viskores::FloatDefault h00 = 2 * t3 - 3 * t2 + 1;
    viskores::FloatDefault h10 = t3 - 2 * t2 + tNorm;
    viskores::FloatDefault h01 = -2 * t3 + 3 * t2;
    viskores::FloatDefault h11 = t3 - t2;

    //pre-calculate terms used in each iteration.
    viskores::FloatDefault h10_x_dt = h10 * dt;
    viskores::FloatDefault h11_x_dt = h11 * dt;

    for (viskores::Id i = 0; i < 3; ++i)
      result[i] = h00 * p0[i] + h10_x_dt * m0[i] + h01 * p1[i] + h11_x_dt * m1[i];

    return result;
  }

  VISKORES_EXEC viskores::Id FindParametricInterval(const viskores::FloatDefault& t) const
  {
    viskores::Id n = this->Knots.GetNumberOfValues();

    if (t < this->Knots.Get(0) || t > this->Knots.Get(n - 1))
      return -1;

    //Binary search for the interval
    viskores::Id left = 0;
    viskores::Id right = n - 1;

    while (left < right)
    {
      viskores::Id mid = left + (right - left) / 2;

      if (t >= this->Knots.Get(mid) && t <= this->Knots.Get(mid + 1))
        return mid;
      else if (t < this->Knots.Get(mid))
        right = mid;
      else
        left = mid;
    }

    // t not within the interval. We should not get here.
    return -1;
  }

  DataPortalType Data;
  KnotsPortalType Knots;
  TangentPortalType Tangents;
};
} //namespace exec
} //namespace viskores

#endif //vtk_m_exec_CubicHermiteSpline_h
