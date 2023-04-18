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
  VTKM_CONT Messenger(vtkmdiy::mpi::communicator& comm, bool useAsyncComm);
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

  bool UsingSyncCommunication() const { return !this->UsingAsyncCommunication(); }
  bool UsingAsyncCommunication() const { return this->UseAsynchronousCommunication; }

protected:
  static std::size_t CalcMessageBufferSize(std::size_t msgSz);

  void InitializeBuffers();
  void CheckPendingSendRequests();
  void CleanupRequests(int tag = TAG_ANY);
  void SendData(int dst, int tag, vtkmdiy::MemoryBuffer& buff)
  {
    if (this->UseAsynchronousCommunication)
      this->SendDataAsync(dst, tag, buff);
    else
      this->SendDataSync(dst, tag, buff);
  }
  bool RecvData(const std::set<int>& tags,
                std::vector<std::pair<int, vtkmdiy::MemoryBuffer>>& buffers,
                bool blockAndWait = false)
  {
    if (this->UseAsynchronousCommunication)
      return this->RecvDataAsync(tags, buffers, blockAndWait);
    else
      return this->RecvDataSync(tags, buffers, blockAndWait);
  }

private:
  void SendDataAsync(int dst, int tag, const vtkmdiy::MemoryBuffer& buff);
  void SendDataSync(int dst, int tag, vtkmdiy::MemoryBuffer& buff);
  bool RecvDataAsync(const std::set<int>& tags,
                     std::vector<std::pair<int, vtkmdiy::MemoryBuffer>>& buffers,
                     bool blockAndWait);
  bool RecvDataSync(const std::set<int>& tags,
                    std::vector<std::pair<int, vtkmdiy::MemoryBuffer>>& buffers,
                    bool blockAndWait);
  void PostRecv(int tag);
  void PostRecv(int tag, std::size_t sz, int src = -1);


  //Message headers.
  typedef struct
  {
    int rank, tag;
    std::size_t id, numPackets, packet, packetSz, dataSz;
  } Header;

  void PrepareForSend(int tag, const vtkmdiy::MemoryBuffer& buff, std::vector<char*>& buffList);
  vtkm::Id GetMsgID() { return this->MsgID++; }
  static bool PacketCompare(const char* a, const char* b);
  void ProcessReceivedBuffers(std::vector<char*>& incomingBuffers,
                              std::vector<std::pair<int, vtkmdiy::MemoryBuffer>>& buffers);

  // Send/Recv buffer management structures.
  using RequestTagPair = std::pair<MPI_Request, int>;
  using RankIdPair = std::pair<int, int>;

  //Member data
  // <tag, {dst, buffer}>
  std::map<int, std::vector<std::pair<int, vtkmdiy::MemoryBuffer>>> SyncSendBuffers;
  std::map<int, std::pair<std::size_t, std::size_t>> MessageTagInfo;
  MPI_Comm MPIComm;
  std::size_t MsgID;
  int NumRanks;
  int Rank;
  std::map<RequestTagPair, char*> RecvBuffers;
  std::map<RankIdPair, std::list<char*>> RecvPackets;
  std::map<RequestTagPair, char*> SendBuffers;
  static constexpr int TAG_ANY = -1;
  bool UseAsynchronousCommunication = true;

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


template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
  os << "[";
  for (std::size_t i = 0; i < v.size(); ++i)
  {
    os << v[i];
    if (i != v.size() - 1)
      os << ", ";
  }
  os << "]";
  return os;
}

}
}
}
} // vtkm::filter::flow::internal

#endif // vtk_m_filter_flow_internal_Messenger_h
