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

#ifndef vtk_m_worklet_PICS_h
#define vtk_m_worklet_PICS_h

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/ScatterUniform.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/exec/ExecutionWholeArray.h>

namespace vtkm {
namespace worklet {

enum ParticleStatus
{
    OK=0,
    TERMINATE,
    AT_SPATIAL_BOUNDARY,
    AT_TEMPORAL_BOUNDARY,
    EXIT_SPATIAL_BOUNDARY,
    EXIT_TEMPORAL_BOUNDARY,
    NUMERICAL_ERROR
};

class GridEvaluate
{
public:
    enum Status
    {
        OK=0,
        OUTSIDE_SPATIAL,
        OUTSIDE_TEMPORAL,
        FAIL
    };
};

template <typename PortalType, typename DeviceAdapter>
class RegularGridEvaluate : public GridEvaluate
{
public:
    RegularGridEvaluate(const vtkm::cont::DataSet &ds)
    {
        bounds = ds.GetCoordinateSystem(0).GetBounds();
        vtkm::cont::CellSetStructured<3> cells;
        ds.GetCellSet(0).CopyTo(cells);
        dims = cells.GetSchedulingRange(vtkm::TopologyElementTagPoint());
        planeSize = dims[0]*dims[1];
        rowSize = dims[0];
        vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3> > fieldArray;
        ds.GetField(0).GetData().CopyTo(fieldArray);
        vecData = fieldArray.PrepareForInput(DeviceAdapter());
    }

