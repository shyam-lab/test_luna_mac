
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

#include "suds.h"

#include <vector>
#include <map>
#include <set>
#include <iomanip>

#include "helper/helper.h"
#include "helper/logger.h"
#include "db/db.h"

#include "dirent.h"

#include "stats/eigen_ops.h"
#include "stats/lda.h"
#include "stats/statistics.h"
#include "miscmath/miscmath.h"
#include "miscmath/crandom.h"

#include "edf/edf.h"
#include "edf/slice.h"

#include "dsp/resample.h"
#include "fftw/fftwrap.h"
#include "dsp/mtm/mtm.h"
#include "dsp/tv.h"

extern logger_t logger;

extern writer_t writer;

void suds_indiv_t::dump_svd( const std::string & froot )
{

  if ( froot == "" ) return;

  const std::string file_U = Helper::expand( froot ) + ".U";
  const std::string file_W = Helper::expand( froot ) + ".W";
  const std::string file_V = Helper::expand( froot ) + ".V";
  
  std::ofstream U1( file_U.c_str() , std::ios::out );
  U1 << "E\tSS";

  for (int i=0; i<nc; i++) U1 << "\tC" << i+1 ;
  U1 << "\n";
  
  for (int e=0; e<nve; e++)
    {
      U1 << e+1 << "\t" << y[e] ;
      for (int i=0; i<nc; i++)
	U1 << "\t" << U( e , i ) ;
      U1 << "\n";
    }
  
  U1.close();
  
  std::ofstream V1( file_V.c_str() , std::ios::out );
  V1 << "VAR";
  for (int i=0; i<nc; i++) V1 << "\tC" << i+1 ;
  V1 << "\n";

  std::vector<std::string> features = suds_t::model.labels();
  if ( features.size() != V.rows() ) Helper::halt( "internal error in dump-SVD" );

  for (int v=0; v<V.rows(); v++)
    {
      V1 << features[v];
      for (int i=0; i<nc; i++)
        V1 << "\t" << V( v , i ) ;
      V1 << "\n";
    }

  V1.close();

  V1.close();
  
  std::ofstream W1( file_W.c_str() , std::ios::out );
  W1 << "C\tW\n";
  for (int i=0;i<nc;i++)
    W1 << i+1 << "\t"
       << W[i] << "\n";
  W1.close();
  
}


void suds_indiv_t::dump_predictor_matrix( edf_t & edf , const std::string & filename )
{

  //
  // either give full matrix (w/ NA epochs) in output stream
  // or dump a simple file of good epochs only ( to match the SVD dump
  // above
  //

  if ( filename == "" )
    {  

      const int cols = X.cols();
      
      std::map<int,int> e2e;
      for (int i=0; i< epochs.size(); i++) e2e[ epochs[i] ] = i ;
      
      const int ne_all = edf.timeline.num_epochs();
      
      for (int i=0; i< ne_all; i++)
	{
	  int e = -1;
	  if ( e2e.find( i ) != e2e.end() ) e = e2e[i];
	  if ( e == -1 ) continue;
	  
	  writer.epoch( edf.timeline.display_epoch( i ) ) ;
	  
	  for (int c=0; c<cols; c++)
	    {
	      writer.level( "P" + Helper::int2str( c+1 ) , "FEAT" );
	      writer.value( "P" , X(e,c) );
	    }
	  writer.unlevel( "FEAT" );
	}
      writer.unepoch();
    }
  else
    {
      std::ofstream X1( filename.c_str() , std::ios::out );
      X1 << "E";
      
      const std::vector<std::string> vars = suds_t::model.labels();
      const int nf = vars.size();
      
      for (int i=0; i<nf; i++) X1 << "\t" << vars[i];
      X1 << "\n"; 
      
      for (int e=0; e<nve; e++)
	{
	  X1 << e+1 ;
	  for (int i=0; i<nf; i++)
	    X1 << "\t" << X( e , i ) ;
	  X1 << "\n";
	}      
      X1.close();      
    }
}

