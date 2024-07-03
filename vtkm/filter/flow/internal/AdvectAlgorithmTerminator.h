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
public:
#ifdef VTKM_ENABLE_MPI
  AdvectAlgorithmTerminator(vtkmdiy::mpi::communicator& comm)
    : MPIComm(vtkmdiy::mpi::mpi_cast(comm.handle()))
#else
  AdvectAlgorithmTerminator(vtkmdiy::mpi::communicator& vtkmNotUsed(comm))
#endif
  {
  }

  void AddWork()
  {
#ifdef VTKM_ENABLE_MPI
    this->Dirty = 1;
#endif
  }

  bool Done() const { return this->State == AdvectAlgorithmTerminatorState::DONE; }

  void Control(bool haveLocalWork)
  {
#ifdef VTKM_ENABLE_MPI
    if (this->State == STATE_0 && !haveLocalWork)
    {
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
        int localDirty = this->Dirty;
        MPI_Iallreduce(
          &localDirty, &this->AllDirty, 1, MPI_INT, MPI_LOR, this->MPIComm, &this->StateReq);
        this->State = STATE_2;
      }
    }
    else if (this->State == STATE_2)
    {
      MPI_Status status;
      int flag;
      MPI_Test(&this->StateReq, &flag, &status);
      if (flag == 1)
      {
        if (this->AllDirty == 0) //done
          this->State = DONE;
        else
          this->State = STATE_0; //reset.
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
    STATE_2,
    DONE
  };

  AdvectAlgorithmTerminatorState State = AdvectAlgorithmTerminatorState::STATE_0;

#ifdef VTKM_ENABLE_MPI
  std::atomic<int> Dirty;
  int AllDirty = 0;
  MPI_Request StateReq;
  MPI_Comm MPIComm;
#endif
};


}
}
}
} //vtkm::filter::flow::internal


#endif //vtk_m_filter_flow_internal_AdvectAlgorithmTerminator_h
