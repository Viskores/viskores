//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_AdvectAlgorithmTerminator_h
#define vtk_m_filter_flow_internal_AdvectAlgorithmTerminator_h

#include "DebugStream.h"

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{

#ifdef VTKM_ENABLE_MPI
namespace //anonymous
{
class ParallelAdvectAlgorithmTerminator
{
  static constexpr int UNSET = -1;
  static constexpr int IDLE = 0;
  static constexpr int ACTIVE = 1;
  static constexpr int DONE = 2;

public:
  ParallelAdvectAlgorithmTerminator(vtkmdiy::mpi::communicator& comm)
    : FirstCall(true)
    , MPIComm(vtkmdiy::mpi::mpi_cast(comm.handle()))
    , NumRanks(comm.size())
    , Rank(comm.rank())
    , Token(UNSET)
  {
    this->FromRank = this->Rank - 1;
    this->ToRank = this->Rank + 1;
    if (this->Rank == 0)
      this->FromRank = this->NumRanks - 1;
    if (this->Rank == this->NumRanks - 1)
      this->ToRank = 0;
  }

  void SetStatus(const bool& haveWork, DebugStreamType& debugStream)
  {
    if (this->Token != DONE)
    {
      if (haveWork)
        this->Token = ACTIVE;
      else
        this->Token = IDLE;
    }
    debugStream << "SetStatus from Work: " << this->StatusToStr()
                << " S,R counters= " << this->SendCnt << " " << this->RecvCnt << std::endl;
  }

  bool GetDone(DebugStreamType& debugStream)
  {
    debugStream << std::endl
                << "GetDone0: from/to " << this->FromRank << " " << this->ToRank << " "
                << this->Info() << std::endl;
    // Once we are done, we need to wait for the sends to complete so that the forward neighbor gets the message.
    if (this->Token == DONE && this->SendCnt == 0)
    {
      //If we have an outstanding recv, cancel it.
      if (this->RecvCnt > 0)
      {
        MPI_Cancel(&this->RecvReq);
        MPI_Request_free(&this->RecvReq);
        this->RecvCnt--;
      }
      VTKM_ASSERT(this->RecvCnt == 0);
      debugStream << "GetDone1: from/to " << this->FromRank << " " << this->ToRank << " "
                  << this->Info() << std::endl;
      return true;
    }

    //Otherwise, pass tokens along the ring.
    if (this->Rank == 0)
      this->RingHead(debugStream);
    else
      this->RingBody(debugStream);

    return false;
  }

private:
  // Rank 0 is the head of the ring.
  void RingHead(DebugStreamType& debugStream)
  {
    //Begin the ring on the first call.
    if (this->FirstCall)
    {
      this->PostSendToken(debugStream);
      debugStream << "GetDone1: from/to " << this->FromRank << " " << this->ToRank << " "
                  << this->Info() << std::endl;
      this->FirstCall = false;
      return;
    }

    //If send is done, then post a receive and return.
    if (this->CheckSendComplete(debugStream))
    {
      this->PostRecvToken(debugStream);
      debugStream << "GetDone1: from/to " << this->FromRank << " " << this->ToRank << " "
                  << this->Info() << std::endl;
      return;
    }

    //Tail of ring responds back to head.
    if (this->CheckRecvComplete(debugStream))
    {
      //If the entire ring is IDLE and root is still IDLE, we are done.
      if (this->RecvToken == IDLE && this->Token == IDLE)
      {
        debugStream << "   everyone idle. DONE" << std::endl;
        this->Token = DONE;
      }
      this->RoundCnt++;
      this->PostSendToken(debugStream);
      debugStream << "GetDone1: from/to " << this->FromRank << " " << this->ToRank << " "
                  << this->Info() << " NEW ROUND" << std::endl;
    }
  }

  // Rank 1 to NumRanks-1 is the body of the ring.
  void RingBody(DebugStreamType& debugStream)
  {
    //We always need 1 active recv.
    if (this->RecvCnt == 0)
    {
      this->PostRecvToken(debugStream);
      //debugStream<<"GetDone1: from/to "<<this->FromRank<<" "<<this->ToRank<<" "<<this->Info()<<std::endl;
      //return;
    }

    //Check for completed send.
    this->CheckSendComplete(debugStream);

    // Check if token received from prev neighbor.
    if (this->CheckRecvComplete(debugStream))
    {
      //If previous token is ACTIVE, pass on ACTIVE;
      //If previous token is DONE, pass on DONE.
      //otherwise, previous is IDLE, so pass on my token.
      if (this->RecvToken == ACTIVE)
        this->Token = ACTIVE;
      else if (this->RecvToken == DONE)
        this->Token = DONE;

      //Pass token to forward neighbor.
      this->PostSendToken(debugStream);
      debugStream << "GetDone1: from/to " << this->FromRank << " " << this->ToRank << " "
                  << this->Info() << std::endl;
    }
  }

  bool CheckSendComplete(DebugStreamType& debugStream)
  {
    if (this->SendCnt > 0)
    {
      MPI_Status status;
      int flag;
      int err = MPI_Test(&this->SendReq, &flag, &status);
      if (flag == 1)
      {
        this->SendCnt--;
        debugStream << " Send completed. cnt=" << this->SendCnt << std::endl;
        return true;
      }
    }

    return false;
  }

  bool CheckRecvComplete(DebugStreamType& debugStream)
  {
    if (this->RecvCnt > 0)
    {
      MPI_Status status;
      int flag;
      int err = MPI_Test(&this->RecvReq, &flag, &status);
      if (flag == 1)
      {
        this->RecvCnt--;
        this->RecvToken = this->RecvBuffer[0];
        this->RoundCnt = this->RecvBuffer[1];
        debugStream << " Recv(" << this->FromRank << ") " << this->StatusToStr(this->RecvToken)
                    << " " << this->RoundCnt << " cnt= " << this->RecvCnt << std::endl;
        return true;
      }
    }
    return false;
  }

  void PostSendToken(DebugStreamType& debugStream)
  {
    this->SendToken = this->Token;
    if (this->SendToken == IDLE && (this->SendCnt > 0 || this->RecvCnt > 0))
    {
      debugStream << "   idle --> active" << std::endl;
      this->SendToken = ACTIVE;
    }
    this->SendCnt++;
    this->SendBuffer[0] = this->SendToken;
    this->SendBuffer[1] = this->RoundCnt;
    MPI_Isend(
      &this->SendBuffer, 2, MPI_INT, this->ToRank, this->Tag, this->MPIComm, &this->SendReq);
    if (this->Rank == 0 && this->FirstCall)
      debugStream << " BEGIN ";

    debugStream << " Send(" << this->ToRank << ") " << this->StatusToStr(this->SendToken) << " "
                << this->RoundCnt << " " << &this->SendReq << " cnt= " << this->SendCnt
                << std::endl;
  }

  void PostRecvToken(DebugStreamType& debugStream)
  {
    MPI_Irecv(
      &this->RecvBuffer, 2, MPI_INT, this->FromRank, this->Tag, this->MPIComm, &this->RecvReq);
    this->RecvCnt++;
    debugStream << " Post Irecv(" << this->FromRank << ") " << &this->RecvReq
                << " cnt= " << this->RecvCnt << std::endl;
  }


  int SendCnt = 0;
  int RecvCnt = 0;
  bool FirstCall;
  MPI_Comm MPIComm;
  MPI_Request RecvReq, SendReq;
  int NumRanks;
  int Rank, ToRank, FromRank;
  int Token;
  int SendToken, RecvToken;
  int Tag = 314;
  int RoundCnt = 0;
  int SendBuffer[2], RecvBuffer[2];


  //stuff to toss.
  std::string StatusToStr() const { return this->StatusToStr(this->Token); }

  std::string StatusToStr(const int& status) const
  {
    std::string str;
    if (status == UNSET)
      str = "UNSET";
    else if (status == IDLE)
      str = "IDLE";
    else if (status == ACTIVE)
      str = "ACTIVE";
    else if (status == DONE)
      str = "DONE";
    else
      str = "ERROR " + std::to_string(status);

    return str;
  }

  std::string Info(int token = -100) const
  {
    int tmp = this->Token;
    if (token != -100)
      tmp = token;
    std::string str = this->StatusToStr(tmp) + " S,R cnt= " + std::to_string(this->SendCnt) + " " +
      std::to_string(this->RecvCnt);
    str = str + " roundCnt= " + std::to_string(this->RoundCnt);
    if (this->SendCnt > 1 || this->RecvCnt > 1)
      str = str + " ********** ERROR";
    return str;
  }
};
} //namespace anonymous
#endif

class AdvectAlgorithmTerminator
{
  static constexpr int UNSET = -1;
  static constexpr int IDLE = 0;
  static constexpr int ACTIVE = 1;
  static constexpr int DONE = 2;

public:
#ifdef VTKM_ENABLE_MPI
  AdvectAlgorithmTerminator(vtkmdiy::mpi::communicator& comm)
    : HaveWork(false)
    , ParallelTerminator(comm)
#else
  AdvectAlgorithmTerminator(vtkmdiy::mpi::communicator& vtkmNotUsed(comm))
    : HaveWork(false)
#endif
  {
  }

  void SetStatus(const bool& haveWork, DebugStreamType& debugStream)
  {
    this->HaveWork = haveWork;
#ifdef VTKM_ENABLE_MPI
    this->ParallelTerminator.SetStatus(this->HaveWork, debugStream);
#endif
  }

  bool GetDone(DebugStreamType& debugStream)
  {
#ifdef VTKM_ENABLE_MPI
    return this->ParallelTerminator.GetDone(debugStream);
#else
    return !this->HaveWork;
#endif
  }

private:
  bool HaveWork;
#ifdef VTKM_ENABLE_MPI
  ParallelAdvectAlgorithmTerminator ParallelTerminator;
#endif
};

}
}
}
} //vtkm::filter::flow::internal


#endif //vtk_m_filter_flow_internal_AdvectAlgorithmTerminator_h