std::map<std::string,std::map<std::string,int> > suds_t::tabulate( const std::vector<std::string> & a , 
								   const std::vector<std::string> & b , 
								   const bool print  )
{
  std::map<std::string,std::map<std::string,int> > res;
  
  const int n = a.size();
  
  if ( n != b.size() ) 
    Helper::halt( "internal error: unequal vectors in tabulate()" );

  // includes unknown stages SUDS_UNKNOWN, '?' in table
  // (but these should be removed from kappa and other stats)

  std::set<std::string> uniq;
  for (int i=0;i<n;i++)
    {
      //std::cout << "CHECK\t" << a[i] << "\t" << b[i] << "\n";
      res[ a[i] ][ b[i] ]++;
      uniq.insert( a[i] );
      uniq.insert( b[i] );
    }

  std::map<std::string,double> rows, cols;
  double tot = 0;
  std::set<std::string>::const_iterator uu = uniq.begin();
  while ( uu != uniq.end() )
    {
      std::set<std::string>::const_iterator jj = uniq.begin();
      while ( jj != uniq.end() )
	{
	  if ( res.find( *uu ) == res.end() )
	    res[ *uu ][ *jj ] = 0;
	  else
	    {
	      std::map<std::string,int> & rjj = res.find(*uu)->second;
	      if ( rjj.find( *jj ) == rjj.end() )
		res[ *uu ][ *jj ] = 0;
	    }	      

	  // col/row marginals
	  rows[ *uu ] += res[ *uu ][ *jj ] ;
	  cols[ *jj ] += res[ *uu ][ *jj ] ;
	  tot += res[ *uu ][ *jj ];
	  
	  ++jj;
	}
      ++uu;
    }
  
  
  if ( print )
    {

      logger << "\t   Obs:";
      std::set<std::string>::const_iterator uu = uniq.begin();
      while ( uu != uniq.end() )
	{
	  logger << "\t" << *uu;
	  ++uu;
	}
      logger << "\tTot\n";	
      
      logger << "  Pred:";
      uu = uniq.begin();
      while ( uu != uniq.end() )
	{
	  logger << "\t" << *uu;

	  std::set<std::string>::const_iterator jj = uniq.begin();
	  while ( jj != uniq.end() )
	    {
	      logger << "\t" << res[ *uu ][ *jj ];
	      ++jj;
	    }	  
	  
	  // row sums
	  logger << "\t" << rows[ *uu ]/tot;
	  logger << "\n";
	  ++uu;
	}


      // col sums
      logger << "\tTot:";
      std::set<std::string>::const_iterator jj = uniq.begin();
      while ( jj != uniq.end() )
	{
	  logger << "\t" << cols[ *jj ]/tot;
	  ++jj;
	}
      logger << "\t1.00\n\n";

      
      // conditional probabilties  / res[][] / cols[] 
      uu = uniq.begin();
      while ( uu != uniq.end() )
        {
	  writer.level(*uu , "PRED" );
	  std::set<std::string>::const_iterator jj = uniq.begin();
          while ( jj != uniq.end() )
	    {
	      writer.level( *jj , "OBS" );
	      writer.value( "N" , res[ *uu ][ *jj ] );
	      if ( cols[ *uu ] > 0 ) 
		writer.value( "P" , res[ *uu ][ *jj ] / cols[ *jj ] );
	      ++jj;
	    }
	  writer.unlevel( "OBS" );
	  ++uu;
	}
      writer.unlevel( "PRED" );
    }

  
  return res;
}


