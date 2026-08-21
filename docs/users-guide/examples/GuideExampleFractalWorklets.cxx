//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/TypeList.h>

#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/ArrayRangeCompute.h>

#include <viskores/cont/arg/Transport.h>
#include <viskores/cont/arg/TypeCheck.h>

#include <viskores/exec/arg/AspectTagDefault.h>
#include <viskores/exec/arg/Fetch.h>
#include <viskores/exec/arg/ThreadIndicesBasic.h>

#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

#include <fstream>
#include <type_traits>

template<typename T>
static viskores::Vec<T, 2> TransformSVGPoint(const viskores::Vec<T, 2>& point,
                                             const viskores::Range xRange,
                                             const viskores::Range yRange,
                                             float padding)
{
  return viskores::Vec<T, 2>(static_cast<T>(point[0] - xRange.Min + padding),
                             static_cast<T>(yRange.Max - point[1] + padding));
}

template<typename T>
static void WriteSVG(const std::string& filename,
                     const viskores::cont::ArrayHandle<viskores::Vec<T, 2>>& data,
                     float width = 2.0,
                     const std::string& color = "black")
{
  static const float PADDING = 0.05f;

  viskores::cont::ArrayHandle<viskores::Range> ranges =
    viskores::cont::ArrayRangeCompute(data);
  viskores::Range xRange = ranges.ReadPortal().Get(0);
  viskores::Range yRange = ranges.ReadPortal().Get(1);

  std::ofstream file(filename.c_str());

  file << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
  file << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" "
       << "width=\"" << xRange.Length() + 2 * PADDING << "in\" "
       << "height=\"" << yRange.Length() + 2 * PADDING << "in\" "
       << ">\n";

  typename viskores::cont::ArrayHandle<viskores::Vec<T, 2>>::ReadPortalType portal =
    data.ReadPortal();
  for (viskores::Id lineIndex = 0; lineIndex < portal.GetNumberOfValues() / 2;
       ++lineIndex)
  {
    viskores::Vec<T, 2> p1 =
      TransformSVGPoint(portal.Get(lineIndex * 2 + 0), xRange, yRange, PADDING);
    viskores::Vec<T, 2> p2 =
      TransformSVGPoint(portal.Get(lineIndex * 2 + 1), xRange, yRange, PADDING);

    file << "  <line x1=\"" << p1[0] << "in\" y1=\"" << p1[1] << "in\" x2=\"" << p2[0]
         << "in\" y2=\"" << p2[1] << "in\" stroke=\"" << color << "\" stroke-width=\""
         << width << "\" stroke-linecap=\"round\" />\n";
  }

  file << "</svg>\n";
  file.close();
}

////
//// BEGIN-EXAMPLE TypeCheckImpl.h
////
namespace viskores
{
namespace cont
{
namespace arg
{

struct TypeCheckTag2DCoordinates
{
};

template<typename ArrayType>
struct TypeCheck<TypeCheckTag2DCoordinates, ArrayType>
{
  static constexpr bool value = false;
};

template<typename T, typename Storage>
struct TypeCheck<TypeCheckTag2DCoordinates, viskores::cont::ArrayHandle<T, Storage>>
{
  static constexpr bool value = viskores::ListHas<viskores::TypeListFieldVec2, T>::value;
};

} // namespace arg
} // namespace cont
} // namespace viskores
////
//// END-EXAMPLE TypeCheckImpl.h
////

////
//// BEGIN-EXAMPLE TransportImpl.h
////
namespace viskores
{
namespace cont
{
namespace arg
{

struct TransportTag2DLineSegmentsIn
{
};

template<typename ContObjectType, typename Device>
struct Transport<viskores::cont::arg::TransportTag2DLineSegmentsIn,
                 ContObjectType,
                 Device>
{
  //// LABEL CheckControlObject
  VISKORES_IS_ARRAY_HANDLE(ContObjectType);

  using GroupedArrayType = viskores::cont::ArrayHandleGroupVec<ContObjectType, 2>;

  using ExecObjectType = typename GroupedArrayType::ReadPortalType;

  template<typename InputDomainType>
  VISKORES_CONT ExecObjectType operator()(const ContObjectType& object,
                                          const InputDomainType&,
                                          viskores::Id inputRange,
                                          viskores::Id,
                                          viskores::cont::Token& token) const
  {
    if (object.GetNumberOfValues() != inputRange * 2)
    {
      throw viskores::cont::ErrorBadValue(
        "2D line segment array size does not agree with input size.");
    }

    GroupedArrayType groupedArray(object);
    return groupedArray.PrepareForInput(Device{}, token);
  }
};

} // namespace arg
} // namespace cont
} // namespace viskores
////
//// END-EXAMPLE TransportImpl.h
////

