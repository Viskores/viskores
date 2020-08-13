//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ParticleMessenger_h
#define vtk_m_filter_ParticleMessenger_h

#include <vtkm/Particle.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/Messenger.h>

#include <list>
#include <map>
#include <set>
#include <vector>

namespace vtkm
{
namespace filter
{

class VTKM_ALWAYS_EXPORT ParticleMessenger : public vtkm::filter::Messenger
{
  //sendRank, message
  using MsgCommType = std::pair<int, std::vector<int>>;

  //particle + blockIDs.
  using ParticleCommType = std::pair<vtkm::Massless, std::vector<vtkm::Id>>;

  //sendRank, vector of ParticleCommType.
  using ParticleRecvCommType = std::pair<int, std::vector<ParticleCommType>>;

public:
  VTKM_CONT ParticleMessenger(vtkmdiy::mpi::communicator& comm,
                              const vtkm::filter::BoundsMap& bm,
                              int msgSz = 1,
                              int numParticles = 128);
  VTKM_CONT ~ParticleMessenger() {}

  VTKM_CONT void Exchange(const std::vector<vtkm::Massless>& outData,
                          const std::map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
                          vtkm::Id numLocalTerm,
                          std::vector<vtkm::Massless>& inData,
                          std::map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap,
                          vtkm::Id& numTerminateMessages);

protected:
#ifdef VTKM_ENABLE_MPI
  static constexpr int MSG_TERMINATE = 1;

  enum
  {
    MESSAGE_TAG = 0x42000,
    PARTICLE_TAG = 0x42001
  };

  VTKM_CONT void RegisterMessages(int msgSz, int nParticles);

  // Send/Recv Integral curves.
  VTKM_CONT
  template <typename P,
            template <typename, typename> class Container,
            typename Allocator = std::allocator<P>>
  void SendParticles(int dst, const Container<P, Allocator>& c);

  VTKM_CONT
  template <typename P,
            template <typename, typename> class Container,
            typename Allocator = std::allocator<P>>
  void SendParticles(const std::map<int, Container<P, Allocator>>& m);

  // Send/Recv messages.
  VTKM_CONT void SendMsg(int dst, const std::vector<int>& msg);
  VTKM_CONT void SendAllMsg(const std::vector<int>& msg);
  VTKM_CONT bool RecvMsg(std::vector<MsgCommType>& msgs) { return RecvAny(&msgs, NULL, false); }

  // Send/Recv datasets.
  VTKM_CONT bool RecvAny(std::vector<MsgCommType>* msgs,
                         std::vector<ParticleRecvCommType>* recvParticles,
                         bool blockAndWait);
  const vtkm::filter::BoundsMap& BoundsMap;

#endif

  VTKM_CONT void SerialExchange(const std::vector<vtkm::Massless>& outData,
                                const std::map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
                                vtkm::Id numLocalTerm,
                                std::vector<vtkm::Massless>& inData,
                                std::map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap) const;

  static int CalcParticleBufferSize(int nParticles, int numBlockIds = 2);
};


template <>
struct Serialization<vtkm::Massless>
{
  static void write(vtkm::filter::MemStream& memstream, const vtkm::Massless& data)
  {
    vtkm::filter::write(memstream, data.Pos[0]);
    vtkm::filter::write(memstream, data.Pos[1]);
    vtkm::filter::write(memstream, data.Pos[2]);
    vtkm::filter::write(memstream, data.ID);
    vtkm::filter::write(memstream, data.Status);
    vtkm::filter::write(memstream, data.NumSteps);
    vtkm::filter::write(memstream, data.Time);
  }

  static void read(vtkm::filter::MemStream& memstream, vtkm::Massless& data)
  {
    vtkm::filter::read(memstream, data.Pos[0]);
    vtkm::filter::read(memstream, data.Pos[1]);
    vtkm::filter::read(memstream, data.Pos[2]);
    vtkm::filter::read(memstream, data.ID);
    vtkm::filter::read(memstream, data.Status);
    vtkm::filter::read(memstream, data.NumSteps);
    vtkm::filter::read(memstream, data.Time);
  }
};
}
} // namespace vtkm::filter


#endif
