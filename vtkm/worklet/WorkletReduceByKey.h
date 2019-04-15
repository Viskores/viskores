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
#include <vtkm/cont/arg/TypeCheckTagArray.h>
#include <vtkm/cont/arg/TypeCheckTagKeys.h>

#include <vtkm/exec/internal/ReduceByKeyLookup.h>

#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <vtkm/exec/arg/FetchTagArrayDirectInOut.h>
#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>
#include <vtkm/exec/arg/FetchTagKeysIn.h>
#include <vtkm/exec/arg/ThreadIndicesReduceByKey.h>
#include <vtkm/exec/arg/ValueCount.h>

namespace vtkm
{
namespace worklet
{

template <typename WorkletType>
class DispatcherReduceByKey;

class WorkletReduceByKey : public vtkm::worklet::internal::WorkletBase
{
public:
  template <typename Worklet>
  using Dispatcher = vtkm::worklet::DispatcherReduceByKey<Worklet>;
  /// \brief A control signature tag for input keys.
  ///
  /// A \c WorkletReduceByKey operates by collecting all identical keys and
  /// then executing the worklet on each unique key. This tag specifies a
  /// \c Keys object that defines and manages these keys.
  ///
  /// A \c WorkletReduceByKey should have exactly one \c KeysIn tag in its \c
  /// ControlSignature, and the \c InputDomain should point to it.
  ///
  struct KeysIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagKeys;
    using TransportTag = vtkm::cont::arg::TransportTagKeysIn;
    using FetchTag = vtkm::exec::arg::FetchTagKeysIn;
  };

  /// \brief A control signature tag for input values.
  ///
  /// A \c WorkletReduceByKey operates by collecting all values associated with
  /// identical keys and then giving the worklet a Vec-like object containing
  /// all values with a matching key. This tag specifies an \c ArrayHandle
  /// object that holds the values.
  ///
  struct ValuesIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagKeyedValuesIn;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for input/output values.
  ///
  /// A \c WorkletReduceByKey operates by collecting all values associated with
  /// identical keys and then giving the worklet a Vec-like object containing
  /// all values with a matching key. This tag specifies an \c ArrayHandle
  /// object that holds the values.
  ///
  /// This tag might not work with scatter operations.
  ///
  struct ValuesInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagKeyedValuesInOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for output values.
  ///
  /// A \c WorkletReduceByKey operates by collecting all values associated with
  /// identical keys and then giving the worklet a Vec-like object containing
  /// all values with a matching key. This tag specifies an \c ArrayHandle
  /// object that holds the values.
  ///
  /// This tag might not work with scatter operations.
  ///
  struct ValuesOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagKeyedValuesOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for reduced output values.
  ///
  /// A \c WorkletReduceByKey operates by collecting all identical keys and
  /// calling one instance of the worklet for those identical keys. The worklet
  /// then produces a "reduced" value per key.
  ///
  /// This tag specifies an \c ArrayHandle object that holds the values. It is
  /// an input array with entries for each reduced value. This could be useful
  /// to access values from a previous run of WorkletReduceByKey.
  ///
  struct ReducedValuesIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagArrayIn;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for reduced output values.
  ///
  /// A \c WorkletReduceByKey operates by collecting all identical keys and
  /// calling one instance of the worklet for those identical keys. The worklet
  /// then produces a "reduced" value per key.
  ///
  /// This tag specifies an \c ArrayHandle object that holds the values. It is
  /// an input/output array with entries for each reduced value. This could be
  /// useful to access values from a previous run of WorkletReduceByKey.
  ///
  struct ReducedValuesInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagArrayInOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectInOut;
  };

  /// \brief A control signature tag for reduced output values.
  ///
  /// A \c WorkletReduceByKey operates by collecting all identical keys and
  /// calling one instance of the worklet for those identical keys. The worklet
  /// then produces a "reduced" value per key. This tag specifies an \c
  /// ArrayHandle object that holds the values.
  ///
  struct ReducedValuesOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagArrayOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectOut;
  };

  /// \brief The \c ExecutionSignature tag to get the number of values.
  ///
  /// A \c WorkletReduceByKey operates by collecting all values associated with
  /// identical keys and then giving the worklet a Vec-like object containing all
  /// values with a matching key. This \c ExecutionSignature tag provides the
  /// number of values associated with the key and given in the Vec-like objects.
  ///
  struct ValueCount : vtkm::exec::arg::ValueCount
  {
  };

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
    const InputDomainType& inputDomain,
    vtkm::Id globalThreadIndexOffset = 0) const
  {
    const vtkm::Id outIndex = threadToOut.Get(threadIndex);
    return vtkm::exec::arg::ThreadIndicesReduceByKey(threadIndex,
                                                     outToIn.Get(outIndex),
                                                     visit.Get(outIndex),
                                                     outIndex,
                                                     inputDomain,
                                                     globalThreadIndexOffset);
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_WorkletReduceByKey_h
