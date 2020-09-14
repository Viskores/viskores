//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <algorithm>
#include <cctype>
#include <memory>

#include <vtkm/cont/ColorTable.h>
#include <vtkm/cont/ColorTable.hxx>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/TryExecute.h>


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

inline bool outside_vrange(double x)
{
  return x < 0.0 || x > 1.0;
}
inline bool outside_vrange(float x)
{
  return x < 0.0f || x > 1.0f;
}
template <typename T>
inline bool outside_vrange(const vtkm::Vec<T, 2>& x)
{
  return outside_vrange(x[0]) || outside_vrange(x[1]);
}
template <typename T>
inline bool outside_vrange(const vtkm::Vec<T, 3>& x)
{
  return outside_vrange(x[0]) || outside_vrange(x[1]) || outside_vrange(x[2]);
}

inline bool outside_range()
{
  return false;
}

template <typename T>
inline bool outside_range(T&& t)
{
  return outside_vrange(t);
}

template <typename T, typename U>
inline bool outside_range(T&& t, U&& u)
{
  return outside_vrange(t) || outside_vrange(u);
}

template <typename T, typename U, typename V, typename... Args>
inline bool outside_range(T&& t, U&& u, V&& v, Args&&... args)
{
  return outside_vrange(t) || outside_vrange(u) || outside_vrange(v) ||
    outside_range(std::forward<Args>(args)...);
}
}

namespace vtkm
{
namespace cont
{

namespace detail
{

struct ColorTableInternals
{
  std::string Name;

  vtkm::ColorSpace Space = vtkm::ColorSpace::Lab;
  vtkm::Range TableRange = { 1.0, 0.0 };

  vtkm::Vec3f_32 NaNColor = { 0.5f, 0.0f, 0.0f };
  vtkm::Vec3f_32 BelowRangeColor = { 0.0f, 0.0f, 0.0f };
  vtkm::Vec3f_32 AboveRangeColor = { 0.0f, 0.0f, 0.0f };

  bool UseClamping = true;

  std::vector<vtkm::Float64> ColorNodePos;
  std::vector<vtkm::Vec3f_32> ColorRGB;

  std::vector<vtkm::Float64> OpacityNodePos;
  std::vector<vtkm::Float32> OpacityAlpha;
  std::vector<vtkm::Vec2f_32> OpacityMidSharp;

  vtkm::cont::ArrayHandle<vtkm::Float64> ColorPosHandle;
  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> ColorRGBHandle;
  vtkm::cont::ArrayHandle<vtkm::Float64> OpacityPosHandle;
  vtkm::cont::ArrayHandle<vtkm::Float32> OpacityAlphaHandle;
  vtkm::cont::ArrayHandle<vtkm::Vec2f_32> OpacityMidSharpHandle;
  bool ColorArraysChanged = true;
  bool OpacityArraysChanged = true;