////
//// BEGIN-EXAMPLE FetchImplBasic.h
////
namespace viskores
{
namespace exec
{
namespace arg
{

struct FetchTag2DLineSegmentsIn
{
};

template<typename ExecObjectType>
struct Fetch<viskores::exec::arg::FetchTag2DLineSegmentsIn,
             viskores::exec::arg::AspectTagDefault,
             ExecObjectType>
{
  using ValueType = typename ExecObjectType::ValueType;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template<typename ThreadIndicesType>
  VISKORES_EXEC ValueType Load(const ThreadIndicesType& indices,
                               const ExecObjectType& arrayPortal) const
  {
    return arrayPortal.Get(indices.GetInputIndex());
  }

  template<typename ThreadIndicesType>
  VISKORES_EXEC void Store(const ThreadIndicesType&,
                           const ExecObjectType&,
                           const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};

} // namespace arg
} // namespace exec
} // namespace viskores
////
//// END-EXAMPLE FetchImplBasic.h
////

////
//// BEGIN-EXAMPLE AspectImpl.h
////
namespace viskores
{
namespace exec
{
namespace arg
{

struct AspectTagFirstPoint
{
};

template<typename ExecObjectType>
struct Fetch<viskores::exec::arg::FetchTag2DLineSegmentsIn,
             viskores::exec::arg::AspectTagFirstPoint,
             ExecObjectType>
{
  using ValueType = typename ExecObjectType::ValueType::ComponentType;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template<typename ThreadIndicesType>
  VISKORES_EXEC ValueType Load(const ThreadIndicesType& indices,
                               const ExecObjectType& arrayPortal) const
  {
    return arrayPortal.Get(indices.GetInputIndex())[0];
  }

  template<typename ThreadIndicesType>
  VISKORES_EXEC void Store(const ThreadIndicesType&,
                           const ExecObjectType&,
                           const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};

//// PAUSE-EXAMPLE
struct AspectTagSecondPoint
{
};

template<typename ExecObjectType>
struct Fetch<viskores::exec::arg::FetchTag2DLineSegmentsIn,
             viskores::exec::arg::AspectTagSecondPoint,
             ExecObjectType>
{
  using ValueType = typename ExecObjectType::ValueType::ComponentType;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template<typename ThreadIndicesType>
  VISKORES_EXEC ValueType Load(const ThreadIndicesType& indices,
                               const ExecObjectType& arrayPortal) const
  {
    return arrayPortal.Get(indices.GetInputIndex())[1];
  }

  template<typename ThreadIndicesType>
  VISKORES_EXEC void Store(const ThreadIndicesType&,
                           const ExecObjectType&,
                           const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};

//// RESUME-EXAMPLE
} // namespace arg
} // namespace exec
} // namespace viskores
////
//// END-EXAMPLE AspectImpl.h
////

struct VecLineSegments : viskores::worklet::WorkletMapField
{
  ////
  //// BEGIN-EXAMPLE CustomControlSignatureTag
  ////
  struct LineSegment2DCoordinatesIn : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTag2DCoordinates;
    using TransportTag = viskores::cont::arg::TransportTag2DLineSegmentsIn;
    using FetchTag = viskores::exec::arg::FetchTag2DLineSegmentsIn;
  };
  ////
  //// END-EXAMPLE CustomControlSignatureTag
  ////

  ////
  //// BEGIN-EXAMPLE CustomExecutionSignatureTag
  ////
  template<typename ArgTag>
  struct FirstPoint : viskores::exec::arg::ExecutionSignatureTagBase
  {
    static const viskores::IdComponent INDEX = ArgTag::INDEX;
    using AspectTag = viskores::exec::arg::AspectTagFirstPoint;
  };
  ////
  //// END-EXAMPLE CustomExecutionSignatureTag
  ////

  template<typename ArgTag>
  struct SecondPoint : viskores::exec::arg::ExecutionSignatureTagBase
  {
    static const viskores::IdComponent INDEX = ArgTag::INDEX;
    using AspectTag = viskores::exec::arg::AspectTagSecondPoint;
  };

  ////
  //// BEGIN-EXAMPLE UseCustomControlSignatureTag
  ////
  ////
  //// BEGIN-EXAMPLE UseCustomExecutionSignatureTag
  ////
  using ControlSignature = void(LineSegment2DCoordinatesIn coordsIn,
                                FieldOut vecOut,
                                FieldIn index);
  ////
  //// END-EXAMPLE UseCustomControlSignatureTag
  ////
  using ExecutionSignature = void(FirstPoint<_1>, SecondPoint<_1>, _2);
  ////
  //// END-EXAMPLE UseCustomExecutionSignatureTag
  ////
  using InputDomain = _3;

