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

#ifndef vtk_m_worklet_particleadvection_ParticleAdvectionFilters_h
#define vtk_m_worklet_particleadvection_ParticleAdvectionFilters_h

#include <vtkm/Types.h>
#include <vtkm/exec/ExecutionObjectBase.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/particleadvection/Particles.h>
#include <vtkm/cont/Timer.h>

namespace vtkm {
namespace worklet {
namespace particleadvection {

template <typename IntegratorType,
          typename FieldType,
          typename DeviceAdapterTag>
class ParticleAdvectionFilter
{
public:
    typedef vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > FieldHandle;
    typedef typename FieldHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst FieldPortalConstType;
    
    ParticleAdvectionFilter(const IntegratorType &it,
                            std::vector<vtkm::Vec<FieldType,3> > &pts,
                            vtkm::cont::DataSet &_ds,
                            const vtkm::Id &nSteps,
                            bool _streamlines) : integrator(it), seeds(pts),
                                                 maxSteps(nSteps), ds(_ds), streamlines(_streamlines)
    {
        vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3> > fieldArray;
        ds.GetField(0).GetData().CopyTo(fieldArray);
        field = fieldArray.PrepareForInput(DeviceAdapterTag());
    }

    ~ParticleAdvectionFilter(){}

    class PICWorklet : public vtkm::worklet::WorkletMapField
    {
    public:
        typedef void ControlSignature(FieldIn<IdType> idx,
                                      ExecObject ic);
        typedef void ExecutionSignature(_1, _2);
        typedef _1 InputDomain;
        
        template<typename IntegralCurveType>
        VTKM_EXEC
        void operator()(const vtkm::Id &idx,
                        IntegralCurveType &ic) const
        {
            vtkm::Vec<FieldType, 3> p = ic.GetPos(idx);
            vtkm::Vec<FieldType, 3> p2;

            while (!ic.Done(idx))
            {
                if (integrator.Step(p, field, p2))
                {
                    ic.TakeStep(idx, p2);
                    p = p2;
                }
                else
                    break;
            }
            //p2 = ic.GetPos(idx);
            //std::cout<<"PIC: "<<idx<<" "<<p0<<" --> "<<p2<<" #steps= "<<ic.GetStep(idx)<<std::endl;
        }
        
        PICWorklet(const IntegratorType &it,
                   const FieldPortalConstType &f) : integrator(it), field(f) {}
        
        IntegratorType integrator;
        FieldPortalConstType field;
    };

    
    FieldPortalConstType field;
    
    void run(bool dumpOutput=false)
    {

        vtkm::Id numSeeds = seeds.size();
        std::vector<vtkm::Vec<FieldType,3> > out(numSeeds);
        std::vector<vtkm::Id> steps(numSeeds, 0);

        vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > posArray = vtkm::cont::make_ArrayHandle(&seeds[0], numSeeds);
        vtkm::cont::ArrayHandle<vtkm::Id> stepArray = vtkm::cont::make_ArrayHandle(&steps[0], numSeeds);
        vtkm::cont::ArrayHandleIndex idxArray(numSeeds);
        
        PICWorklet picW(integrator, field);
        typedef typename vtkm::worklet::DispatcherMapField<PICWorklet> picWDispatcher;
        picWDispatcher picWD(picW);

        if (streamlines)
        {
            vtkm::worklet::particleadvection::StateRecordingParticle<FieldType, DeviceAdapterTag> sl(posArray,
                                                                                                     stepArray,
                                                                                                     maxSteps);
            vtkm::cont::Timer<DeviceAdapterTag> timer;
            picWD.Invoke(idxArray, sl);
            std::cerr<<" ***** Invoke: "<<timer.GetElapsedTime()<<std::endl;
            
            if (dumpOutput)
            {
                for (vtkm::Id i = 0; i < numSeeds; i++)
                {
                    vtkm::Id ns = sl.GetStep(i);
                    for (int j = 0; j < ns; j++)
                    {
                        vtkm::Vec<FieldType,3> pos = sl.GetHistory(i, j);
                        std::cout<<pos[0]<<" "<<pos[1]<<" "<<pos[2]<<std::endl;
                    }
                }
            }
        }
        else
        {
            vtkm::worklet::particleadvection::Particles<FieldType, DeviceAdapterTag> p(posArray,
                                                                                       stepArray,
                                                                                       maxSteps);
            vtkm::cont::Timer<DeviceAdapterTag> timer;
            picWD.Invoke(idxArray, p);
            std::cerr<<" ***** Invoke: "<<timer.GetElapsedTime()<<std::endl;
/*
            vtkm::Id totSteps = 0;
            for (vtkm::Id i = 0; i < numSeeds; i++)
                totSteps += p.GetStep(i);
            std::cerr<<"*** TotSteps= "<<totSteps<<std::endl;
*/

            if (dumpOutput)
            {
                vtkm::Vec<FieldType, 3> pos;
                for (int i = 0; i < numSeeds; i++)
                {
                    pos = seeds[i];
                    std::cout<<pos[0]<<" "<<pos[1]<<" "<<pos[2]<<std::endl;                    
                    pos = p.GetPos(i);
                    std::cout<<pos[0]<<" "<<pos[1]<<" "<<pos[2]<<std::endl;
                }
            }            
        }
    }

private:
    vtkm::Id maxSteps;
    IntegratorType integrator;
    std::vector<vtkm::Vec<FieldType,3> > seeds;
    //StateRecorder *recorder;
    bool streamlines;

    vtkm::cont::DataSet ds;
};

}
}
}

#endif // vtk_m_worklet_particleadvection_ParticleAdvectionFilters_h
    
