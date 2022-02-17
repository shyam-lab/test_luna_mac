
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

#ifndef __MICROSTATES_H__
#define __MICROSTATES_H__

struct edf_t;
struct param_t;
struct signal_list_t;

#include <vector>
#include "stats/matrix.h"
#include "stats/Eigen/Dense"
#include "edf/signal-list.h"

namespace dsptools
{
  void microstates( edf_t & edf , param_t & param );
}


struct ms_prototypes_t {

  // CH1 A1 A2 .. AK
  // CH2 A1 A2 .. AK

  ms_prototypes_t() { }
  
  ms_prototypes_t( const signal_list_t & signals , const Eigen::MatrixXd  A_ )
  {
    A = A_;
    C = signals.size();
    K = A_.cols();
    if ( A_.rows() != C ) Helper::halt( "internal inconsistency in ms_prototypes_t()" );    
    chs.resize( C );
    for (int s=0;s<C; s++) chs[s] = signals.label(s);
    // set default 1, 2, 3, ... encoding
    ms_labels.resize( K );
    for (int k=0;k<K;k++) ms_labels[k] = 49 + k;
  }
  
  void write( const std::string & filename );
  void read( const std::string & filename );
  
  // i.e. store explicitly, as might not be A, B, C, D (i.e. if skips A, C, E, F ) 
  // enfore that these are single chars, i.e. to make the sequence analysis work
  static std::vector<char> ms_labels;

  void map_to_canonicals( const std::string & filename );
  
  static double spatial_correlation( const Eigen::VectorXd & M1 , const Eigen::VectorXd & M2 );

  int K;
  int C;
  std::vector<std::string> chs; // C
  Eigen::MatrixXd A; // C x K
};



struct ms_assignment_t {

  ms_assignment_t( int l , double g ) : label(l) , gmd(g) { } 

  int label;

  double gmd;

  bool operator<( const ms_assignment_t & rhs ) const {
    if ( gmd < rhs.gmd ) return true;
    if ( gmd > rhs.gmd ) return false;
    return label < rhs.label;			  
  }

};

struct ms_assignments_t {

  //ms_assignment_t( const int K ) { label.resize( K ); } 

  // original, ordered values
  std::set<ms_assignment_t> assignments;

  void add( int l , double g )
  {
    assignments.insert( ms_assignment_t( l , g ) );
  }

  // populate ordered 
  std::vector<int> picks;

  void set_picks()
  {
    picks.clear();
    std::set<ms_assignment_t>::const_iterator aa = assignments.begin();
    while ( aa != assignments.end() )
      {
	picks.push_back( aa->label );
	++aa;
      }
    // now done w/ assignments
    assignments.clear();
  }

  // (circular) shift of labels (i.e. get next best) for smoothing
  void shift()
  {
    const int n = picks.size();
    std::vector<int> old = picks;
    // shift current best to last
    picks[ n-1 ] = picks[0]; 
    // fill in the rest
    for (int i=0; i<n-1; i++) picks[i] = old[i+1];
  }

  int best() const
  {
    return picks[0];
  }
    
};

struct ms_backfit_t {

  ms_backfit_t( const int N ) { labels.resize(N); ambiguous.resize( N, false ); } 

  // class to keep track of best (and 2nd, 3rd, etc) best picks, for each N
  std::vector<ms_assignments_t> labels;

  // return -1 for an ambiguous assignment
  std::vector<int> best() const {
    const int n = labels.size();
    std::vector<int> L( n );
    for (int i=0;i<n;i++) 
      {
	if ( ambiguous[i] ) L[i] = -1;
	else L[i] = labels[i].best();
      }
    return L;
  }
  
  // confidence threshold 
  void determine_ambiguity( double conf , double th2 );
  
  std::vector<bool> ambiguous;
  
  // and also store full GMD for best class separately
  Data::Matrix<double> GMD;

};

struct ms_rle_t {
  std::vector<int> d;
  std::vector<int> c;
};


struct ms_kmer_results_t {

  // key is sequence as a string
  
  // observed statistic (count / relative freq)
  std::map<std::string,double>  obs;    // OBS
  
  // track all NREP permuted statistics
  std::map<std::string,std::vector<double> > perm;   
  
  // expected statistic (mean of perm)
  std::map<std::string,double> exp;    // EXP
  
