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

#ifndef vtk_m_worklet_Wavelets_h
#define vtk_m_worklet_Wavelets_h

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/worklet/WaveletsFilterBanks.h>

#include <vtkm/Math.h>

namespace vtkm {
namespace worklet {

// Wavelet filter class; 
// functionally equivalent to WaveFiltBase and its subclasses in VAPoR.
class WaveletFilter
{
public:
	// constructor
	WaveletFilter( const std::string &wname )
	{
		lowDecomposeFilter = highDecomposeFilter = 
			lowReconstructFilter = highReconstructFilter = NULL;
		this->filterLength = 0;
		if( wname.compare("CDF9/7") == 0 )
		{
			this->filterLength = 9;
			AllocateFilterMemory();
			wrev( vtkm::worklet::internal::hm4_44,      lowDecomposeFilter, filterLength );
			qmf_wrev( vtkm::worklet::internal::h4,      highDecomposeFilter, filterLength );
			verbatim_copy( vtkm::worklet::internal::h4, lowReconstructFilter, filterLength );
			qmf_even( vtkm::worklet::internal::hm4_44,  highReconstructFilter, filterLength );
		}
		else
		{
			// throw an error here
		}
	}

	// destructor
	virtual ~WaveletFilter()
	{
		if(  lowDecomposeFilter )		  	delete[] lowDecomposeFilter;
		if(  highDecomposeFilter )		  delete[] highDecomposeFilter;
		if(  lowReconstructFilter )			delete[] lowReconstructFilter; 
		if(  highReconstructFilter )		delete[] highReconstructFilter;
	}

protected:
	vtkm::Id				 filterLength;
	vtkm::Float64* 	 lowDecomposeFilter;
	vtkm::Float64* 	 highDecomposeFilter;
	vtkm::Float64*   lowReconstructFilter;
	vtkm::Float64*	 highReconstructFilter;

	void AllocateFilterMemory()
	{
		lowDecomposeFilter    = new vtkm::Float64[ this->filterLength ];
		highDecomposeFilter   = new vtkm::Float64[ this->filterLength ];
		lowReconstructFilter  = new vtkm::Float64[ this->filterLength ];
		highReconstructFilter = new vtkm::Float64[ this->filterLength ];
	}
	
	// Flipping operation; helper function to initialize a filter.
	void wrev( const vtkm::Float64* sigIn, vtkm::Float64* sigOut, vtkm::Id sigLength )
	{
		for( vtkm::Id count = 0; count < sigLength; count++)
			sigOut[count] = sigIn[sigLength - count - 1];
	}

	// Quadrature mirror filtering operation: helper function to initialize a filter.
	void qmf_even ( const vtkm::Float64* sigIn, vtkm::Float64* sigOut, vtkm::Id sigLength )
	{
		for (vtkm::Id count = 0; count < sigLength; count++) 
		{
			sigOut[count] = sigIn[sigLength - count - 1];

			if (sigLength % 2 == 0) {
				if (count % 2 != 0) {
					sigOut[count] = -1.0 * sigOut[count];
				}
			}
			else {
				if (count % 2 == 0) {
					sigOut[count] = -1.0 * sigOut[count];
				}
			}
		}
	}
	
	// Flipping and QMF at the same time: helper function to initialize a filter.
	void qmf_wrev ( const vtkm::Float64* sigIn, vtkm::Float64* sigOut, vtkm::Id sigLength )
	{
		for (vtkm::Id count = 0; count < sigLength; count++) {
			sigOut[count] = sigIn[sigLength - count - 1];

			if (sigLength % 2 == 0) {
				if (count % 2 != 0) {
					sigOut[count] = -1 * sigOut[count];
				}
			}
			else {
				if (count % 2 == 0) {
					sigOut[count] = -1 * sigOut[count];
				}
			}
		}

		vtkm::Float64 tmp;
		for (vtkm::Id count = 0; count < sigLength/2; count++) {
			tmp = sigOut[count];
			sigOut[count] = sigOut[sigLength - count - 1];
			sigOut[sigLength - count - 1] = tmp;
		}
	}

