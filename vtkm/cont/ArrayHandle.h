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
#ifndef vtk_m_cont_ArrayHandle_h
#define vtk_m_cont_ArrayHandle_h

#include <vtkm/Types.h>

#include <vtkm/cont/Assert.h>
#include <vtkm/cont/ErrorControlBadValue.h>
#include <vtkm/cont/ErrorControlInternal.h>
#include <vtkm/cont/Storage.h>
#include <vtkm/cont/StorageBasic.h>

#include <vtkm/cont/internal/ArrayHandleExecutionManager.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>

#include <boost/concept_check.hpp>
#include <boost/mpl/not.hpp>
#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/type_traits/is_base_of.hpp>

#include <vector>

namespace vtkm {
namespace cont {

namespace internal {

/// \brief Base class of all ArrayHandle classes.
///
/// This is an empty class that is used to check if something is an \c
/// ArrayHandle class (or at least something that behaves exactly like one).
/// The \c ArrayHandle template class inherits from this.
///
class ArrayHandleBase {  };

/// Checks to see if the given type and storage can form a valid array handle
/// (some storage objects cannot support all types). This check is compatable
/// with the Boost meta-template programming library (MPL). It contains a
/// typedef named type that is either boost::mpl::true_ or boost::mpl::false_.
/// Both of these have a typedef named value with the respective boolean value.
///
template<typename T, typename StorageTag>
struct IsValidArrayHandle {
  typedef typename boost::mpl::not_<
    typename boost::is_base_of<
      vtkm::cont::internal::UndefinedStorage,
      vtkm::cont::internal::Storage<T,StorageTag>
      >::type
    >::type type;
};

/// Checks to see if the given object is an array handle. This check is
/// compatiable with the Boost meta-template programming library (MPL). It
/// contains a typedef named type that is eitehr boost::mpl::true_ or
/// boost::mpl::false_. Both of these have a typedef named value with the
/// respective boolean value.
///
/// Unlike \c IsValidArrayHandle, if an \c ArrayHandle is used with this
/// class, then it must be created by the compiler and therefore must already
/// be valid. Where \c IsValidArrayHandle is used when you know something is
/// an \c ArrayHandle but you are not sure if the \c StorageTag is valid, this
/// class is used to ensure that a given type is an \c ArrayHandle. It is
/// used internally in the VTKM_IS_ARRAY_HANDLE macro.
///
template<typename T>
struct ArrayHandleCheck
{
  typedef typename boost::is_base_of<
      ::vtkm::cont::internal::ArrayHandleBase, T>::type type;
};

#define VTKM_IS_ARRAY_HANDLE(type) \
  BOOST_MPL_ASSERT(( ::vtkm::cont::internal::ArrayHandleCheck<type> ))

} // namespace internal

/// \brief Manages an array-worth of data.
///
/// \c ArrayHandle manages as array of data that can be manipulated by VTKm
/// algorithms. The \c ArrayHandle may have up to two copies of the array, one
/// for the control environment and one for the execution environment, although
/// depending on the device and how the array is being used, the \c ArrayHandle
/// will only have one copy when possible.
///
/// An ArrayHandle can be constructed one of two ways. Its default construction
/// creates an empty, unallocated array that can later be allocated and filled
/// either by the user or a VTKm algorithm. The \c ArrayHandle can also be
/// constructed with iterators to a user's array. In this case the \c
/// ArrayHandle will keep a reference to this array but may drop it if the
/// array is reallocated.
///
/// \c ArrayHandle behaves like a shared smart pointer in that when it is copied
/// each copy holds a reference to the same array.  These copies are reference
/// counted so that when all copies of the \c ArrayHandle are destroyed, any
/// allocated memory is released.
///
///
template<
    typename T,
    typename StorageTag_ = VTKM_DEFAULT_STORAGE_TAG>
class ArrayHandle : public internal::ArrayHandleBase
{
private:
  typedef vtkm::cont::internal::Storage<T,StorageTag_> StorageType;
  typedef vtkm::cont::internal::ArrayHandleExecutionManagerBase<T,StorageTag_>
      ExecutionManagerType;
public:
  typedef T ValueType;
  typedef StorageTag_ StorageTag;
  typedef typename StorageType::PortalType PortalControl;
  typedef typename StorageType::PortalConstType PortalConstControl;
  template <typename DeviceAdapterTag>
  struct ExecutionTypes
  {
    typedef typename ExecutionManagerType
        ::template ExecutionTypes<DeviceAdapterTag>::Portal Portal;
    typedef typename ExecutionManagerType
        ::template ExecutionTypes<DeviceAdapterTag>::PortalConst PortalConst;
  };