  // Z score = ( OBS - mean(perm) ) / sd(perm) 
  std::map<std::string,double> zscr;   // Z score
  
  // (optional) enirhcment (1-sided) empirical p-value ( obs >= perm ) 
  std::map<std::string,double> pval;   

};

struct ms_kmer_t {
  
  ms_kmer_t() { }

  // single obs
  ms_kmer_t( const std::vector<int> & x , int k1 , int k2 , int nreps , bool verbose = false )
  {
    run(x,k1,k2,nreps, verbose );
  }

  // multiple-obs, as strings
  ms_kmer_t( const std::map<std::string,std::string>  & s , int k1 , int k2 , int nreps ,
	     const std::map<std::string,int> * grp = NULL , bool verbose = false ) 
  {
    run(s,k1,k2,nreps,grp, verbose );
  }
  
  // multi-obs, as int-vectors
  ms_kmer_t( const std::map<std::string,std::vector<int> > & l , int k1 , int k2 , int nreps , 
	     const std::map<std::string,int> * grp = NULL , bool verbose = false )  
  {
    run(l,k1,k2,nreps,grp, verbose );
  }
  
  // single obs
  void run( const std::vector<int> & l , int k1 , int k2 , int nreps , bool verbose = false )
  {
    std::map<std::string,std::vector<int> > l1;
    l1[ "__single_obs" ] = l;
    run( l1 , k1 , k2 , nreps , NULL , verbose );
  }

  // multiple obs, optional grp variable coded 0/1 
  void run( const std::map<std::string,std::vector<int> > & l ,
	    int k1 , int k2 , int nreps ,
	    const std::map<std::string,int> * grp = NULL , 
	    bool verbose = false );
  
  // multiple obs, optional grp variable coded 0/1 
  void run( const std::map<std::string,std::string> & s ,
	    int k1 , int k2 , int nreps ,
	    const std::map<std::string,int> * grp = NULL , 
	    bool verbose = false );
  
  std::set<std::string> permute( std::string str );
  std::string first_permute( std::string str );
  std::string modified_random_draw( const std::string & );
  char pick( const std::map<char,int> & urns , char skip = '.' );
  std::string s;
  
  //
  // Results
  // 

  std::map<std::string,int> equiv_set_size;
  std::map<std::string,std::string> obs2equiv;
  std::map<std::string,std::set<std::string> > equivs;

  // raw counts
  ms_kmer_results_t basic;
  
  // equivalence group sum counts
  ms_kmer_results_t group;

  // equivalence group relative enrichment
  ms_kmer_results_t equiv;
  
  // phenotype group comparisons (assumes two groups)
  ms_kmer_results_t basic_controls;
  ms_kmer_results_t basic_cases;
  ms_kmer_results_t basic_diffs;
  
  // as above, but for sum of E-group
  ms_kmer_results_t group_controls;
  ms_kmer_results_t group_cases;
  ms_kmer_results_t group_diffs;

  // equivalence group relative enrichment
  ms_kmer_results_t equiv_controls;
  ms_kmer_results_t equiv_cases;
  ms_kmer_results_t equiv_diffs;
    
};


struct ms_stats_t {

  // global statistics
  double GEV_tot;

  Data::Vector<double> GFP;

  Data::Matrix<double> SpatCorr;

  Data::Vector<double> m_gfp;
  Data::Vector<double> m_dur;
  Data::Vector<double> m_occ;
  Data::Vector<double> m_occ_unambig;
  Data::Vector<double> m_cov;
  Data::Vector<double> m_cov_unambig;
  Data::Vector<double> m_wcov; // weighted coverage
  Data::Vector<double> m_gev;
  Data::Vector<double> m_spc;

  // transition probs
  Data::Matrix<double> tr;

  // LWZ complexity
  double lwz_states;

  // SE (M)
  std::map<int,double> samplen;

  // k-mers
  ms_kmer_t kmers;
    
};


struct microstates_t {
  
  microstates_t( param_t & param , const std::string & subj_id , const int sr_ );
  
  std::vector<int>  find_peaks( const Data::Matrix<double> & X , 
				const signal_list_t & signals );

