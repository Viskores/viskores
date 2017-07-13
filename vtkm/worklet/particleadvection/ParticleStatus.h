//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_particleadvection_ParticleStatus_h
#define vtk_m_worklet_particleadvection_ParticleStatus_h

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

class ParticleStatus
{
public:
  ParticleStatus()
  {
    Clear();
    SetOK();
  }
  ParticleStatus(const ParticleStatus& s) { status = s.status; }
  ~ParticleStatus() { Clear(); }

  //Get
  bool OK() const { return CheckBit(STATUS_OK); }
  bool TerminatedOnly() const // i.e. terminated only due the maxSteps
  {
    if (Error() || EncounteredSpatialBoundary() || EncounteredTemporalBoundary() ||
        ExitedSpatialBoundary() || ExitedTemporalBoundary())
      return false;
    else if (OK() && Terminated())
      return true;
    else
      return false;
  }
  bool Terminated() const
  {
    return (CheckBit(TERMINATED) || Error() || ExitedSpatialBoundary() || ExitedTemporalBoundary());
  }
  bool Integrateable() const
  {
    return (!Terminated() && !EncounteredSpatialBoundary() && !EncounteredTemporalBoundary());
  }
  bool OutsideBoundary() const
  {
    return EncounteredSpatialBoundary() || EncounteredTemporalBoundary();
  }
  bool EncounteredSpatialBoundary() const { return CheckBit(ENCOUNTERED_SPATIAL_BOUNDARY); }
  bool EncounteredTemporalBoundary() const { return CheckBit(ENCOUNTERED_TEMPORAL_BOUNDARY); }
  bool ExitedSpatialBoundary() const { return CheckBit(EXITED_SPATIAL_BOUNDARY); }
  bool ExitedTemporalBoundary() const { return CheckBit(EXITED_TEMPORAL_BOUNDARY); }
  bool Error() const { return CheckBit(STATUS_ERROR); }

  //Set
  void Clear() { status = 0; }
  void SetOK() { SetBit(STATUS_OK); }
  void SetTerminated() { SetBit(TERMINATED); }
  void SetAtSpatialBoundary() { SetBit(ENCOUNTERED_SPATIAL_BOUNDARY); }
  void SetAtTemporalBoundary() { SetBit(ENCOUNTERED_TEMPORAL_BOUNDARY); }
  void SetExitSpatialBoundary() { SetBit(EXITED_SPATIAL_BOUNDARY); }
  void SetExitTemporalBoundary() { SetBit(EXITED_TEMPORAL_BOUNDARY); }
  void SetError()
  {
    ClearBit(STATUS_OK);
    SetBit(STATUS_ERROR);
  }

  //Clear
  void ClearTerminated() { ClearBit(TERMINATED); }
  void ClearAtSpatialBoundary() { ClearBit(ENCOUNTERED_SPATIAL_BOUNDARY); }
  void ClearExitSpatialBoundary() { ClearBit(EXITED_SPATIAL_BOUNDARY); }
  void ClearSpatialBoundary()
  {
    ClearAtSpatialBoundary();
    ClearExitSpatialBoundary();
  }
  void ClearAtTemporalBoundary() { ClearBit(ENCOUNTERED_TEMPORAL_BOUNDARY); }
  void ClearExitTemporalBoundary() { ClearBit(EXITED_TEMPORAL_BOUNDARY); }
  void ClearTemporalBoundary()
  {
    ClearAtTemporalBoundary();
    ClearExitTemporalBoundary();
  }
  void ClearError() { ClearBit(STATUS_ERROR); }

  unsigned long GetStatus() const { return status; };

private:
  //bit assignments:
  //1:   OK
  //2:   Terminated
  //3,4: At spatial/temporal boundary
  //5,6: Exited spatial/temporal boundary
  //7:   Error
  unsigned long status;

  enum ParticleStatusBits
  {
    STATUS_OK = 0x0001,
    TERMINATED = 0x0002,
    ENCOUNTERED_SPATIAL_BOUNDARY = 0x0004,
    ENCOUNTERED_TEMPORAL_BOUNDARY = 0x008,
    EXITED_SPATIAL_BOUNDARY = 0x0010,
    EXITED_TEMPORAL_BOUNDARY = 0x0020,
    STATUS_ERROR = 0x0040,
  };

  void SetBit(const ParticleStatusBits& b) { status |= b; }
  void ClearBit(const ParticleStatusBits& b) { status &= ~b; }
  bool CheckBit(const ParticleStatusBits& b) const { return status & b; }
};

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif //vtk_m_worklet_particleadvection_ParticleStatus_h