  template<typename T>
  VISKORES_EXEC void operator()(const viskores::Vec<T, 2>& firstPoint,
                                const viskores::Vec<T, 2>& secondPoint,
                                viskores::Vec<T, 2>& vecOut) const
  {
    vecOut = secondPoint - firstPoint;
  }
};

void TryVecLineSegments()
{
  static const viskores::Id ARRAY_SIZE = 10;

  viskores::cont::ArrayHandle<viskores::Vec2f> inputArray;
  inputArray.Allocate(ARRAY_SIZE * 2);
  SetPortal(inputArray.WritePortal());

  viskores::cont::ArrayHandle<viskores::Vec2f> outputArray;

  viskores::cont::Invoker invoke;
  invoke(VecLineSegments{},
         inputArray,
         outputArray,
         viskores::cont::ArrayHandleIndex(ARRAY_SIZE));

  VISKORES_TEST_ASSERT(outputArray.GetNumberOfValues() == ARRAY_SIZE,
                       "Output wrong size.");

  for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    viskores::Vec2f expectedVec = TestValue(index * 2 + 1, viskores::Vec2f()) -
      TestValue(index * 2, viskores::Vec2f());
    viskores::Vec2f computedVec = outputArray.ReadPortal().Get(index);
    VISKORES_TEST_ASSERT(test_equal(expectedVec, computedVec), "Bad value.");
  }
}

////
//// BEGIN-EXAMPLE TransportImpl2.h
////
namespace viskores
{
namespace cont
{
namespace arg
{

template<viskores::IdComponent NumOutputPerInput>
struct TransportTag2DLineSegmentsOut
{
};

template<viskores::IdComponent NumOutputPerInput,
         typename ContObjectType,
         typename Device>
struct Transport<viskores::cont::arg::TransportTag2DLineSegmentsOut<NumOutputPerInput>,
                 ContObjectType,
                 Device>
{
  VISKORES_IS_ARRAY_HANDLE(ContObjectType);

  using GroupedArrayType = viskores::cont::ArrayHandleGroupVec<
    viskores::cont::ArrayHandleGroupVec<ContObjectType, 2>,
    NumOutputPerInput>;

  using ExecObjectType = typename GroupedArrayType::WritePortalType;

  template<typename InputDomainType>
  VISKORES_CONT ExecObjectType operator()(const ContObjectType& object,
                                          const InputDomainType&,
                                          viskores::Id,
                                          viskores::Id outputRange,
                                          viskores::cont::Token& token) const
  {
    GroupedArrayType groupedArray(viskores::cont::make_ArrayHandleGroupVec<2>(object));
    return groupedArray.PrepareForOutput(outputRange, Device{}, token);
  }
};

} // namespace arg
} // namespace cont
} // namespace viskores
////
//// END-EXAMPLE TransportImpl2.h
////

////
//// BEGIN-EXAMPLE ThreadIndicesLineFractal.h
////
namespace viskores
{
namespace exec
{
namespace arg
{

class ThreadIndicesLineFractal : public viskores::exec::arg::ThreadIndicesBasic
{
  using Superclass = viskores::exec::arg::ThreadIndicesBasic;

public:
  using CoordinateType = viskores::Vec2f;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template<typename InputPointPortal>
  VISKORES_EXEC ThreadIndicesLineFractal(viskores::Id threadIndex,
                                         viskores::Id inputIndex,
                                         viskores::IdComponent visitIndex,
                                         viskores::Id outputIndex,
                                         const InputPointPortal& inputPoints)
    : Superclass(threadIndex, inputIndex, visitIndex, outputIndex)
  {
    this->Point0 = inputPoints.Get(this->GetInputIndex())[0];
    this->Point1 = inputPoints.Get(this->GetInputIndex())[1];
  }

  VISKORES_EXEC
  const CoordinateType& GetPoint0() const { return this->Point0; }

  VISKORES_EXEC
  const CoordinateType& GetPoint1() const { return this->Point1; }

private:
  CoordinateType Point0;
  CoordinateType Point1;
};

} // namespace arg
} // namespace exec
} // namespace viskores
////
//// END-EXAMPLE ThreadIndicesLineFractal.h
////

////
//// BEGIN-EXAMPLE LineFractalTransform.h
////
namespace viskores
{
namespace exec
{

class LineFractalTransform
{
public:
  template<typename T>
  VISKORES_EXEC LineFractalTransform(const viskores::Vec<T, 2>& point0,
                                     const viskores::Vec<T, 2>& point1)
  {
    this->Offset = point0;
    this->UAxis = point1 - point0;
    this->VAxis = viskores::make_Vec(-this->UAxis[1], this->UAxis[0]);
  }

  template<typename T>
  VISKORES_EXEC viskores::Vec<T, 2> operator()(const viskores::Vec<T, 2>& ppoint) const
  {
    viskores::Vec2f ppointCast(ppoint);
    viskores::Vec2f transform =
      ppointCast[0] * this->UAxis + ppointCast[1] * this->VAxis + this->Offset;
    return viskores::Vec<T, 2>(transform);
  }

