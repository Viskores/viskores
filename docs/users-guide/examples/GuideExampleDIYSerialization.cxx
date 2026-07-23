//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayGetValues.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/DIYMemoryManagement.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/Initialize.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/thirdparty/diy/diy.h>

#include <iostream>
#include <memory>

const int TEST_NUM_POINTS = 5;

struct TimedCoords;
TimedCoords ComputeLocalCoords(int gid);
bool operator!=(const TimedCoords& a, const TimedCoords& b);

//// BEGIN-EXAMPLE DIYSerialization
struct TimedCoords
{
  viskores::cont::ArrayHandle<viskores::UInt64> TimeStamps;
  viskores::cont::ArrayHandle<viskores::Vec3i> Coordinates;
};

namespace viskoresdiy
{
template<>
struct Serialization<TimedCoords>
{
  static void save(BinaryBuffer& bb, const TimedCoords& p)
  {
    viskoresdiy::save(bb, p.TimeStamps);
    viskoresdiy::save(bb, p.Coordinates);
  }
  static void load(BinaryBuffer& bb, TimedCoords& p)
  {
    viskoresdiy::load(bb, p.TimeStamps);
    viskoresdiy::load(bb, p.Coordinates);
  }
};
}

void compute(viskoresdiy::Master& master, viskoresdiy::Link* link, int gid)
{
  TimedCoords timedCoords;
  master.add(gid, &timedCoords, link);

  master.foreach (
    [&](TimedCoords* tc, const viskoresdiy::Master::ProxyWithLink& cp)
    {
      *tc = ComputeLocalCoords(gid);
      cp.enqueue(cp.link()->target(0), *tc);
    });
  viskores::cont::DIYMasterExchange(master);
  master.foreach (
    [&](TimedCoords* tc, const viskoresdiy::Master::ProxyWithLink& cp)
    {
      cp.dequeue(cp.link()->target(0).gid, *tc);
      auto expectedVec = ComputeLocalCoords(gid);
      if (*tc != expectedVec)
      {
        std::cerr << "ERROR: recieved incorrect vec values." << std::endl;
        //// PAUSE-EXAMPLE
        VISKORES_TEST_FAIL("Received incorrect vec values.");
        //// RESUME-EXAMPLE
      }
    });
}
//// END-EXAMPLE DIYSerialization

TimedCoords ComputeLocalCoords(int gid)
{
  auto timeStamps =
    viskores::cont::make_ArrayHandleCounting<viskores::UInt64>(0, 1, TEST_NUM_POINTS);
  auto coordinates = viskores::cont::make_ArrayHandleCounting<viskores::Vec3i>(
    viskores::make_Vec(0, 0, 0), viskores::make_Vec(1, gid, 0), TEST_NUM_POINTS);

  // This depends on RTV optimization
  TimedCoords output;
  viskores::cont::ArrayCopy(timeStamps, output.TimeStamps);
  viskores::cont::ArrayCopy(coordinates, output.Coordinates);
  return output;
}

bool operator!=(const TimedCoords& a, const TimedCoords& b)
{
  if (!a.TimeStamps.GetNumberOfValues() || !b.TimeStamps.GetNumberOfValues())
  {
    VISKORES_TEST_FAIL("Received incorrect vec values.");
    return true;
  }
  if (a.TimeStamps.GetNumberOfValues() != b.TimeStamps.GetNumberOfValues() ||
      a.Coordinates.GetNumberOfValues() != b.Coordinates.GetNumberOfValues())
  {
    return true;
  }

  for (viskores::Id i = 0; i < a.Coordinates.GetNumberOfValues(); i++)
  {
    if (viskores::cont::ArrayGetValue(i, a.Coordinates) !=
        viskores::cont::ArrayGetValue(i, b.Coordinates))
    {
      return true;
    }
  }
  return false;
}

void TestDiySerialization()
{
  viskoresdiy::mpi::communicator comm;
  viskores::cont::EnvironmentTracker::SetCommunicator(comm);
  viskoresdiy::Master master(comm);

  auto nblocks = comm.size();
  std::vector<int> gids;

  viskoresdiy::RoundRobinAssigner assigner(comm.size(), nblocks);
  assigner.local_gids(comm.rank(), gids);

  // In our example nblocks == num_ranks, thus gids.size() == 1
  auto gid = gids[0];

  // The link will be eventually freed by DIY.
  auto link = new viskoresdiy::Link;

  // Connect each blocks with themself.
  viskoresdiy::BlockID neighbor;
  neighbor.gid = gid;
  neighbor.proc = assigner.rank(neighbor.gid);
  link->add_neighbor(neighbor);

  compute(master, link, gid);
}

int GuideExampleDIYSerialization(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestDiySerialization, argc, argv);
}