void suds_indiv_t::summarize_epochs( const Eigen::MatrixXd & pp , // posterior probabilities
				     const std::vector<std::string> & labels, // column labels
				     int ne_all , edf_t & edf ) // total number of epochs in EDF
{
  // output epoch-level results: most likely stage, PP, observed stage, flag for discordance, missing/unknown

  bool prior_staging = obs_stage.size() != 0 ;
  
  // epochs[] contains the codes of epochs actually present in the model/valid
  std::map<int,int> e2e;
  for (int i=0; i< epochs.size(); i++) e2e[ epochs[i] ] = i ;  

  for (int i=0;i<ne_all;i++)
    {

      int e = -1;

      if ( e2e.find( i ) != e2e.end() ) e = e2e[i];
      
      writer.epoch( edf.timeline.display_epoch( i ) );

      if ( e != -1 ) 
	{
	  
	  writer.value( "INC" , 1 );

	  double pp_nr = 0;
	  bool has_nr = false;
	  for (int j=0;j<labels.size();j++)
	    {
	      if ( labels[j] == "NR" ) has_nr = true;
	      if ( labels[j] == "N1" || labels[j] == "N2" || labels[j] == "N3" ) pp_nr += pp(e,j); 
	      writer.value( "PP_" + labels[j] , pp(e,j) );
	    }

	  // automatically aggregate N1+N2+N3 under the 5-class model (or whatever NREM stages are present)
	  if ( ! has_nr )
	    writer.value( "PP_NR" , pp_nr );
	
	  // most likely value
	  std::string predss = suds_t::max_inrow( pp.row(e) , labels );
	  writer.value( "PRED" , predss );

	  if ( prior_staging )
	    {
	      // discordance if prior/obs staging available
	      bool disc = obs_stage[i] != SUDS_UNKNOWN && predss !=  suds_t::str( obs_stage[i] ) ;
	      writer.value( "DISC" , disc );

	      // collapse 5->3 ?
	      if ( suds_t::n_stages == 5 )
		{
		  bool disc3 =  obs_stage[i] != SUDS_UNKNOWN && suds_t::NRW( predss ) != suds_t::NRW( suds_t::str( obs_stage[i] ) ) ;
		  writer.value( "DISC3" , disc3 );
		}
	      
	      writer.value( "PRIOR" ,  suds_t::str( obs_stage[i] ) );
	      
	      if ( suds_t::soap_mode == 2 ) 
		writer.value( "PROPOSAL" ,  y[e] );
	      
	    }
	}
      else
	{
	  writer.value( "INC" , 0 );
	  
	  // lookup from all stages
	  if ( prior_staging )
	    writer.value( "PRIOR" ,  suds_t::str( obs_stage[i] ) );	  
	}
      
    }

  writer.unepoch();
  
}