  template<typename T>
  VISKORES_EXEC viskores::Vec<T, 2> operator()(T x, T y) const
  {
    return (*this)(viskores::Vec<T, 2>(x, y));
  }

private:
  viskores::Vec2f Offset;
  viskores::Vec2f UAxis;
  viskores::Vec2f VAxis;
};

} // namespace exec
} // namespace viskores
////
//// END-EXAMPLE LineFractalTransform.h
////

////
//// BEGIN-EXAMPLE InputDomainFetch.h
////
namespace viskores
{
namespace exec
{
namespace arg
{

struct AspectTagLineFractalTransform
{
};

template<typename FetchTag, typename ExecObjectType>
struct Fetch<FetchTag,
             viskores::exec::arg::AspectTagLineFractalTransform,
             ExecObjectType>
{
  using ValueType = LineFractalTransform;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  ValueType Load(const viskores::exec::arg::ThreadIndicesLineFractal& indices,
                 const ExecObjectType&) const
  {
    return ValueType(indices.GetPoint0(), indices.GetPoint1());
  }

  VISKORES_EXEC
  void Store(const viskores::exec::arg::ThreadIndicesLineFractal&,
             const ExecObjectType&,
             const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};

} // namespace arg
} // namespace exec
} // namespace viskores
////
//// END-EXAMPLE InputDomainFetch.h
////

////
//// BEGIN-EXAMPLE WorkletLineFractal.h
////
namespace viskores
{
namespace worklet
{

template<typename WorkletType>
class DispatcherLineFractal;

class WorkletLineFractal : public viskores::worklet::internal::WorkletBase
{
public:
  /// The dispatcher used to invoke worklets of this type.
  ///
  template<typename Worklet>
  using Dispatcher = viskores::worklet::DispatcherLineFractal<Worklet>;

  /// Control signature tag for line segments in the plane. Used as the input
  /// domain.
  ///
  ////
  //// BEGIN-EXAMPLE WorkletLineFractalInputDomainTag
  ////
  struct SegmentsIn : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTag2DCoordinates;
    using TransportTag = viskores::cont::arg::TransportTag2DLineSegmentsIn;
    using FetchTag = viskores::exec::arg::FetchTag2DLineSegmentsIn;
  };
  ////
  //// END-EXAMPLE WorkletLineFractalInputDomainTag
  ////

  /// Control signature tag for a group of output line segments. The template
  /// argument specifies how many line segments are outputted for each input.
  /// The type is a Vec-like (of size NumSegments) of Vec-2's.
  ///
  ////
  //// BEGIN-EXAMPLE WorkletLineFractalOutputTag
  ////
  template<viskores::IdComponent NumSegments>
  struct SegmentsOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTag2DCoordinates;
    using TransportTag = viskores::cont::arg::TransportTag2DLineSegmentsOut<NumSegments>;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectOut;
  };
  ////
  //// END-EXAMPLE WorkletLineFractalOutputTag
  ////

  /// Control signature tag for input fields. There is one entry per input line
  /// segment. This tag takes a template argument that is a type list tag that
  /// limits the possible value types in the array.
  ///
  ////
  //// BEGIN-EXAMPLE WorkletLineFractalFieldInTag
  ////
  struct FieldIn : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = viskores::cont::arg::TransportTagArrayIn;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectIn;
  };
  ////
  //// END-EXAMPLE WorkletLineFractalFieldInTag
  ////

  /// Control signature tag for input fields. There is one entry per input line
  /// segment. This tag takes a template argument that is a type list tag that
  /// limits the possible value types in the array.
  ///
  struct FieldOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayOut;
    using TransportTag = viskores::cont::arg::TransportTagArrayOut;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectOut;
  };

  /// Execution signature tag for a LineFractalTransform from the input.
  ///
  ////
  //// BEGIN-EXAMPLE WorkletLineFractalTransformTag
  ////
  struct Transform : viskores::exec::arg::ExecutionSignatureTagBase
  {
    static const viskores::IdComponent INDEX = 1;
    using AspectTag = viskores::exec::arg::AspectTagLineFractalTransform;
  };
  ////
  //// END-EXAMPLE WorkletLineFractalTransformTag
  ////

  ////
  //// BEGIN-EXAMPLE GetThreadIndices
  ////
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template<typename OutToInPortalType,
           typename VisitPortalType,
           typename ThreadToOutType,
           typename InputDomainType>
  VISKORES_EXEC viskores::exec::arg::ThreadIndicesLineFractal GetThreadIndices(
    viskores::Id threadIndex,
    const OutToInPortalType& outToIn,
    const VisitPortalType& visit,
    const ThreadToOutType& threadToOut,
    const InputDomainType& inputPoints) const
  {
    viskores::Id outputIndex = threadToOut.Get(threadIndex);
    viskores::Id inputIndex = outToIn.Get(outputIndex);
    viskores::IdComponent visitIndex = visit.Get(outputIndex);
    return viskores::exec::arg::ThreadIndicesLineFractal(
      threadIndex, inputIndex, visitIndex, outputIndex, inputPoints);
  }
  ////
  //// END-EXAMPLE GetThreadIndices
  ////
};

} // namespace worklet
} // namespace viskores
////
//// END-EXAMPLE WorkletLineFractal.h
////

