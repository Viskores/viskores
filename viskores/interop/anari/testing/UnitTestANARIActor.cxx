//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

// viskores::anari
#include <viskores/interop/anari/ANARIActor.h>
// viskores
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

bool SetPrimaryFieldIndexThrows(viskores::interop::anari::ANARIActor& actor,
                                viskores::IdComponent idx)
{
  try
  {
    actor.SetPrimaryFieldIndex(idx);
  }
  catch (const viskores::cont::ErrorBadValue&)
  {
    return true;
  }
  return false;
}

bool GetFieldThrows(const viskores::interop::anari::ANARIActor& actor, viskores::IdComponent idx)
{
  try
  {
    actor.GetField(idx);
  }
  catch (const viskores::cont::ErrorBadValue&)
  {
    return true;
  }
  return false;
}

// An actor holds four fields, so only 0-3 name one of them.
void PrimaryFieldIndexTests()
{
  viskores::interop::anari::ANARIActor actor;

  for (viskores::IdComponent idx = 0; idx < 4; ++idx)
  {
    VISKORES_TEST_ASSERT(
      !SetPrimaryFieldIndexThrows(actor, idx), "SetPrimaryFieldIndex rejected valid index ", idx);
    VISKORES_TEST_ASSERT(actor.GetPrimaryFieldIndex() == idx, "Primary field index not stored");
  }

  for (viskores::IdComponent idx : { -1, 4, 7 })
  {
    VISKORES_TEST_ASSERT(SetPrimaryFieldIndexThrows(actor, idx),
                         "SetPrimaryFieldIndex accepted out-of-range index ",
                         idx);
  }
  VISKORES_TEST_ASSERT(actor.GetPrimaryFieldIndex() == 3,
                       "A rejected index changed the primary field index");
}

// GetField takes 0-3, or a negative index for the primary field.
void GetFieldTests()
{
  viskores::interop::anari::ANARIActor actor;
  for (viskores::IdComponent idx : { -1, 0, 3 })
  {
    VISKORES_TEST_ASSERT(!GetFieldThrows(actor, idx), "GetField rejected valid index ", idx);
  }
  VISKORES_TEST_ASSERT(GetFieldThrows(actor, 4), "GetField accepted index 4");
}

void ActorTests()
{
  PrimaryFieldIndexTests();
  GetFieldTests();
}

} // namespace

int UnitTestANARIActor(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(ActorTests, argc, argv);
}
