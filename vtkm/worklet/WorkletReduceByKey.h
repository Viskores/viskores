//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_WorkletReduceByKey_h
#define vtk_m_worklet_WorkletReduceByKey_h

#include <vtkm/worklet/internal/WorkletBase.h>

#include <vtkm/cont/arg/TransportTagArrayIn.h>
#include <vtkm/cont/arg/TransportTagArrayInOut.h>
#include <vtkm/cont/arg/TransportTagArrayOut.h>
#include <vtkm/cont/arg/TransportTagKeyedValuesIn.h>
#include <vtkm/cont/arg/TransportTagKeyedValuesInOut.h>
#include <vtkm/cont/arg/TransportTagKeyedValuesOut.h>
#include <vtkm/cont/arg/TransportTagKeysIn.h>
#include <vtkm/cont/arg/TypeCheckTagArrayIn.h>
#include <vtkm/cont/arg/TypeCheckTagArrayInOut.h>
#include <vtkm/cont/arg/TypeCheckTagArrayOut.h>
#include <vtkm/cont/arg/TypeCheckTagKeys.h>

#include <vtkm/exec/internal/ReduceByKeyLookup.h>

#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <vtkm/exec/arg/FetchTagArrayDirectInOut.h>
#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>
#include <vtkm/exec/arg/FetchTagKeysIn.h>
#include <vtkm/exec/arg/ThreadIndicesReduceByKey.h>
#include <vtkm/exec/arg/ValueCount.h>
#include <vtkm/worklet/DispatcherReduceByKey.h>

namespace vtkm
{
namespace worklet
{

/// @brief Base class for worklets that group elements by keys.
///
/// The `InputDomain` of this worklet is a `vtkm::worklet::Keys` object,
/// which holds an array of keys. All entries of this array with the same
/// key are collected together, and the operator of the worklet is called
/// once for each unique key.
///
/// Input arrays are (typically) the same size as the number of keys. When
/// these objects are passed to the operator of the worklet, all values of
/// the associated key are placed in a Vec-like object. Output arrays get
/// sized by the number of unique keys, and each call to the operator produces
/// one result for each output.
class WorkletReduceByKey : public vtkm::worklet::internal::WorkletBase
{
public:
  template <typename Worklet>
  using Dispatcher = vtkm::worklet::DispatcherReduceByKey<Worklet>;

  /// @defgroup WorkletReduceByKeyControlSigTags `ControlSignature` tags
  /// Tags that can be used in the `ControlSignature` of a `WorkletMapField`.
  /// @{

  /// @brief A control signature tag for input keys.
  ///
  /// A `WorkletReduceByKey` operates by collecting all identical keys and
  /// then executing the worklet on each unique key. This tag specifies a
  /// `vtkm::worklet::Keys` object that defines and manages these keys.
  ///
  /// A `WorkletReduceByKey` should have exactly one `KeysIn` tag in its
  /// `ControlSignature`, and the `InputDomain` should point to it.
  ///
  struct KeysIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagKeys;
    using TransportTag = vtkm::cont::arg::TransportTagKeysIn;
    using FetchTag = vtkm::exec::arg::FetchTagKeysIn;
  };

  /// @brief A control signature tag for input values associated with the keys.
  ///
  /// A `WorkletReduceByKey` operates by collecting all values associated with
  /// identical keys and then giving the worklet a Vec-like object containing
  /// all values with a matching key. This tag specifies an `vtkm::cont::ArrayHandle`
  /// object that holds the values. The number of values in this array must be equal
  /// to the size of the array used with the `KeysIn` argument.
  ///
  struct ValuesIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = vtkm::cont::arg::TransportTagKeyedValuesIn;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// @brief A control signature tag for input/output values associated with the keys.
  ///
  /// A `WorkletReduceByKey` operates by collecting all values associated with
  /// identical keys and then giving the worklet a Vec-like object containing
  /// all values with a matching key. This tag specifies an `vtkm::cont::ArrayHandle`
  /// object that holds the values. The number of values in this array must be equal
  /// to the size of the array used with the `KeysIn` argument.
  ///
  /// This tag might not work with scatter operations.
  ///
  struct ValuesInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayInOut;
    using TransportTag = vtkm::cont::arg::TransportTagKeyedValuesInOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for output values associated with the keys.
  ///
  /// This tag behaves the same as `ValuesInOut` except that the array is resized
  /// appropriately and no input values are passed to the worklet. As with
  /// `ValuesInOut`, values the worklet writes to its |Veclike| object get placed
  /// in the location of the original arrays.
  ///
  /// Use of `ValuesOut` is rare.
  ///
  /// This tag might not work with scatter operations.
  ///
  struct ValuesOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayOut;
    using TransportTag = vtkm::cont::arg::TransportTagKeyedValuesOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// @brief A control signature tag for reduced output values.
  ///
  /// A `WorkletReduceByKey` operates by collecting all identical keys and
  /// calling one instance of the worklet for those identical keys. The worklet
  /// then produces a "reduced" value per key. This tag specifies a
  /// `vtkm::cont::ArrayHandle` object that holds the values. The array is resized
  /// to be the number of unique keys, and each call of the operator sets
  /// a single value in the array
  ///
  struct ReducedValuesOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayOut;
    using TransportTag = vtkm::cont::arg::TransportTagArrayOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectOut;
  };

