//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/Bounds.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE ScatterCounting
////
struct ClipPoints
{
  class Count : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn points, FieldOut count);
    using ExecutionSignature = _2(_1);
    using InputDomain = _1;

    VISKORES_CONT Count(const viskores::Bounds& bounds)
      : Bounds(bounds)
    {
    }

    template<typename T>
    VISKORES_EXEC viskores::IdComponent operator()(
      const viskores::Vec<T, 3>& point) const
    {
      return (this->Bounds.Contains(point) ? 1 : 0);
    }

  private:
    viskores::Bounds Bounds;
  };

  ////
  //// BEGIN-EXAMPLE DeclareScatter
  ////
  class Generate : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn inPoints, FieldOut outPoints);
    using ExecutionSignature = void(_1, _2);
    using InputDomain = _1;

    using ScatterType = viskores::worklet::ScatterCounting;
    ////
    //// END-EXAMPLE DeclareScatter
    ////

    template<typename InType, typename OutType>
    VISKORES_EXEC void operator()(const viskores::Vec<InType, 3>& inPoint,
                                  viskores::Vec<OutType, 3>& outPoint) const
    {
      // The scatter ensures that this method is only called for input points
      // that are passed to the output (where the count was 1). Thus, in this
      // case we know that we just need to copy the input to the output.
      outPoint = viskores::Vec<OutType, 3>(inPoint[0], inPoint[1], inPoint[2]);
    }
  };
};

//
// Later in the associated Filter class...
//

//// PAUSE-EXAMPLE
struct DemoClipPoints
{
  viskores::cont::Invoker Invoke;

  viskores::Bounds Bounds;

  template<typename T, typename Storage>
  VISKORES_CONT viskores::cont::ArrayHandle<T> Run(
    const viskores::cont::ArrayHandle<T, Storage>& inField)
  {
    //// RESUME-EXAMPLE
    viskores::cont::ArrayHandle<viskores::IdComponent> countArray;

    this->Invoke(ClipPoints::Count(this->Bounds), inField, countArray);

    viskores::cont::ArrayHandle<T> clippedPointsArray;

    ////
    //// BEGIN-EXAMPLE ConstructScatterForInvoke
    ////
    viskores::worklet::ScatterCounting generateScatter(countArray);
    this->Invoke(ClipPoints::Generate{}, generateScatter, inField, clippedPointsArray);
    ////
    //// END-EXAMPLE ConstructScatterForInvoke
    ////
    ////
    //// END-EXAMPLE ScatterCounting
    ////

    return clippedPointsArray;
  }
};

void Run()
{
  std::cout << "Trying clip points." << std::endl;
  viskores::cont::ArrayHandleUniformPointCoordinates points(viskores::Id3(10, 10, 10));
  viskores::Bounds bounds(
    viskores::Range(0.5, 8.5), viskores::Range(0.5, 8.5), viskores::Range(0.5, 8.5));

  VISKORES_TEST_ASSERT(points.GetNumberOfValues() == 1000,
                       "Unexpected number of input points.");

  DemoClipPoints demo;
  demo.Bounds = bounds;
  viskores::cont::ArrayHandle<viskores::Vec3f> clippedPoints = demo.Run(points);

  viskores::cont::printSummary_ArrayHandle(clippedPoints, std::cout);
  std::cout << std::endl;
  VISKORES_TEST_ASSERT(clippedPoints.GetNumberOfValues() == 512,
                       "Unexpected number of output points.");
}

} // anonymous namespace

int GuideExampleScatterCounting(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
