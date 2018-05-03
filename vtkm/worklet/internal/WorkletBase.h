//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_internal_WorkletBase_h
#define vtk_m_worklet_internal_WorkletBase_h

#include <vtkm/TopologyElementTag.h>
#include <vtkm/TypeListTag.h>

#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/arg/BasicArg.h>
#include <vtkm/exec/arg/FetchTagExecObject.h>
#include <vtkm/exec/arg/FetchTagWholeCellSetIn.h>
#include <vtkm/exec/arg/InputIndex.h>
#include <vtkm/exec/arg/OutputIndex.h>
#include <vtkm/exec/arg/ThreadIndices.h>
#include <vtkm/exec/arg/ThreadIndicesBasic.h>
#include <vtkm/exec/arg/VisitIndex.h>
#include <vtkm/exec/arg/WorkIndex.h>

#include <vtkm/cont/arg/ControlSignatureTagBase.h>
#include <vtkm/cont/arg/TransportTagAtomicArray.h>
#include <vtkm/cont/arg/TransportTagCellSetIn.h>
#include <vtkm/cont/arg/TransportTagExecObject.h>
#include <vtkm/cont/arg/TransportTagWholeArrayIn.h>
#include <vtkm/cont/arg/TransportTagWholeArrayInOut.h>
#include <vtkm/cont/arg/TransportTagWholeArrayOut.h>
#include <vtkm/cont/arg/TypeCheckTagArray.h>
#include <vtkm/cont/arg/TypeCheckTagAtomicArray.h>
#include <vtkm/cont/arg/TypeCheckTagCellSet.h>
#include <vtkm/cont/arg/TypeCheckTagExecObject.h>

#include <vtkm/worklet/ScatterIdentity.h>

namespace vtkm
{
namespace placeholders
{

template <int ControlSignatureIndex>
struct Arg : vtkm::exec::arg::BasicArg<ControlSignatureIndex>
{
};

/// Basic execution argument tags
struct _1 : Arg<1>
{
};
struct _2 : Arg<2>
{
};
struct _3 : Arg<3>
{
};
struct _4 : Arg<4>
{
};
struct _5 : Arg<5>
{
};
struct _6 : Arg<6>
{
};
struct _7 : Arg<7>
{
};
struct _8 : Arg<8>
{
};
struct _9 : Arg<9>
{
};
struct _10 : Arg<10>
{
};
struct _11 : Arg<11>
{
};
struct _12 : Arg<12>
{
};
struct _13 : Arg<13>
{
};
struct _14 : Arg<14>
{
};
struct _15 : Arg<15>
{
};
struct _16 : Arg<16>
{
};
struct _17 : Arg<17>
{
};
struct _18 : Arg<18>
{
};
struct _19 : Arg<19>
{
};
struct _20 : Arg<20>
{
};
}

namespace worklet
{
namespace internal
{

/// Base class for all worklet classes. Worklet classes are subclasses and a
/// operator() const is added to implement an algorithm in VTK-m. Different
/// worklets have different calling semantics.
///
class WorkletBase : public vtkm::exec::FunctorBase
{
public:
  using _1 = vtkm::placeholders::_1;
  using _2 = vtkm::placeholders::_2;
  using _3 = vtkm::placeholders::_3;
  using _4 = vtkm::placeholders::_4;
  using _5 = vtkm::placeholders::_5;
  using _6 = vtkm::placeholders::_6;
  using _7 = vtkm::placeholders::_7;
  using _8 = vtkm::placeholders::_8;
  using _9 = vtkm::placeholders::_9;
  using _10 = vtkm::placeholders::_10;
  using _11 = vtkm::placeholders::_11;
  using _12 = vtkm::placeholders::_12;
  using _13 = vtkm::placeholders::_13;
  using _14 = vtkm::placeholders::_14;
  using _15 = vtkm::placeholders::_15;
  using _16 = vtkm::placeholders::_16;
  using _17 = vtkm::placeholders::_17;
  using _18 = vtkm::placeholders::_18;
  using _19 = vtkm::placeholders::_19;
  using _20 = vtkm::placeholders::_20;

  /// \c ExecutionSignature tag for getting the work index.
  ///
  using WorkIndex = vtkm::exec::arg::WorkIndex;

  /// \c ExecutionSignature tag for getting the input index.
  ///
  using InputIndex = vtkm::exec::arg::InputIndex;

  /// \c ExecutionSignature tag for getting the output index.
  ///
  using OutputIndex = vtkm::exec::arg::OutputIndex;

  /// \c ExecutionSignature tag for getting the thread indices.
  ///
  using ThreadIndices = vtkm::exec::arg::ThreadIndices;

  /// \c ExecutionSignature tag for getting the visit index.
  ///
  using VisitIndex = vtkm::exec::arg::VisitIndex;

  /// \c ControlSignature tag for execution object inputs.
  struct ExecObject : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagExecObject;
    using TransportTag = vtkm::cont::arg::TransportTagExecObject;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };

  /// Default input domain is the first argument. Worklet subclasses can
  /// override this by redefining this type.
  using InputDomain = _1;

  /// All worklets must define their scatter operation. The scatter defines
  /// what output each input contributes to. The default scatter is the
  /// identity scatter (1-to-1 input to output).
  using ScatterType = vtkm::worklet::ScatterIdentity;

  /// \brief A type list containing the type vtkm::Id.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using IdType = vtkm::TypeListTagId;

  /// \brief A type list containing the type vtkm::Id2.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using Id2Type = vtkm::TypeListTagId2;

