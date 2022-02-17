
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


#include "clocs.h"
#include "helper/helper.h"
#include "edf/edf.h"
#include "legendre_polynomial.h"
#include "db/db.h"

#include <fstream>
#include <cmath>

extern writer_t writer;

cart_t sph_t::cart() const { return clocs_t::sph2cart( *this ); } 
sph_t cart_t::sph() const { return clocs_t::cart2sph( *this ); } 


polar_t::polar_t( sph_t & sph )
{
  // following EEGLAB sph2topo() 
  // Assumes a spherical coordinate system in which horizontal angles 
  // have a range [-180,180] deg,  with zero pointing to the right ear. 
  // In the output polar coordinate system, zero points to the nose.
  
  // When az>0, horiz=0 -> right ear, 90 -> nose 
  // When az<0, horiz=0 -> left ear, -90 -> nose
  
  // returns:
  // angle   = horizontal angle (0 -> nose; 90 -> right ear; -90 -> left ear)
  // radius  = arc_lengrh from vertex (Note: 90 deg az -> 0.5/shrink_factor);
  // By topoplot() convention, radius=0.5 is the nasion-ear_canal plane.
  // note: topoplot() 'plotrad' to plot chans with abs(az) > 90 deg.
  
  angle  = clocs_t::deg2rad( - clocs_t::rad2deg( sph.elevation ) ) ;
  angle  = clocs_t::deg2rad( - clocs_t::rad2deg( sph.elevation ) ) ;
  radius = 0.5 - clocs_t::rad2deg( sph.azimuth ) / 180.0 ;  
  
}


int clocs_t::load_cart( const std::string & f0 , bool verbose )
{

  std::string filename = Helper::expand( f0 );
  
  if ( ! Helper::fileExists( filename ) ) 
    Helper::halt( "could not find clocs file; " + filename );
  
  cloc.clear();

  // assume LABEL X Y Z 
  std::ifstream IN1( filename.c_str() , std::ios::in );

  // stor channel labels (if needed for verbose output)

  std::vector<std::string> channels;
  
  while ( !IN1.eof() )
    {

      std::string s;
      Helper::safe_getline( IN1 , s );
      if ( IN1.eof() ) break;
      if ( s == "" ) continue;	  
      if ( s[0] == '#' || s[0] == '%' ) continue; // skip comments
      
      // expecting 4 columnds
      std::vector<std::string> tok = Helper::parse( s , "\t ," );
      if ( tok.size() != 4 ) Helper::halt( "bad format: expecting CH X Y Z" );
      
      // store all channel names as upper case
      std::string lab = Helper::toupper( tok[0] );
      channels.push_back( lab );
      
      double x,y,z;
      if ( ! ( Helper::str2dbl( tok[1] , &x ) 
	       && Helper::str2dbl( tok[2] , &y ) 
	       && Helper::str2dbl( tok[3] , &z ) ) )
	Helper::halt( "bad format:  expecting CH X Y Z" );

      // add
      add_cart( lab , x , y , z );

    }
  IN1.close();

  logger << "  read " << cloc.size() << " channel locations from " << filename << "\n"; 

  //
  // Convert to unit sphere
  //

  convert_to_unit_sphere();

  //
  // Output
  //

  if ( verbose ) 
    {
      std::map<std::string,cart_t>::const_iterator ii = cloc.begin();
      while ( ii != cloc.end() ) 
	{
	  sph_t sph = ii->second.sph();
	  cart_t c2 = sph.cart();
	  
	  polar_t polar( sph );
	  cart_t  pc = polar.cart();
	  
	  writer.level( ii->first , globals::signal_strat );

	  writer.value( "X" , ii->second.x );
	  writer.value( "Y" , ii->second.y );
	  writer.value( "Z" , ii->second.z );
	  
	  writer.value( "SPH_R"  , sph.r );
	  writer.value( "SPH_AZ" , sph.azimuth );
	  writer.value( "SPH_E"  , sph.elevation );
	  
	  writer.value( "POLAR_ANGLE" , polar.angle );
	  writer.value( "POLAR_RAD" , polar.radius );
	  
	  //       << sph.r << " "
	  //       << sph.azimuth << " "
	  //       << sph.elevation << "\t[ polar angle/radius ] = "
	  //       << polar.angle << " " << polar.radius << "\t[X,Y,Z]' = "
	  
	  //      << pc.x << " " << pc.y << " " << pc.z << "\n";
	  
	  ++ii;
	}
      
      writer.unlevel( globals::signal_strat );
    }

  
  //
  // Calculate and dump pairwise similarities/distances? 
  //

  if ( verbose )
    {
      signal_list_t signals;
      for (int i=0; i<channels.size(); i++)
	signals.add( i , channels[i] );

      // mode = 1 , 2 : returns difference distance / similarity measures
      Data::Matrix<double> D1 = interelectrode_distance_matrix( signals , 1 );
      Data::Matrix<double> D2 = interelectrode_distance_matrix( signals , 2 );

      for (int i=0; i<channels.size(); i++)
	{
	  writer.level( channels[i] , globals::signal1_strat );
	  for (int j=0; j<channels.size(); j++)
	    {
	      writer.level( channels[j] , globals::signal2_strat );
	      writer.value( "S" , D1(i,j) ); // similarity measure
	      writer.value( "D" , D2(i,j) );	      
	    }
	  writer.unlevel( globals::signal2_strat );
	}
      writer.unlevel( globals::signal1_strat );      
    }

  return cloc.size();
}


