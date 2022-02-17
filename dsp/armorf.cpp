
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


//
// The armorf_t() function is a straight port of the original Matlab
// code implementing Morf et al as part of the BSMART Matlab Toolbox:
//   armorf.m    https://brain-smart.org
//

//% ARMORF   AR parameter estimation via LWR method by Morf modified.
//%    x is a matrix whose every row is one variable's time series
//%    Nr is the number of realizations, Nl is the length of every realization
//%    If the time series are stationary long, just let Nr=1, Nl=length(x)
//%    p is the order of AR model 
//%       

//%    A = ARMORF(X,NR,NL,P) returns the polynomial coefficients A corresponding to
//%      the AR model estimate of matrix X using Morf's method.                                                   

//%
//%   [A,E] = ARMORF(...) returns the final prediction error E (the
//%   covariance matrix of the white noise of the AR model).
//%

//%   [A,E,K] = ARMORF(...) returns the vector K of reflection
//%     coefficients (parcor coefficients).
//%

//%   Ref: M. Morf, etal, Recursive Multichannel Maximum Entropy Spectral Estimation,
//%              IEEE trans. GeoSci. Elec., 1978, Vol.GE-16, No.2, pp85-94.

//%        S. Haykin, Nonlinear Methods of Spectral Analysis, 2nd Ed.
//%              Springer-Verlag, 1983, Chapter 2
//% 

//%   finished on Aug.9, 2002 by Yonghong Chen


#include "dsp/gc.h"

#include <iostream>
#include "stats/Eigen/Cholesky"
#include <vector>

