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
#include <vtkm/cont/ColorTablePrivate.hxx>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/TryExecute.h>


namespace vtkm
{
namespace cont
{

namespace internal
{
std::set<std::string> GetPresetNames();
bool LoadColorTablePreset(vtkm::cont::ColorTable::Preset preset, vtkm::cont::ColorTable& table);
bool LoadColorTablePreset(std::string name, vtkm::cont::ColorTable& table);
} // namespace internal

//----------------------------------------------------------------------------
ColorTable::ColorTable(vtkm::cont::ColorTable::Preset preset)
  : Impl(std::make_shared<detail::ColorTableInternals>())
{
  const bool loaded = this->LoadPreset(preset);
  if (!loaded)
  { //if we failed to load the requested color table, call SetColorSpace
    //so that the internal host side cache is constructed and we leave
    //the constructor in a valid state. We use LAB as it is the default
    //when the no parameter constructor is called
    this->SetColorSpace(ColorSpace::LAB);
  }
  this->AddSegmentAlpha(this->Impl->TableRange.Min, 1.0f, this->Impl->TableRange.Max, 1.0f);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(const std::string& name)
  : Impl(std::make_shared<detail::ColorTableInternals>())
{
  const bool loaded = this->LoadPreset(name);
  if (!loaded)
  { //if we failed to load the requested color table, call SetColorSpace
    //so that the internal host side cache is constructed and we leave
    //the constructor in a valid state. We use LAB as it is the default
    //when the no parameter constructor is called
    this->SetColorSpace(ColorSpace::LAB);
  }
  this->AddSegmentAlpha(this->Impl->TableRange.Min, 1.0f, this->Impl->TableRange.Max, 1.0f);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(ColorSpace space)
  : Impl(std::make_shared<detail::ColorTableInternals>())
{
  this->SetColorSpace(space);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(const vtkm::Range& range,
                       const vtkm::Vec<float, 3>& rgb1,
                       const vtkm::Vec<float, 3>& rgb2,
                       ColorSpace space)
  : Impl(std::make_shared<detail::ColorTableInternals>())
{
  this->AddSegment(range.Min, rgb1, range.Max, rgb2);
  this->AddSegmentAlpha(range.Min, 1.0f, range.Max, 1.0f);
  this->SetColorSpace(space);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(const vtkm::Range& range,
                       const vtkm::Vec<float, 4>& rgba1,
                       const vtkm::Vec<float, 4>& rgba2,
                       ColorSpace space)
  : Impl(std::make_shared<detail::ColorTableInternals>())
{
  vtkm::Vec<float, 3> rgb1(rgba1[0], rgba1[1], rgba1[2]);
  vtkm::Vec<float, 3> rgb2(rgba2[0], rgba2[1], rgba2[2]);
  this->AddSegment(range.Min, rgb1, range.Max, rgb2);
  this->AddSegmentAlpha(range.Min, rgba1[3], range.Max, rgba2[3]);
  this->SetColorSpace(space);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(const std::string& name,
                       vtkm::cont::ColorSpace colorSpace,
                       const vtkm::Vec<double, 3>& nanColor,
                       const std::vector<double>& rgbPoints,
                       const std::vector<double>& alphaPoints)
  : Impl(std::make_shared<detail::ColorTableInternals>())
{
  this->SetName(name);
  this->SetColorSpace(colorSpace);
  this->SetNaNColor(nanColor);
  this->FillColorTableFromDataPointer(static_cast<vtkm::Int32>(rgbPoints.size()), rgbPoints.data());
  this->FillOpacityTableFromDataPointer(static_cast<vtkm::Int32>(alphaPoints.size()),
                                        alphaPoints.data());
}

//----------------------------------------------------------------------------
ColorTable::~ColorTable()
{
}

//----------------------------------------------------------------------------
const std::string& ColorTable::GetName() const
{
  return this->Impl->Name;
}

//----------------------------------------------------------------------------
void ColorTable::SetName(const std::string& name)
{
  this->Impl->Name = name;
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
  ColorTable dcopy(this->Impl->CSpace);

  dcopy.Impl->TableRange = this->Impl->TableRange;

  dcopy.Impl->HostSideCache->NaNColor = this->Impl->HostSideCache->NaNColor;
  dcopy.Impl->HostSideCache->BelowRangeColor = this->Impl->HostSideCache->BelowRangeColor;
  dcopy.Impl->HostSideCache->AboveRangeColor = this->Impl->HostSideCache->AboveRangeColor;

  dcopy.Impl->HostSideCache->UseClamping = this->Impl->HostSideCache->UseClamping;

  dcopy.Impl->ColorNodePos = this->Impl->ColorNodePos;
  dcopy.Impl->ColorRGB = this->Impl->ColorRGB;

  dcopy.Impl->OpacityNodePos = this->Impl->OpacityNodePos;
  dcopy.Impl->OpacityAlpha = this->Impl->OpacityAlpha;
  dcopy.Impl->OpacityMidSharp = this->Impl->OpacityMidSharp;
  return dcopy;
}

//----------------------------------------------------------------------------
ColorSpace ColorTable::GetColorSpace() const
{
  return this->Impl->CSpace;
}

//----------------------------------------------------------------------------
void ColorTable::SetColorSpace(ColorSpace space)
{

  if (this->Impl->CSpace != space || this->Impl->HostSideCache.get() == nullptr)
  {
    this->Impl->HostSideCacheChanged = true;
    this->Impl->CSpace = space;
    //Remove any existing host information

    switch (space)
    {
      case vtkm::cont::ColorSpace::RGB:
      {
        auto* hostPortal = new vtkm::exec::ColorTableRGB();
        this->Impl->HostSideCache.reset(hostPortal);
        break;
      }
      case vtkm::cont::ColorSpace::HSV:
      {
        auto* hostPortal = new vtkm::exec::ColorTableHSV();
        this->Impl->HostSideCache.reset(hostPortal);
        break;
      }
      case vtkm::cont::ColorSpace::HSV_WRAP:
      {
        auto* hostPortal = new vtkm::exec::ColorTableHSVWrap();
        this->Impl->HostSideCache.reset(hostPortal);
        break;
      }
      case vtkm::cont::ColorSpace::LAB:
      {
        auto* hostPortal = new vtkm::exec::ColorTableLab();
        this->Impl->HostSideCache.reset(hostPortal);
        break;
      }
      case vtkm::cont::ColorSpace::DIVERGING:
      {
        auto* hostPortal = new vtkm::exec::ColorTableDiverging();
        this->Impl->HostSideCache.reset(hostPortal);
        break;
      }
      default:
        throw vtkm::cont::ErrorBadType("unknown vtkm::cont::ColorType requested");
    }
  }
}

//----------------------------------------------------------------------------
void ColorTable::SetClamping(bool state)
{
  this->Impl->HostSideCache->UseClamping = state;
  this->Impl->HostSideCache->Modified();
}

//----------------------------------------------------------------------------
bool ColorTable::GetClamping() const
{
  return this->Impl->HostSideCache->UseClamping;
}

//----------------------------------------------------------------------------
void ColorTable::SetBelowRangeColor(const vtkm::Vec<float, 3>& c)
{
  this->Impl->HostSideCache->BelowRangeColor = c;
  this->Impl->HostSideCache->Modified();
}

//----------------------------------------------------------------------------
const vtkm::Vec<float, 3>& ColorTable::GetBelowRangeColor() const
{
  return this->Impl->HostSideCache->BelowRangeColor;
}

//----------------------------------------------------------------------------
void ColorTable::SetAboveRangeColor(const vtkm::Vec<float, 3>& c)
{
  this->Impl->HostSideCache->AboveRangeColor = c;
  this->Impl->HostSideCache->Modified();
}

//----------------------------------------------------------------------------
const vtkm::Vec<float, 3>& ColorTable::GetAboveRangeColor() const
{
  return this->Impl->HostSideCache->AboveRangeColor;
}

//----------------------------------------------------------------------------
void ColorTable::SetNaNColor(const vtkm::Vec<float, 3>& c)
{
  this->Impl->HostSideCache->NaNColor = c;
  this->Impl->HostSideCache->Modified();
}

//----------------------------------------------------------------------------
const vtkm::Vec<float, 3>& ColorTable::GetNaNColor() const
{
  return this->Impl->HostSideCache->NaNColor;
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
  this->Impl->ColorNodePos.clear();
  this->Impl->ColorRGB.clear();
  this->Impl->ColorArraysChanged = true;
}

//---------------------------------------------------------------------------
void ColorTable::ClearAlpha()
{
  this->Impl->OpacityNodePos.clear();
  this->Impl->OpacityAlpha.clear();
  this->Impl->OpacityMidSharp.clear();
  this->Impl->OpacityArraysChanged = true;
}

//---------------------------------------------------------------------------
void ColorTable::ReverseColors()
{
  std::reverse(this->Impl->ColorRGB.begin(), this->Impl->ColorRGB.end());
  this->Impl->ColorArraysChanged = true;
}

//---------------------------------------------------------------------------
void ColorTable::ReverseAlpha()
{
  std::reverse(this->Impl->OpacityAlpha.begin(), this->Impl->OpacityAlpha.end());
  //To keep the shape correct the mid and sharp values of the last node are not included in the reversal
  std::reverse(this->Impl->OpacityMidSharp.begin(), this->Impl->OpacityMidSharp.end() - 1);
  this->Impl->OpacityArraysChanged = true;
}

//---------------------------------------------------------------------------
const vtkm::Range& ColorTable::GetRange() const
{
  return this->Impl->TableRange;
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
  const double minv = this->GetRange().Min;
  const double oldScale = this->GetRange().Length();
  const double newScale = newRange.Length();
  VTKM_ASSERT(oldScale > 0);
  VTKM_ASSERT(newScale > 0);
  for (auto i = this->Impl->ColorNodePos.begin(); i != this->Impl->ColorNodePos.end(); ++i)
  {
    const auto t = (*i - minv) / oldScale;
    *i = (t * newScale) + newRange.Min;
  }
  for (auto i = this->Impl->OpacityNodePos.begin(); i != this->Impl->OpacityNodePos.end(); ++i)
  {
    const auto t = (*i - minv) / oldScale;
    *i = (t * newScale) + newRange.Min;
  }

  this->Impl->ColorArraysChanged = true;
  this->Impl->OpacityArraysChanged = true;
  this->Impl->TableRange = newRange;
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::AddPoint(double x, const vtkm::Vec<float, 3>& rgb)
{
  if (outside_range(rgb))
  {
    return -1;
  }

  std::size_t index = 0;
  if (this->Impl->ColorNodePos.size() == 0 || this->Impl->ColorNodePos.back() < x)
  {
    this->Impl->ColorNodePos.emplace_back(x);
    this->Impl->ColorRGB.emplace_back(rgb);
    index = this->Impl->ColorNodePos.size();
  }
  else
  {
    auto begin = this->Impl->ColorNodePos.begin();
    auto pos = std::lower_bound(begin, this->Impl->ColorNodePos.end(), x);
    index = static_cast<std::size_t>(std::distance(begin, pos));

    if (*pos == x)
    {
      this->Impl->ColorRGB[index] = rgb;
    }
    else
    {
      this->Impl->ColorRGB.emplace(this->Impl->ColorRGB.begin() + std::distance(begin, pos), rgb);
      this->Impl->ColorNodePos.emplace(pos, x);
    }
  }
  this->Impl->TableRange.Include(x); //update range to include x
  this->Impl->ColorArraysChanged = true;
  return static_cast<vtkm::Int32>(index);
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::AddPointHSV(double x, const vtkm::Vec<float, 3>& hsv)
{
  return this->AddPoint(x, hsvTorgb(hsv));
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::AddSegment(double x1,
                                   const vtkm::Vec<float, 3>& rgb1,
                                   double x2,
                                   const vtkm::Vec<float, 3>& rgb2)
{
  if (outside_range(rgb1, rgb2))
  {
    return -1;
  }
  if (this->Impl->ColorNodePos.size() > 0)
  {
    //Todo:
    // - This could be optimized so we do 2 less lower_bound calls when
    // the table already exists

    //When we add a segment we remove all points that are inside the line

    auto nodeBegin = this->Impl->ColorNodePos.begin();
    auto nodeEnd = this->Impl->ColorNodePos.end();

    auto rgbBegin = this->Impl->ColorRGB.begin();

    auto nodeStart = std::lower_bound(nodeBegin, nodeEnd, x1);
    auto nodeStop = std::lower_bound(nodeBegin, nodeEnd, x2);

    auto rgbStart = rgbBegin + std::distance(nodeBegin, nodeStart);
    auto rgbStop = rgbBegin + std::distance(nodeBegin, nodeStop);

    //erase is exclusive so if end->x == x2 it will be kept around, and
    //than we will update it in AddPoint
    this->Impl->ColorNodePos.erase(nodeStart, nodeStop);
    this->Impl->ColorRGB.erase(rgbStart, rgbStop);
  }
  vtkm::Int32 pos = this->AddPoint(x1, rgb1);
  this->AddPoint(x2, rgb2);
  return pos;
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::AddSegmentHSV(double x1,
                                      const vtkm::Vec<float, 3>& hsv1,
                                      double x2,
                                      const vtkm::Vec<float, 3>& hsv2)
{
  return this->AddSegment(x1, hsvTorgb(hsv1), x2, hsvTorgb(hsv2));
}

//---------------------------------------------------------------------------
bool ColorTable::GetPoint(vtkm::Int32 index, vtkm::Vec<double, 4>& data) const
{
  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Impl->ColorNodePos.size();
  if (index < 0 || i >= size)
  {
    return false;
  }

  const auto& pos = this->Impl->ColorNodePos[i];
  const auto& rgb = this->Impl->ColorRGB[i];

  data[0] = pos;
  data[1] = rgb[0];
  data[2] = rgb[1];
  data[3] = rgb[2];
  return true;
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::UpdatePoint(vtkm::Int32 index, const vtkm::Vec<double, 4>& data)
{
  //skip data[0] as we don't care about position
  if (outside_range(data[1], data[2], data[3]))
  {
    return -1;
  }

  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Impl->ColorNodePos.size();
  if (index < 0 || i >= size)
  {
    return -1;
  }

  //When updating the first question is has the relative position of the point changed?
  //If it hasn't we can quickly just update the RGB value
  auto oldPos = this->Impl->ColorNodePos.begin() + index;
  auto newPos =
    std::lower_bound(this->Impl->ColorNodePos.begin(), this->Impl->ColorNodePos.end(), data[0]);
  if (oldPos == newPos)
  { //node's relative location hasn't changed
    this->Impl->ColorArraysChanged = true;
    auto& rgb = this->Impl->ColorRGB[i];
    *newPos = data[0];
    rgb[0] = static_cast<float>(data[1]);
    rgb[1] = static_cast<float>(data[2]);
    rgb[2] = static_cast<float>(data[3]);
    return index;
  }
  else
  { //remove the point, and add the new values as the relative location is different
    this->RemovePoint(index);
    vtkm::Vec<float, 3> newrgb(
      static_cast<float>(data[1]), static_cast<float>(data[2]), static_cast<float>(data[3]));
    return this->AddPoint(data[0], newrgb);
  }
}

//---------------------------------------------------------------------------
bool ColorTable::RemovePoint(double x)
{
  auto begin = this->Impl->ColorNodePos.begin();
  auto pos = std::lower_bound(begin, this->Impl->ColorNodePos.end(), x);
  return this->RemovePoint(static_cast<vtkm::Int32>(std::distance(begin, pos)));
}

//---------------------------------------------------------------------------
bool ColorTable::RemovePoint(vtkm::Int32 index)
{
  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Impl->ColorNodePos.size();
  if (index < 0 || i >= size)
  {
    return false;
  }

  this->Impl->ColorNodePos.erase(this->Impl->ColorNodePos.begin() + index);
  this->Impl->ColorRGB.erase(this->Impl->ColorRGB.begin() + index);
  this->Impl->ColorArraysChanged = true;
  this->Impl->RecalculateRange();
  return true;
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::GetNumberOfPoints() const
{
  return static_cast<vtkm::Int32>(this->Impl->ColorNodePos.size());
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::AddPointAlpha(double x, float alpha, float midpoint, float sharpness)
{
  if (outside_range(alpha, midpoint, sharpness))
  {
    return -1;
  }

  const vtkm::Vec<float, 2> midsharp(midpoint, sharpness);
  std::size_t index = 0;
  if (this->Impl->OpacityNodePos.size() == 0 || this->Impl->OpacityNodePos.back() < x)
  {
    this->Impl->OpacityNodePos.emplace_back(x);
    this->Impl->OpacityAlpha.emplace_back(alpha);
    this->Impl->OpacityMidSharp.emplace_back(midsharp);
    index = this->Impl->OpacityNodePos.size();
  }
  else
  {
    auto begin = this->Impl->OpacityNodePos.begin();
    auto pos = std::lower_bound(begin, this->Impl->OpacityNodePos.end(), x);
    index = static_cast<std::size_t>(std::distance(begin, pos));
    if (*pos == x)
    {
      this->Impl->OpacityAlpha[index] = alpha;
      this->Impl->OpacityMidSharp[index] = midsharp;
    }
    else
    {
      this->Impl->OpacityAlpha.emplace(this->Impl->OpacityAlpha.begin() + std::distance(begin, pos),
                                       alpha);
      this->Impl->OpacityMidSharp.emplace(
        this->Impl->OpacityMidSharp.begin() + std::distance(begin, pos), midsharp);
      this->Impl->OpacityNodePos.emplace(pos, x);
    }
  }
  this->Impl->OpacityArraysChanged = true;
  this->Impl->TableRange.Include(x); //update range to include x
  return static_cast<vtkm::Int32>(index);
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::AddSegmentAlpha(double x1,
                                        float alpha1,
                                        double x2,
                                        float alpha2,
                                        const vtkm::Vec<float, 2>& mid_sharp1,
                                        const vtkm::Vec<float, 2>& mid_sharp2)
{
  if (outside_range(alpha1, alpha2, mid_sharp1, mid_sharp2))
  {
    return -1;
  }

  if (this->Impl->OpacityNodePos.size() > 0)
  {
    //Todo:
    // - This could be optimized so we do 2 less lower_bound calls when
    // the table already exists

    //When we add a segment we remove all points that are inside the line

    auto nodeBegin = this->Impl->OpacityNodePos.begin();
    auto nodeEnd = this->Impl->OpacityNodePos.end();

    auto alphaBegin = this->Impl->OpacityAlpha.begin();
    auto midBegin = this->Impl->OpacityMidSharp.begin();

    auto nodeStart = std::lower_bound(nodeBegin, nodeEnd, x1);
    auto nodeStop = std::lower_bound(nodeBegin, nodeEnd, x2);

    auto alphaStart = alphaBegin + std::distance(nodeBegin, nodeStart);
    auto alphaStop = alphaBegin + std::distance(nodeBegin, nodeStop);
    auto midStart = midBegin + std::distance(nodeBegin, nodeStart);
    auto midStop = midBegin + std::distance(nodeBegin, nodeStop);

    //erase is exclusive so if end->x == x2 it will be kept around, and
    //than we will update it in AddPoint
    this->Impl->OpacityNodePos.erase(nodeStart, nodeStop);
    this->Impl->OpacityAlpha.erase(alphaStart, alphaStop);
    this->Impl->OpacityMidSharp.erase(midStart, midStop);
  }

  vtkm::Int32 pos = this->AddPointAlpha(x1, alpha1, mid_sharp1[0], mid_sharp1[1]);
  this->AddPointAlpha(x2, alpha2, mid_sharp2[0], mid_sharp2[1]);
  return pos;
}

//---------------------------------------------------------------------------
bool ColorTable::GetPointAlpha(vtkm::Int32 index, vtkm::Vec<double, 4>& data) const
{
  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Impl->OpacityNodePos.size();
  if (index < 0 || i >= size)
  {
    return false;
  }

  const auto& pos = this->Impl->OpacityNodePos[i];
  const auto& alpha = this->Impl->OpacityAlpha[i];
  const auto& midsharp = this->Impl->OpacityMidSharp[i];

  data[0] = pos;
  data[1] = alpha;
  data[2] = midsharp[0];
  data[3] = midsharp[1];
  return true;
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::UpdatePointAlpha(vtkm::Int32 index, const vtkm::Vec<double, 4>& data)
{
  //skip data[0] as we don't care about position
  if (outside_range(data[1], data[2], data[3]))
  {
    return -1;
  }

  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Impl->OpacityNodePos.size();
  if (index < 0 || i >= size)
  {
    return -1;
  }
  //When updating the first question is has the relative position of the point changed?
  //If it hasn't we can quickly just update the RGB value
  auto oldPos = this->Impl->OpacityNodePos.begin() + index;
  auto newPos =
    std::lower_bound(this->Impl->OpacityNodePos.begin(), this->Impl->OpacityNodePos.end(), data[0]);
  if (oldPos == newPos)
  { //node's relative location hasn't changed
    this->Impl->OpacityArraysChanged = true;
    auto& alpha = this->Impl->OpacityAlpha[i];
    auto& midsharp = this->Impl->OpacityMidSharp[i];
    *newPos = data[0];
    alpha = static_cast<float>(data[1]);
    midsharp[0] = static_cast<float>(data[2]);
    midsharp[1] = static_cast<float>(data[3]);
    return index;
  }
  else
  { //remove the point, and add the new values as the relative location is different
    this->RemovePointAlpha(index);
    return this->AddPointAlpha(data[0],
                               static_cast<float>(data[1]),
                               static_cast<float>(data[2]),
                               static_cast<float>(data[3]));
  }
}

//---------------------------------------------------------------------------
bool ColorTable::RemovePointAlpha(double x)
{
  auto begin = this->Impl->OpacityNodePos.begin();
  auto pos = std::lower_bound(begin, this->Impl->OpacityNodePos.end(), x);
  return this->RemovePointAlpha(static_cast<vtkm::Int32>(std::distance(begin, pos)));
}

//---------------------------------------------------------------------------
bool ColorTable::RemovePointAlpha(vtkm::Int32 index)
{
  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Impl->OpacityNodePos.size();
  if (index < 0 || i >= size)
  {
    return false;
  }

  this->Impl->OpacityNodePos.erase(this->Impl->OpacityNodePos.begin() + index);
  this->Impl->OpacityAlpha.erase(this->Impl->OpacityAlpha.begin() + index);
  this->Impl->OpacityMidSharp.erase(this->Impl->OpacityMidSharp.begin() + index);
  this->Impl->OpacityArraysChanged = true;
  this->Impl->RecalculateRange();
  return true;
}

//---------------------------------------------------------------------------
vtkm::Int32 ColorTable::GetNumberOfPointsAlpha() const
{
  return static_cast<vtkm::Int32>(this->Impl->OpacityNodePos.size());
}

//---------------------------------------------------------------------------
bool ColorTable::FillColorTableFromDataPointer(vtkm::Int32 n, const double* ptr)
{
  if (n <= 0 || ptr == nullptr)
  {
    return false;
  }
  this->ClearColors();

  std::size_t size = static_cast<std::size_t>(n / 4);
  this->Impl->ColorNodePos.reserve(size);
  this->Impl->ColorRGB.reserve(size);
  for (std::size_t i = 0; i < size; ++i)
  { //allows us to support unsorted arrays
    vtkm::Vec<float, 3> rgb(
      static_cast<float>(ptr[1]), static_cast<float>(ptr[2]), static_cast<float>(ptr[3]));
    this->AddPoint(ptr[0], rgb);
    ptr += 4;
  }
  this->Impl->ColorArraysChanged = true;

  return true;
}

//---------------------------------------------------------------------------
bool ColorTable::FillColorTableFromDataPointer(vtkm::Int32 n, const float* ptr)
{
  if (n <= 0 || ptr == nullptr)
  {
    return false;
  }
  this->ClearColors();

  std::size_t size = static_cast<std::size_t>(n / 4);
  this->Impl->ColorNodePos.reserve(size);
  this->Impl->ColorRGB.reserve(size);
  for (std::size_t i = 0; i < size; ++i)
  { //allows us to support unsorted arrays
    vtkm::Vec<float, 3> rgb(ptr[1], ptr[2], ptr[3]);
    this->AddPoint(ptr[0], rgb);
    ptr += 4;
  }
  this->Impl->ColorArraysChanged = true;
  return true;
}

//---------------------------------------------------------------------------
bool ColorTable::FillOpacityTableFromDataPointer(vtkm::Int32 n, const double* ptr)
{
  if (n <= 0 || ptr == nullptr)
  {
    return false;
  }
  this->ClearAlpha();

  std::size_t size = static_cast<std::size_t>(n / 4);
  this->Impl->OpacityNodePos.reserve(size);
  this->Impl->OpacityAlpha.reserve(size);
  this->Impl->OpacityMidSharp.reserve(size);
  for (std::size_t i = 0; i < size; ++i)
  { //allows us to support unsorted arrays
    this->AddPointAlpha(
      ptr[0], static_cast<float>(ptr[1]), static_cast<float>(ptr[2]), static_cast<float>(ptr[3]));
    ptr += 4;
  }

  this->Impl->OpacityArraysChanged = true;
  return true;
}

//---------------------------------------------------------------------------
bool ColorTable::FillOpacityTableFromDataPointer(vtkm::Int32 n, const float* ptr)
{
  if (n <= 0 || ptr == nullptr)
  {
    return false;
  }
  this->ClearAlpha();


  std::size_t size = static_cast<std::size_t>(n / 4);
  this->Impl->OpacityNodePos.reserve(size);
  this->Impl->OpacityAlpha.reserve(size);
  this->Impl->OpacityMidSharp.reserve(size);
  for (std::size_t i = 0; i < size; ++i)
  { //allows us to support unsorted arrays
    this->AddPointAlpha(ptr[0], ptr[1], ptr[2], ptr[3]);
    ptr += 4;
  }
  this->Impl->OpacityArraysChanged = true;
  return true;
}

//---------------------------------------------------------------------------
vtkm::Id ColorTable::GetModifiedCount() const
{
  return this->Impl->HostSideCache->GetModifiedCount();
}

//----------------------------------------------------------------------------
bool ColorTable::NeedToCreateExecutionColorTable() const
{
  return this->Impl->HostSideCacheChanged;
}

//----------------------------------------------------------------------------
void ColorTable::UpdateExecutionColorTable(
  vtkm::cont::VirtualObjectHandle<vtkm::exec::ColorTableBase>* handle) const
{
  this->Impl->ExecHandle.reset(handle);
}

//----------------------------------------------------------------------------
ColorTable::TransferState ColorTable::GetExecutionDataForTransfer() const
{
  //Only rebuild the array handles that have changed since the last time
  //we have modified or color / opacity information

  if (this->Impl->ColorArraysChanged)
  {
    this->Impl->ColorPosHandle = vtkm::cont::make_ArrayHandle(this->Impl->ColorNodePos);
    this->Impl->ColorRGBHandle = vtkm::cont::make_ArrayHandle(this->Impl->ColorRGB);
  }

  if (this->Impl->OpacityArraysChanged)
  {
    this->Impl->OpacityPosHandle = vtkm::cont::make_ArrayHandle(this->Impl->OpacityNodePos);
    this->Impl->OpacityAlphaHandle = vtkm::cont::make_ArrayHandle(this->Impl->OpacityAlpha);
    this->Impl->OpacityMidSharpHandle = vtkm::cont::make_ArrayHandle(this->Impl->OpacityMidSharp);
  }

  TransferState state = { (this->Impl->ColorArraysChanged || this->Impl->OpacityArraysChanged ||
                           this->Impl->HostSideCacheChanged),
                          this->Impl->HostSideCache.get(),
                          this->Impl->ColorPosHandle,
                          this->Impl->ColorRGBHandle,
                          this->Impl->OpacityPosHandle,
                          this->Impl->OpacityAlphaHandle,
                          this->Impl->OpacityMidSharpHandle };

  this->Impl->ColorArraysChanged = false;
  this->Impl->OpacityArraysChanged = false;
  this->Impl->HostSideCacheChanged = false;
  return state;
}

//----------------------------------------------------------------------------
vtkm::exec::ColorTableBase* ColorTable::GetControlRepresentation() const
{
  return this->Impl->HostSideCache.get();
}

//----------------------------------------------------------------------------
vtkm::cont::VirtualObjectHandle<vtkm::exec::ColorTableBase> const* ColorTable::GetExecutionHandle()
  const
{
  return this->Impl->ExecHandle.get();
}
}
} //namespace vtkm::cont