    template<typename FieldType>
    bool
    Evaluate(const vtkm::Vec<FieldType, 3> &pos,
             vtkm::Vec<FieldType,3> &out) const
    {
        if (!bounds.Contains(pos))
        {
            //std::cout<<pos<<" : "<<bounds<<" --> OOB"<<std::endl;
            return false;
        }
        
        //DRP:: This all assumes bounds of [0,n] in x,y,z. Need to fix this for the general case.
        //Also, I don't think this interpolation is right. (0,0,0) doesn't give the right vector.
        
        // Set the eight corner indices with no wraparound
        vtkm::Id3 idx000, idx001, idx010, idx011, idx100, idx101, idx110, idx111;
        idx000[0] = static_cast<vtkm::Id>(floor(pos[0]));
        idx000[1] = static_cast<vtkm::Id>(floor(pos[1]));
        idx000[2] = static_cast<vtkm::Id>(floor(pos[2]));

        idx001 = idx000; idx001[0] = (idx001[0] + 1) <= dims[0] - 1 ? idx001[0] + 1 : dims[0] - 1;
        idx010 = idx000; idx010[1] = (idx010[1] + 1) <= dims[1] - 1 ? idx010[1] + 1 : dims[1] - 1;
        idx011 = idx010; idx011[0] = (idx011[0] + 1) <= dims[0] - 1 ? idx011[0] + 1 : dims[0] - 1;
        idx100 = idx000; idx100[2] = (idx100[2] + 1) <= dims[2] - 1 ? idx100[2] + 1 : dims[2] - 1;
        idx101 = idx100; idx101[0] = (idx101[0] + 1) <= dims[0] - 1 ? idx101[0] + 1 : dims[0] - 1;
        idx110 = idx100; idx110[1] = (idx110[1] + 1) <= dims[1] - 1 ? idx110[1] + 1 : dims[1] - 1;
        idx111 = idx110; idx111[0] = (idx111[0] + 1) <= dims[0] - 1 ? idx111[0] + 1 : dims[0] - 1;

        // Get the vecdata at the eight corners
        vtkm::Vec<FieldType, 3> v000, v001, v010, v011, v100, v101, v110, v111;
        v000 = vecData.Get(idx000[2] * planeSize + idx000[1] * rowSize + idx000[0]);
        v001 = vecData.Get(idx001[2] * planeSize + idx001[1] * rowSize + idx001[0]);
        v010 = vecData.Get(idx010[2] * planeSize + idx010[1] * rowSize + idx010[0]);
        v011 = vecData.Get(idx011[2] * planeSize + idx011[1] * rowSize + idx011[0]);
        v100 = vecData.Get(idx100[2] * planeSize + idx100[1] * rowSize + idx100[0]);
        v101 = vecData.Get(idx101[2] * planeSize + idx101[1] * rowSize + idx101[0]);
        v110 = vecData.Get(idx110[2] * planeSize + idx110[1] * rowSize + idx110[0]);
        v111 = vecData.Get(idx111[2] * planeSize + idx111[1] * rowSize + idx111[0]);

        //std::cout<<"idx000: "<<idx000<<std::endl;
        //std::cout<<idx001<<idx010<<idx011<<idx100<<idx101<<idx110<<idx111<<std::endl;
        //std::cout<<v000<<v001<<v010<<v011<<v100<<v101<<v110<<v111<<std::endl;
        
        // Interpolation in X
        vtkm::Vec<FieldType, 3> v00, v01, v10, v11;
        FieldType a = pos[0] - static_cast<FieldType>(floor(pos[0]));
        //std::cout<<"Xa: "<<a<<std::endl;
        v00[0] = (1.0f - a) * v000[0] + a * v001[0];
        v00[1] = (1.0f - a) * v000[1] + a * v001[1];
        v00[2] = (1.0f - a) * v000[2] + a * v001[2];

        v01[0] = (1.0f - a) * v010[0] + a * v011[0];
        v01[1] = (1.0f - a) * v010[1] + a * v011[1];
        v01[2] = (1.0f - a) * v010[2] + a * v011[2];

        v10[0] = (1.0f - a) * v100[0] + a * v101[0];
        v10[1] = (1.0f - a) * v100[1] + a * v101[1];
        v10[2] = (1.0f - a) * v100[2] + a * v101[2];

        v11[0] = (1.0f - a) * v110[0] + a * v111[0];
        v11[1] = (1.0f - a) * v110[1] + a * v111[1];
        v11[2] = (1.0f - a) * v110[2] + a * v111[2];
        //std::cout<<v00<<v01<<v10<<v11<<std::endl;

        // Interpolation in Y
        vtkm::Vec<FieldType, 3> v0, v1;
        a = pos[1] - static_cast<FieldType>(floor(pos[1]));
        //std::cout<<"Ya: "<<a<<std::endl;        
        v0[0] = (1.0f - a) * v00[0] + a * v01[0];
        v0[1] = (1.0f - a) * v00[1] + a * v01[1];
        v0[2] = (1.0f - a) * v00[2] + a * v01[2];
        
        v1[0] = (1.0f - a) * v10[0] + a * v11[0];
        v1[1] = (1.0f - a) * v10[1] + a * v11[1];
        v1[2] = (1.0f - a) * v10[2] + a * v11[2];
        //std::cout<<v0<<v1<<std::endl;

        // Interpolation in Z
        //vtkm::Vec<FieldType, 3> v;
        a = pos[2] - static_cast<FieldType>(floor(pos[2]));
        //std::cout<<"Za: "<<a<<std::endl;
        out[0] = (1.0f - a) * v0[0] + v1[0];
        out[1] = (1.0f - a) * v0[1] + v1[1];
        out[2] = (1.0f - a) * v0[2] + v1[2];
        //std::cout<<out<<std::endl;

        //std::cout<<pos<<" : "<<bounds<<" --> "<<out<<std::endl;
        return true;
    }
    
private:
    vtkm::Bounds bounds;
    vtkm::Id3 dims;
    vtkm::Id planeSize;
    vtkm::Id rowSize;    
    PortalType vecData;
};

template<typename FieldEvaluateType, typename FieldType>
class RK4Integrator
{
public:

    enum StepState
    {
        STEP_STAGE_0 = 0,
        STEP_STAGE_1 = 1,
        STEP_STAGE_2 = 2,
        STEP_STAGE_3 = 3,
        STEP_FINISHED = 4
    };

    struct IntegrationStep
    {
        IntegrationStep(const vtkm::Vec<FieldType, 3> &start_pt,
                        const FieldType step)
        : start_point(start_pt), end_point(start_pt),
          current_k(0, 0, 0), k_sum(0, 0, 0),
          time_step(step), next_stage(STEP_STAGE_0) {}
        ~IntegrationStep() = default;
        void RestartAt(const vtkm::Vec<FieldType,3> &start)
        {
            start_point = start;
            k_sum[0] = 0;
            k_sum[1] = 0;
            k_sum[2] = 0;
            next_stage = STEP_STAGE_0;
        }

        bool HasFinished() const { return next_stage == STEP_FINISHED; }

        vtkm::Vec<FieldType, 3> start_point;
        vtkm::Vec<FieldType, 3> end_point;
        vtkm::Vec<FieldType, 3> current_k;
        vtkm::Vec<FieldType, 3> k_sum;
        FieldType time_step;
        StepState next_stage;
    }; 

    RK4Integrator(const FieldEvaluateType &field,
                  FieldType _h) : f(field), h(_h), h_2(_h/2.f) {}