	// Verbatim Copying: helper function to initialize a filter.
	void verbatim_copy ( const vtkm::Float64* sigIn, vtkm::Float64* sigOut, vtkm::Id sigLength )
	{
		for (vtkm::Id count = 0; count < sigLength; count++)
			sigOut[count] = sigIn[count];
	}
};	// class Filter

class Wavelets
{
public:
	
  // helper worklet
  class ForwardTransform: public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // sigIn
                                  WholeArrayIn<Scalar>,        // lowFilter
                                  WholeArrayIn<Scalar>,        // highFilter
                                  FieldOut<ScalarAll>);        // cA in even indices, 
                                                               // cD in odd indices
    typedef void ExecutionSignature(_1, _2, _3, _4, WorkIndex);
    typedef _1   InputDomain;

    // ForwardTransform constructor
    VTKM_CONT_EXPORT
    ForwardTransform() 
    {
      magicNum  = 3.14159265;
      filterLen = 9;
      oddlow    = oddhigh = false;
      xlstart   = xhstart = 0;
    }

    // Specify odd or even for low and high coeffs
    VTKM_CONT_EXPORT
    void SetOddness(const bool &odd_low, const bool &odd_high )
    {
      this->oddlow  = odd_low;
      this->xlstart = odd_low ? 1 : 0;

      this->oddhigh = odd_high;
      this->xhstart = odd_high ? 1 : 0;
    }

    // Set the filter length
    VTKM_CONT_EXPORT
    void SetFilterLength(const vtkm::Id &len )
    {
      this->filterLen = len;
    }

    // Use 64-bit float for internal calculation
    #define VAL        vtkm::Float64
    #define MAKEVAL(a) (static_cast<VAL>(a))

    template <typename InputSignalPortalType,
              typename FilterPortalType,
              typename OutputCoeffType>
    VTKM_EXEC_EXPORT
    void operator()(const InputSignalPortalType &signalIn, 
                    const FilterPortalType      &lowFilter,
                    const FilterPortalType      &highFilter,
                    OutputCoeffType &coeffOut,
                    const vtkm::Id &workIndex) const
    {
      if( workIndex % 2 == 0 )    // calculate cA, approximate coeffs
      {
        VAL sum=MAKEVAL(0.0);
        vtkm::Id xl = xlstart + workIndex;
        if( xl + filterLen < signalIn.GetNumberOfValues() )
        {
          for( vtkm::Id k = filterLen - 1; k >= 0; k-- )
            sum += lowFilter.Get(k) * MAKEVAL( signalIn.Get(xl++) );
          coeffOut = static_cast<OutputCoeffType>( sum );
        }
        else
          coeffOut = static_cast<OutputCoeffType>( magicNum );
      }
      else                        // calculate cD, detail coeffs
      {
        VAL sum=MAKEVAL(0.0);
        vtkm::Id xh = xhstart + workIndex - 1;
        if( xh + filterLen < signalIn.GetNumberOfValues() )
        {
          for( vtkm::Id k = filterLen - 1; k >= 0; k-- )
            sum += highFilter.Get(k) * MAKEVAL( signalIn.Get(xh++) );
          coeffOut = static_cast<OutputCoeffType>( sum );
        }
        else
          coeffOut = static_cast<OutputCoeffType>( magicNum );
      }
    }

    #undef MAKEVAL
    #undef VAL

  private:
    vtkm::Float64 magicNum;
    vtkm::Id      filterLen, xlstart, xhstart;
    bool oddlow, oddhigh;

  };  // class ForwardTransform

};    // class Wavelets

}     // namespace worlet
}     // namespace vtkm

#endif // vtk_m_worklet_Wavelets_h
