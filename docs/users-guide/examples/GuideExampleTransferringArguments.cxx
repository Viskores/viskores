//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/arg/TransportTagArrayIn.h>
#include <viskores/cont/arg/TransportTagArrayOut.h>
#include <viskores/cont/arg/TransportTagWholeArrayInOut.h>
#include <viskores/cont/arg/TypeCheckTagArrayIn.h>
#include <viskores/cont/arg/TypeCheckTagExecObject.h>

#include <viskores/cont/DeviceAdapter.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

namespace TypeCheckNamespace
{

////
//// BEGIN-EXAMPLE TypeCheck
////
struct MyExecObject : viskores::cont::ExecutionObjectBase
{
  viskores::Id Value;
};

void DoTypeChecks()
{
  ////
  //// PAUSE-EXAMPLE
  ////
  std::cout << "Testing type checks" << std::endl;
  ////
  //// RESUME-EXAMPLE
  ////
  using viskores::cont::arg::TypeCheck;
  using viskores::cont::arg::TypeCheckTagArrayIn;
  using viskores::cont::arg::TypeCheckTagExecObject;

  bool check1 = TypeCheck<TypeCheckTagExecObject, MyExecObject>::value; // true
  bool check2 = TypeCheck<TypeCheckTagExecObject, viskores::Id>::value; // false

  using ArrayType = viskores::cont::ArrayHandle<viskores::Float32>;

  bool check3 = TypeCheck<TypeCheckTagArrayIn, ArrayType>::value;    // true
  bool check4 = TypeCheck<TypeCheckTagExecObject, ArrayType>::value; // false
  ////
  //// PAUSE-EXAMPLE
  ////
  VISKORES_TEST_ASSERT(check1 == true, "Type check failed.");
  VISKORES_TEST_ASSERT(check2 == false, "Type check failed.");
  VISKORES_TEST_ASSERT(check3 == true, "Type check failed.");
  VISKORES_TEST_ASSERT(check4 == false, "Type check failed.");
  ////
  //// RESUME-EXAMPLE
  ////
}
////
//// END-EXAMPLE TypeCheck
////

} // namespace TypeCheckNamespace

using namespace TypeCheckNamespace;

namespace TransportNamespace
{

////
//// BEGIN-EXAMPLE Transport
////
template<typename ArrayType, typename Device>
void DoTransport(ArrayType inArray, ArrayType outArray, Device)
{
  VISKORES_IS_ARRAY_HANDLE(ArrayType);
  VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
  ////
  //// PAUSE-EXAMPLE
  ////
  std::cout << "Testing transports." << std::endl;
  ////
  //// RESUME-EXAMPLE
  ////

  using viskores::cont::arg::Transport;
  using viskores::cont::arg::TransportTagArrayIn;
  using viskores::cont::arg::TransportTagArrayOut;
  using viskores::cont::arg::TransportTagWholeArrayInOut;

  viskores::cont::Token token;

  // The array in transport returns a read-only array portal.
  using ArrayInTransport = Transport<TransportTagArrayIn, ArrayType, Device>;
  typename ArrayInTransport::ExecObjectType inPortal =
    ArrayInTransport()(inArray, inArray, 10, 10, token);

  // The array out transport returns an allocated array portal.
  using ArrayOutTransport = Transport<TransportTagArrayOut, ArrayType, Device>;
  typename ArrayOutTransport::ExecObjectType outPortal =
    ArrayOutTransport()(outArray, inArray, 10, 10, token);

  // The whole array in transport returns a read-only array portal wrapped in
  // a viskores::exec::ExecutionWholeArrayConst.
  using WholeArrayTransport = Transport<TransportTagWholeArrayInOut, ArrayType, Device>;
  typename WholeArrayTransport::ExecObjectType wholeArray =
    WholeArrayTransport()(inArray, inArray, 10, 10, token);
  ////
  //// PAUSE-EXAMPLE
  ////
  CheckPortal(inPortal);
  VISKORES_TEST_ASSERT(outPortal.GetNumberOfValues() == 10, "Bad array out.");
  CheckPortal(wholeArray);
  ////
  //// RESUME-EXAMPLE
  ////
}
////
//// END-EXAMPLE Transport
////

void DoTransport()
{
  viskores::cont::ArrayHandle<viskores::Id> arrayHandle;
  arrayHandle.Allocate(10);
  SetPortal(arrayHandle.WritePortal());

  DoTransport(arrayHandle,
              viskores::cont::ArrayHandle<viskores::Id>(),
              viskores::cont::DeviceAdapterTagSerial());
}

} // namespace TransportNamespace

using namespace TransportNamespace;

void Test()
{
  DoTypeChecks();
  DoTransport();
}

} // anonymous namespace

int GuideExampleTransferringArguments(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