    //Need to add status control.
    bool
    Step(const vtkm::Vec<FieldType, 3> &pos,
         vtkm::Vec<FieldType, 3> &out) const
    {
        vtkm::Vec<FieldType, 3> k1, k2, k3, k4, y;

        GridEvaluate::Status s1, s2, s3, s4;
        if (f.Evaluate(pos, k1) &&
            f.Evaluate(pos+h_2*k1, k2) &&
            f.Evaluate(pos+h_2*k2, k3) &&
            f.Evaluate(pos+h*k3, k4))
        {
            out = pos + h/6.0f*(k1+2*k2+2*k3+k4);
            return true;
        }
        
        return false;
    }

    FieldEvaluateType f;
    FieldType h, h_2;
};

template<typename T,typename DeviceAdapterTag>
class IntegralCurve : public vtkm::exec::ExecutionObjectBase
{
private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id>
        ::template ExecutionTypes<DeviceAdapterTag>::Portal IdPortal;    
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<T,3> >
        ::template ExecutionTypes<DeviceAdapterTag>::Portal PosPortal;
public:
    VTKM_CONT
    IntegralCurve() : pos(), steps(), maxSteps(0)
    {
    }
    VTKM_CONT
    IntegralCurve(const IntegralCurve &ic) :
        pos(ic.pos), steps(ic.steps), maxSteps(ic.maxSteps)
    {
    }

    VTKM_CONT
    IntegralCurve(const PosPortal &_pos,
                   const IdPortal &_steps,
                   const vtkm::Id &_maxSteps) : pos(_pos), steps(_steps), maxSteps(_maxSteps)
    {
    }

    VTKM_CONT    
    IntegralCurve(vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > &posArray,
                  const vtkm::Id &_maxSteps) :
        maxSteps(_maxSteps)
    {
        pos = posArray.PrepareForInPlace(DeviceAdapterTag());
        vtkm::Id nPos = posArray.GetNumberOfValues();
        
        s.resize(nPos, 0);
        sa = vtkm::cont::make_ArrayHandle(&s[0], nPos);
        steps = sa.PrepareForInPlace(DeviceAdapterTag());
    }

    VTKM_EXEC
    void TakeStep(const vtkm::Id &idx,
                  const vtkm::Vec<T,3> &pt)
    {
        pos.Set(idx, pt);
        steps.Set(idx, steps.Get(idx)+1);
    }

    VTKM_EXEC
    bool Done(const vtkm::Id &idx)
    {
        return steps.Get(idx) == maxSteps;
    }

    VTKM_EXEC
    vtkm::Vec<T,3> GetPos(const vtkm::Id &idx) const {return pos.Get(idx);}
    VTKM_EXEC
    vtkm::Id GetStep(const vtkm::Id &idx) const {return steps.Get(idx);}

private:
    std::vector<vtkm::Id> s;
    vtkm::cont::ArrayHandle<vtkm::Id> sa;
    vtkm::Id maxSteps;
    
    IdPortal steps;
    PosPortal pos;
};


template<typename T,typename DeviceAdapterTag>
class StateRecordingIntegralCurve : public vtkm::exec::ExecutionObjectBase
{
private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id>
        ::template ExecutionTypes<DeviceAdapterTag>::Portal IdPortal;    
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<T,3> >
        ::template ExecutionTypes<DeviceAdapterTag>::Portal PosPortal;
public:
    VTKM_CONT
    StateRecordingIntegralCurve(const StateRecordingIntegralCurve &s) :
        pos(s.pos), steps(s.steps), maxSteps(s.maxSteps), history(s.history)
    {
    }
    VTKM_CONT
    StateRecordingIntegralCurve() : pos(), steps(), maxSteps(0)
    {
    }

    VTKM_CONT
    StateRecordingIntegralCurve(const PosPortal &_pos,
                                const IdPortal &_steps,
                                const vtkm::Id &_maxSteps) :
        pos(_pos), steps(_steps), maxSteps(_maxSteps)
    {
    }

    VTKM_CONT    
    StateRecordingIntegralCurve(vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > &posArray,
                                const vtkm::Id &_maxSteps) :
        maxSteps(_maxSteps)
    {
        pos = posArray.PrepareForInPlace(DeviceAdapterTag());

        numPos = posArray.GetNumberOfValues();
        s.resize(numPos, 0);
        sa = vtkm::cont::make_ArrayHandle(&s[0], numPos);
        steps = sa.PrepareForInPlace(DeviceAdapterTag());

        vtkm::Id nHist = numPos * maxSteps;
        history = ha.PrepareForOutput(nHist, DeviceAdapterTag());
    }

