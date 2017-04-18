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

#ifndef vtk_m_worklet_particleadvection_Particles_h
#define vtk_m_worklet_particleadvection_Particles_h

#include <vtkm/Types.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/exec/ExecutionObjectBase.h>

namespace vtkm {
namespace worklet {
namespace particleadvection {

template<typename T,typename DeviceAdapterTag>
class Particles : public vtkm::exec::ExecutionObjectBase
{
private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id>
        ::template ExecutionTypes<DeviceAdapterTag>::Portal IdPortal;    
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<T,3> >
        ::template ExecutionTypes<DeviceAdapterTag>::Portal PosPortal;
public:
    VTKM_EXEC_CONT
    Particles() : pos(), steps(), maxSteps(0)
    {
    }
    VTKM_EXEC_CONT    
    Particles(const Particles &ic) :
        pos(ic.pos), steps(ic.steps), maxSteps(ic.maxSteps)
    {
    }

    VTKM_EXEC_CONT        
    Particles(const PosPortal &_pos,
              const IdPortal &_steps,
              const vtkm::Id &_maxSteps) : pos(_pos), steps(_steps), maxSteps(_maxSteps)
    {
    }

    VTKM_EXEC_CONT            
    Particles(vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > &posArray,
              vtkm::cont::ArrayHandle<vtkm::Id> &stepsArray,
              const vtkm::Id &_maxSteps) :
        maxSteps(_maxSteps)
    {
        pos = posArray.PrepareForInPlace(DeviceAdapterTag());
        steps = stepsArray.PrepareForInPlace(DeviceAdapterTag());
    }

    VTKM_EXEC_CONT                
    void TakeStep(const vtkm::Id &idx,
                  const vtkm::Vec<T,3> &pt)
    {
        pos.Set(idx, pt);
        steps.Set(idx, steps.Get(idx)+1);
    }

    VTKM_EXEC_CONT                    
    bool Done(const vtkm::Id &idx)
    {
        return steps.Get(idx) == maxSteps;
    }

    VTKM_EXEC_CONT                        
    vtkm::Vec<T,3> GetPos(const vtkm::Id &idx) const {return pos.Get(idx);}    
    
private:
    vtkm::Id maxSteps;
    IdPortal steps;
    PosPortal pos;
};

}
}
}


#endif // vtk_m_worklet_particleadvection_Particles_h