  /// \brief A type list containing the type vtkm::Id3.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using Id3Type = vtkm::TypeListTagId3;

  /// \brief A type list containing the type vtkm::IdComponent.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using IdComponentType = vtkm::TypeListTagIdComponent;

  /// \brief A list of types commonly used for indexing.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using Index = vtkm::TypeListTagIndex;

  /// \brief A list of types commonly used for scalar fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using Scalar = vtkm::TypeListTagFieldScalar;

  /// \brief A list of all basic types used for scalar fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using ScalarAll = vtkm::TypeListTagScalarAll;

  /// \brief A list of types commonly used for vector fields of 2 components.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using Vec2 = vtkm::TypeListTagFieldVec2;

  /// \brief A list of types commonly used for vector fields of 3 components.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using Vec3 = vtkm::TypeListTagFieldVec3;

  /// \brief A list of types commonly used for vector fields of 4 components.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using Vec4 = vtkm::TypeListTagFieldVec4;

  /// \brief A list of all basic types used for vector fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using VecAll = vtkm::TypeListTagVecAll;

  /// \brief A list of types (scalar and vector) commonly used in fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using FieldCommon = vtkm::TypeListTagField;

  /// \brief A list of vector types commonly used in fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using VecCommon = vtkm::TypeListTagVecCommon;

  /// \brief A list of generally common types.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using CommonTypes = vtkm::TypeListTagCommon;

  /// \brief A list of all basic types.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  using AllTypes = vtkm::TypeListTagAll;

  /// \c ControlSignature tag for whole input arrays.
  ///
  /// The \c WholeArrayIn control signature tag specifies an \c ArrayHandle
  /// passed to the \c Invoke operation of the dispatcher. This is converted
  /// to an \c ArrayPortal object and passed to the appropriate worklet
  /// operator argument with one of the default args.
  ///
  /// The template operator specifies all the potential value types of the
  /// array. The default value type is all types.
  ///
  template <typename TypeList = AllTypes>
  struct WholeArrayIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
    using TransportTag = vtkm::cont::arg::TransportTagWholeArrayIn;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };

  /// \c ControlSignature tag for whole output arrays.
  ///
  /// The \c WholeArrayOut control signature tag specifies an \c ArrayHandle
  /// passed to the \c Invoke operation of the dispatcher. This is converted to
  /// an \c ArrayPortal object and passed to the appropriate worklet operator
  /// argument with one of the default args. Care should be taken to not write
  /// a value in one instance that will be overridden by another entry.
  ///
  /// The template operator specifies all the potential value types of the
  /// array. The default value type is all types.
  ///
  template <typename TypeList = AllTypes>
  struct WholeArrayOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
    using TransportTag = vtkm::cont::arg::TransportTagWholeArrayOut;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };

  /// \c ControlSignature tag for whole input/output arrays.
  ///
  /// The \c WholeArrayOut control signature tag specifies an \c ArrayHandle
  /// passed to the \c Invoke operation of the dispatcher. This is converted to
  /// an \c ArrayPortal object and passed to the appropriate worklet operator
  /// argument with one of the default args. Care should be taken to not write
  /// a value in one instance that will be read by or overridden by another
  /// entry.
  ///
  /// The template operator specifies all the potential value types of the
  /// array. The default value type is all types.
  ///
  template <typename TypeList = AllTypes>
  struct WholeArrayInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
    using TransportTag = vtkm::cont::arg::TransportTagWholeArrayInOut;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };

  /// \c ControlSignature tag for whole input/output arrays.
  ///
  /// The \c AtomicArrayInOut control signature tag specifies an \c ArrayHandle
  /// passed to the \c Invoke operation of the dispatcher. This is converted to
  /// a \c vtkm::exec::AtomicArray object and passed to the appropriate worklet
  /// operator argument with one of the default args. The provided atomic
  /// operations can be used to resolve concurrency hazards, but have the
  /// potential to slow the program quite a bit.
  ///
  /// The template operator specifies all the potential value types of the
  /// array. The default value type is all types.
  ///
  template <typename TypeList = AllTypes>
  struct AtomicArrayInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagAtomicArray<TypeList>;
    using TransportTag = vtkm::cont::arg::TransportTagAtomicArray;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };

  /// \c ControlSignature tag for whole input topology.
  ///
  /// The \c WholeCellSetIn control signature tag specifies an \c CellSet
  /// passed to the \c Invoke operation of the dispatcher. This is converted to
  /// a \c vtkm::exec::Connectivity* object and passed to the appropriate worklet
  /// operator argument with one of the default args. This can be used to
  /// global lookup for arbitrary topology information

  using Cell = vtkm::TopologyElementTagCell;
  using Point = vtkm::TopologyElementTagPoint;
  template <typename FromType = Point, typename ToType = Cell>
  struct WholeCellSetIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagCellSet;
    using TransportTag = vtkm::cont::arg::TransportTagCellSetIn<FromType, ToType>;
    using FetchTag = vtkm::exec::arg::FetchTagWholeCellSetIn;
  };

  /// \brief Creates a \c ThreadIndices object.
  ///
  /// Worklet types can add additional indices by returning different object
  /// types.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T,
            typename OutToInArrayType,
            typename VisitArrayType,
            typename InputDomainType>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const T& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const InputDomainType&,
    const T& globalThreadIndexOffset = 0) const
  {
    return vtkm::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(threadIndex), visit.Get(threadIndex), globalThreadIndexOffset);
  }
};
}
}
} // namespace vtkm::worklet::internal

#endif //vtk_m_worklet_internal_WorkletBase_h
