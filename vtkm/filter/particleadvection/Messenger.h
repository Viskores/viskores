//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Messenger_h
#define vtk_m_filter_Messenger_h

#include <vtkm/Types.h>
#include <vtkm/filter/particleadvection/MemStream.h>
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
namespace particleadvection
{

class VTKM_ALWAYS_EXPORT Messenger
{
public:
  VTKM_CONT Messenger(vtkmdiy::mpi::communicator& comm);
  VTKM_CONT virtual ~Messenger()
  {
#ifdef VTKM_ENABLE_MPI
    this->CleanupRequests();
#endif
  }

#ifdef VTKM_ENABLE_MPI
  VTKM_CONT void RegisterTag(int tag, int numRecvs, int size);

protected:
  void InitializeBuffers();
  void CleanupRequests(int tag = TAG_ANY);
  void CheckPendingSendRequests();
  void PostRecv(int tag);
  void PostRecv(int tag, int sz, int src = -1);
  void SendData(int dst, int tag, MemStream* buff);
  bool RecvData(std::set<int>& tags,
                std::vector<std::pair<int, MemStream*>>& buffers,
                bool blockAndWait = false);

  //Message headers.
  typedef struct
  {
    int rank, id, tag, numPackets, packet, packetSz, dataSz;
  } Header;

  bool RecvData(int tag, std::vector<MemStream*>& buffers, bool blockAndWait = false);
  void AddHeader(MemStream* buff);
  void RemoveHeader(MemStream* input, MemStream* header, MemStream* buff);

  template <typename P>
  bool DoSendICs(int dst, std::vector<P>& ics);
  void PrepareForSend(int tag, MemStream* buff, std::vector<unsigned char*>& buffList);
  static bool PacketCompare(const unsigned char* a, const unsigned char* b);
  void ProcessReceivedBuffers(std::vector<unsigned char*>& incomingBuffers,
                              std::vector<std::pair<int, MemStream*>>& buffers);

  // Send/Recv buffer management structures.
  using RequestTagPair = std::pair<MPI_Request, int>;
  using RankIdPair = std::pair<int, int>;

  //Member data
  std::map<int, std::pair<int, int>> MessageTagInfo;
  MPI_Comm MPIComm;
  vtkm::Id MsgID;
  int NumRanks;
  int Rank;
  std::map<RequestTagPair, unsigned char*> RecvBuffers;
  std::map<RankIdPair, std::list<unsigned char*>> RecvPackets;
  std::map<RequestTagPair, unsigned char*> SendBuffers;
  static constexpr int TAG_ANY = -1;
#else
  static constexpr int NumRanks = 1;
  static constexpr int Rank = 0;
#endif

  static int CalcMessageBufferSize(int msgSz);
};
}
}
} // namespace vtkm::filter::particleadvection


#endif