  /// Constructs an empty ArrayHandle. Typically used for output or
  /// intermediate arrays that will be filled by a VTKm algorithm.
  ///
  VTKM_CONT_EXPORT ArrayHandle() : Internals(new InternalStruct)
  {
    this->Internals->ControlArrayValid = false;
    this->Internals->ExecutionArrayValid = false;
  }

  /// Special constructor for subclass specializations that need to set the
  /// initial state of the control array. When this constructor is used, it
  /// is assumed that the control array is valid.
  ///
  ArrayHandle(const StorageType &storage)
    : Internals(new InternalStruct)
  {
    this->Internals->ControlArray = storage;
    this->Internals->ControlArrayValid = true;
    this->Internals->ExecutionArrayValid = false;
  }

  /// Get the array portal of the control array.
  ///
  VTKM_CONT_EXPORT PortalControl GetPortalControl()
  {
    this->SyncControlArray();
    if (this->Internals->ControlArrayValid)
    {
      // If the user writes into the iterator we return, then the execution
      // array will become invalid. Play it safe and release the execution
      // resources. (Use the const version to preserve the execution array.)
      this->ReleaseResourcesExecutionInternal();
      return this->Internals->ControlArray.GetPortal();
    }
    else
    {
      throw vtkm::cont::ErrorControlInternal(
            "ArrayHandle::SyncControlArray did not make control array valid.");
    }
  }

  /// Get the array portal of the control array.
  ///
  VTKM_CONT_EXPORT PortalConstControl GetPortalConstControl() const
  {
    this->SyncControlArray();
    if (this->Internals->ControlArrayValid)
    {
      return this->Internals->ControlArray.GetPortalConst();
    }
    else
    {
      throw vtkm::cont::ErrorControlInternal(
            "ArrayHandle::SyncControlArray did not make control array valid.");
    }
  }

  /// Returns the number of entries in the array.
  ///
  VTKM_CONT_EXPORT vtkm::Id GetNumberOfValues() const
  {
    if (this->Internals->ControlArrayValid)
    {
      return this->Internals->ControlArray.GetNumberOfValues();
    }
    else if (this->Internals->ExecutionArrayValid)
    {
      return this->Internals->ExecutionArray->GetNumberOfValues();
    }
    else
    {
      return 0;
    }
  }

  /// \brief Allocates an array large enough to hold the given number of values.
  ///
  /// The allocation may be done on an already existing array, but can wipe out
  /// any data already in the array. This method can throw
  /// ErrorControlOutOfMemory if the array cannot be allocated or
  /// ErrorControlBadValue if the allocation is not feasible (for example, the
  /// array storage is read-only).
  ///
  VTKM_CONT_EXPORT
  void Allocate(vtkm::Id numberOfValues)
  {
    this->ReleaseResourcesExecutionInternal();
    this->Internals->ControlArray.Allocate(numberOfValues);
    this->Internals->ControlArrayValid = true;
  }

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  void Shrink(vtkm::Id numberOfValues)
  {
    vtkm::Id originalNumberOfValues = this->GetNumberOfValues();

    if (numberOfValues < originalNumberOfValues)
    {
      if (this->Internals->ControlArrayValid)
      {
        this->Internals->ControlArray.Shrink(numberOfValues);
      }
      if (this->Internals->ExecutionArrayValid)
      {
        this->Internals->ExecutionArray->Shrink(numberOfValues);
      }
    }
    else if (numberOfValues == originalNumberOfValues)
    {
      // Nothing to do.
    }
    else // numberOfValues > originalNumberOfValues
    {
      throw vtkm::cont::ErrorControlBadValue(
        "ArrayHandle::Shrink cannot be used to grow array.");
    }

    VTKM_ASSERT_CONT(this->GetNumberOfValues() == numberOfValues);
  }

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  VTKM_CONT_EXPORT void ReleaseResourcesExecution()
  {
    // Save any data in the execution environment by making sure it is synced
    // with the control environment.
    this->SyncControlArray();

    this->ReleaseResourcesExecutionInternal();
  }

  /// Releases all resources in both the control and execution environments.
  ///
  VTKM_CONT_EXPORT void ReleaseResources()
  {
    this->ReleaseResourcesExecutionInternal();

    if (this->Internals->ControlArrayValid)
    {
      this->Internals->ControlArray.ReleaseResources();
      this->Internals->ControlArrayValid = false;
    }
  }