int suds_indiv_t::summarize_stage_durations( const Eigen::MatrixXd & pp , const std::vector<std::string> & labels, int ne_all , double epoch_sec )
{
  
  bool prior_staging = obs_stage.size() != 0 ;
 
  std::map<std::string,double> prd_dur;   // sum of PP
  std::map<std::string,double> prd2_dur;  // based on most likely
  std::map<std::string,double> obs_dur;   // obserevd  (if present)... but based on same epochs as used in the staging (i.e. removing some outliers) 

  std::map<int,int> e2e;
  for (int i=0; i<epochs.size(); i++) e2e[ epochs[i] ] = i ;  
  

  //
  // Get labels -> slots
  //
  
  int n1_slot, n2_slot, n3_slot, nr_slot, rem_slot, wake_slot;
  n1_slot = n2_slot = n3_slot = nr_slot = rem_slot = wake_slot = -1;

  for (int i=0;i< labels.size(); i++)
    {
      if      ( labels[i] == "N1" ) n1_slot = i;
      else if ( labels[i] == "N2" ) n2_slot = i;
      else if ( labels[i] == "N3" ) n3_slot = i;
      else if ( labels[i] == "NR" ) nr_slot = i;
      else if ( labels[i] == "R" ) rem_slot = i;
      else if ( labels[i] == "W" ) wake_slot = i;      
    }

  
  double unknown = 0;
  int unknown_epochs = 0; 

  //
  // Aggregate over epochs
  //

  
  for (int i=0;i<ne_all;i++)
    {
      
      int e = -1;
      
      if ( e2e.find( i ) != e2e.end() ) e = e2e[i];
      
      if ( e != -1 ) 
	{
	
	  // most likely value
	  std::string predss = suds_t::max_inrow( pp.row(e) , labels );

	  // track stage duration (based on probabilistic calls)
	  // nb. we do not assume all five/three stages are present here

	  if ( n1_slot != -1 ) prd_dur[ "N1" ]  += pp(e,n1_slot) * epoch_sec ;
	  if ( n2_slot != -1 ) prd_dur[ "N2" ]  += pp(e,n2_slot) * epoch_sec ;
	  if ( n3_slot != -1 ) prd_dur[ "N3" ]  += pp(e,n3_slot) * epoch_sec ;
	  if ( nr_slot != -1 ) prd_dur[ "NR" ]  += pp(e,nr_slot) * epoch_sec ;
	  if ( rem_slot != -1 ) prd_dur[ "R" ]  += pp(e,rem_slot) * epoch_sec ;
	  if ( wake_slot != -1 ) prd_dur[ "W" ]  += pp(e,wake_slot) * epoch_sec ;
	  
	  // duration based on MAP estimate
	  prd2_dur[ predss ] += epoch_sec;

	  // comparable OBS duration
	  if ( prior_staging )	    
	    obs_dur[ suds_t::str( obs_stage[i] ) ] += epoch_sec; 

	}
      else
	{
	  // track extent of 'bad' epochs
	  unknown += epoch_sec;
	  ++unknown_epochs;
	}

    }
  
  //
  // Report stage durations (in minutes)
  //

  for (int s=0; s < suds_t::labels.size(); s++) 
    {
      writer.level( suds_t::labels[ s ] , globals::stage_strat );      
      writer.value( "DUR_PRD" , prd_dur[ suds_t::labels[ s ] ] / 60.0 );

      // alternate estimates, based on most likely predicted epoch
      if ( suds_t::verbose )
	writer.value( "DUR_PRD2" , prd_dur[ suds_t::labels[ s ] ] / 60.0 );

    }  
  
  // unknown/missed epochs
  writer.level( suds_t::str( SUDS_UNKNOWN ) , globals::stage_strat );
  writer.value( "DUR_OBS" , unknown / 60.0 );

  // and done
  writer.unlevel( globals::stage_strat );


  //
  // estimates of observed stage duration (based on comparable set of epochs)
  //

  if ( prior_staging )
    {
      std::map<std::string,double>::const_iterator ss = obs_dur.begin();
      while ( ss != obs_dur.end() )
	{
	  if ( ss->first != "?" )
	    {
	      writer.level( ss->first , globals::stage_strat );
	      writer.value( "DUR_OBS" , ss->second / 60.0 );
	    }
	  ++ss;
	}
      writer.unlevel( globals::stage_strat );
    }

  return unknown_epochs;
  
}