////
//// BEGIN-EXAMPLE DispatcherLineFractal.h
////
namespace viskores
{
namespace worklet
{

////
//// BEGIN-EXAMPLE DispatcherSuperclass
////
////
//// BEGIN-EXAMPLE DispatcherTemplate
////
template<typename WorkletType>
class DispatcherLineFractal
  ////
  //// END-EXAMPLE DispatcherTemplate
  ////
  : public viskores::worklet::internal::DispatcherBase<
      DispatcherLineFractal<WorkletType>,
      WorkletType,
      viskores::worklet::WorkletLineFractal>
////
//// END-EXAMPLE DispatcherSuperclass
////
{
  using Superclass =
    viskores::worklet::internal::DispatcherBase<DispatcherLineFractal<WorkletType>,
                                                WorkletType,
                                                viskores::worklet::WorkletLineFractal>;
  using ScatterType = typename Superclass::ScatterType;

public:
  ////
  //// BEGIN-EXAMPLE DispatcherConstructor
  ////
  // If you get a compile error here about there being no appropriate constructor
  // for ScatterType, then that probably means that the worklet you are trying to
  // execute has defined a custom ScatterType and that you need to create one
  // (because there is no default way to construct the scatter). By convention,
  // worklets that define a custom scatter type usually provide a static method
  // named MakeScatter that constructs a scatter object.
  VISKORES_CONT
  DispatcherLineFractal(const WorkletType& worklet = WorkletType(),
                        const ScatterType& scatter = ScatterType())
    : Superclass(worklet, scatter)
  {
  }

  VISKORES_CONT
  DispatcherLineFractal(const ScatterType& scatter)
    : Superclass(WorkletType(), scatter)
  {
  }
  ////
  //// END-EXAMPLE DispatcherConstructor
  ////

  ////
  //// BEGIN-EXAMPLE DispatcherDoInvokePrototype
  ////
  template<typename Invocation>
  VISKORES_CONT void DoInvoke(Invocation& invocation) const
  ////
  //// END-EXAMPLE DispatcherDoInvokePrototype
  ////
  {
    ////
    //// BEGIN-EXAMPLE CheckInputDomainType
    ////
    // Get the control signature tag for the input domain.
    using InputDomainTag = typename Invocation::InputDomainTag;

    // If you get a compile error on this line, then you have set the input
    // domain to something that is not a SegmentsIn parameter, which is not
    // valid.
    VISKORES_STATIC_ASSERT(
      (std::is_same<InputDomainTag,
                    viskores::worklet::WorkletLineFractal::SegmentsIn>::value));

    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;

    // If you get a compile error on this line, then you have tried to use
    // something that is not a viskores::cont::ArrayHandle as the input domain to a
    // topology operation (that operates on a cell set connection domain).
    VISKORES_IS_ARRAY_HANDLE(InputDomainType);
    ////
    //// END-EXAMPLE CheckInputDomainType
    ////

    ////
    //// BEGIN-EXAMPLE CallBasicInvoke
    ////
    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();

    // Now that we have the input domain, we can extract the range of the
    // scheduling and call BasicInvoke.
    this->BasicInvoke(invocation, inputDomain.GetNumberOfValues() / 2);
    ////
    //// END-EXAMPLE CallBasicInvoke
    ////
  }
};

} // namespace worklet
} // namespace viskores
////
//// END-EXAMPLE DispatcherLineFractal.h
////

////
//// BEGIN-EXAMPLE KochSnowflake
////
struct KochSnowflake
{
  struct FractalWorklet : viskores::worklet::WorkletLineFractal
  {
    using ControlSignature = void(SegmentsIn, SegmentsOut<4>);
    using ExecutionSignature = void(Transform, _2);
    using InputDomain = _1;

    template<typename SegmentsOutVecType>
    void operator()(const viskores::exec::LineFractalTransform& transform,
                    SegmentsOutVecType& segmentsOutVec) const
    {
      segmentsOutVec[0][0] = transform(0.00f, 0.00f);
      segmentsOutVec[0][1] = transform(0.33f, 0.00f);

      segmentsOutVec[1][0] = transform(0.33f, 0.00f);
      segmentsOutVec[1][1] = transform(0.50f, 0.29f);

      segmentsOutVec[2][0] = transform(0.50f, 0.29f);
      segmentsOutVec[2][1] = transform(0.67f, 0.00f);

      segmentsOutVec[3][0] = transform(0.67f, 0.00f);
      segmentsOutVec[3][1] = transform(1.00f, 0.00f);
    }
  };

