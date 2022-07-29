//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_Messenger_h
#define vtk_m_filter_flow_internal_Messenger_h

#include <vtkm/Types.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>
#include <vtkm/thirdparty/diy/diy.h>

#include <list>
#include <map>
#include <set>
#include <vector>

#ifdef VTKM_ENABLE_MPI
#include <mpi.h>
#endif

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{

class VTKM_FILTER_FLOW_EXPORT Messenger
{
public:
  VTKM_CONT Messenger(vtkmdiy::mpi::communicator& comm);
  VTKM_CONT virtual ~Messenger()
  {
#ifdef VTKM_ENABLE_MPI
    this->CleanupRequests();
#endif
  }

  int GetRank() const { return this->Rank; }
  int GetNumRanks() const { return this->NumRanks; }

#ifdef VTKM_ENABLE_MPI
  VTKM_CONT void RegisterTag(int tag, std::size_t numRecvs, std::size_t size);

protected:
  static std::size_t CalcMessageBufferSize(std::size_t msgSz);

  void InitializeBuffers();
  void CheckPendingSendRequests();
  void CleanupRequests(int tag = TAG_ANY);
  void SendData(int dst, int tag, const vtkmdiy::MemoryBuffer& buff);
  bool RecvData(const std::set<int>& tags,
                std::vector<std::pair<int, vtkmdiy::MemoryBuffer>>& buffers,
                bool blockAndWait = false);

private:
  void PostRecv(int tag);
  void PostRecv(int tag, std::size_t sz, int src = -1);


  //Message headers.
  typedef struct
  {
    int rank, tag;
    std::size_t id, numPackets, packet, packetSz, dataSz;
  } Header;

  bool RecvData(int tag, std::vector<vtkmdiy::MemoryBuffer>& buffers, bool blockAndWait = false);

  void PrepareForSend(int tag, const vtkmdiy::MemoryBuffer& buff, std::vector<char*>& buffList);
  vtkm::Id GetMsgID() { return this->MsgID++; }
  static bool PacketCompare(const char* a, const char* b);
  void ProcessReceivedBuffers(std::vector<char*>& incomingBuffers,
                              std::vector<std::pair<int, vtkmdiy::MemoryBuffer>>& buffers);

  // Send/Recv buffer management structures.
  using RequestTagPair = std::pair<MPI_Request, int>;
  using RankIdPair = std::pair<int, int>;

  //Member data
  std::map<int, std::pair<std::size_t, std::size_t>> MessageTagInfo;
  MPI_Comm MPIComm;
  std::size_t MsgID;
  int NumRanks;
  int Rank;
  std::map<RequestTagPair, char*> RecvBuffers;
  std::map<RankIdPair, std::list<char*>> RecvPackets;
  std::map<RequestTagPair, char*> SendBuffers;
  static constexpr int TAG_ANY = -1;

  void CheckRequests(const std::map<RequestTagPair, char*>& buffer,
                     const std::set<int>& tags,
                     bool BlockAndWait,
                     std::vector<RequestTagPair>& reqTags);
#else
protected:
  static constexpr int NumRanks = 1;
  static constexpr int Rank = 0;
#endif
};

}
}
}
} // vtkm::filter::flow::internal

#endif // vtk_m_filter_flow_internal_Messenger_h
