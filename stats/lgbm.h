
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

#ifndef __LUNA_LGBM_H__
#define __LUNA_LGBM_H__

#ifdef HAS_LGBM

#include "LightGBM/c_api.h"

#include <string>
#include "helper/helper.h"
#include "stats/Eigen/Dense"
#include <fstream>

// wrappers
struct param_t;
void lgbm_cli_wrapper( param_t & param );


struct lgbm_label_t;

struct lgbm_t {
  
  lgbm_t( const int n_iterations = 100 )
    : has_booster(false) , has_training(false ) , has_validation(false), n_iterations(n_iterations)
  {
    params = "";
  }
  
  lgbm_t( const std::string & config_file , const int n_iterations = 100 )
    : has_booster(false) , has_training(false ) , has_validation(false) , n_iterations(n_iterations)
  {
    load_config( config_file );
  }
  
  void load_config( const std::string & config_file )
  {
    params = parse_config( config_file );
  }

  //
  // default config for POPS
  //

  void load_pops_default_config();
  
  //
  // Attach data (labels and weights specified via the config)
  //
  
  bool load_training_data( const std::string & filename );

  bool attach_training_matrix( const Eigen::MatrixXd & X );
  
  bool attach_training_labels( const std::vector<int> & labels );

  
  bool load_validation_data( const std::string & filename );

  bool attach_validation_matrix( const Eigen::MatrixXd & d );
  
  bool attach_validation_labels( const std::vector<int> & labels );

  //
  // Weights
  //

  bool load_weights( DatasetHandle d , const std::string & f );

  bool apply_label_weights( DatasetHandle d , const lgbm_label_t & l );
  
  //
  // Set up a booster 
  //

  bool create_booster();
  

  //
  // Load/save models
  //
  
  bool load_model( const std::string & f );
  
  bool load_model_string( const std::string & str );

  bool save_model( const std::string & f ) ;


  //
  // Core learning/prediction
  //
  
  bool train(  );
  
  Eigen::MatrixXd predict( const Eigen::MatrixXd & X );
  

  
  //
  // Helpers
  //

  static std::string parse_config( const std::string & f );

  static int rows( DatasetHandle d );

  static int cols( DatasetHandle d );

  static int label_column( DatasetHandle d );

  static std::vector<int> labels( DatasetHandle d );
  
  static std::vector<double> weights( DatasetHandle d );

  static std::vector<std::string> features( DatasetHandle d );

  static int classes( BoosterHandle b );
  
  
  //
  // clean-up
  //
  
  ~lgbm_t()
  {
    if ( has_booster && LGBM_BoosterFree( booster ) )
      Helper::halt( "problem freeing LGBM booster" );
    
    if ( has_training && LGBM_DatasetFree( training ) )
      Helper::halt( "problem freeing LGBM training data" );

    if ( has_validation && LGBM_DatasetFree( validation ) )
      Helper::halt( "problem freeing LGBM validation data" );

  }
  

  //
  // Members
  //

  // config
  std::string params;
  
  // booster
  bool has_booster;  
  BoosterHandle booster;
  
  // training data
  bool has_training;
  DatasetHandle training;

  // validation data
  bool has_validation;
  DatasetHandle validation;

  // not used yet
  FastConfigHandle fastconfig;
  // then needs LGBM_FastConfigFree

  int n_iterations;
  
};


struct lgbm_label_t {

  lgbm_label_t( const int n ) : n(n)
  {
    label.resize(n);
    for (int i=0;i<n;i++) label[i] = "C" + Helper::int2str( i+1 ); 
    weight.resize(n,1.0);
  }

  lgbm_label_t( const std::vector<std::string> label ) : label(label)
  {
    n = label.size();
    weight.resize(n,1.0);
  }

  // from file
  lgbm_label_t( const std::string & f )
  {
    std::string filename = Helper::expand( f ) ;
    if ( ! Helper::fileExists( filename ) )
      Helper::halt( "could not open " + filename );
    std::ifstream IN1( filename.c_str() , std::ios::in );
    n = 0;
    label.clear();
    weight.clear();

    while ( 1 )
      {
	// label, weight
	std::string s1;
	IN1 >> s1 ;
	if ( IN1.eof() || IN1.bad() ) break;
	if ( s1 == "" ) continue;
	double w1;
	IN1 >> w1;
	++n;
	label.push_back( s1 );
	weight.push_back( w1 );
      }
    IN1.close();
  }

  int n; // 5 : 0,1,2,3,4
  std::vector<std::string> label;
  std::vector<double> weight; 
};


#endif
#endif
  