  static Data::Matrix<double> eig2mat( const Eigen::MatrixXd & E );
  static Eigen::MatrixXd mat2eig( const Data::Matrix<double> & M );
  static Eigen::MatrixXd mat2eig_tr( const Data::Matrix<double> & M );

  static void aggregate2edf( const Data::Matrix<double> & X ,
			     const signal_list_t & signals ,
			     const std::vector<int> & peak_idx ,	
			     const int srate ,
			     const double pmin , const double pmax ,
			     const std::string & edfname );

  
  ms_prototypes_t  segment( const Data::Matrix<double> & X , 
			    const signal_list_t & signals ,
			    const std::vector<int> & peaks , 
			    const std::string * canonical_file );
  
  ms_backfit_t backfit( const Data::Matrix<double> & X_ , 
			const Data::Matrix<double> & A_ , 
			const double lambda , 
			bool return_GMD );
  
  ms_backfit_t smooth_reject( const ms_backfit_t & labels ,
			      int minTime  );  // in samples
  
  ms_backfit_t smooth_windowed( const ms_backfit_t & labels ,
				const Eigen::MatrixXd & X_ ,
				const Eigen::MatrixXd & A_ ,
				int smooth_width = 3 ,
				double smooth_weight = 5 ,
				int max_iterations = 1000 ,
				double threshold = 1e-6 );
  
  ms_rle_t rle( const std::vector<int> & x );

  ms_stats_t stats( const Data::Matrix<double> & X , 
		    const Data::Matrix<double> & A , 
		    const std::vector<int> & L );
  
  static std::map<int,std::pair<int,double> > counts( const std::vector<int> & l )
  {
    // k --> N , %
    const int n = l.size();
    std::map<int,std::pair<int,double> > cnts;
    for (int i=0; i<n; i++) cnts[ l[i] ].first++;
    std::map<int,std::pair<int,double> >::iterator cc = cnts.begin();
    while ( cc != cnts.end() )
      {
	cc->second.second = cc->second.first / (double)n;
	++cc;
      }
    return cnts;      
  }
  
  
  //
  // Options
  //

  // number of classes (vector)
  std::vector<int> ks;

  // sample rate
  int sr;
  
  // modes
  bool single_sample; // find peaks, segment, backfit, smooth, calc stats
  bool multi_peaks;   // aggregate peaks across EDFs
  bool multi_segment; // fit to a single EDF (but that contains peaks from all)
  bool multi_backfit; // apply the above solution to all EDFs
  
  // misc options

  //
  // dump GPF matrix prior to clustering?
  //
  
  std::string dump_file;

  //
  // write sequences to file (i.e. for subsequent grouped KMER analysis)
  //

  std::string statesfile;
  std::string subj_id;

  //
  // write individual prototype maps (for --compare-maps)
  //

  std::string mapsfile;

  
  
  bool standardize;
  
  bool verbose;

  bool skip_peaks;
  
  double gfp_max_threshold;
  double gfp_min_threshold;
  double gfp_kurt_threshold;
  
  int restrict_npeaks;
  
  double min_peak_dist;

  int kmers_nreps;
  int kmers_min;
  int kmers_max;
  
};



struct ms_cmp_maps_t {

  ms_cmp_maps_t( const std::map<std::string,std::map<std::string,std::map<std::string,double> > > & d ,
		 const Eigen::MatrixXd * fixed ,
		 const std::vector<std::string> * fixed_chs , 
		 const std::map<std::string,int> & phe ,
		 const int nreps ,
		 const bool brute_force = false );

  double cmp_maps( const Eigen::MatrixXd & A , const Eigen::MatrixXd & B );

  double cmp_maps_bf( const Eigen::MatrixXd & A , const Eigen::MatrixXd & B );

  double cmp_maps_template( const Eigen::MatrixXd & A , const Eigen::MatrixXd & B , std::vector<int> * best = NULL );

  double statistic( const std::vector<int> & phe ,
		    const std::vector<int> & perm ,
		    const Eigen::MatrixXd & R ,
		    Eigen::VectorXd * ires );

  double het_statistic( const std::vector<int> & phe ,
			const std::vector<int> & perm ,
			const Eigen::MatrixXd & R ,
			double *within );

  double het_template_statistic( const std::vector<int> & phe ,
				 const std::vector<int> & perm ,
				 const Eigen::VectorXd & R ,
				 double *within );
				 

  
};


#endif
