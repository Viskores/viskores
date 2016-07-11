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
			this->symmetricity= true;
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

	vtkm::Id GetFilterLength()		{ return this->filterLength; }
	bool		 isSymmetric()				{ return this->symmetricity;	 }

protected:
	bool						 symmetricity;
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
				if (count % 2 != 0) 
					sigOut[count] = -1.0 * sigOut[count];
			}
			else {
				if (count % 2 == 0) 
					sigOut[count] = -1.0 * sigOut[count];
			}
		}
	}
	
	// Flipping and QMF at the same time: helper function to initialize a filter.
	void qmf_wrev ( const vtkm::Float64* sigIn, vtkm::Float64* sigOut, vtkm::Id sigLength )
	{
		for (vtkm::Id count = 0; count < sigLength; count++) {
			sigOut[count] = sigIn[sigLength - count - 1];

			if (sigLength % 2 == 0) {
				if (count % 2 != 0) 
					sigOut[count] = -1 * sigOut[count];
			}
			else {
				if (count % 2 == 0) 
					sigOut[count] = -1 * sigOut[count];
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
};	// Finish class WaveletFilter.


class WaveletBase
{
public:
	// Constructor
	WaveletBase( const std::string &w_name )
	{
		filter = NULL;
		this->wmode = PER;
		this->wname = w_name;
		if( wname.compare("CDF9/7") == 0 )
		{
			this->wmode = SYMW;
			filter = new vtkm::worklet::WaveletFilter( wname );
		}
	}

	// Destructor
	virtual ~WaveletBase()
	{
		if( filter )	
			delete filter;
		filter = NULL;
	}

	// Get the wavelet filter
	const vtkm::worklet::WaveletFilter* GetWaveletFilter() { return filter; }

	// Returns length of approximation coefficients from a decompostition pass.
	vtkm::Id GetApproxLength( vtkm::Id sigInLen )
	{
		vtkm::Id filterLen = this->filter->GetFilterLength();

		if (this->wmode == PER) 
			return static_cast<vtkm::Id>(vtkm::Ceil( (static_cast<vtkm::Float64>(sigInLen)) / 2.0 ));
		else if (this->filter->isSymmetric()) 
		{
			if ( (this->wmode == SYMW && (filterLen % 2 != 0)) ||
				   (this->wmode == SYMH && (filterLen % 2 == 0)) )  
			{
				if (sigInLen % 2 != 0)
					return((sigInLen+1) / 2);
				else 
					return((sigInLen) / 2);
			}
		}

		return static_cast<vtkm::Id>( vtkm::Floor(
					 static_cast<vtkm::Float64>(sigInLen + filterLen - 1) / 2.0 ) );
	}

	// Returns length of detail coefficients from a decompostition pass
	vtkm::Id GetDetailLength( vtkm::Id sigInLen )
	{
		vtkm::Id filterLen = this->filter->GetFilterLength();

		if (this->wmode == PER) 
			return static_cast<vtkm::Id>(vtkm::Ceil( (static_cast<vtkm::Float64>(sigInLen)) / 2.0 ));
		else if (this->filter->isSymmetric()) 
		{
			if ( (this->wmode == SYMW && (filterLen % 2 != 0)) ||
				   (this->wmode == SYMH && (filterLen % 2 == 0)) )  
			{
				if (sigInLen % 2 != 0)
					return((sigInLen-1) / 2);
				else 
					return((sigInLen) / 2);
			}
		}

		return static_cast<vtkm::Id>( vtkm::Floor(
					 static_cast<vtkm::Float64>(sigInLen + filterLen - 1) / 2.0 ) );
	}

	// Returns length of coefficients generated in a decompostition pass
	vtkm::Id GetCoeffLength( vtkm::Id sigInLen )
	{
		return( GetApproxLength( sigInLen ) + GetDetailLength( sigInLen ) );
	}
	vtkm::Id GetCoeffLength2( vtkm::Id sigInX, vtkm::Id sigInY )
	{
		return( GetCoeffLength( sigInX) * GetCoeffLength( sigInY ) );
	}
	vtkm::Id GetCoeffLength3( vtkm::Id sigInX, vtkm::Id sigInY, vtkm::Id sigInZ)
	{
		return( GetCoeffLength( sigInX) * GetCoeffLength( sigInY ) * GetCoeffLength( sigInZ ) );
	}

	// Returns maximum wavelet decompostion level
	vtkm::Id GetWaveletMaxLevel( vtkm::Id s )
	{
		return 0;
	}

protected:
	enum DwtMode {		// boundary extension modes
		INVALID = -1,
		ZPD, 
		SYMH, 
		SYMW,
		ASYMH, ASYMW, SP0, SP1, PPD, PER
	};

private:
  DwtMode 													wmode;
	vtkm::worklet::WaveletFilter* 		filter;
	std::string 											wname;

	void WaveLengthValidate( vtkm::Id sigInLen, vtkm::Id filterLength, vtkm::Id &level)
	{
    // *lev = (int) (log((double) sigInLen / (double) (waveLength)) / log(2.0)) + 1;
		if( sigInLen < filterLength )
			level = 0;
		else
			level = static_cast<vtkm::Id>( vtkm::Floor( 
									vtkm::Log2( static_cast<vtkm::Float64>(sigInLen) / 
															static_cast<vtkm::Float64>(filterLength) ) + 1.0 ) );
	}
};	// Finish class WaveletBase.

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
      oddlow    = oddhigh   = true;
			filterLen = approxLen = detailLen = 0;
			this->SetStartPosition();
    }

    // Specify odd or even for low and high coeffs
    VTKM_CONT_EXPORT
    void SetOddness(const bool &odd_low, const bool &odd_high )
    {
      this->oddlow  = odd_low;
      this->oddhigh = odd_high;
			this->SetStartPosition();
    }

    // Set the filter length
    VTKM_CONT_EXPORT
    void SetFilterLength(const vtkm::Id &len )
    {
      this->filterLen = len;
    }

    // Set the outcome coefficient length
    VTKM_CONT_EXPORT
    void SetCoeffLength(const vtkm::Id &approx_len, const vtkm::Id &detail_len )
    {
      this->approxLen = approx_len;
      this->detailLen = detail_len;
    }

    // Use 64-bit float for convolution calculation
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
				if( workIndex < approxLen + detailLen )
        {
        	vtkm::Id xl = xlstart + workIndex;
        	VAL sum=MAKEVAL(0.0);
          for( vtkm::Id k = filterLen - 1; k >= 0; k-- )
            sum += lowFilter.Get(k) * MAKEVAL( signalIn.Get(xl++) );
          coeffOut = static_cast<OutputCoeffType>( sum );
        }
        else
          coeffOut = static_cast<OutputCoeffType>( magicNum );
      else                        // calculate cD, detail coeffs
        if( workIndex < approxLen + detailLen )
        {
					VAL sum=MAKEVAL(0.0);
					vtkm::Id xh = xhstart + workIndex - 1;
          for( vtkm::Id k = filterLen - 1; k >= 0; k-- )
            sum += highFilter.Get(k) * MAKEVAL( signalIn.Get(xh++) );
          coeffOut = static_cast<OutputCoeffType>( sum );
        }
        else
          coeffOut = static_cast<OutputCoeffType>( magicNum );
    }

    #undef MAKEVAL
    #undef VAL

  private:
    vtkm::Float64 magicNum;
    vtkm::Id filterLen, approxLen, detailLen;	// filter and outcome coeff length.
		vtkm::Id xlstart, xhstart;
    bool oddlow, oddhigh;
		
    VTKM_CONT_EXPORT
		void SetStartPosition()
		{
      this->xlstart = this->oddlow  ? 1 : 0;
      this->xhstart = this->oddhigh ? 1 : 0;
		}

  };  // Finish class ForwardTransform

};    // Finish class Wavelets

}     // Finish namespace worlet
}     // Finish namespace vtkm

#endif // vtk_m_worklet_Wavelets_h
