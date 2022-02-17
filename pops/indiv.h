
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


#ifdef HAS_LGBM

#ifndef __LUNA_POPS_INDIV_H__
#define __LUNA_POPS_INDIV_H__

struct edf_t;
struct param_t;

#include "pops/pops.h"

struct pops_indiv_t {
  
  pops_indiv_t( edf_t & , param_t & );

  void staging( edf_t & , param_t & );
  
  void save1( const std::string & , const std::string & );
  
  void level1( edf_t & );

  void level2();
  
  void predict( );

  void summarize( );


  // trainer/target?
  bool trainer;
  
  // number of epochs
  int ne;
  
  // level 1 features
  Eigen::MatrixXd X1;
  
  // staging
  std::vector<int> S;

  // epoch number
  std::vector<int> E;

  // predictions
  Eigen::MatrixXd P;
  
  
  //
  // binary I/O functions
  //
  
  inline static void bwrite( std::ofstream & O , const std::string & s );
  inline static void bwrite( std::ofstream & O , int i );
  inline static void bwrite( std::ofstream & O , double d );
  inline static std::string bread_str( std::ifstream & I );
  inline static int bread_int( std::ifstream & I );
  inline static double bread_dbl( std::ifstream & I );
  inline static void bskip_dbl( std::ifstream & I , const int n );
  inline static void bskip_int( std::ifstream & I , const int n );
  
  
};


#endif
#endif