  vtkm::Id ModifiedCount = 1;
  void Modified() { ++this->ModifiedCount; }

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

} // namespace detail

namespace internal
{
std::set<std::string> GetPresetNames();
bool LoadColorTablePreset(vtkm::cont::ColorTable::Preset preset, vtkm::cont::ColorTable& table);
bool LoadColorTablePreset(std::string name, vtkm::cont::ColorTable& table);
} // namespace internal

//----------------------------------------------------------------------------
ColorTable::ColorTable(vtkm::cont::ColorTable::Preset preset)
  : Internals(std::make_shared<detail::ColorTableInternals>())
{
  const bool loaded = this->LoadPreset(preset);
  if (!loaded)
  { //if we failed to load the requested color table, call SetColorSpace
    //so that the internal host side cache is constructed and we leave
    //the constructor in a valid state. We use LAB as it is the default
    //when the no parameter constructor is called
    this->SetColorSpace(vtkm::ColorSpace::Lab);
  }
  this->AddSegmentAlpha(
    this->Internals->TableRange.Min, 1.0f, this->Internals->TableRange.Max, 1.0f);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(const std::string& name)
  : Internals(std::make_shared<detail::ColorTableInternals>())
{
  const bool loaded = this->LoadPreset(name);
  if (!loaded)
  { //if we failed to load the requested color table, call SetColorSpace
    //so that the internal host side cache is constructed and we leave
    //the constructor in a valid state. We use LAB as it is the default
    //when the no parameter constructor is called
    this->SetColorSpace(vtkm::ColorSpace::Lab);
  }
  this->AddSegmentAlpha(
    this->Internals->TableRange.Min, 1.0f, this->Internals->TableRange.Max, 1.0f);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(vtkm::ColorSpace space)
  : Internals(std::make_shared<detail::ColorTableInternals>())
{
  this->SetColorSpace(space);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(const vtkm::Range& range,
                       const vtkm::Vec3f_32& rgb1,
                       const vtkm::Vec3f_32& rgb2,
                       vtkm::ColorSpace space)
  : Internals(std::make_shared<detail::ColorTableInternals>())
{
  this->AddSegment(range.Min, rgb1, range.Max, rgb2);
  this->AddSegmentAlpha(range.Min, 1.0f, range.Max, 1.0f);
  this->SetColorSpace(space);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(const vtkm::Range& range,
                       const vtkm::Vec4f_32& rgba1,
                       const vtkm::Vec4f_32& rgba2,
                       vtkm::ColorSpace space)
  : Internals(std::make_shared<detail::ColorTableInternals>())
{
  vtkm::Vec3f_32 rgb1(rgba1[0], rgba1[1], rgba1[2]);
  vtkm::Vec3f_32 rgb2(rgba2[0], rgba2[1], rgba2[2]);
  this->AddSegment(range.Min, rgb1, range.Max, rgb2);
  this->AddSegmentAlpha(range.Min, rgba1[3], range.Max, rgba2[3]);
  this->SetColorSpace(space);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(const std::string& name,
                       vtkm::ColorSpace colorSpace,
                       const vtkm::Vec3f_64& nanColor,
                       const std::vector<vtkm::Float64>& rgbPoints,
                       const std::vector<vtkm::Float64>& alphaPoints)
  : Internals(std::make_shared<detail::ColorTableInternals>())
{
  this->SetName(name);
  this->SetColorSpace(colorSpace);
  this->SetNaNColor(nanColor);
  this->FillColorTableFromDataPointer(static_cast<vtkm::Int32>(rgbPoints.size()), rgbPoints.data());
  this->FillOpacityTableFromDataPointer(static_cast<vtkm::Int32>(alphaPoints.size()),
                                        alphaPoints.data());
}

//----------------------------------------------------------------------------
ColorTable::~ColorTable() {}

//----------------------------------------------------------------------------
const std::string& ColorTable::GetName() const
{
  return this->Internals->Name;
}

//----------------------------------------------------------------------------
void ColorTable::SetName(const std::string& name)
{
  this->Internals->Name = name;
}

//----------------------------------------------------------------------------
bool ColorTable::LoadPreset(vtkm::cont::ColorTable::Preset preset)
{
  return internal::LoadColorTablePreset(preset, *this);
}

//----------------------------------------------------------------------------
std::set<std::string> ColorTable::GetPresets()
{
  return internal::GetPresetNames();
}

//----------------------------------------------------------------------------
bool ColorTable::LoadPreset(const std::string& name)
{
  return internal::LoadColorTablePreset(name, *this);
}

//----------------------------------------------------------------------------
ColorTable ColorTable::MakeDeepCopy()
{
  ColorTable dcopy(this->Internals->Space);

  dcopy.Internals->TableRange = this->Internals->TableRange;

  dcopy.Internals->NaNColor = this->Internals->NaNColor;
  dcopy.Internals->BelowRangeColor = this->Internals->BelowRangeColor;
  dcopy.Internals->AboveRangeColor = this->Internals->AboveRangeColor;

  dcopy.Internals->UseClamping = this->Internals->UseClamping;

  dcopy.Internals->ColorNodePos = this->Internals->ColorNodePos;
  dcopy.Internals->ColorRGB = this->Internals->ColorRGB;

  dcopy.Internals->OpacityNodePos = this->Internals->OpacityNodePos;
  dcopy.Internals->OpacityAlpha = this->Internals->OpacityAlpha;
  dcopy.Internals->OpacityMidSharp = this->Internals->OpacityMidSharp;
  return dcopy;
}

//----------------------------------------------------------------------------
vtkm::ColorSpace ColorTable::GetColorSpace() const
{
  return this->Internals->Space;
}

//----------------------------------------------------------------------------
void ColorTable::SetColorSpace(vtkm::ColorSpace space)
{
  this->Internals->Space = space;
  this->Internals->Modified();
}

//----------------------------------------------------------------------------
void ColorTable::SetClamping(bool state)
{
  this->Internals->UseClamping = state;
  this->Internals->Modified();
}

//----------------------------------------------------------------------------
bool ColorTable::GetClamping() const
{
  return this->Internals->UseClamping;
}

//----------------------------------------------------------------------------
void ColorTable::SetBelowRangeColor(const vtkm::Vec3f_32& c)
{
  this->Internals->BelowRangeColor = c;
  this->Internals->Modified();
}

//----------------------------------------------------------------------------
const vtkm::Vec3f_32& ColorTable::GetBelowRangeColor() const
{
  return this->Internals->BelowRangeColor;
}

//----------------------------------------------------------------------------
void ColorTable::SetAboveRangeColor(const vtkm::Vec3f_32& c)
{
  this->Internals->AboveRangeColor = c;
  this->Internals->Modified();
}

//----------------------------------------------------------------------------
const vtkm::Vec3f_32& ColorTable::GetAboveRangeColor() const
{
  return this->Internals->AboveRangeColor;
}

//----------------------------------------------------------------------------
void ColorTable::SetNaNColor(const vtkm::Vec3f_32& c)
{
  this->Internals->NaNColor = c;
  this->Internals->Modified();
}

//----------------------------------------------------------------------------
const vtkm::Vec3f_32& ColorTable::GetNaNColor() const
{
  return this->Internals->NaNColor;
}

//----------------------------------------------------------------------------
void ColorTable::Clear()
{
  this->ClearColors();
  this->ClearAlpha();
}

//---------------------------------------------------------------------------
void ColorTable::ClearColors()
{
  this->Internals->ColorNodePos.clear();
  this->Internals->ColorRGB.clear();
  this->Internals->ColorArraysChanged = true;
  this->Internals->Modified();
}

//---------------------------------------------------------------------------
void ColorTable::ClearAlpha()
{
  this->Internals->OpacityNodePos.clear();
  this->Internals->OpacityAlpha.clear();
  this->Internals->OpacityMidSharp.clear();
  this->Internals->OpacityArraysChanged = true;
  this->Internals->Modified();
}

//---------------------------------------------------------------------------
void ColorTable::ReverseColors()
{
  std::reverse(this->Internals->ColorRGB.begin(), this->Internals->ColorRGB.end());
  this->Internals->ColorArraysChanged = true;
  this->Internals->Modified();
}

//---------------------------------------------------------------------------
void ColorTable::ReverseAlpha()
{
  std::reverse(this->Internals->OpacityAlpha.begin(), this->Internals->OpacityAlpha.end());
  //To keep the shape correct the mid and sharp values of the last node are not included in the reversal
  std::reverse(this->Internals->OpacityMidSharp.begin(),
               this->Internals->OpacityMidSharp.end() - 1);
  this->Internals->OpacityArraysChanged = true;
  this->Internals->Modified();
}

//---------------------------------------------------------------------------
const vtkm::Range& ColorTable::GetRange() const
{
  return this->Internals->TableRange;
}

//---------------------------------------------------------------------------
void ColorTable::RescaleToRange(const vtkm::Range& r)
{
  if (r == this->GetRange())
  {
    return;
  }
  //make sure range has space.
  auto newRange = adjustRange(r);

  //slam control points down to 0.0 - 1.0, and than rescale to new range
  const vtkm::Float64 minv = this->GetRange().Min;
  const vtkm::Float64 oldScale = this->GetRange().Length();
  const vtkm::Float64 newScale = newRange.Length();
  VTKM_ASSERT(oldScale > 0);
  VTKM_ASSERT(newScale > 0);
  for (auto i = this->Internals->ColorNodePos.begin(); i != this->Internals->ColorNodePos.end();
       ++i)
  {
    const auto t = (*i - minv) / oldScale;
    *i = (t * newScale) + newRange.Min;
  }
  for (auto i = this->Internals->OpacityNodePos.begin(); i != this->Internals->OpacityNodePos.end();
       ++i)
  {
    const auto t = (*i - minv) / oldScale;
    *i = (t * newScale) + newRange.Min;
  }

  this->Internals->ColorArraysChanged = true;
  this->Internals->OpacityArraysChanged = true;
  this->Internals->TableRange = newRange;
  this->Internals->Modified();
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::AddPoint(vtkm::Float64 x, const vtkm::Vec3f_32& rgb)
{
  if (outside_range(rgb))
  {
    return -1;
  }

  std::size_t index = 0;
  if (this->Internals->ColorNodePos.size() == 0 || this->Internals->ColorNodePos.back() < x)
  {
    this->Internals->ColorNodePos.emplace_back(x);
    this->Internals->ColorRGB.emplace_back(rgb);
    index = this->Internals->ColorNodePos.size();
  }
  else
  {
    auto begin = this->Internals->ColorNodePos.begin();
    auto pos = std::lower_bound(begin, this->Internals->ColorNodePos.end(), x);
    index = static_cast<std::size_t>(std::distance(begin, pos));

    if (*pos == x)
    {
      this->Internals->ColorRGB[index] = rgb;
    }
    else
    {
      this->Internals->ColorRGB.emplace(
        this->Internals->ColorRGB.begin() + std::distance(begin, pos), rgb);
      this->Internals->ColorNodePos.emplace(pos, x);
    }
  }
  this->Internals->TableRange.Include(x); //update range to include x
  this->Internals->ColorArraysChanged = true;
  this->Internals->Modified();
  return static_cast<vtkm::Int32>(index);
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::AddPointHSV(vtkm::Float64 x, const vtkm::Vec3f_32& hsv)
{
  return this->AddPoint(x, hsvTorgb(hsv));
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::AddSegment(vtkm::Float64 x1,
                                   const vtkm::Vec3f_32& rgb1,
                                   vtkm::Float64 x2,
                                   const vtkm::Vec3f_32& rgb2)
{
  if (outside_range(rgb1, rgb2))
  {
    return -1;
  }
  if (this->Internals->ColorNodePos.size() > 0)
  {
    //Todo:
    // - This could be optimized so we do 2 less lower_bound calls when
    // the table already exists

    //When we add a segment we remove all points that are inside the line

    auto nodeBegin = this->Internals->ColorNodePos.begin();
    auto nodeEnd = this->Internals->ColorNodePos.end();

    auto rgbBegin = this->Internals->ColorRGB.begin();

    auto nodeStart = std::lower_bound(nodeBegin, nodeEnd, x1);
    auto nodeStop = std::lower_bound(nodeBegin, nodeEnd, x2);

    auto rgbStart = rgbBegin + std::distance(nodeBegin, nodeStart);
    auto rgbStop = rgbBegin + std::distance(nodeBegin, nodeStop);

    //erase is exclusive so if end->x == x2 it will be kept around, and
    //than we will update it in AddPoint
    this->Internals->ColorNodePos.erase(nodeStart, nodeStop);
    this->Internals->ColorRGB.erase(rgbStart, rgbStop);
  }
  vtkm::Int32 pos = this->AddPoint(x1, rgb1);
  this->AddPoint(x2, rgb2);
  return pos;
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::AddSegmentHSV(vtkm::Float64 x1,
                                      const vtkm::Vec3f_32& hsv1,
                                      vtkm::Float64 x2,
                                      const vtkm::Vec3f_32& hsv2)
{
  return this->AddSegment(x1, hsvTorgb(hsv1), x2, hsvTorgb(hsv2));
}

//---------------------------------------------------------------------------
bool ColorTable::GetPoint(vtkm::Int32 index, vtkm::Vec4f_64& data) const
{
  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Internals->ColorNodePos.size();
  if (index < 0 || i >= size)
  {
    return false;
  }

  const auto& pos = this->Internals->ColorNodePos[i];
  const auto& rgb = this->Internals->ColorRGB[i];

  data[0] = pos;
  data[1] = rgb[0];
  data[2] = rgb[1];
  data[3] = rgb[2];
  return true;
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::UpdatePoint(vtkm::Int32 index, const vtkm::Vec4f_64& data)
{
  //skip data[0] as we don't care about position
  if (outside_range(data[1], data[2], data[3]))
  {
    return -1;
  }

  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Internals->ColorNodePos.size();
  if (index < 0 || i >= size)
  {
    return -1;
  }

  //When updating the first question is has the relative position of the point changed?
  //If it hasn't we can quickly just update the RGB value
  auto oldPos = this->Internals->ColorNodePos.begin() + index;
  auto newPos = std::lower_bound(
    this->Internals->ColorNodePos.begin(), this->Internals->ColorNodePos.end(), data[0]);
  if (oldPos == newPos)
  { //node's relative location hasn't changed
    this->Internals->ColorArraysChanged = true;
    auto& rgb = this->Internals->ColorRGB[i];
    *newPos = data[0];
    rgb[0] = static_cast<vtkm::Float32>(data[1]);
    rgb[1] = static_cast<vtkm::Float32>(data[2]);
    rgb[2] = static_cast<vtkm::Float32>(data[3]);
    this->Internals->Modified();
    return index;
  }
  else
  { //remove the point, and add the new values as the relative location is different
    this->RemovePoint(index);
    vtkm::Vec3f_32 newrgb(static_cast<vtkm::Float32>(data[1]),
                          static_cast<vtkm::Float32>(data[2]),
                          static_cast<vtkm::Float32>(data[3]));
    return this->AddPoint(data[0], newrgb);
  }
}

//---------------------------------------------------------------------------
bool ColorTable::RemovePoint(vtkm::Float64 x)
{
  auto begin = this->Internals->ColorNodePos.begin();
  auto pos = std::lower_bound(begin, this->Internals->ColorNodePos.end(), x);
  return this->RemovePoint(static_cast<vtkm::Int32>(std::distance(begin, pos)));
}

//---------------------------------------------------------------------------
bool ColorTable::RemovePoint(vtkm::Int32 index)
{
  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Internals->ColorNodePos.size();
  if (index < 0 || i >= size)
  {
    return false;
  }

  this->Internals->ColorNodePos.erase(this->Internals->ColorNodePos.begin() + index);
  this->Internals->ColorRGB.erase(this->Internals->ColorRGB.begin() + index);
  this->Internals->ColorArraysChanged = true;
  this->Internals->RecalculateRange();
  this->Internals->Modified();
  return true;
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::GetNumberOfPoints() const
{
  return static_cast<vtkm::Int32>(this->Internals->ColorNodePos.size());
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::AddPointAlpha(vtkm::Float64 x,
                                      vtkm::Float32 alpha,
                                      vtkm::Float32 midpoint,
                                      vtkm::Float32 sharpness)
{
  if (outside_range(alpha, midpoint, sharpness))
  {
    return -1;
  }

  const vtkm::Vec2f_32 midsharp(midpoint, sharpness);
  std::size_t index = 0;
  if (this->Internals->OpacityNodePos.size() == 0 || this->Internals->OpacityNodePos.back() < x)
  {
    this->Internals->OpacityNodePos.emplace_back(x);
    this->Internals->OpacityAlpha.emplace_back(alpha);
    this->Internals->OpacityMidSharp.emplace_back(midsharp);
    index = this->Internals->OpacityNodePos.size();
  }
  else
  {
    auto begin = this->Internals->OpacityNodePos.begin();
    auto pos = std::lower_bound(begin, this->Internals->OpacityNodePos.end(), x);
    index = static_cast<std::size_t>(std::distance(begin, pos));
    if (*pos == x)
    {
      this->Internals->OpacityAlpha[index] = alpha;
      this->Internals->OpacityMidSharp[index] = midsharp;
    }
    else
    {
      this->Internals->OpacityAlpha.emplace(
        this->Internals->OpacityAlpha.begin() + std::distance(begin, pos), alpha);
      this->Internals->OpacityMidSharp.emplace(
        this->Internals->OpacityMidSharp.begin() + std::distance(begin, pos), midsharp);
      this->Internals->OpacityNodePos.emplace(pos, x);
    }
  }
  this->Internals->OpacityArraysChanged = true;
  this->Internals->TableRange.Include(x); //update range to include x
  this->Internals->Modified();
  return static_cast<vtkm::Int32>(index);
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::AddSegmentAlpha(vtkm::Float64 x1,
                                        vtkm::Float32 alpha1,
                                        vtkm::Float64 x2,
                                        vtkm::Float32 alpha2,
                                        const vtkm::Vec2f_32& mid_sharp1,
                                        const vtkm::Vec2f_32& mid_sharp2)
{
  if (outside_range(alpha1, alpha2, mid_sharp1, mid_sharp2))
  {
    return -1;
  }

  if (this->Internals->OpacityNodePos.size() > 0)
  {
    //Todo:
    // - This could be optimized so we do 2 less lower_bound calls when
    // the table already exists

    //When we add a segment we remove all points that are inside the line

    auto nodeBegin = this->Internals->OpacityNodePos.begin();
    auto nodeEnd = this->Internals->OpacityNodePos.end();

    auto alphaBegin = this->Internals->OpacityAlpha.begin();
    auto midBegin = this->Internals->OpacityMidSharp.begin();

    auto nodeStart = std::lower_bound(nodeBegin, nodeEnd, x1);
    auto nodeStop = std::lower_bound(nodeBegin, nodeEnd, x2);

    auto alphaStart = alphaBegin + std::distance(nodeBegin, nodeStart);
    auto alphaStop = alphaBegin + std::distance(nodeBegin, nodeStop);
    auto midStart = midBegin + std::distance(nodeBegin, nodeStart);
    auto midStop = midBegin + std::distance(nodeBegin, nodeStop);

    //erase is exclusive so if end->x == x2 it will be kept around, and
    //than we will update it in AddPoint
    this->Internals->OpacityNodePos.erase(nodeStart, nodeStop);
    this->Internals->OpacityAlpha.erase(alphaStart, alphaStop);
    this->Internals->OpacityMidSharp.erase(midStart, midStop);
  }

  vtkm::Int32 pos = this->AddPointAlpha(x1, alpha1, mid_sharp1[0], mid_sharp1[1]);
  this->AddPointAlpha(x2, alpha2, mid_sharp2[0], mid_sharp2[1]);
  return pos;
}

//---------------------------------------------------------------------------
bool ColorTable::GetPointAlpha(vtkm::Int32 index, vtkm::Vec4f_64& data) const
{
  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Internals->OpacityNodePos.size();
  if (index < 0 || i >= size)
  {
    return false;
  }

  const auto& pos = this->Internals->OpacityNodePos[i];
  const auto& alpha = this->Internals->OpacityAlpha[i];
  const auto& midsharp = this->Internals->OpacityMidSharp[i];

  data[0] = pos;
  data[1] = alpha;
  data[2] = midsharp[0];
  data[3] = midsharp[1];
  return true;
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::UpdatePointAlpha(vtkm::Int32 index, const vtkm::Vec4f_64& data)
{
  //skip data[0] as we don't care about position
  if (outside_range(data[1], data[2], data[3]))
  {
    return -1;
  }

  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Internals->OpacityNodePos.size();
  if (index < 0 || i >= size)
  {
    return -1;
  }
  //When updating the first question is has the relative position of the point changed?
  //If it hasn't we can quickly just update the RGB value
  auto oldPos = this->Internals->OpacityNodePos.begin() + index;
  auto newPos = std::lower_bound(
    this->Internals->OpacityNodePos.begin(), this->Internals->OpacityNodePos.end(), data[0]);
  if (oldPos == newPos)
  { //node's relative location hasn't changed
    this->Internals->OpacityArraysChanged = true;
    auto& alpha = this->Internals->OpacityAlpha[i];
    auto& midsharp = this->Internals->OpacityMidSharp[i];
    *newPos = data[0];
    alpha = static_cast<vtkm::Float32>(data[1]);
    midsharp[0] = static_cast<vtkm::Float32>(data[2]);
    midsharp[1] = static_cast<vtkm::Float32>(data[3]);
    this->Internals->Modified();
    return index;
  }
  else
  { //remove the point, and add the new values as the relative location is different
    this->RemovePointAlpha(index);
    return this->AddPointAlpha(data[0],
                               static_cast<vtkm::Float32>(data[1]),
                               static_cast<vtkm::Float32>(data[2]),
                               static_cast<vtkm::Float32>(data[3]));
  }
}

//---------------------------------------------------------------------------
bool ColorTable::RemovePointAlpha(vtkm::Float64 x)
{
  auto begin = this->Internals->OpacityNodePos.begin();
  auto pos = std::lower_bound(begin, this->Internals->OpacityNodePos.end(), x);
  return this->RemovePointAlpha(static_cast<vtkm::Int32>(std::distance(begin, pos)));
}

//---------------------------------------------------------------------------
bool ColorTable::RemovePointAlpha(vtkm::Int32 index)
{
  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Internals->OpacityNodePos.size();
  if (index < 0 || i >= size)
  {
    return false;
  }

  this->Internals->OpacityNodePos.erase(this->Internals->OpacityNodePos.begin() + index);
  this->Internals->OpacityAlpha.erase(this->Internals->OpacityAlpha.begin() + index);
  this->Internals->OpacityMidSharp.erase(this->Internals->OpacityMidSharp.begin() + index);
  this->Internals->OpacityArraysChanged = true;
  this->Internals->RecalculateRange();
  this->Internals->Modified();
  return true;
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::GetNumberOfPointsAlpha() const
{
  return static_cast<vtkm::Int32>(this->Internals->OpacityNodePos.size());
}

//---------------------------------------------------------------------------
bool ColorTable::FillColorTableFromDataPointer(vtkm::Int32 n, const vtkm::Float64* ptr)
{
  if (n <= 0 || ptr == nullptr)
  {
    return false;
  }
  this->ClearColors();

  std::size_t size = static_cast<std::size_t>(n / 4);
  this->Internals->ColorNodePos.reserve(size);
  this->Internals->ColorRGB.reserve(size);
  for (std::size_t i = 0; i < size; ++i)
  { //allows us to support unsorted arrays
    vtkm::Vec3f_32 rgb(static_cast<vtkm::Float32>(ptr[1]),
                       static_cast<vtkm::Float32>(ptr[2]),
                       static_cast<vtkm::Float32>(ptr[3]));
    this->AddPoint(ptr[0], rgb);
    ptr += 4;
  }
  this->Internals->ColorArraysChanged = true;
  this->Internals->Modified();

  return true;
}

//---------------------------------------------------------------------------
bool ColorTable::FillColorTableFromDataPointer(vtkm::Int32 n, const vtkm::Float32* ptr)
{
  if (n <= 0 || ptr == nullptr)
  {
    return false;
  }
  this->ClearColors();

  std::size_t size = static_cast<std::size_t>(n / 4);
  this->Internals->ColorNodePos.reserve(size);
  this->Internals->ColorRGB.reserve(size);
  for (std::size_t i = 0; i < size; ++i)
  { //allows us to support unsorted arrays
    vtkm::Vec3f_32 rgb(ptr[1], ptr[2], ptr[3]);
    this->AddPoint(ptr[0], rgb);
    ptr += 4;
  }
  this->Internals->ColorArraysChanged = true;
  this->Internals->Modified();
  return true;
}

//---------------------------------------------------------------------------
bool ColorTable::FillOpacityTableFromDataPointer(vtkm::Int32 n, const vtkm::Float64* ptr)
{
  if (n <= 0 || ptr == nullptr)
  {
    return false;
  }
  this->ClearAlpha();

  std::size_t size = static_cast<std::size_t>(n / 4);
  this->Internals->OpacityNodePos.reserve(size);
  this->Internals->OpacityAlpha.reserve(size);
  this->Internals->OpacityMidSharp.reserve(size);
  for (std::size_t i = 0; i < size; ++i)
  { //allows us to support unsorted arrays
    this->AddPointAlpha(ptr[0],
                        static_cast<vtkm::Float32>(ptr[1]),
                        static_cast<vtkm::Float32>(ptr[2]),
                        static_cast<vtkm::Float32>(ptr[3]));
    ptr += 4;
  }

  this->Internals->OpacityArraysChanged = true;
  this->Internals->Modified();
  return true;
}

//---------------------------------------------------------------------------
bool ColorTable::FillOpacityTableFromDataPointer(vtkm::Int32 n, const vtkm::Float32* ptr)
{
  if (n <= 0 || ptr == nullptr)
  {
    return false;
  }
  this->ClearAlpha();


  std::size_t size = static_cast<std::size_t>(n / 4);
  this->Internals->OpacityNodePos.reserve(size);
  this->Internals->OpacityAlpha.reserve(size);
  this->Internals->OpacityMidSharp.reserve(size);
  for (std::size_t i = 0; i < size; ++i)
  { //allows us to support unsorted arrays
    this->AddPointAlpha(ptr[0], ptr[1], ptr[2], ptr[3]);
    ptr += 4;
  }
  this->Internals->OpacityArraysChanged = true;
  this->Internals->Modified();
  return true;
}

//----------------------------------------------------------------------------
void ColorTable::UpdateArrayHandles() const
{
  //Only rebuild the array handles that have changed since the last time
  //we have modified or color / opacity information

  if (this->Internals->ColorArraysChanged)
  {
    this->Internals->ColorPosHandle =
      vtkm::cont::make_ArrayHandle(this->Internals->ColorNodePos, vtkm::CopyFlag::Off);
    this->Internals->ColorRGBHandle =
      vtkm::cont::make_ArrayHandle(this->Internals->ColorRGB, vtkm::CopyFlag::Off);
    this->Internals->ColorArraysChanged = false;
  }

  if (this->Internals->OpacityArraysChanged)
  {
    this->Internals->OpacityPosHandle =
      vtkm::cont::make_ArrayHandle(this->Internals->OpacityNodePos, vtkm::CopyFlag::Off);
    this->Internals->OpacityAlphaHandle =
      vtkm::cont::make_ArrayHandle(this->Internals->OpacityAlpha, vtkm::CopyFlag::Off);
    this->Internals->OpacityMidSharpHandle =
      vtkm::cont::make_ArrayHandle(this->Internals->OpacityMidSharp, vtkm::CopyFlag::Off);
    this->Internals->OpacityArraysChanged = false;
  }
}

//---------------------------------------------------------------------------
vtkm::exec::ColorTable ColorTable::PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                       vtkm::cont::Token& token) const
{
  this->UpdateArrayHandles();

  vtkm::exec::ColorTable execTable;

  execTable.Space = this->Internals->Space;
  execTable.NaNColor = this->Internals->NaNColor;
  execTable.BelowRangeColor = this->Internals->BelowRangeColor;
  execTable.AboveRangeColor = this->Internals->AboveRangeColor;
  execTable.UseClamping = this->Internals->UseClamping;

  VTKM_ASSERT(static_cast<vtkm::Id>(this->Internals->ColorNodePos.size()) ==
              this->Internals->ColorPosHandle.GetNumberOfValues());
  execTable.ColorSize =
    static_cast<vtkm::Int32>(this->Internals->ColorPosHandle.GetNumberOfValues());
  VTKM_ASSERT(static_cast<vtkm::Id>(execTable.ColorSize) ==
              this->Internals->ColorRGBHandle.GetNumberOfValues());
  execTable.ColorNodes = this->Internals->ColorPosHandle.PrepareForInput(device, token).GetArray();
  execTable.RGB = this->Internals->ColorRGBHandle.PrepareForInput(device, token).GetArray();

  VTKM_ASSERT(static_cast<vtkm::Id>(this->Internals->OpacityNodePos.size()) ==
              this->Internals->OpacityPosHandle.GetNumberOfValues());
  execTable.OpacitySize =
    static_cast<vtkm::Int32>(this->Internals->OpacityPosHandle.GetNumberOfValues());
  VTKM_ASSERT(static_cast<vtkm::Id>(execTable.OpacitySize) ==
              this->Internals->OpacityAlphaHandle.GetNumberOfValues());
  VTKM_ASSERT(static_cast<vtkm::Id>(execTable.OpacitySize) ==
              this->Internals->OpacityMidSharpHandle.GetNumberOfValues());
  execTable.ONodes = this->Internals->OpacityPosHandle.PrepareForInput(device, token).GetArray();
  execTable.Alpha = this->Internals->OpacityAlphaHandle.PrepareForInput(device, token).GetArray();
  execTable.MidSharp =
    this->Internals->OpacityMidSharpHandle.PrepareForInput(device, token).GetArray();

  return execTable;
}

vtkm::exec::ColorTable ColorTable::PrepareForExecution(vtkm::cont::DeviceAdapterId device) const
{
  vtkm::cont::Token token;
  return this->PrepareForExecution(device, token);
}

//---------------------------------------------------------------------------
vtkm::Id ColorTable::GetModifiedCount() const
{
  return this->Internals->ModifiedCount;
}
}
} //namespace vtkm::cont