  VISKORES_CONT static viskores::cont::ArrayHandle<viskores::Vec2f> Run(
    viskores::IdComponent numIterations)
  {
    viskores::cont::ArrayHandle<viskores::Vec2f> points;

    // Initialize points array with a single line
    points.Allocate(2);
    points.WritePortal().Set(0, viskores::Vec2f(0.0f, 0.0f));
    points.WritePortal().Set(1, viskores::Vec2f(1.0f, 0.0f));

    viskores::cont::Invoker invoke;
    KochSnowflake::FractalWorklet worklet;

    for (viskores::IdComponent i = 0; i < numIterations; ++i)
    {
      viskores::cont::ArrayHandle<viskores::Vec2f> outPoints;
      invoke(worklet, points, outPoints);
      points = outPoints;
    }

    return points;
  }
};
////
//// END-EXAMPLE KochSnowflake
////

static void TryKoch()
{
  // Demonstrate a single line.
  viskores::cont::ArrayHandle<viskores::Vec2f> points;

  points = KochSnowflake::Run(1);
  WriteSVG("Koch1.svg", points);

  for (viskores::Id index = 0; index < points.GetNumberOfValues() / 2; ++index)
  {
    std::cout << index << ": " << points.ReadPortal().Get(index * 2 + 0) << " "
              << points.ReadPortal().Get(index * 2 + 1) << std::endl;
  }

  points = KochSnowflake::Run(2);
  WriteSVG("Koch2.svg", points);

  for (viskores::Id index = 0; index < points.GetNumberOfValues() / 2; ++index)
  {
    std::cout << index << ": " << points.ReadPortal().Get(index * 2 + 0) << " "
              << points.ReadPortal().Get(index * 2 + 1) << std::endl;
  }

  points = KochSnowflake::Run(5);
  WriteSVG("Koch5.svg", points, 0.1f);
}

////
//// BEGIN-EXAMPLE QuadraticType2
////
struct QuadraticType2
{
  struct FractalWorklet : viskores::worklet::WorkletLineFractal
  {
    using ControlSignature = void(SegmentsIn, SegmentsOut<8>);
    using ExecutionSignature = void(Transform, _2);
    using InputDomain = _1;

    template<typename SegmentsOutVecType>
    void operator()(const viskores::exec::LineFractalTransform& transform,
                    SegmentsOutVecType& segmentsOutVec) const
    {
      segmentsOutVec[0][0] = transform(0.00f, 0.00f);
      segmentsOutVec[0][1] = transform(0.25f, 0.00f);

      segmentsOutVec[1][0] = transform(0.25f, 0.00f);
      segmentsOutVec[1][1] = transform(0.25f, 0.25f);

      segmentsOutVec[2][0] = transform(0.25f, 0.25f);
      segmentsOutVec[2][1] = transform(0.50f, 0.25f);

      segmentsOutVec[3][0] = transform(0.50f, 0.25f);
      segmentsOutVec[3][1] = transform(0.50f, 0.00f);

      segmentsOutVec[4][0] = transform(0.50f, 0.00f);
      segmentsOutVec[4][1] = transform(0.50f, -0.25f);

      segmentsOutVec[5][0] = transform(0.50f, -0.25f);
      segmentsOutVec[5][1] = transform(0.75f, -0.25f);

      segmentsOutVec[6][0] = transform(0.75f, -0.25f);
      segmentsOutVec[6][1] = transform(0.75f, 0.00f);

      segmentsOutVec[7][0] = transform(0.75f, 0.00f);
      segmentsOutVec[7][1] = transform(1.00f, 0.00f);
    }
  };

  VISKORES_CONT static viskores::cont::ArrayHandle<viskores::Vec2f> Run(
    viskores::IdComponent numIterations)
  {
    viskores::cont::ArrayHandle<viskores::Vec2f> points;

    // Initialize points array with a single line
    points.Allocate(2);
    points.WritePortal().Set(0, viskores::Vec2f(0.0f, 0.0f));
    points.WritePortal().Set(1, viskores::Vec2f(1.0f, 0.0f));

    viskores::cont::Invoker invoke;
    QuadraticType2::FractalWorklet worklet;

    for (viskores::IdComponent i = 0; i < numIterations; ++i)
    {
      viskores::cont::ArrayHandle<viskores::Vec2f> outPoints;
      invoke(worklet, points, outPoints);
      points = outPoints;
    }

    return points;
  }
};
////
//// END-EXAMPLE QuadraticType2
////

