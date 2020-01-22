//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_StorageImplicit
#define vtk_m_cont_StorageImplicit

#include <vtkm/Types.h>

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Storage.h>

#include <vtkm/cont/internal/ArrayTransfer.h>

namespace vtkm
{
namespace cont
{

/// \brief An implementation for read-only implicit arrays.
///
/// It is sometimes the case that you want VTK-m to operate on an array of
/// implicit values. That is, rather than store the data in an actual array, it
/// is gerenated on the fly by a function. This is handled in VTK-m by creating
/// an ArrayHandle in VTK-m with a StorageTagImplicit type of \c Storage. This
/// tag itself is templated to specify an ArrayPortal that generates the
/// desired values. An ArrayHandle created with this tag will raise an error on
/// any operation that tries to modify it.
///
template <class ArrayPortalType>
struct VTKM_ALWAYS_EXPORT StorageTagImplicit
{
  using PortalType = ArrayPortalType;
};

namespace internal
{

template <class ArrayPortalType>
class VTKM_ALWAYS_EXPORT
  Storage<typename ArrayPortalType::ValueType, StorageTagImplicit<ArrayPortalType>>
{
  using ClassType =
    Storage<typename ArrayPortalType::ValueType, StorageTagImplicit<ArrayPortalType>>;

public:
  using ValueType = typename ArrayPortalType::ValueType;
  using PortalConstType = ArrayPortalType;

  // Note that this portal is likely to be read-only, so you will probably get an error
  // if you try to write to it.
  using PortalType = ArrayPortalType;

  VTKM_CONT
  Storage(const PortalConstType& portal = PortalConstType())
    : Portal(portal)
    , NumberOfValues(portal.GetNumberOfValues())
  {
  }

  VTKM_CONT Storage(const ClassType&) = default;
  VTKM_CONT Storage(ClassType&&) = default;
  VTKM_CONT ClassType& operator=(const ClassType&) = default;
  VTKM_CONT ClassType& operator=(ClassType&&) = default;

  // All these methods do nothing but raise errors.
  VTKM_CONT
  PortalType GetPortal() { throw vtkm::cont::ErrorBadValue("Implicit arrays are read-only."); }
  VTKM_CONT
  PortalConstType GetPortalConst() const { return this->Portal; }
  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }
  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(numberOfValues <= this->Portal.GetNumberOfValues());
    this->NumberOfValues = numberOfValues;
  }
  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(numberOfValues <= this->Portal.GetNumberOfValues());
    this->NumberOfValues = numberOfValues;
  }
  VTKM_CONT
  void ReleaseResources() {}

private:
  PortalConstType Portal;
  vtkm::Id NumberOfValues;
};

template <typename T, class ArrayPortalType, class DeviceAdapterTag>
class ArrayTransfer<T, StorageTagImplicit<ArrayPortalType>, DeviceAdapterTag>
{
public:
  using StorageTag = StorageTagImplicit<ArrayPortalType>;
  using StorageType = vtkm::cont::internal::Storage<T, StorageTag>;

  using ValueType = T;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;
  using PortalExecution = PortalControl;
  using PortalConstExecution = PortalConstControl;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Storage(storage)
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return this->Storage->GetPortalConst();
  }

#if defined(VTKM_GCC) && defined(VTKM_ENABLE_OPENMP) && (__GNUC__ == 6 && __GNUC_MINOR__ == 1)
// When using GCC 6.1 with OpenMP enabled we cause a compiler ICE that is
// an identified compiler regression (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71210)
// The easiest way to work around this is to make sure we aren't building with >= O2
#define NO_OPTIMIZE_FUNC_ATTRIBUTE __attribute__((optimize(1)))
#else // gcc 6.1 openmp compiler ICE workaround
#define NO_OPTIMIZE_FUNC_ATTRIBUTE
#endif

  VTKM_CONT
  NO_OPTIMIZE_FUNC_ATTRIBUTE
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    throw vtkm::cont::ErrorBadValue("Implicit arrays cannot be used for output or in place.");
  }

  VTKM_CONT
  NO_OPTIMIZE_FUNC_ATTRIBUTE
  PortalExecution PrepareForOutput(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadValue("Implicit arrays cannot be used for output.");
  }

#undef NO_OPTIMIZE_FUNC_ATTRIBUTE

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(controlArray)) const
  {
    throw vtkm::cont::ErrorBadValue("Implicit arrays cannot be used for output.");
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadValue("Implicit arrays cannot be resized.");
  }

  VTKM_CONT
  void ReleaseResources() {}

private:
  StorageType* Storage;
};

} // namespace internal
}
} // namespace vtkm::cont

#endif //vtk_m_cont_StorageImplicit