void suds_indiv_t::summarize_kappa( const std::vector<std::string> & prd , const bool to_console )
{
  
  if ( to_console )
    logger << std::fixed << std::setprecision(2);
  
  // original reporting (5 or 3 level)
  double kappa = MiscMath::kappa( prd , suds_t::str( obs_stage_valid ) , suds_t::str( SUDS_UNKNOWN ) );

  // accuracy, precision/recall, kappa:   nb. ordering: 'truth' first, then 'predicted' 
  double macro_f1 = 0 , macro_precision = 0 , macro_recall = 0;
  double wgt_f1 = 0 , wgt_precision = 0 , wgt_recall = 0 , mcc = 0;
  std::vector<double> precision, recall, f1;

  double acc = MiscMath::accuracy( suds_t::str( obs_stage_valid ) , prd ,
				   suds_t::str( SUDS_UNKNOWN ) , 
				   &suds_t::labels ,
				   &precision, &recall, &f1,
				   &macro_precision, &macro_recall, &macro_f1 ,
				   &wgt_precision, &wgt_recall, &wgt_f1 , &mcc);
  
  writer.value( "K" , kappa );
  writer.value( "ACC" , acc );

  writer.value( "F1" , macro_f1 );
  writer.value( "MCC" , mcc );
  writer.value( "PREC" , macro_precision );
  writer.value( "RECALL" , macro_recall );
  
  writer.value( "F1_WGT" , wgt_f1 );
  writer.value( "PREC_WGT" , wgt_precision );
  writer.value( "RECALL_WGT" , wgt_recall );

  for ( int l=0;l<suds_t::labels.size();l++)
    {
      writer.level( suds_t::labels[l] , globals::stage_strat );
      writer.value( "F1" , f1[l] );
      writer.value( "PREC" , precision[l] );
      writer.value( "RECALL" , recall[l] );
    }
  writer.unlevel( globals::stage_strat );
  
  if ( to_console ) 
    {
      logger << "  Confusion matrix: " << suds_t::n_stages
	     << "-level classification: kappa = " << kappa << ", acc = " << acc << ", MCC = " << mcc << "\n\n";
      writer.level( 5 , "NSS" );
      suds_t::tabulate(  prd , suds_t::str( obs_stage_valid ) , true );
      writer.unlevel( "NSS" );
    }      
  
  // collapse 5->3?
  if ( suds_t::n_stages == 5 )
    {
      
      double kappa3 = MiscMath::kappa( suds_t::NRW( prd ) , suds_t::NRW( suds_t::str( obs_stage_valid ) ) , suds_t::str( SUDS_UNKNOWN ) );

      // nb. 'truth' / pred
      double macro_f1 = 0 , macro_precision = 0 , macro_recall = 0;
      double wgt_f1 = 0 , wgt_precision = 0 , wgt_recall = 0 , mcc = 0;
      std::vector<double> precision, recall, f1;
      std::vector<std::string> lab3 = { "NR" , "R" , "W" };
      
      double acc3 = MiscMath::accuracy( suds_t::NRW( suds_t::str( obs_stage_valid ) ) , suds_t::NRW( prd ) ,
					suds_t::str( SUDS_UNKNOWN ) , 
					&lab3 ,
					&precision, &recall, &f1,
					&macro_precision, &macro_recall, &macro_f1 ,
					&wgt_precision, &wgt_recall, &wgt_f1 , &mcc );
      
      writer.value( "K3" , kappa3 );
      writer.value( "ACC3" , acc3 );
      
      writer.value( "F13" , macro_f1 );
      writer.value( "MCC3" , mcc );
      writer.value( "PREC3", macro_precision );
      writer.value( "RECALL3" , macro_recall );

      if ( to_console )
	{
	  logger << "\n  Confusion matrix: 3-level classification: kappa = " << kappa3 << ", acc = " << acc3 << ", MCC = " << mcc << "\n\n";
	  writer.level( 3 , "NSS" );
	  suds_t::tabulate(  suds_t::NRW( prd ) , suds_t::NRW( suds_t:: str( obs_stage_valid ) ) , true );
	  writer.unlevel( "NSS" );
	}
    }

  if ( to_console ) 
    logger << std::defaultfloat << std::setprecision(6);
  
}