armorf_t::armorf_t( const Eigen::MatrixXd & X , 
		    const int Nr , 
		    const int Nl , 
		    const int p ) 
{

  // Note: lots of horrible 0-based versus 1-based confusions here in the matlab->C port...
  
  const int L = X.cols();
  const int N = X.rows();

  Eigen::MatrixXd R0 = Eigen::MatrixXd::Zero( L , L );
  Eigen::MatrixXd R0f = Eigen::MatrixXd::Zero( L , L );
  Eigen::MatrixXd R0b = Eigen::MatrixXd::Zero( L , L );

  Eigen::MatrixXd pf = Eigen::MatrixXd::Zero( L , L );
  Eigen::MatrixXd pb = Eigen::MatrixXd::Zero( L , L );
  Eigen::MatrixXd pfb = Eigen::MatrixXd::Zero( L , L );

  std::vector<Eigen::MatrixXd> ap( p + 2 ); 
  std::vector<Eigen::MatrixXd> bp( p + 2 ); 

  for (int m=0;m<p+2; m++)
    {
      ap[m] = Eigen::MatrixXd::Zero( L , L );
      bp[m] = Eigen::MatrixXd::Zero( L , L );
    }

  Eigen::MatrixXd En = Eigen::MatrixXd::Zero( L , L );
  
  for (int i=0; i<Nr; i++)
    {
      En += X.block( i*Nl , 0 , Nl , L ).transpose() * X.block( i*Nl , 0 , Nl , L ) ; 
      ap[0] += X.block( i*Nl+1 , 0 , Nl-1 , L ).transpose() * X.block( i*Nl+1 , 0 , Nl-1 , L ) ; 
      bp[0] += X.block( i*Nl , 0 , Nl-1 , L ).transpose() * X.block( i*Nl , 0 , Nl-1 , L ) ;             
  }
  
  Eigen::MatrixXd I = Eigen::MatrixXd::Identity( L , L );  
  ap[0] /= Nr;
  ap[0] *= Nl-1;  
  Eigen::MatrixXd ap2 = ap[0].llt().matrixL(); 
  ap[0] = ap2.inverse();

  bp[0] /= Nr;
  bp[0] *= Nl-1;
  Eigen::MatrixXd bp2 = bp[0].llt().matrixL();
  bp[0] = bp2.inverse();
  
  for (int i=0; i<Nr; i++)
    {
      Eigen::MatrixXd efp = ap[0] * X.block( i*Nl+1 , 0 , Nl-1 , L ).transpose();
      Eigen::MatrixXd ebp = bp[0] * X.block( i*Nl , 0 , Nl-1 , L ).transpose();
      pf += efp * efp.transpose();
      pb += ebp * ebp.transpose();
      pfb += efp * ebp.transpose();
    }
  
 En /= N;
 En = En.llt().matrixL();

 //  Initial output variables
 // will both grow to be L rows by P*L cols, so set here
 
 coeff = Eigen::MatrixXd::Zero( L , p * L );
 Eigen::MatrixXd kr = Eigen::MatrixXd::Zero( L , p * L );

 for (int m=0; m<p; m++)
   {
     
     const int mp1 = m + 1;

     // % Calculate the next order reflection (parcor) coefficient
     // note : matrixL() gives lower, whereas Matlab chol() gives upper triangular
     Eigen::MatrixXd chol_pf = pf.llt().matrixL();
     Eigen::MatrixXd chol_pb = pb.llt().matrixL();
     
     Eigen::MatrixXd ck = chol_pf.inverse() * pfb * chol_pb.transpose().inverse();
     kr.block( 0 , m*L , L , L ) = ck;

     // Update the forward and backward prediction errors

     Eigen::MatrixXd ef = Eigen::MatrixXd::Identity(L,L) - ck * ck.transpose();

     Eigen::MatrixXd eb = Eigen::MatrixXd::Identity(L,L) - ck.transpose() * ck;

     // Update the prediction error
   
     Eigen::MatrixXd chol_ef = ef.llt().matrixL();

     En = En * chol_ef;

     //    Update the coefficients of the forward and backward prediction errors

     ap[mp1] = Eigen::MatrixXd::Zero(L,L); 
     bp[mp1] = Eigen::MatrixXd::Zero(L,L);
     pf = Eigen::MatrixXd::Zero(L,L);
     pb = Eigen::MatrixXd::Zero(L,L);
     pfb = Eigen::MatrixXd::Zero(L,L);
     
     std::vector<Eigen::MatrixXd> a( mp1+1 );
     std::vector<Eigen::MatrixXd> b( mp1+1 );

     for (int i=0; i<mp1+1; i++)
       {
	 Eigen::MatrixXd chol_ef = ef.llt().matrixL();
	 Eigen::MatrixXd chol_eb = eb.llt().matrixL();
	 a[i] = chol_ef.inverse() * ( ap[i] - ck * bp[m+1-i] ) ; // nb. +1 not +2
	 b[i] = chol_eb.inverse() * ( bp[i] - ck.transpose() * ap[m+1-i] ) ; 	
       }
        
     for (int k=1; k<=Nr; k++)
       {
	 Eigen::MatrixXd efp = Eigen::MatrixXd::Zero( L, Nl-m-2);
	 Eigen::MatrixXd ebp = Eigen::MatrixXd::Zero( L, Nl-m-2);
	 
	 for (int i=0; i<mp1+1; i++)
	   {

	     // nb. edits for m and i 0-base encoding (not k)
	     const int k1 = mp1+2-(i+1)+(k-1)*Nl+1;  
	     const int k2 = Nl-i+(k-1)*Nl; 

	     efp += a[i] * X.block( k1-1 , 0 , k2-k1+1 , L ).transpose();
	     ebp += b[m+1-i] * X.block( k1-2 , 0 , k2-k1+1 , L ).transpose(); // nb. m+1-i 
	   }

	 pf += efp * efp.transpose();
	 pb += ebp * ebp.transpose();
	 pfb += efp * ebp.transpose();

       }

     // update but keep size of ap and bp
     //  (i.e. matlab code automatically resizes)

     for (int j=0; j<a.size(); j++)
       {
	 ap[j] = a[j];
	 bp[j] = b[j];
       }

   }
 
 // nb. use 'ap' as in scope here
 for (int j=0; j<p; j++)
   {
     coeff.block( 0 , j * L , L , L ) = ap[0].inverse() * ap[j+1] ;
   }
 
 // reverse signs
 coeff *= -1;

 // save E
 E = En * En.transpose();

 // parcor : kr
 
}