  /// Prepares this array to be used as an input to an operation in the
  /// execution environment. If necessary, copies data to the execution
  /// environment. Can throw an exception if this array does not yet contain
  /// any data. Returns a portal that can be used in code running in the
  /// execution environment.
  ///
  template<typename DeviceAdapterTag>
  VTKM_CONT_EXPORT
  typename ExecutionTypes<DeviceAdapterTag>::PortalConst
  PrepareForInput(DeviceAdapterTag) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

    if (!this->Internals->ControlArrayValid
        && !this->Internals->ExecutionArrayValid)
    {
      throw vtkm::cont::ErrorControlBadValue(
        "ArrayHandle has no data when PrepareForInput called.");
    }

    this->PrepareForDevice(DeviceAdapterTag());
    typename ExecutionTypes<DeviceAdapterTag>::PortalConst portal =
        this->Internals->ExecutionArray->PrepareForInput(
          !this->Internals->ExecutionArrayValid, DeviceAdapterTag());

    this->Internals->ExecutionArrayValid = true;

    return portal;
  }

  /// Prepares (allocates) this array to be used as an output from an operation
  /// in the execution environment. The internal state of this class is set to
  /// have valid data in the execution array with the assumption that the array
  /// will be filled soon (i.e. before any other methods of this object are
  /// called). Returns a portal that can be used in code running in the
  /// execution environment.
  ///
  template<typename DeviceAdapterTag>
  VTKM_CONT_EXPORT
  typename ExecutionTypes<DeviceAdapterTag>::Portal
  PrepareForOutput(vtkm::Id numberOfValues, DeviceAdapterTag)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

    // Invalidate any control arrays.
    // Should the control array resource be released? Probably not a good
    // idea when shared with execution.
    this->Internals->ControlArrayValid = false;

    this->PrepareForDevice(DeviceAdapterTag());
    typename ExecutionTypes<DeviceAdapterTag>::Portal portal =
        this->Internals->ExecutionArray->PrepareForOutput(numberOfValues,
                                                          DeviceAdapterTag());

    // We are assuming that the calling code will fill the array using the
    // iterators we are returning, so go ahead and mark the execution array as
    // having valid data. (A previous version of this class had a separate call
    // to mark the array as filled, but that was onerous to call at the the
    // right time and rather pointless since it is basically always the case
    // that the array is going to be filled before anything else. In this
    // implementation the only access to the array is through the iterators
    // returned from this method, so you would have to work to invalidate this
    // assumption anyway.)
    this->Internals->ExecutionArrayValid = true;

    return portal;
  }

  /// Prepares this array to be used in an in-place operation (both as input
  /// and output) in the execution environment. If necessary, copies data to
  /// the execution environment. Can throw an exception if this array does not
  /// yet contain any data. Returns a portal that can be used in code running
  /// in the execution environment.
  ///
  template<typename DeviceAdapterTag>
  VTKM_CONT_EXPORT
  typename ExecutionTypes<DeviceAdapterTag>::Portal
  PrepareForInPlace(DeviceAdapterTag)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

    if (!this->Internals->ControlArrayValid
        && !this->Internals->ExecutionArrayValid)
    {
      throw vtkm::cont::ErrorControlBadValue(
        "ArrayHandle has no data when PrepareForInput called.");
    }

    this->PrepareForDevice(DeviceAdapterTag());
    typename ExecutionTypes<DeviceAdapterTag>::Portal portal =
        this->Internals->ExecutionArray->PrepareForInPlace(
          !this->Internals->ExecutionArrayValid, DeviceAdapterTag());

    this->Internals->ExecutionArrayValid = true;

    // Invalidate any control arrays since their data will become invalid when
    // the execution data is overwritten. Don't actually release the control
    // array. It may be shared as the execution array.
    this->Internals->ControlArrayValid = false;

    return portal;
  }

