//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_ParticleExchanger_h
#define vtk_m_filter_flow_internal_ParticleExchanger_h

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{

template <typename ParticleType>
class ParticleExchanger
{
public:
#ifdef VTKM_ENABLE_MPI
  ParticleExchanger(vtkmdiy::mpi::communicator& comm)
    : MPIComm(vtkmdiy::mpi::mpi_cast(comm.handle()))
    , NumRanks(comm.size())
    , Rank(comm.rank())
#else
  ParticleExchanger(vtkmdiy::mpi::communicator& vtkmNotUsed(comm))
#endif
  {
  }
#ifdef VTKM_ENABLE_MPI
  ~ParticleExchanger() { this->CleanupSendBuffers(); }
#endif

  void Exchange(const std::vector<ParticleType>& outData,
                const std::vector<vtkm::Id>& outRanks,
                const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
                std::vector<ParticleType>& inData,
                std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap,
                bool blockAndWait)
  {
    VTKM_ASSERT(outData.size() == outRanks.size());

    if (this->NumRanks == 1)
      this->SerialExchange(outData, outBlockIDsMap, inData, inDataBlockIDsMap);
#ifdef VTKM_ENABLE_MPI
    else
    {
      this->SendParticles(outData, outRanks, outBlockIDsMap);
      this->RecvParticles(inData, inDataBlockIDsMap, blockAndWait);
      this->CleanupSendBuffers();
    }
#endif
  }

private:
  // pair(vector of particles, vector of blockIds)
  //using ParticleCommType = std::pair<std::vector<ParticleType>, std::vector<vtkm::Id>>;
  // pair(particle, bids);
  using ParticleCommType = std::pair<ParticleType, std::vector<vtkm::Id>>;

  void CleanupSendBuffers() { std::cout << "IMPLEMENT ME!!!" << std::endl; }

  void SendParticles(const std::vector<ParticleType>& outData,
                     const std::vector<vtkm::Id>& outRanks,
                     const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap)
  {
    if (outData.empty())
      return;

    //create the send data: vector of particles, vector of vector of blockIds.
    std::size_t n = outData.size();
    std::unordered_map<int, std::vector<ParticleCommType>> sendData;

    // dst, vector of pair(particles, blockIds)
    for (std::size_t i = 0; i < n; i++)
    {
      const auto& bids = outBlockIDsMap.find(outData[i].GetID())->second;
      sendData[outRanks[i]].emplace_back(std::make_pair(std::move(outData[i]), std::move(bids)));
    }

    //Send to dst, vector<pair<particle, bids>>
    for (auto& si : sendData)
      this->SendParticlesToDst(si.first, si.second);
  }

  void SendParticlesToDst(int dst, const std::vector<ParticleCommType>& data)
  {
    if (dst == this->Rank)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Error, "Error. Sending a particle to yourself.");
      return;
    }

    //Serialize vector(pair(particle, bids)) and send.
    vtkmdiy::MemoryBuffer* bb = new vtkmdiy::MemoryBuffer();
    vtkmdiy::save(*bb, data);

    MPI_Request req;
    int err = MPI_Isend(bb->buffer.data(), bb->size(), MPI_BYTE, dst, 0, this->MPIComm, &req);
    if (err != MPI_SUCCESS)
      throw vtkm::cont::ErrorFilterExecution("Error in MPI_Isend inside Messenger::SendData");
    this->SendBuffers[req] = bb;
  }

  void RecvParticles(std::vector<ParticleType>& inData,
                     std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap,
                     bool blockAndWait) const
  {
    inData.resize(0);
    inDataBlockIDsMap.clear();

    std::vector<vtkmdiy::MemoryBuffer> buffers;

    MPI_Status status;
    while (true)
    {
      bool msgReceived = false;
      int err;
      if (blockAndWait)
      {
        err = MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, this->MPIComm, &status);
        if (err != MPI_SUCCESS)
          throw vtkm::cont::ErrorFilterExecution(
            "Error in MPI_Probe in ParticleExchanger::RecvParticles");
        msgReceived = true;
      }
      else
      {
        int flag = 0;
        err = MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, this->MPIComm, &flag, &status);
        if (err != MPI_SUCCESS)
          throw vtkm::cont::ErrorFilterExecution(
            "Error in MPI_Probe in ParticleExchanger::RecvParticles");
        msgReceived = (flag == 1);
      }

      if (msgReceived)
      {
        int incomingSize;
        err = MPI_Get_count(&status, MPI_BYTE, &incomingSize);
        if (err != MPI_SUCCESS)
          throw vtkm::cont::ErrorFilterExecution(
            "Error in MPI_Probe in ParticleExchanger::RecvParticles");

        std::vector<char> recvBuff;
        recvBuff.resize(incomingSize);
        MPI_Status recvStatus;

        err = MPI_Recv(recvBuff.data(),
                       incomingSize,
                       MPI_BYTE,
                       status.MPI_SOURCE,
                       status.MPI_TAG,
                       this->MPIComm,
                       &recvStatus);
        if (err != MPI_SUCCESS)
          throw vtkm::cont::ErrorFilterExecution(
            "Error in MPI_Probe in ParticleExchanger::RecvParticles");

        vtkmdiy::MemoryBuffer memBuff;
        vtkmdiy::save(memBuff, recvBuff);
        buffers.emplace_back(std::move(memBuff));

        blockAndWait = false; //Check one more time to see if anything else arrived.
      }
      else
      {
        break;
      }
    }

    //Unpack buffers into particle data.
    //buffers: vector<pair(particle, vector<vtkm::Id>)>
    for (auto& b : buffers)
    {
      std::vector<ParticleCommType> data;
      vtkmdiy::load(b, data);

      for (auto& d : data)
      {
        const auto& particle = d.first;
        const auto& bids = d.second;
        inDataBlockIDsMap[particle.GetID()] = std::move(bids);
        inData.emplace_back(std::move(particle));
      }
    }
  }

  void SerialExchange(const std::vector<ParticleType>& outData,
                      const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
                      std::vector<ParticleType>& inData,
                      std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap)
  {
    //Copy output to input.
    for (const auto& p : outData)
    {
      const auto& bids = outBlockIDsMap.find(p.GetID())->second;
      inData.emplace_back(p);
      inDataBlockIDsMap[p.GetID()] = bids;
    }
  }


#ifdef VTKM_ENABLE_MPI
  MPI_Comm MPIComm;
  vtkm::Id NumRanks;
  vtkm::Id Rank;
  std::unordered_map<MPI_Request, vtkmdiy::MemoryBuffer*> SendBuffers;
#else
  vtkm::Id NumRanks = 1;
  vtkm::Id Rank = 0;
#endif
};

}
}
}
} //vtkm::filter::flow::internal


#endif //vtk_m_filter_flow_internal_ParticleExchanger_h
