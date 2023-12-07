//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellSet_h
#define vtk_m_cont_CellSet_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/StaticAssert.h>

#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace cont
{

/// @brief Defines the topological structure of the data in a `DataSet`.
///
/// Fundamentally, any cell set is a collection of cells, which typically (but not always)
/// represent some region in space.
class VTKM_CONT_EXPORT CellSet
{
public:
  CellSet() = default;
  CellSet(const CellSet&) = default;
  CellSet(CellSet&&) noexcept = default;

  CellSet& operator=(const CellSet&) = default;
  CellSet& operator=(CellSet&&) noexcept = default;

  virtual ~CellSet();

  /// @brief Get the number of cells in the topology.
  virtual vtkm::Id GetNumberOfCells() const = 0;

  virtual vtkm::Id GetNumberOfFaces() const = 0;

  virtual vtkm::Id GetNumberOfEdges() const = 0;

  /// @brief Get the number of points in the topology.
  virtual vtkm::Id GetNumberOfPoints() const = 0;

  /// @brief Get the shell shape of a particular cell.
  virtual vtkm::UInt8 GetCellShape(vtkm::Id id) const = 0;
  /// @brief Get the number of points incident to a particular cell.
  virtual vtkm::IdComponent GetNumberOfPointsInCell(vtkm::Id id) const = 0;
  /// @brief Get a list of points incident to a particular cell.
  virtual void GetCellPointIds(vtkm::Id id, vtkm::Id* ptids) const = 0;

  /// @brief Return a new `CellSet` that is the same derived class.
  virtual std::shared_ptr<CellSet> NewInstance() const = 0;
  /// @brief Copy the provided `CellSet` into this object.
  ///
  /// The provided `CellSet` must be the same type as this one.
  virtual void DeepCopy(const CellSet* src) = 0;

  /// @brief Print a summary of this cell set.
  virtual void PrintSummary(std::ostream&) const = 0;

  /// @brief Remove the `CellSet` from any devices.
  ///
  /// Any memory used on a device to store this object will be deleted.
  /// However, the data will still remain on the host.
  virtual void ReleaseResourcesExecution() = 0;
};

namespace internal
{

/// Checks to see if the given object is a cell set. It contains a
/// typedef named \c type that is either std::true_type or
/// std::false_type. Both of these have a typedef named value with the
/// respective boolean value.
///
template <typename T>
struct CellSetCheck
{
  using U = typename std::remove_pointer<T>::type;
  using type = typename std::is_base_of<vtkm::cont::CellSet, U>::type;
};

#define VTKM_IS_CELL_SET(T) VTKM_STATIC_ASSERT(::vtkm::cont::internal::CellSetCheck<T>::type::value)

} // namespace internal
}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSet_h