// private:
  struct InternalStruct
  {
    StorageType ControlArray;
    bool ControlArrayValid;

    boost::scoped_ptr<
      vtkm::cont::internal::ArrayHandleExecutionManagerBase<
        ValueType,StorageTag> > ExecutionArray;
    bool ExecutionArrayValid;
  };

  ArrayHandle(boost::shared_ptr<InternalStruct> i)
    : Internals(i)
  { }

  /// Gets this array handle ready to interact with the given device. If the
  /// array handle has already interacted with this device, then this method
  /// does nothing. Although the internal state of this class can change, the
  /// method is declared const because logically the data does not.
  ///
  template<typename DeviceAdapterTag>
  VTKM_CONT_EXPORT
  void PrepareForDevice(DeviceAdapterTag) const
  {
    if (this->Internals->ExecutionArray != NULL)
    {
      if (this->Internals->ExecutionArray->IsDeviceAdapter(DeviceAdapterTag()))
      {
        // Already have manager for correct device adapter. Nothing to do.
        return;
      }
      else
      {
        // Have the wrong manager. Delete the old one and create a new one
        // of the right type. (BTW, it would be possible for the array handle
        // to hold references to execution arrays on multiple devices. However,
        // there is not a clear use case for that yet and it is unclear what
        // the behavior of "dirty" arrays should be, so it is not currently
        // implemented.)
        this->SyncControlArray();
        // Need to change some state that does not change the logical state from
        // an external point of view.
        InternalStruct *internals
            = const_cast<InternalStruct*>(this->Internals.get());
        internals->ExecutionArray.reset();
        internals->ExecutionArrayValid = false;
        }
      }

    VTKM_ASSERT_CONT(this->Internals->ExecutionArray == NULL);
    VTKM_ASSERT_CONT(!this->Internals->ExecutionArrayValid);
    // Need to change some state that does not change the logical state from
    // an external point of view.
    InternalStruct *internals
        = const_cast<InternalStruct*>(this->Internals.get());
    internals->ExecutionArray.reset(
          new vtkm::cont::internal::ArrayHandleExecutionManager<
            T, StorageTag, DeviceAdapterTag>(&internals->ControlArray));
  }

  /// Synchronizes the control array with the execution array. If either the
  /// user array or control array is already valid, this method does nothing
  /// (because the data is already available in the control environment).
  /// Although the internal state of this class can change, the method is
  /// declared const because logically the data does not.
  ///
  VTKM_CONT_EXPORT void SyncControlArray() const
  {
    if (!this->Internals->ControlArrayValid)
    {
      // Need to change some state that does not change the logical state from
      // an external point of view.
      InternalStruct *internals
        = const_cast<InternalStruct*>(this->Internals.get());
      if (this->Internals->ExecutionArrayValid)
      {
        internals->ExecutionArray->RetrieveOutputData(&internals->ControlArray);
        internals->ControlArrayValid = true;
      }
      else
      {
        // This array is in the null state (there is nothing allocated), but
        // the calling function wants to do something with the array. Put this
        // class into a valid state by allocating an array of size 0.
        internals->ControlArray.Allocate(0);
        internals->ControlArrayValid = true;
      }
    }
  }

  VTKM_CONT_EXPORT
  void ReleaseResourcesExecutionInternal()
  {
    if (this->Internals->ExecutionArrayValid)
    {
      this->Internals->ExecutionArray->ReleaseResources();
      this->Internals->ExecutionArrayValid = false;
    }
  }

  boost::shared_ptr<InternalStruct> Internals;
};

/// A convenience function for creating an ArrayHandle from a standard C array.
///
template<typename T>
VTKM_CONT_EXPORT
vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>
make_ArrayHandle(const T *array,
                 vtkm::Id length)
{
  typedef vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>
      ArrayHandleType;
  typedef vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagBasic>
      StorageType;
  return ArrayHandleType(StorageType(array, length));
}

/// A convenience function for creating an ArrayHandle from an std::vector.
///
template<typename T,
         typename Allocator>
VTKM_CONT_EXPORT
vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>
make_ArrayHandle(const std::vector<T,Allocator> &array)
{
  return make_ArrayHandle(&array.front(), static_cast<vtkm::Id>(array.size()));
}

template<typename T>
VTKM_CONT_EXPORT
void
printSummary_ArrayHandle(const vtkm::cont::ArrayHandle<T> &array,
			 std::ostream &out)
{
    vtkm::Id sz = array.GetNumberOfValues();
    out<<"sz= "<<sz<<" [";
    if (sz <= 5)
	for (vtkm::Id i = 0 ; i < sz; i++)
	{
	    out<<array.GetPortalConstControl().Get(i);
	    if (i != (sz-1)) out<<" ";
	}
    else
    {
	out<<array.GetPortalConstControl().Get(0)<<" "<<array.GetPortalConstControl().Get(1);
	out<<" ... ";
	out<<array.GetPortalConstControl().Get(sz-2)<<" "<<array.GetPortalConstControl().Get(sz-1);
    }
    out<<"]";
}

}
}

#endif //vtk_m_cont_ArrayHandle_h
