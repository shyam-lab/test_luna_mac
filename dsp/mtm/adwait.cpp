
//    --------------------------------------------------------------------
//
//    This file is part of Luna.
//
//    LUNA is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Luna is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Luna. If not, see <http://www.gnu.org/licenses/>.
//
//    Please see LICENSE.txt for more details.
//
//    --------------------------------------------------------------------

// All mtm functions are adapted code from: Lees, J. M. and J. Park
// (1995): Multiple-taper spectral analysis: A stand-alone
// C-subroutine: Computers & Geology: 21, 199-236.


#include <cstdio>
#include <cmath>

#include "mtm.h"

#define ABS(a) ((a) < (0) ? -(a) : (a))
#define DIAG1 0
#define MAX(a,b) ((a) >= (b) ? (a) : (b))


int mtm_t::adwait( double *sqr_spec,  double *dcf,
		   double *el, int nwin, int num_freq, double *ares, double *degf, double avar)
{
  
  // Thomson's algorithm for calculating the adaptive spectrum estimate

  double as,das,a1,ax,fn,fx;
  
  double test_tol, dif;

  double df;
  
  // set tolerance for iterative scheme exit */
  
  const double tol=3.0e-4;

  int jitter = 0;

  const double scale = avar;
  
  //  we scale the bias by the total variance of the frequency transform
  //  from zero freq to the nyquist

  //  in this application we scale the eigenspectra by the bias in order to avoid
  //  possible floating point overflow
  
  std::vector<double> spw( nwin , 0 );
  std::vector<double> bias( nwin , 0 );

  for( int i=0;i<nwin; i++)
    bias[i]=(1.00-el[i]);
  
  for( int jloop=0; jloop< num_freq; jloop++)
    {   
      
      for( int i=0;i<nwin; i++)
	{
	  const int kpoint=jloop+i*num_freq;
	  spw[i]=(sqr_spec[kpoint])/scale ;
	}
      
      
      // first guess is the average of the two 
      // lowest-order eigenspectral estimates
      
      as=(spw[0]+spw[1])/2.00;
      
      // find coefficients

      bool converged = false;
      
      for( int k=0; k<20 ; k++) 
	{
	  
	  fn=0.00;
	  fx=0.00;
	  
	  for( int i=0;i<nwin; i++)
	    {
	      a1=sqrt(el[i])*as/(el[i]*as+bias[i]);
	      a1=a1*a1;
	      fn=fn+a1*spw[i];
	      fx=fx+a1;
	    }
	  
	  ax=fn/fx;
	  dif = ax-as;
	  das=ABS(dif);
	  
	  test_tol = das/as;
	  if( test_tol < tol )
	    {
	      converged = true;
	      break;
	    }
	  
	  as=ax;
	}
      
      //  flag if iteration does not converge

      //if ( k>=20)  jitter++;
      if ( ! converged )  jitter++;
      
      ares[jloop]=as*scale;

      // calculate degrees of freedom

      df=0.0;
      for( int i=0;i< nwin; i++)
	{
	  const int kpoint=jloop+i*num_freq;
	  dcf[kpoint]=sqrt(el[i])*as/(el[i]*as+bias[i]);
	  df=df+dcf[kpoint]*dcf[kpoint];
	}
      
      
      // we normalize degrees of freedom by the weight of
      // the first eigenspectrum this way we never have
      // fewer than two degrees of freedom
      
      degf[jloop]=df*2./(dcf[jloop]*dcf[jloop]);
      
    } 
    
    return jitter;
}
