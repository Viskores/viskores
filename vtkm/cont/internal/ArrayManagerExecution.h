//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_internal_ArrayManagerExecution_h
#define vtk_m_cont_internal_ArrayManagerExecution_h

#include <vtkm/cont/internal/DeviceAdapterTag.h>

namespace vtkm {
namespace cont {
namespace internal {

/// \brief Class that manages data in the execution environment.
///
/// This templated class must be partially specialized for each
/// DeviceAdapterTag crated, which will define the implementation for that tag.
///
/// This is a class that is responsible for allocating data in the execution
/// environment and copying data back and forth between control and
/// execution. It is also expected that this class will automatically release
/// any resources in its destructor.
///
/// This class typically takes on one of two forms. If the control and
/// execution environments have seperate memory spaces, then this class
/// behaves how you would expect. It allocates/deallocates arrays and copies
/// data. However, if the control and execution environments share the same
/// memory space, this class should delegate all its operations to the
/// \c Storage. The latter can probably be implemented with a
/// trivial subclass of
/// vtkm::cont::internal::ArrayManagerExecutionShareWithControl.
///
template<typename T, class StorageTag, class DeviceAdapterTag>
class ArrayManagerExecution
#ifdef VTKM_DOXYGEN_ONLY
{
private:
  typedef vtkm::cont::internal::Storage<T,StorageTag> StorageType;

public:
  /// The type of value held in the array (vtkm::FloatDefault, vtkm::Vec, etc.)
  ///
  typedef T ValueType;

  /// An array portal that can be used in the execution environment to access
  /// portions of the arrays. This example defines the portal with a pointer,
  /// but any portal with methods that can be called and data that can be
  /// accessed from the execution environment can be used.
  ///
  typedef vtkm::exec::internal::ArrayPortalFromIterators<ValueType*> PortalType;

  /// Const version of PortalType.  You must be able to cast PortalType to
  /// PortalConstType.
  ///
  typedef vtkm::exec::internal::ArrayPortalFromIterators<const ValueType*>
      PortalConstType;

  /// Returns the number of values stored in the array.  Results are undefined
  /// if data has not been loaded or allocated.
  ///
  VTKM_CONT_EXPORT vtkm::Id GetNumberOfValues() const;

  /// Allocates a large enough array in the execution environment and copies
  /// the given data to that array. The allocated array can later be accessed
  /// via the GetPortalConst method. If control and execution share arrays,
  /// then this method may save the iterators to be returned in the \c
  /// GetPortalConst method.
  ///
  VTKM_CONT_EXPORT void LoadDataForInput(
      const typename StorageType::PortalConstType& portal);

  /// Allocates a large enough array in the execution environment and copies
  /// the given data to that array. The allocated array can later be accessed
  /// via the GetPortal method. If control and execution share arrays, then
  /// this method may save the iterators of the storage to be returned in the
  /// \c GetPortal* methods.
  ///
  VTKM_CONT_EXPORT void LoadDataForInPlace(PortalType portal);

  /// Allocates an array in the execution environment of the specified size.
  /// If control and execution share arrays, then this class can allocate
  /// data using the given Storage object and remember its iterators
  /// so that it can be used directly in the execution environment.
  ///
  VTKM_CONT_EXPORT void AllocateArrayForOutput(StorageType &controlArray,
                                               vtkm::Id numberOfValues);

  /// Allocates data in the given Storage and copies data held
  /// in the execution environment (managed by this class) into the control
  /// array. If control and execution share arrays, this can be no operation.
  /// This method should only be called after AllocateArrayForOutput is
  /// called.
  ///
  VTKM_CONT_EXPORT void RetrieveOutputData(StorageType &controlArray) const;

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  ///
  VTKM_CONT_EXPORT void Shrink(vtkm::Id numberOfValues);

  /// Returns an array portal that can be used in the execution environment.
  /// This portal was defined in either LoadDataForInput or
  /// AllocateArrayForOutput. If control and environment share memory space,
  /// this class may return the iterator from the \c controlArray.
  ///
  VTKM_CONT_EXPORT PortalType GetPortal();

  /// Const version of GetPortal.
  ///
  VTKM_CONT_EXPORT PortalConstType GetPortalConst() const;

  /// Frees any resources (i.e. memory) allocated for the exeuction
  /// environment, if any.
  ///
  VTKM_CONT_EXPORT void ReleaseResources();
};
#else // VTKM_DOXGEN_ONLY
;
#endif // VTKM_DOXYGEN_ONLY

}
}
} // namespace vtkm::cont::internal


//-----------------------------------------------------------------------------
// These includes are intentionally placed here after the declaration of the
// ArrayManagerExecution template prototype, which all the implementations
// need.

#if VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_SERIAL
#include <vtkm/cont/internal/ArrayManagerExecutionSerial.h>
#elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_CUDA
#include <vtkm/cont/cuda/internal/ArrayManagerExecutionCuda.h>
// #elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_OPENMP
// #include <vtkm/openmp/cont/internal/ArrayManagerExecutionOpenMP.h>
// #elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_TBB
// #include <vtkm/tbb/cont/internal/ArrayManagerExecutionTBB.h>
#endif

#endif //vtk_m_cont_internal_ArrayManagerExecution_h