void suds_indiv_t::write_annots( const std::string & annot_folder , const std::string & aname , 
				 const Eigen::MatrixXd & pp , const std::vector<std::string> & labels , 
				 int ne_all , edf_t & edf )
{

  bool prior_staging = obs_stage.size() != 0 ;
  if ( ! prior_staging ) return;

  const std::string delim = annot_folder[ annot_folder.size() - 1 ] != '/' ? "/" : "";
  
  if ( annot_folder != "" && annot_folder != "./" ) 
    {
      std::string syscmd = globals::mkdir_command + " " + annot_folder ;
      int retval = system( syscmd.c_str() );
    }
  
  // annot label
  annot_t * a_disc3 = edf.timeline.annotations.add( aname + "_disc3" );
  a_disc3->description = "SOAP NR/R/W discordance";

  annot_t * a_disc5 = NULL;
  if ( suds_t::n_stages == 5 )
    {
      a_disc5 = edf.timeline.annotations.add( aname + "_disc5" );
      a_disc5->description = "SOAP N1/N2/N3/R/W discordance";
    }

  annot_t * a_unscr = edf.timeline.annotations.add( aname + "_unscr" );
  a_unscr->description = "SOAP unscored epoch";

  const std::string a_filename3 = annot_folder + delim + aname + "_disc3.annot";
  const std::string a_filename5 = annot_folder + delim + aname + "_disc5.annot";
  const std::string a_filenameU = annot_folder + delim + aname + "_unscr.annot";

  logger << "  writing NR/R/W discordant epochs to " << a_filename3 << "\n";
  if ( suds_t::n_stages == 5 )
    logger << "  writing N1/N2/N3/R/W discordant epochs to " << a_filename5 << "\n";  
  logger << "  writing unscored epochs to " << a_filenameU << "\n";
   
  // epochs[] contains the codes of epochs actually present in the model/valid
  std::map<int,int> e2e;
  for (int i=0; i < epochs.size(); i++) 
    e2e[ epochs[i] ] = i ;  
  
  for ( int i=0;i<ne_all;i++)
    {
      int e = -1;
      if ( e2e.find( i ) != e2e.end() ) e = e2e[i];
            
      // track interval
      interval_t interval = edf.timeline.epoch( i );

      // value found in scoring?
      if ( e != -1 ) 
	{
	  std::string predss = suds_t::max_inrow( pp.row(e) , labels );

	  if ( suds_t::n_stages == 5 )
	    {
	      if ( predss !=  suds_t::str( obs_stage[i] ) )
		a_disc5->add( suds_t::str( obs_stage[i] ) + "->" + predss , interval , "." );
	      
	      if ( suds_t::NRW( predss ) != suds_t::NRW( suds_t::str( obs_stage[i] ) ) )
		a_disc3->add( suds_t::NRW( suds_t::str( obs_stage[i] ) ) + "->" + suds_t::NRW( predss ) , interval , "." );
	    }
	  else
	    {
	      if ( predss !=  suds_t::str( obs_stage[i] ) )
		a_disc3->add( suds_t::str( obs_stage[i] ) + "->" + predss , interval , "." );
	    }
	}
      else
       	{
	  a_unscr->add( "." , interval , "." );
	}
      
    }

  a_disc3->save( a_filename3 );

  if ( a_disc5 ) 
    a_disc5->save( a_filename5 );

  a_unscr->save( a_filenameU );

}



void suds_indiv_t::dump_trainer_epoch_matrix( edf_t & edf ,
					      std::map<trkap_t,std::vector<suds_stage_t> > & p ,
					      std::map<std::string,double> & wgt ,
					      const std::string & filename )
{

  if ( filename == "" ) Helper::halt( "empty file name" );
  
  std::ofstream P1( Helper::expand( filename ).c_str() , std::ios::out );

  // header
  
  std::map<int,int> e2e;
  for (int i=0; i<epochs.size(); i++)
    e2e[ epochs[i] ] = i ;  
  const int ne_all = edf.timeline.num_epochs();

  // do all epochs
  P1 << "TRAINER\tK\tWGT";
  for (int i=0; i< ne_all; i++)
    P1 << "\tE" << i+1;
  P1 << "\n";
  
  // iterator over trainers
  std::map<trkap_t,std::vector<suds_stage_t> >::const_iterator pp = p.begin();
  
  while ( pp != p.end() )
    {
      P1 << pp->first.id << "\t" << pp->first.k;

      if ( wgt.find( pp->first.id ) != wgt.end() )
	P1 << "\t" << wgt[ pp->first.id ];
      else
	P1 << "\tNA";
	  
      for (int i=0; i< ne_all; i++)
	{
	  int e = -1;
	  if ( e2e.find( i ) != e2e.end() ) e = e2e[i];
	  if ( e == -1 )
	    {
	      P1 << "\t?";
	    }
	  else
	    {
	      P1 << "\t" << suds_t::str( pp->second[ e ] );
	    }
	}

      P1 << "\n";
      
      // next trainer
      ++pp;
    }
  
  // all done
  P1.close();
  
}