    VTKM_EXEC
    void TakeStep(const vtkm::Id &idx,
                  const vtkm::Vec<T,3> &pt)
    {
        vtkm::Id loc = idx*maxSteps + steps.Get(idx);
        //std::cout<<"TakeStep("<<idx<<", "<<pt<<"); loc= "<<loc<<" "<<numPos*maxSteps<<std::endl;
        history.Set(loc, pt);
        steps.Set(idx, steps.Get(idx)+1);
    }

    VTKM_EXEC
    bool Done(const vtkm::Id &idx)
    {
        //vtkm::Id s = steps.Get(idx);
        //std::cout<<idx<<" steps= "<<s<<std::endl;
        return steps.Get(idx) >= maxSteps;
    }

    VTKM_EXEC
    vtkm::Vec<T,3> GetPos(const vtkm::Id &idx) const {return pos.Get(idx);}
    VTKM_EXEC
    vtkm::Id GetStep(const vtkm::Id &idx) const {return steps.Get(idx);}
    VTKM_EXEC
    vtkm::Vec<T,3> GetHistory(const vtkm::Id &idx, const vtkm::Id &step) const
    {
        return history.Get(idx*maxSteps+step);
    }    

private:
    vtkm::Id maxSteps, numPos;
    IdPortal steps;
    PosPortal pos, history;

    std::vector<vtkm::Id> s;
    vtkm::cont::ArrayHandle<vtkm::Id> sa;    
    vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > ha;
};
    
template <typename IntegratorType,
          typename FieldType,
          typename DeviceAdapterTag>
class PICSFilter
{
public:
    PICSFilter(const IntegratorType &it,
               std::vector<vtkm::Vec<FieldType,3> > &pts,
               const vtkm::Id &nSteps) : integrator(it), seeds(pts), maxSteps(nSteps)
    {
    }

    class GoPIC : public vtkm::worklet::WorkletMapField
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
            vtkm::Vec<FieldType, 3> p2, p0 = p;

            while (!ic.Done(idx))
            {
                if (integrator.Step(p, p2))
                {
                    ic.TakeStep(idx, p2);
                    p = p2;
                }
                else
                    break;
            }

            p2 = ic.GetPos(idx);
            //std::cout<<"PIC: "<<idx<<" "<<p0<<" --> "<<p2<<" #steps= "<<ic.GetStep(idx)<<std::endl;
        }
        
        GoPIC(const IntegratorType &it) : integrator(it) {}
        
        IntegratorType integrator;
    };
    
    void run()
    {
        vtkm::Id numSeeds = seeds.size();
        std::vector<vtkm::Vec<FieldType,3> > out(numSeeds);
        std::vector<vtkm::Id> steps(numSeeds, 0);

        vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > posArray = vtkm::cont::make_ArrayHandle(&seeds[0], numSeeds);
        vtkm::cont::ArrayHandleIndex idxArray(numSeeds);
        
        GoPIC go(integrator);
        typedef typename vtkm::worklet::DispatcherMapField<GoPIC> goPICDispatcher;
        goPICDispatcher goPICD(go);

        //IntegralCurve<FieldType, DeviceAdapterTag> ic(posArray, maxSteps);
        StateRecordingIntegralCurve<FieldType, DeviceAdapterTag> ic(posArray, maxSteps);
        goPICD.Invoke(idxArray, ic);


#if 0
        if (true)
        {
            int stepCnt = 0;
            for (int i = 0; i < numSeeds; i++)
            {
                int ns = ic.GetStep(i);
                stepCnt += ns;
            }
            std::cout<<"Total num steps: "<<stepCnt<<std::endl;
        }

        if (true)
        {
            for (int i = 0; i < numSeeds; i++)
            {
                int ns = ic.GetStep(i);
                for (int j = 0; j < ns; j++)
                {
                    vtkm::Vec<FieldType,3> p = ic.GetHistory(i, j);
                    std::cout<<p[0]<<" "<<p[1]<<" "<<p[2]<<std::endl;
                    //std::cout<<"   "<<j<<" "<<p<<std::endl;
                }
            }
        }
#endif
    }

private:
    vtkm::Id maxSteps;
    IntegratorType integrator;
    std::vector<vtkm::Vec<FieldType,3> > seeds;
};

}
}

#endif // vtk_m_worklet_PICS_h
