//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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

class WorkletReduceByKey : public vtkm::worklet::internal::WorkletBase
{
public:
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
  template <typename TypeList = AllTypes>
  struct ValuesIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
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
  template <typename TypeList = AllTypes>
  struct ValuesInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
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
  template <typename TypeList = AllTypes>
  struct ValuesOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
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
  template <typename TypeList = AllTypes>
  struct ReducedValuesIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
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
  template <typename TypeList = AllTypes>
  struct ReducedValuesInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
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
  template <typename TypeList = AllTypes>
  struct ReducedValuesOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
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
  template <typename T,
            typename OutToInArrayType,
            typename VisitArrayType,
            typename InputDomainType>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesReduceByKey GetThreadIndices(
    const T& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const InputDomainType& inputDomain,
    const T& globalThreadIndexOffset = 0) const
  {
    return vtkm::exec::arg::ThreadIndicesReduceByKey(threadIndex,
                                                     outToIn.Get(threadIndex),
                                                     visit.Get(threadIndex),
                                                     inputDomain,
                                                     globalThreadIndexOffset);
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_WorkletReduceByKey_h