static void TryQuadraticType2()
{
  // Demonstrate a single line.
  viskores::cont::ArrayHandle<viskores::Vec2f> points;

  points = QuadraticType2::Run(1);
  WriteSVG("QuadraticType2_1.svg", points);

  for (viskores::Id index = 0; index < points.GetNumberOfValues() / 2; ++index)
  {
    std::cout << index << ": " << points.ReadPortal().Get(index * 2 + 0) << " "
              << points.ReadPortal().Get(index * 2 + 1) << std::endl;
  }

  points = QuadraticType2::Run(2);
  WriteSVG("QuadraticType2_2.svg", points);

  for (viskores::Id index = 0; index < points.GetNumberOfValues() / 2; ++index)
  {
    std::cout << index << ": " << points.ReadPortal().Get(index * 2 + 0) << " "
              << points.ReadPortal().Get(index * 2 + 1) << std::endl;
  }

  points = QuadraticType2::Run(4);
  WriteSVG("QuadraticType2_4.svg", points, 0.1f);
}

////
//// BEGIN-EXAMPLE DragonFractal
////
struct DragonFractal
{
  struct FractalWorklet : viskores::worklet::WorkletLineFractal
  {
    using ControlSignature = void(SegmentsIn, SegmentsOut<2>);
    using ExecutionSignature = void(Transform, _2);
    using InputDomain = _1;

    template<typename SegmentsOutVecType>
    void operator()(const viskores::exec::LineFractalTransform& transform,
                    SegmentsOutVecType& segmentsOutVec) const
    {
      segmentsOutVec[0][0] = transform(0.5f, 0.5f);
      segmentsOutVec[0][1] = transform(0.0f, 0.0f);

      segmentsOutVec[1][0] = transform(0.5f, 0.5f);
      segmentsOutVec[1][1] = transform(1.0f, 0.0f);
    }
  };

  VISKORES_CONT static viskores::cont::ArrayHandle<viskores::Vec2f> Run(
    viskores::IdComponent numIterations)
  {
    viskores::cont::ArrayHandle<viskores::Vec2f> points;

    // Initialize points array with a single line
    points.Allocate(2);
    points.WritePortal().Set(0, viskores::Vec2f(0.0f, 0.0f));
    points.WritePortal().Set(1, viskores::Vec2f(1.0f, 0.0f));

    viskores::cont::Invoker invoke;
    DragonFractal::FractalWorklet worklet;

    for (viskores::IdComponent i = 0; i < numIterations; ++i)
    {
      viskores::cont::ArrayHandle<viskores::Vec2f> outPoints;
      invoke(worklet, points, outPoints);
      points = outPoints;
    }

    return points;
  }
};
////
//// END-EXAMPLE DragonFractal
////

static void TryDragon()
{
  // Demonstrate a single line.
  viskores::cont::ArrayHandle<viskores::Vec2f> points;

  for (viskores::IdComponent numIterations = 1; numIterations <= 13; ++numIterations)
  {
    points = DragonFractal::Run(numIterations);
    char filename[FILENAME_MAX];
    snprintf(filename, FILENAME_MAX, "Dragon%02d.svg", numIterations);
    WriteSVG(filename, points, 2.0f / numIterations);
  }
}

////
//// BEGIN-EXAMPLE HilbertCurve
////
struct HilbertCurve
{
  struct FractalWorklet : viskores::worklet::WorkletLineFractal
  {
    using ControlSignature = void(SegmentsIn,
                                  FieldIn directionIn,
                                  SegmentsOut<4>,
                                  FieldOut directionOut);
    using ExecutionSignature = void(Transform, _2, _3, _4);
    using InputDomain = _1;

    template<typename SegmentsOutVecType>
    void operator()(const viskores::exec::LineFractalTransform& transform,
                    viskores::Int8 directionIn,
                    SegmentsOutVecType& segmentsOutVec,
                    viskores::Vec4i_8& directionOut) const
    {
      segmentsOutVec[0][0] = transform(0.0f, directionIn * 0.0f);
      segmentsOutVec[0][1] = transform(0.0f, directionIn * 0.5f);
      directionOut[0] = -directionIn;

      segmentsOutVec[1][0] = transform(0.0f, directionIn * 0.5f);
      segmentsOutVec[1][1] = transform(0.5f, directionIn * 0.5f);
      directionOut[1] = directionIn;

      segmentsOutVec[2][0] = transform(0.5f, directionIn * 0.5f);
      segmentsOutVec[2][1] = transform(1.0f, directionIn * 0.5f);
      directionOut[2] = directionIn;

      segmentsOutVec[3][0] = transform(1.0f, directionIn * 0.5f);
      segmentsOutVec[3][1] = transform(1.0f, directionIn * 0.0f);
      directionOut[3] = -directionIn;
    }
  };

