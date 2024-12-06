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

class AdvectAlgorithmTerminator
{
  bool FirstCall;

public:
#ifdef VTKM_ENABLE_MPI
  AdvectAlgorithmTerminator(vtkmdiy::mpi::communicator& comm)
    : AllDirty(1)
    , Dirty(1)
    , LocalWork(0)
    , MPIComm(vtkmdiy::mpi::mpi_cast(comm.handle()))
    , Rank(comm.rank())
    , State(STATE_0)
#else
  AdvectAlgorithmTerminator(vtkmdiy::mpi::communicator& vtkmNotUsed(comm))
#endif
  {
    this->FirstCall = true;
  }

  std::string StateToStr() const
  {
    if (this->State == STATE_0)
      return "STATE_0";
    else if (this->State == STATE_1)
      return "STATE_1";
    else if (this->State == STATE_1B)
      return "STATE_1B";
    else if (this->State == STATE_2)
      return "STATE_2";
    else if (this->State == DONE)
      return "STATE_DONE";
    else
      return "******STATE_ERROR";
  }

  void AddWork(int numWork, DebugStreamType& DebugStream)
  {
#if 0
#ifdef VTKM_ENABLE_MPI
    this->LocalWork += numWork;
    this->Dirty = 1;
    //this->State = STATE_0;
    DebugStream<<this->StateToStr()<<": AddWork: localWork= "<<this->LocalWork<<" dirty= "<<this->Dirty<<std::endl;
#endif
#endif
  }
  void RemoveWork(int numWork, DebugStreamType& DebugStream)
  {
#if 0
    //If we are removing work, we better have added it already.
    VTKM_ASSERT(this->LocalWork > 0);
    this->LocalWork -= numWork;
    if (this->LocalWork == 0)
      this->Dirty = 0;
    VTKM_ASSERT(this->LocalWork >= 0);

    DebugStream<<this->StateToStr()<<": RemoveWork: localWork= "<<this->LocalWork<<" dirty= "<<this->Dirty<<std::endl;
#endif
  }

  bool Done() const { return this->State == AdvectAlgorithmTerminatorState::DONE; }

  void Control(bool haveLocalWork, DebugStreamType& DebugStream)
  {
#ifdef VTKM_ENABLE_MPI
    //DebugStream<<this->StateToStr()<<": Control: localWork= "<<haveLocalWork<<std::endl;
    //DebugStream<<"Control: haveLocalWork: "<<haveLocalWork<<" LocalWork= "<<this->LocalWork<<" dirty= "<<this->Dirty<<" State= "<<this->State<<std::endl;
    if (this->FirstCall)
    {
      haveLocalWork = true;
      this->FirstCall = false;
    }

    if (this->State == STATE_2 && haveLocalWork)
    {
      DebugStream << " DEATH" << std::endl;
    }
    if (haveLocalWork)
    {
      this->Dirty = 1;
      DebugStream << this->StateToStr() << ": Control: Have local work" << std::endl;
    }
    else
      DebugStream << this->StateToStr() << ": Control NO WORK Dirty= " << this->Dirty << " local "
                  << this->LocalDirty << std::endl;

    if (this->State == STATE_0 && !haveLocalWork)
    {
      DebugStream << this->StateToStr() << ": Control: --> STATE_1 (no local work), call barrier("
                  << this->BarrierCnt << ") Dirty=0" << std::endl;
      MPI_Ibarrier(this->MPIComm, &this->StateReq);
      this->Dirty = 0;
      this->State = STATE_1;
    }
    else if (this->State == STATE_1)
    {
      MPI_Status status;
      int flag;
      MPI_Test(&this->StateReq, &flag, &status);
      if (flag == 1)
      {
        DebugStream << this->StateToStr() << ": Control: HIT barrier(" << this->BarrierCnt
                    << ") Dirty= " << this->Dirty << std::endl;
        this->State = STATE_1B;
      }
    }
    else if (this->State == STATE_1B)
    {
      DebugStream << this->StateToStr() << ": Control: Check for new work. dirty= " << this->Dirty
                  << std::endl;
      this->LocalDirty = this->Dirty;
      DebugStream << this->StateToStr() << ": Control: call ireduce(" << this->IReduceCnt
                  << ") : localDirty=" << this->LocalDirty << std::endl;
      DebugStream << this->StateToStr() << ": Control: --> STATE_2" << std::endl;
      MPI_Iallreduce(
        &this->LocalDirty, &this->AllDirty, 1, MPI_INT, MPI_LOR, this->MPIComm, &this->StateReq);
      this->State = STATE_2;
      this->BarrierCnt++;
    }
    else if (this->State == STATE_2)
    {
      MPI_Status status;
      int flag;
      MPI_Test(&this->StateReq, &flag, &status);
      if (flag == 1)
      {
        DebugStream << this->StateToStr() << ": Control: HIT ireduce(" << this->IReduceCnt
                    << ") AllDirty= " << this->AllDirty << std::endl;
        if (this->AllDirty == 0) //done
        {
          DebugStream << this->StateToStr() << ": Control: --> DONE allDirty= " << this->AllDirty
                      << " Dirty= " << this->Dirty << std::endl;
          this->State = DONE;
        }
        else
        {
          DebugStream << this->StateToStr() << ": Control: --> STATE_0 allDirty= " << this->AllDirty
                      << " (reset)" << std::endl;
          this->State = STATE_0; //reset.
        }
        this->IReduceCnt++;
      }
    }
#else
    if (!haveLocalWork)
      this->State = DONE;
#endif
  }

private:
  enum AdvectAlgorithmTerminatorState
  {
    STATE_0,
    STATE_1,
    STATE_1B,
    STATE_2,
    DONE
  };

#ifdef VTKM_ENABLE_MPI
  int AllDirty;
  int BarrierCnt = 0;
  int IReduceCnt = 0;
  //Dirty: Has this rank seen any work since entering state?
  std::atomic<int> Dirty;
  int LocalDirty;
  std::atomic<int> LocalWork;
  MPI_Comm MPIComm;
  vtkm::Id Rank;
  AdvectAlgorithmTerminatorState State = AdvectAlgorithmTerminatorState::STATE_0;
  MPI_Request StateReq;
#endif
};


}
}
}
} //vtkm::filter::flow::internal


#endif //vtk_m_filter_flow_internal_AdvectAlgorithmTerminator_h
