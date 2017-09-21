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
  typedef vtkm::placeholders::_1 _1;
  typedef vtkm::placeholders::_2 _2;
  typedef vtkm::placeholders::_3 _3;
  typedef vtkm::placeholders::_4 _4;
  typedef vtkm::placeholders::_5 _5;
  typedef vtkm::placeholders::_6 _6;
  typedef vtkm::placeholders::_7 _7;
  typedef vtkm::placeholders::_8 _8;
  typedef vtkm::placeholders::_9 _9;

  /// \c ExecutionSignature tag for getting the work index.
  ///
  typedef vtkm::exec::arg::WorkIndex WorkIndex;

  /// \c ExecutionSignature tag for getting the input index.
  ///
  typedef vtkm::exec::arg::InputIndex InputIndex;

  /// \c ExecutionSignature tag for getting the output index.
  ///
  typedef vtkm::exec::arg::OutputIndex OutputIndex;

  /// \c ExecutionSignature tag for getting the thread indices.
  ///
  typedef vtkm::exec::arg::ThreadIndices ThreadIndices;

  /// \c ExecutionSignature tag for getting the visit index.
  ///
  typedef vtkm::exec::arg::VisitIndex VisitIndex;

  /// \c ControlSignature tag for execution object inputs.
  struct ExecObject : vtkm::cont::arg::ControlSignatureTagBase
  {
    typedef vtkm::cont::arg::TypeCheckTagExecObject TypeCheckTag;
    typedef vtkm::cont::arg::TransportTagExecObject TransportTag;
    typedef vtkm::exec::arg::FetchTagExecObject FetchTag;
  };

  /// Default input domain is the first argument. Worklet subclasses can
  /// override this by redefining this type.
  typedef _1 InputDomain;

  /// All worklets must define their scatter operation. The scatter defines
  /// what output each input contributes to. The default scatter is the
  /// identity scatter (1-to-1 input to output).
  typedef vtkm::worklet::ScatterIdentity ScatterType;

  /// In addition to defining the scatter type, the worklet must produce the
  /// scatter. The default ScatterIdentity has no state, so just return an
  /// instance.
  VTKM_CONT
  ScatterType GetScatter() const { return ScatterType(); }

  /// \brief A type list containing the type vtkm::Id.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagId IdType;

  /// \brief A type list containing the type vtkm::Id2.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagId2 Id2Type;

  /// \brief A type list containing the type vtkm::Id3.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagId3 Id3Type;

  /// \brief A type list containing the type vtkm::IdComponent.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagIdComponent IdComponentType;

  /// \brief A list of types commonly used for indexing.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagIndex Index;

  /// \brief A list of types commonly used for scalar fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagFieldScalar Scalar;

  /// \brief A list of all basic types used for scalar fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagScalarAll ScalarAll;

  /// \brief A list of types commonly used for vector fields of 2 components.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagFieldVec2 Vec2;

  /// \brief A list of types commonly used for vector fields of 3 components.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagFieldVec3 Vec3;

  /// \brief A list of types commonly used for vector fields of 4 components.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagFieldVec4 Vec4;

  /// \brief A list of all basic types used for vector fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagVecAll VecAll;

  /// \brief A list of types (scalar and vector) commonly used in fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagField FieldCommon;

  /// \brief A list of vector types commonly used in fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagVecCommon VecCommon;

  /// \brief A list of generally common types.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagCommon CommonTypes;

  /// \brief A list of all basic types.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagAll AllTypes;

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
    typedef vtkm::cont::arg::TypeCheckTagArray<TypeList> TypeCheckTag;
    typedef vtkm::cont::arg::TransportTagWholeArrayIn TransportTag;
    typedef vtkm::exec::arg::FetchTagExecObject FetchTag;
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
    typedef vtkm::cont::arg::TypeCheckTagArray<TypeList> TypeCheckTag;
    typedef vtkm::cont::arg::TransportTagWholeArrayOut TransportTag;
    typedef vtkm::exec::arg::FetchTagExecObject FetchTag;
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
    typedef vtkm::cont::arg::TypeCheckTagArray<TypeList> TypeCheckTag;
    typedef vtkm::cont::arg::TransportTagWholeArrayInOut TransportTag;
    typedef vtkm::exec::arg::FetchTagExecObject FetchTag;
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
    typedef vtkm::cont::arg::TypeCheckTagAtomicArray<TypeList> TypeCheckTag;
    typedef vtkm::cont::arg::TransportTagAtomicArray TransportTag;
    typedef vtkm::exec::arg::FetchTagExecObject FetchTag;
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
    typedef vtkm::cont::arg::TypeCheckTagCellSet TypeCheckTag;
    typedef vtkm::cont::arg::TransportTagCellSetIn<FromType, ToType> TransportTag;
    typedef vtkm::exec::arg::FetchTagWholeCellSetIn FetchTag;
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