  VISKORES_CONT static viskores::cont::ArrayHandle<viskores::Vec2f> Run(
    viskores::IdComponent numIterations)
  {
    viskores::cont::ArrayHandle<viskores::Vec2f> points;

    // Initialize points array with a single line
    points.Allocate(2);
    points.WritePortal().Set(0, viskores::Vec2f(0.0f, 0.0f));
    points.WritePortal().Set(1, viskores::Vec2f(1.0f, 0.0f));

    viskores::cont::ArrayHandle<viskores::Int8> directions;

    // Initialize direction with positive.
    directions.Allocate(1);
    directions.WritePortal().Set(0, 1);

    viskores::cont::Invoker invoke;
    HilbertCurve::FractalWorklet worklet;

    for (viskores::IdComponent i = 0; i < numIterations; ++i)
    {
      viskores::cont::ArrayHandle<viskores::Vec2f> outPoints;
      viskores::cont::ArrayHandle<viskores::Int8> outDirections;
      invoke(worklet,
             points,
             directions,
             outPoints,
             viskores::cont::make_ArrayHandleGroupVec<4>(outDirections));
      points = outPoints;
      directions = outDirections;
    }

    return points;
  }
};
////
//// END-EXAMPLE HilbertCurve
////

static void TryHilbert()
{
  // Demonstrate a single line.
  viskores::cont::ArrayHandle<viskores::Vec2f> points;

  for (viskores::IdComponent numIterations = 1; numIterations <= 6; ++numIterations)
  {
    points = HilbertCurve::Run(numIterations);
    char filename[FILENAME_MAX];
    snprintf(filename, FILENAME_MAX, "Hilbert%02d.svg", numIterations);
    WriteSVG(filename, points, 2.0f / numIterations);
  }
}

////
//// BEGIN-EXAMPLE TreeFractal
////
struct TreeFractal
{
  struct FractalWorklet : viskores::worklet::WorkletLineFractal
  {
    using ControlSignature = void(SegmentsIn,
                                  SegmentsOut<1>,
                                  FieldOut countNextIteration);
    using ExecutionSignature = void(Transform, VisitIndex, _2, _3);
    using InputDomain = _1;

    using ScatterType = viskores::worklet::ScatterCounting;

    template<typename Storage>
    VISKORES_CONT static ScatterType MakeScatter(
      const viskores::cont::ArrayHandle<viskores::IdComponent, Storage>& count)
    {
      return ScatterType(count);
    }

    template<typename SegmentsOutVecType>
    void operator()(const viskores::exec::LineFractalTransform& transform,
                    viskores::IdComponent visitIndex,
                    SegmentsOutVecType& segmentsOutVec,
                    viskores::IdComponent& countNextIteration) const
    {
      switch (visitIndex)
      {
        case 0:
          segmentsOutVec[0][0] = transform(0.0f, 0.0f);
          segmentsOutVec[0][1] = transform(1.0f, 0.0f);
          countNextIteration = 1;
          break;
        case 1:
          segmentsOutVec[0][0] = transform(1.0f, 0.0f);
          segmentsOutVec[0][1] = transform(1.5f, -0.25f);
          countNextIteration = 3;
          break;
        case 2:
          segmentsOutVec[0][0] = transform(1.0f, 0.0f);
          segmentsOutVec[0][1] = transform(1.5f, 0.35f);
          countNextIteration = 3;
          break;
        default:
          this->RaiseError("Unexpected visit index.");
      }
    }
  };

  VISKORES_CONT static viskores::cont::ArrayHandle<viskores::Vec2f> Run(
    viskores::IdComponent numIterations)
  {
    viskores::cont::ArrayHandle<viskores::Vec2f> points;

    // Initialize points array with a single line
    points.Allocate(2);
    points.WritePortal().Set(0, viskores::Vec2f(0.0f, 0.0f));
    points.WritePortal().Set(1, viskores::Vec2f(0.0f, 1.0f));

    viskores::cont::ArrayHandle<viskores::IdComponent> count;

    // Initialize count array with 3 (meaning iterate)
    count.Allocate(1);
    count.WritePortal().Set(0, 3);

    viskores::cont::Invoker invoke;
    TreeFractal::FractalWorklet worklet;

    for (viskores::IdComponent i = 0; i < numIterations; ++i)
    {
      auto scatter = TreeFractal::FractalWorklet::MakeScatter(count);
      viskores::cont::ArrayHandle<viskores::Vec2f> outPoints;
      invoke(worklet, scatter, points, outPoints, count);
      points = outPoints;
    }

    return points;
  }
};
////
//// END-EXAMPLE TreeFractal
////

static void TryTree()
{
  // Demonstrate a single line.
  viskores::cont::ArrayHandle<viskores::Vec2f> points;

  for (viskores::IdComponent numIterations = 1; numIterations <= 8; ++numIterations)
  {
    points = TreeFractal::Run(numIterations);
    char filename[FILENAME_MAX];
    snprintf(filename, FILENAME_MAX, "Tree%02d.svg", numIterations);
    WriteSVG(filename, points, 2.0f / numIterations);
  }
}

static void RunTests()
{
  TryVecLineSegments();
  TryKoch();
  TryQuadraticType2();
  TryDragon();
  TryHilbert();
  TryTree();
}

int GuideExampleFractalWorklets(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTests, argc, argv);
}
