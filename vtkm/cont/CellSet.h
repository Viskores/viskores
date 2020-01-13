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
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/VariantArrayHandle.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT CellSet
{
public:
  CellSet() = default;
  CellSet(const CellSet& src) = default;
  CellSet(CellSet&& src) noexcept = default;

  CellSet& operator=(const CellSet& src) = default;
  CellSet& operator=(CellSet&& src) noexcept = default;

  virtual ~CellSet();

  virtual vtkm::Id GetNumberOfCells() const = 0;

  virtual vtkm::Id GetNumberOfFaces() const = 0;

  virtual vtkm::Id GetNumberOfEdges() const = 0;

  virtual vtkm::Id GetNumberOfPoints() const = 0;

  virtual vtkm::UInt8 GetCellShape(vtkm::Id id) const = 0;
  virtual vtkm::IdComponent GetNumberOfPointsInCell(vtkm::Id id) const = 0;
  virtual void GetCellPointIds(vtkm::Id id, vtkm::Id* ptids) const = 0;

  virtual std::shared_ptr<CellSet> NewInstance() const = 0;
  virtual void DeepCopy(const CellSet* src) = 0;

  virtual void PrintSummary(std::ostream&) const = 0;

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
  using type = typename std::is_base_of<vtkm::cont::CellSet, U>;
};

#define VTKM_IS_CELL_SET(T) VTKM_STATIC_ASSERT(::vtkm::cont::internal::CellSetCheck<T>::type::value)

} // namespace internal
}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSet_h
