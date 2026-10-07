//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayGetValues.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DIYMemoryManagement.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/Initialize.h>
#include <viskores/thirdparty/diy/diy.h>

#include <viskores/cont/testing/Testing.h>

#include <memory>

namespace
{

void DiyTest()
{
  //// BEGIN-EXAMPLE DIYSetupComm
  viskoresdiy::mpi::communicator comm;
  //// LABEL SetCommunicator
  viskores::cont::EnvironmentTracker::SetCommunicator(comm);

  auto nblocks = comm.size();
  std::vector<int> gids;

  viskoresdiy::RoundRobinAssigner assigner(comm.size(), nblocks);
  assigner.local_gids(comm.rank(), gids);

  // In our example nblocks == num_ranks, thus gids.size() == 1
  auto gid = gids[0];

  // The link will be eventually freed by DIY.
  auto link = new viskoresdiy::Link;

  // Connect each blocks with itself.
  viskoresdiy::BlockID neighbor;
  neighbor.gid = gid;
  neighbor.proc = assigner.rank(neighbor.gid);
  link->add_neighbor(neighbor);
  //// END-EXAMPLE DIYSetupComm

  viskores::cont::ArrayHandle<viskores::Int32> inputArrayHandle;
  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex{ 1000 }, inputArrayHandle);

  //// BEGIN-EXAMPLE DIYForeach
  viskoresdiy::Master master(comm);

  struct MyBlock
  {
    viskores::cont::ArrayHandle<viskores::Int32> in;
  };

  MyBlock block{ inputArrayHandle };
  master.add(gid, &block, link);

  master.foreach (
    [&](MyBlock* b, const viskoresdiy::Master::ProxyWithLink& cp)
    {
      viskores::cont::Algorithm::Sort(b->in);
      cp.enqueue(cp.link()->target(0), b->in);
    });
  //// LABEL DIYMasterExchange
  viskores::cont::DIYMasterExchange(master);
  master.foreach (
    [&](MyBlock* b, const viskoresdiy::Master::ProxyWithLink& cp)
    {
      cp.dequeue(cp.link()->target(0).gid, b->in);

      auto median_idx = (b->in.GetNumberOfValues() / 2) - 1;
      auto median = viskores::cont::ArrayGetValue(median_idx, b->in);

      cp.all_reduce(median, viskoresdiy::mpi::maximum<viskores::Int32>());
    });
  viskores::cont::DIYMasterExchange(master);

  if (comm.rank() == 0)
  {
    std::cout << "Max(median): "
              << master.proxy(master.loaded_block()).get<viskores::Int32>() << std::endl;
  }
  //// END-EXAMPLE DIYForeach
}

} // anonymous namespace

int GuideExampleDIY(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DiyTest, argc, argv);
}