  /// @brief A control signature tag for reduced input values.
  ///
  /// A`WorkletReduceByKey` operates by collecting all identical keys and
  /// calling one instance of the worklet for those identical keys. The worklet
  /// then produces a "reduced" value per key.
  ///
  /// This tag specifies a `vtkm::cont::ArrayHandle` object that holds the values.
  /// It is an input array with entries for each reduced value. The number of values
  /// in the array must equal the number of _unique_ keys.
  ///
  /// A `ReducedValuesIn` argument is usually used to pass reduced values from one
  /// invoke of a reduce by key worklet to another invoke of a reduced by key worklet
  /// such as in an algorithm that requires iterative steps.
  ///
  struct ReducedValuesIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = vtkm::cont::arg::TransportTagArrayIn;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// @brief A control signature tag for reduced output values.
  ///
  /// A `WorkletReduceByKey` operates by collecting all identical keys and
  /// calling one instance of the worklet for those identical keys. The worklet
  /// then produces a "reduced" value per key.
  ///
  /// This tag specifies a `vtkm::cont::ArrayHandle` object that holds the values.
  /// It is an input/output array with entries for each reduced value. The number
  /// of values in the array must equal the number of _unique_ keys.
  ///
  /// This tag behaves the same as `ReducedValuesIn` except that the worklet may
  /// write values back into the array. Make sure that the associated parameter to
  /// the worklet operator is a reference so that the changed value gets written
  /// back to the array.
  ///
  struct ReducedValuesInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayInOut;
    using TransportTag = vtkm::cont::arg::TransportTagArrayInOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectInOut;
  };

#ifdef VTKM_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

  /// @copydoc vtkm::worklet::internal::WorkletBase::WholeArrayIn
  struct WholeArrayIn : vtkm::worklet::internal::WorkletBase::WholeArrayIn
  {
  };

  /// @copydoc vtkm::worklet::internal::WorkletBase::WholeArrayOut
  struct WholeArrayOut : vtkm::worklet::internal::WorkletBase::WholeArrayOut
  {
  };

  /// @copydoc vtkm::worklet::internal::WorkletBase::WholeArrayInOut
  struct WholeArrayInOut : vtkm::worklet::internal::WorkletBase::WholeArrayInOut
  {
  };

  /// @copydoc vtkm::worklet::internal::WorkletBase::AtomicArrayInOut
  struct AtomicArrayInOut : vtkm::worklet::internal::WorkletBase::AtomicArrayInOut
  {
  };

  /// @copydoc vtkm::worklet::internal::WorkletBase::WholeCellSetIn
  template <typename VisitTopology = Cell, typename IncidentTopology = Point>
  struct WholeCellSetIn
    : vtkm::worklet::internal::WorkletBase::WholeCellSetIn<VisitTopology, IncidentTopology>
  {
  };

  /// @copydoc vtkm::worklet::internal::WorkletBase::ExecObject
  struct ExecObject : vtkm::worklet::internal::WorkletBase::ExecObject
  {
  };
#endif

  /// @}

  /// @defgroup WorkletReduceByKeyExecutionSigTags `ExecutionSignature` tags
  /// Tags that can be used in the `ExecutionSignature` of a `WorkletMapField`.
  /// @{

#ifdef VTKM_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

  /// @copydoc vtkm::placeholders::Arg
  struct _1 : vtkm::worklet::internal::WorkletBase::_1
  {
  };
#endif

  /// @brief The `ExecutionSignature` tag to get the number of values.
  ///
  /// A `WorkletReduceByKey` operates by collecting all values associated with
  /// identical keys and then giving the worklet a Vec-like object containing all
  /// values with a matching key. This tag produces a `vtkm::IdComponent` that is
  /// equal to the number of times the key associated with this call to the worklet
  /// occurs in the input. This is the same size as the Vec-like objects provided
  /// by `ValuesIn` arguments.
  ///
  struct ValueCount : vtkm::exec::arg::ValueCount
  {
  };

#ifdef VTKM_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

  /// @copydoc vtkm::exec::arg::WorkIndex
  struct WorkIndex : vtkm::worklet::internal::WorkletBase::WorkIndex
  {
  };

  /// @copydoc vtkm::exec::arg::VisitIndex
  struct VisitIndex : vtkm::worklet::internal::WorkletBase::VisitIndex
  {
  };

  /// @copydoc vtkm::exec::arg::InputIndex
  struct InputIndex : vtkm::worklet::internal::WorkletBase::InputIndex
  {
  };

  /// @copydoc vtkm::exec::arg::OutputIndex
  struct OutputIndex : vtkm::worklet::internal::WorkletBase::OutputIndex
  {
  };

  /// @copydoc vtkm::exec::arg::ThreadIndices
  struct ThreadIndices : vtkm::worklet::internal::WorkletBase::ThreadIndices
  {
  };

  /// @copydoc vtkm::worklet::internal::WorkletBase::Device
  struct Device : vtkm::worklet::internal::WorkletBase::Device
  {
  };
#endif

  /// @}

  /// Reduce by key worklets use the related thread indices class.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesReduceByKey GetThreadIndices(
    vtkm::Id threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType& inputDomain) const
  {
    const vtkm::Id outIndex = threadToOut.Get(threadIndex);
    return vtkm::exec::arg::ThreadIndicesReduceByKey(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, inputDomain);
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_WorkletReduceByKey_h