void clocs_t::convert_to_unit_sphere()
{
  double maxrad = 0;
  std::map<std::string,cart_t>::iterator ii = cloc.begin();
  while ( ii != cloc.end() )
    {
      sph_t sph = ii->second.sph();
      if ( sph.r > maxrad ) maxrad = sph.r;
      ++ii;
    }

  ii = cloc.begin();
  while ( ii != cloc.end() )
    {
      cart_t & cart = ii->second; 
      cart.x /= maxrad;
      cart.y /= maxrad;
      cart.z /= maxrad;
      ++ii;
    }
}


double clocs_t::distance( const std::string & ch1 , const std::string & ch2 , const int mode ) const
{

  cart_t c1 = cart( ch1 );
  cart_t c2 = cart( ch2 );
  return mode == 1 ?      
    1 - ( ( (c1.x-c2.x)*(c1.x-c2.x) +
	    (c1.y-c2.y)*(c1.y-c2.y) +
	    (c1.z-c2.z)*(c1.z-c2.z) ) / 2.0 )
    : 
    sqrt( (c1.x-c2.x)*(c1.x-c2.x) + (c1.y-c2.y)*(c1.y-c2.y) + (c1.z-c2.z)*(c1.z-c2.z) );
  
}


Data::Matrix<double> clocs_t::interelectrode_distance_matrix( const signal_list_t & signals , const int mode ) const
{
  
  for (int s=0;s<signals.size();s++)
    if ( ! has( signals.label(s) ) ) 
      Helper::halt( "could not find cloc for: " 
		    + signals.label(s) 
		    + "\navailable clocs: " + print() );
      

  const int ns = signals.size();
  
  Data::Matrix<double> D(ns,ns);
  
  for (int s1=0;s1<ns;s1++)
    {      

      cart_t c1 = cart( signals.label(s1) );
      
      for (int s2=s1;s2<ns;s2++)
	{

	  cart_t c2 = cart( signals.label( s2 ) );
	  
	  if ( mode == 1 ) 
	    {
	      double d = 1 - ( ( (c1.x-c2.x)*(c1.x-c2.x) +
				 (c1.y-c2.y)*(c1.y-c2.y) +
				 (c1.z-c2.z)*(c1.z-c2.z) ) / 2.0 );	      
	      D[s1][s2] = D[s2][s1] = d;
	    }
	  else
	    {
	      double d = sqrt( (c1.x-c2.x)*(c1.x-c2.x) + (c1.y-c2.y)*(c1.y-c2.y) + (c1.z-c2.z)*(c1.z-c2.z) );
	      D[s1][s2] = D[s2][s1] = d;
	    }
	}
      
    }
  
  return D;
}



Data::Matrix<double> clocs_t::interelectrode_distance_matrix( const signal_list_t & signals1 , 
							      const signal_list_t & signals2 ) const
{
  
  for (int s=0;s<signals1.size();s++)
    if ( ! has(signals1.label(s) ) ) 
      Helper::halt( "could not find cloc for: " + signals1.label(s) + "\navailable clocs: " + print() );

  for (int s=0;s<signals2.size();s++)
    if ( ! has(signals2.label(s) ) ) 
      Helper::halt( "could not find cloc for: " + signals2.label(s) + "\navailable clocs: " + print() );

  const int ns1 = signals1.size();

  const int ns2 = signals2.size();
  
  Data::Matrix<double> D(ns1,ns2);
  for (int s1=0;s1<ns1;s1++)
    {      

      cart_t c1 = cart( signals1.label(s1) );

      for (int s2=0;s2<ns2;s2++)
	{

	  cart_t c2 = cart( signals2.label( s2 ) ); 

	  double d = 1 - ( ( (c1.x-c2.x)*(c1.x-c2.x) +
			     (c1.y-c2.y)*(c1.y-c2.y) +
			     (c1.z-c2.z)*(c1.z-c2.z) ) / 2.0 );
	  D[s1][s2] = d;
	}
      
    }
  return D;
}


