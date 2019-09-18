//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/Range.h>
#include <vtkm/Types.h>

#include <vtkm/cont/VirtualObjectHandle.h>
#include <vtkm/exec/ColorTable.h>

#include <limits>
#include <vector>

namespace vtkm
{
namespace cont
{

namespace detail
{
struct ColorTableInternals
{
  std::string Name;

  ColorSpace CSpace = ColorSpace::LAB;
  vtkm::Range TableRange = { 1.0, 0.0 };

  //Host side version of the ColorTableBase. This holds data such as:
  //  NanColor
  //  BelowRangeColor
  //  AboveRangeColor
  //  UseClamping
  //  BelowRangeColor
  //  AboveRangeColor
  //Note the pointers inside the host side portal are not valid, as they
  //are execution pointers
  std::unique_ptr<vtkm::exec::ColorTableBase> HostSideCache;
  //Execution side version of the ColorTableBase.
  std::unique_ptr<vtkm::cont::VirtualObjectHandle<vtkm::exec::ColorTableBase>> ExecHandle;

  std::vector<double> ColorNodePos;
  std::vector<vtkm::Vec<float, 3>> ColorRGB;

  std::vector<double> OpacityNodePos;
  std::vector<float> OpacityAlpha;
  std::vector<vtkm::Vec<float, 2>> OpacityMidSharp;

  vtkm::cont::ArrayHandle<double> ColorPosHandle;
  vtkm::cont::ArrayHandle<vtkm::Vec<float, 3>> ColorRGBHandle;
  vtkm::cont::ArrayHandle<double> OpacityPosHandle;
  vtkm::cont::ArrayHandle<float> OpacityAlphaHandle;
  vtkm::cont::ArrayHandle<vtkm::Vec<float, 2>> OpacityMidSharpHandle;
  bool ColorArraysChanged = true;
  bool OpacityArraysChanged = true;
  bool HostSideCacheChanged = true;

