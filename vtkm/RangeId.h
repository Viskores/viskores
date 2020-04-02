//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_RangeId_h
#define vtk_m_RangeId_h

#include <vtkm/Math.h>
#include <vtkm/Types.h>

namespace vtkm
{

/// \brief Represent a range of vtkm::Id values.
///
/// \c vtkm::RangeId is a helper class for representing a range of vtkm::Id
/// values. This is specified simply with a \c Min and \c Max value, where
/// \c Max is exclusive.
///
/// \c RangeId also contains several helper functions for computing and
/// maintaining the range.
///
struct RangeId
{
  vtkm::Id Min;
  vtkm::Id Max;

  VTKM_EXEC_CONT
  RangeId()
    : Min(0)
    , Max(0)
  {
  }

  VTKM_EXEC_CONT
  RangeId(vtkm::Id min, vtkm::Id max)
    : Min(min)
    , Max(max)
  {
  }

  /// \b Determine if the range is valid.
  ///
  /// \c IsNonEmpty return true if the range contains some valid values between
  /// \c Min and \c Max. If \c Max <= \c Min, then no values satisfy
  /// the range and \c IsNonEmpty returns false. Otherwise, return true.
  ///
  VTKM_EXEC_CONT
  bool IsNonEmpty() const { return (this->Min < this->Max); }

  /// \b Determines if a value is within the range.
  ///
  /// \c Contains returns true if the give value is within the range, false
  /// otherwise.
  ///
  VTKM_EXEC_CONT
  bool Contains(vtkm::Id value) const { return ((this->Min <= value) && (this->Max > value)); }

  /// \b Returns the length of the range.
  ///
  /// \c Length computes the distance between the min and max. If the range
  /// is empty, 0 is returned.
  ///
  VTKM_EXEC_CONT
  vtkm::Id Length() const { return this->Max - this->Min; }

  /// \b Returns the center of the range.
  ///
  /// \c Center computes the middle value of the range.
  ///
  VTKM_EXEC_CONT
  vtkm::Id Center() const { return (this->Min + this->Max) / 2; }

  /// \b Expand range to include a value.
  ///
  /// This version of \c Include expands the range just enough to include the
  /// given value. If the range already includes this value, then nothing is
  /// done.
  ///
  VTKM_EXEC_CONT
  void Include(vtkm::Id value)
  {
    this->Min = vtkm::Min(this->Min, value);
    this->Max = vtkm::Max(this->Max, value + 1);
  }

  /// \b Expand range to include other range.
  ///
  /// This version of \c Include expands this range just enough to include that
  /// of another range. Essentially it is the union of the two ranges.
  ///
  VTKM_EXEC_CONT
  void Include(const vtkm::RangeId& range)
  {
    this->Min = vtkm::Min(this->Min, range.Min);
    this->Max = vtkm::Max(this->Max, range.Max);
  }

  /// \b Return the union of this and another range.
  ///
  /// This is a nondestructive form of \c Include.
  ///
  VTKM_EXEC_CONT
  vtkm::RangeId Union(const vtkm::RangeId& other) const
  {
    vtkm::RangeId unionRange(*this);
    unionRange.Include(other);
    return unionRange;
  }

  /// \b Operator for union
  ///
  VTKM_EXEC_CONT
  vtkm::RangeId operator+(const vtkm::RangeId& other) const { return this->Union(other); }

  VTKM_EXEC_CONT
  bool operator==(const vtkm::RangeId& other) const
  {
    return ((this->Min == other.Min) && (this->Max == other.Max));
  }

  VTKM_EXEC_CONT
  bool operator!=(const vtkm::RangeId& other) const
  {
    return ((this->Min != other.Min) || (this->Max != other.Max));
  }
};

/// Helper function for printing ranges during testing
///
static inline VTKM_CONT std::ostream& operator<<(std::ostream& stream, const vtkm::RangeId& range)
{
  return stream << "[" << range.Min << ".." << range.Max << ")";
} // Declared inside of vtkm namespace so that the operator work with ADL lookup
} // namespace vtkm

#endif // vtk_m_RangeId_h