bool clocs_t::make_interpolation_matrices( const signal_list_t & good_signals , 
					   const signal_list_t & bad_signals , 
					   Data::Matrix<double> * G , 
					   Data::Matrix<double> * Gi )
{
    
  // 'm' parameter (Perrin et al, m = 4, otherwise m = 2..6 reasonable
  const int m = 2;   // m=2 in interpolate_perrinX

  // order of Legendre polynomials; 7 also suggested Perry et al.
  const int N = 10;

  // smoothing parameter/ 1e-5 suggested for 64 electrodes
  // for > 64 electrodes, 1e-6 or 5e-6
  const double smoothing = 1e-5;
  
  convert_to_unit_sphere();
  
  int ns  = good_signals.size();
  int nsi = bad_signals.size();
  
  // get interelectrode distance matrix
  Data::Matrix<double> D = interelectrode_distance_matrix( good_signals , good_signals );
  
  // std::cout << "cosdist\n\n";
  // std::cout << D.print() << "\n";

  // Evaluate Legendre polynomials
  std::vector<Data::Matrix<double> > L = legendre( N , D );

  // given signals in the signal-list, make a matching G matrix  
  //  const int ns = signals.size();
  
  // precompute electrode-independent variables
  std::vector<int> twoN1;
  std::vector<double> gdenom;
  for (int i=1;i<=N;i++) 
    { 
      twoN1.push_back( ( 2 * i ) + 1 ) ; 
      gdenom.push_back( pow( i*(i+1)  , m ) ) ; 
    }

  
  // compute G (for all good x all good electrodes)  
  G->resize( ns , ns , 0 );
  
  // for each pair of good x good electrodes, get element of G
  for (int i=0;i<ns;i++)
    for (int j=i;j<ns;j++)
      {
	double g = 0;
	for (int n=0;n<N;n++)
	  {
	    g += (twoN1[n] * L[n](i,j) ) / gdenom[n];
	  }
	(*G)(i,j) = g / ( 4.0 * M_PI );
	(*G)(j,i) = (*G)(i,j);
      }

    
  // 
  // Optionally, add smoothing to each diagonal element
  //

  if ( 0 ) 
    {
      for (int i=0;i<ns;i++) (*G)(i,i) = (*G)(i,i) + smoothing ; 
    }

  //
  // G for the to-be-interpolated electrodes
  //

  Gi->resize( nsi, ns , 0 );
  
  Data::Matrix<double> Di = interelectrode_distance_matrix( bad_signals , good_signals );
  
  // Evaluate Legendre polynomials
  std::vector<Data::Matrix<double> > Li = legendre( N , Di );

  // for each bad x good pair, compute element of Gi

  for (int i=0;i<nsi;i++)
    for (int j=0;j<ns;j++)
      {
	double g = 0;
	for (int n=0;n<N;n++)
	  {
	    g += (twoN1[n] * Li[n](i,j) ) / gdenom[n];
	  }
	(*Gi)(i,j) = g / ( 4.0 * M_PI );
      }

  // return inverse of G
  bool okay = true;
  Data::Matrix<double> invG = Statistics::inverse( *G , &okay );
  if ( ! okay ) Helper::halt( "problem inverting G" );
  //  std::cout << "invG\n\n" << invG.print() << "\n\n";
  *G = invG;

  return true;
}




Data::Matrix<double> clocs_t::interpolate( const Data::Matrix<double> & data , 
					   const std::vector<int> & good_channels , 
					   const Data::Matrix<double> & invG , 
					   const Data::Matrix<double> & Gi )
{

  
  const int nrows = data.dim1();
  const int nbad  = Gi.dim1();
  const int ngood = Gi.dim2();

//    for (int i = 0 ; i < nrows ; i++) 
//      {
//        for (int j = 0 ; j < good_channels.size()  ; j++) 
// 	 {
// 	   std::cout << data( i , good_channels[j] ) << " ";
// 	 }
//        std::cout << "\n";
//      }
//    std::cout << "\n";
 
  // sanity check
  if ( invG.dim1() != ngood || invG.dim2() != invG.dim1() || good_channels.size() != ngood ) 
    Helper::halt( "internal problem in interpolate" );
  

  
  // IMPUTED (BxR)  =    BxG * ( GxG * GxR ) 
  //                     Gi  * ( invG * data' )
  
  // as we need to transpose data for noral mat mult, just do by hand here
  // swapping rows and cols

  Data::Matrix<double> t( ngood , nrows );

  for (int i=0;i<ngood; i++)
    for (int j=0;j<nrows;j++)
      for (int k=0;k<ngood;k++)
	t(i,j) += invG(i,k) * data(j,good_channels[k]);

  
//   Data::Matrix<double> tt = Statistics::transpose( t );
//   std::cout << "t\n\n" << tt.print() << "\n";
  
  Data::Matrix<double> y( nrows , nbad );

  // IMPUTED (BxR)  =    BxG * ( GxG * GxR ) 
  // this is also implicilty transposed back into y
  // i.e. RxB rather than BxR
  
  for (int i=0;i<nbad; i++)
    for (int j=0;j<nrows;j++)
      for (int k=0;k<ngood;k++)
	y(j,i) += Gi(i,k) * t(k,j);

  //  std::cout << "y = \n" << y.print() << "\n";
  
  return y;
  
}