  void RecalculateRange()
  {
    vtkm::Range r;
    if (this->ColorNodePos.size() > 0)
    {
      r.Include(this->ColorNodePos.front());
      r.Include(this->ColorNodePos.back());
    }

    if (this->OpacityNodePos.size() > 0)
    {
      r.Include(this->OpacityNodePos.front());
      r.Include(this->OpacityNodePos.back());
    }

    this->TableRange = r;
  }
};
} //namespace detail

namespace
{

template <typename T>
struct MinDelta
{
};
// This value seems to work well for float ranges we have tested
template <>
struct MinDelta<float>
{
  static constexpr int value = 2048;
};
template <>
struct MinDelta<double>
{
  static constexpr vtkm::Int64 value = 2048L;
};

// Reperesents the following:
// T m = std::numeric_limits<T>::min();
// EquivSizeIntT im;
// std::memcpy(&im, &m, sizeof(T));
//
template <typename EquivSizeIntT>
struct MinRepresentable
{
};
template <>
struct MinRepresentable<float>
{
  static constexpr int value = 8388608;
};
template <>
struct MinRepresentable<double>
{
  static constexpr vtkm::Int64 value = 4503599627370496L;
};

inline bool rangeAlmostEqual(const vtkm::Range& r)
{
  vtkm::Int64 irange[2];
  // needs to be a memcpy to avoid strict aliasing issues, doing a count
  // of 2*sizeof(T) to couple both values at the same time
  std::memcpy(irange, &r.Min, sizeof(vtkm::Int64));
  std::memcpy(irange + 1, &r.Max, sizeof(vtkm::Int64));
  // determine the absolute delta between these two numbers.
  const vtkm::Int64 delta = std::abs(irange[1] - irange[0]);
  // If the numbers are not nearly equal, we don't touch them. This avoids running into
  // pitfalls like BUG PV #17152.
  return (delta < 1024) ? true : false;
}

template <typename T>
inline double expandRange(T r[2])
{
  constexpr bool is_float32_type = std::is_same<T, vtkm::Float32>::value;
  using IRange = typename std::conditional<is_float32_type, vtkm::Int32, vtkm::Int64>::type;
  IRange irange[2];
  // needs to be a memcpy to avoid strict aliasing issues, doing a count
  // of 2*sizeof(T) to couple both values at the same time
  std::memcpy(irange, r, sizeof(T) * 2);

  const bool denormal = !std::isnormal(r[0]);
  const IRange minInt = MinRepresentable<T>::value;
  const IRange minDelta = denormal ? minInt + MinDelta<T>::value : MinDelta<T>::value;

  // determine the absolute delta between these two numbers.
  const vtkm::Int64 delta = std::abs(irange[1] - irange[0]);

  // if our delta is smaller than the min delta push out the max value
  // so that it is equal to minRange + minDelta. When our range is entirely
  // negative we should instead subtract from our max, to max a larger negative
  // value
  if (delta < minDelta)
  {
    if (irange[0] < 0)
    {
      irange[1] = irange[0] - minDelta;
    }
    else
    {
      irange[1] = irange[0] + minDelta;
    }

    T result;
    std::memcpy(&result, irange + 1, sizeof(T));
    return static_cast<double>(result);
  }
  return static_cast<double>(r[1]);
}

inline vtkm::Range adjustRange(const vtkm::Range& r)
{
  const bool spans_zero_boundary = r.Min < 0 && r.Max > 0;
  if (spans_zero_boundary)
  { // nothing needs to be done, but this check is required.
    // if we convert into integer space the delta difference will overflow
    // an integer
    return r;
  }
  if (rangeAlmostEqual(r))
  {
    return r;
  }

  // range should be left untouched as much as possible to
  // to avoid loss of precision whenever possible. That is why
  // we only modify the Max value
  vtkm::Range result = r;
  if (r.Min > static_cast<double>(std::numeric_limits<float>::lowest()) &&
      r.Max < static_cast<double>(std::numeric_limits<float>::max()))
  { //We've found it best to offset it in float space if the numbers
    //lay inside that representable range
    float frange[2] = { static_cast<float>(r.Min), static_cast<float>(r.Max) };
    result.Max = expandRange(frange);
  }
  else
  {
    double drange[2] = { r.Min, r.Max };
    result.Max = expandRange(drange);
  }
  return result;
}


inline vtkm::Vec<float, 3> hsvTorgb(const vtkm::Vec<float, 3>& hsv)
{
  vtkm::Vec<float, 3> rgb;
  constexpr float onethird = 1.0f / 3.0f;
  constexpr float onesixth = 1.0f / 6.0f;
  constexpr float twothird = 2.0f / 3.0f;
  constexpr float fivesixth = 5.0f / 6.0f;

  // compute RGB from HSV
  if (hsv[0] > onesixth && hsv[0] <= onethird) // green/red
  {
    rgb[1] = 1.0f;
    rgb[0] = (onethird - hsv[0]) * 6.0f;
    rgb[2] = 0.0f;
  }
  else if (hsv[0] > onethird && hsv[0] <= 0.5f) // green/blue
  {
    rgb[1] = 1.0f;
    rgb[2] = (hsv[0] - onethird) * 6.0f;
    rgb[0] = 0.0f;
  }
  else if (hsv[0] > 0.5 && hsv[0] <= twothird) // blue/green
  {
    rgb[2] = 1.0f;
    rgb[1] = (twothird - hsv[0]) * 6.0f;
    rgb[0] = 0.0f;
  }
  else if (hsv[0] > twothird && hsv[0] <= fivesixth) // blue/red
  {
    rgb[2] = 1.0f;
    rgb[0] = (hsv[0] - twothird) * 6.0f;
    rgb[1] = 0.0f;
  }
  else if (hsv[0] > fivesixth && hsv[0] <= 1.0) // red/blue
  {
    rgb[0] = 1.0f;
    rgb[2] = (1.0f - hsv[0]) * 6.0f;
    rgb[1] = 0.0f;
  }
  else // red/green
  {
    rgb[0] = 1.0f;
    rgb[1] = hsv[0] * 6;
    rgb[2] = 0.0f;
  }

  // add Saturation to the equation.
  rgb[0] = (hsv[1] * rgb[0] + (1.0f - hsv[1]));
  rgb[1] = (hsv[1] * rgb[1] + (1.0f - hsv[1]));
  rgb[2] = (hsv[1] * rgb[2] + (1.0f - hsv[1]));

  rgb[0] *= hsv[2];
  rgb[1] *= hsv[2];
  rgb[2] *= hsv[2];
  return rgb;
}

// clang-format off
inline bool outside_vrange(double x) { return x < 0.0 || x > 1.0; }
inline bool outside_vrange(const vtkm::Vec<double, 2>& x)
  { return x[0] < 0.0 || x[0] > 1.0 || x[1] < 0.0 || x[1] > 1.0; }
inline bool outside_vrange(const vtkm::Vec<double, 3>& x)
  { return x[0] < 0.0 || x[0] > 1.0 || x[1] < 0.0 || x[1] > 1.0 || x[2] < 0.0 || x[2] > 1.0; }
inline bool outside_vrange(float x) { return x < 0.0f || x > 1.0f; }
inline bool outside_vrange(const vtkm::Vec<float, 2>& x)
  { return x[0] < 0.0f || x[0] > 1.0f || x[1] < 0.0f || x[1] > 1.0f; }
inline bool outside_vrange(const vtkm::Vec<float, 3>& x)
  { return x[0] < 0.0f || x[0] > 1.0f || x[1] < 0.0f || x[1] > 1.0f || x[2] < 0.0f || x[2] > 1.0f; }

inline bool outside_range() { return false; }

template <typename T>
inline bool outside_range(T&& t) { return outside_vrange(t); }

template <typename T, typename U>
inline bool outside_range(T&& t, U&& u) { return outside_vrange(t) || outside_vrange(u); }

template <typename T, typename U, typename V, typename... Args>
inline bool outside_range(T&& t, U&& u, V&& v, Args&&... args)
{
  return outside_vrange(t) || outside_vrange(u) || outside_vrange(v) ||
         outside_range(std::forward<Args>(args)...);
}
// clang-format on
}

} //namespace cont
} //namespace vtkm
