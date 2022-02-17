
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

#include "sstore/sstore.h"
#include <iostream>
#include "helper/helper.h"
#include "helper/logger.h"

extern logger_t logger;

//
// loadss, a simple loader for a sstore_t 
// 

int main(int argc , char ** argv )
{
  
  logger.off();
  
  if ( argc != 3 ) 
    { 
      std::cerr << "usage: ./loadss {ss.db} {-a|-e|-i|index|unindex} < input\n"
		<< "where ss.db      --> sstore_t database file\n"
		<< "      [-a|-e|-i] --> to specify baseline/epoch-level/interval-level data\n"	
		<< "      input      --> as prepared by prepss\n"
		<< "\n";          
      std::exit(1); 
    } 
  

  // 
  // Input format, tab-delimited (START/STOP are in SECONDS)
  //
  
  // all      :   ID LVL CH              N VALUE(S)
  // epoch    :   ID LVL CH  E           N VALUE(S)
  // interval :   ID LVL CH  START STOP  N VALUE(S)
  
  

  // LVL and CH are optional ( set to . if missing)

  std::string filename = argv[1];
  std::string mode     = argv[2];
  
  // special case of indexing/dropping index
  
  if ( mode == "index" ) 
    {
      sstore_t ss( filename );
      ss.index();
      std::exit(0);
    }

  if ( mode == "unindex" ) 
    { 
      sstore_t ss( filename );
      ss.drop_index();
      std::exit(0);
    }

  
  bool mode_baseline = mode == "-a";
  bool mode_epoch    = mode == "-e";
  bool mode_interval = mode == "-i";
  
  if ( ! ( mode_baseline || mode_epoch || mode_interval ) ) 
    {
      Helper::halt( "mode argument should be -a, -e or -i" );
    }
  
  //
  // Open/create sstore_t
  //

  sstore_t ss( filename );

  ss.begin();

  ss.drop_index();
  
  int lines = 0 ; 

  while ( ! std::cin.eof() ) 
    {
      std::string line;
      Helper::safe_getline( std::cin , line );
      if ( std::cin.eof() ) break;
      std::vector<std::string> tok = Helper::parse( line , "\t" );

      const int t = tok.size();
      if ( tok.size() == 0 ) continue;

      std::cerr << "read " << ++lines << " lines\n";
      
      //
      // Baseline-level inputs
      //
      
      if ( mode_baseline )
	{
	  
	  if ( t < 5 ) Helper::halt( "base: format problem:\n" + line );
	  
	  int n;
	  if ( ! Helper::str2int( tok[3] , &n ) ) 
	    Helper::halt( "format problem" );
	  
	  int expected = 5; 
	  if ( n > 1 ) expected += n - 1; 
	  if ( t != expected ) Helper::halt( "format problem:\n" + line );
	  	  
	  bool has_level = tok[1] != ".";
	  bool has_channel = tok[2] != ".";
	  
	  const std::string * level_ptr = has_level ? &(tok)[1] : NULL ; 
	  const std::string * channel_ptr = has_channel ? &(tok)[2] : NULL ; 
	  
	  if ( n == 0 ) // text
	    {
	      ss.insert_base( tok[0] , tok[4] , channel_ptr , level_ptr );
	    }
	  else if ( n == 1 ) // double 
	    {
	      double d;
	      if ( ! Helper::str2dbl( tok[4] , &d ) ) 
		Helper::halt( "format problem, expecting double:\n" + line );
	      
	      ss.insert_base( tok[0] , d , channel_ptr , level_ptr );
	      
	    }
	  else // array of doubles
	    {
	      std::vector<double> d( n , 0 );
	      for (int i=0;i<n;i++)
		if ( ! Helper::str2dbl( tok[4+i] , &(d)[i] ) ) 
		  Helper::halt( "format problem, expecting double:\n" + line );

		ss.insert_base( tok[0] , d , channel_ptr , level_ptr );
	      
	    }
	}
      
      //
      // Epoch-level inputs 
      //

      if ( mode_epoch ) 
	{
	  
	  if ( t < 6 ) Helper::halt( "format problem:\n" + line );
	  
	  int n;
	  if ( ! Helper::str2int( tok[4] , &n ) ) 
	    Helper::halt( "format problem:\n" + line );
	  
	  int e;
	  if ( ! Helper::str2int( tok[3] , &e ) ) 
	    Helper::halt( "format problem:\n" + line  );

	  int expected = 6; 
	  if ( n > 1 ) expected += n - 1; 
	  if ( t != expected ) Helper::halt( "format problem:\n" + line );
	  
	  bool has_level   = tok[1] != ".";
	  bool has_channel = tok[2] != ".";

	  const std::string * level_ptr = has_level ? &(tok)[1] : NULL ; 
	  const std::string * channel_ptr = has_channel ? &(tok)[2] : NULL ; 
	  
	  if ( n == 0 ) // text
	    {
	      ss.insert_epoch( e , tok[0] , tok[5] , channel_ptr , level_ptr );	      
	    }
	  else if ( n == 1 ) // double 
	    {
	      double d;
	      if ( ! Helper::str2dbl( tok[5] , &d ) ) 
		Helper::halt( "format problem, expecting double:\n" + line );
	      
	      ss.insert_epoch( e, tok[0] , d , channel_ptr , level_ptr );
	      
	    }
	  else // array of doubles
	    {
	      std::vector<double> d( n , 0 );
	      for (int i=0;i<n;i++)
		if ( ! Helper::str2dbl( tok[5+i] , &(d)[i] ) ) 
		  Helper::halt( "format problem, expecting double:\n" + line );

	      ss.insert_epoch( e, tok[0] , d , channel_ptr , level_ptr );
	      
	    }
	  
	}



      //
      // Interval-level inputs
      //

      if ( mode_interval )
	{
	  
	  if ( t < 7 ) Helper::halt( "format problem:\n" + line );
	  
	  int n;
	  if ( ! Helper::str2int( tok[5] , &n ) ) 
	    Helper::halt( "format problem:\n" + line );
	  
	  double a;
	  if ( ! Helper::str2dbl( tok[3] , &a ) ) 
	    Helper::halt( "format problem:\n" + line  );

	  double b;
	  if ( ! Helper::str2dbl( tok[4] , &b ) ) 
	    Helper::halt( "format problem:\n" + line  );

	  int expected = 7; 
	  if ( n > 1 ) expected += n - 1; 
	  if ( t != expected ) Helper::halt( "format problem:\n" + line );
	  
	  bool has_level   = tok[1] != ".";
	  bool has_channel = tok[2] != ".";
	  
	  const std::string * level_ptr = has_level ? &(tok)[1] : NULL ; 
	  const std::string * channel_ptr = has_channel ? &(tok)[2] : NULL ; 
	  
	  if ( n == 0 ) // text
	    {
	      ss.insert_interval( a , b , tok[0] , tok[6] , channel_ptr , level_ptr );
	    }
	  else if ( n == 1 ) // double 
	    {
	      double d;
	      if ( ! Helper::str2dbl( tok[6] , &d ) ) 
		Helper::halt( "format problem, expecting double:\n" + line );
	      
	      ss.insert_interval( a, b , tok[0] , d , channel_ptr , level_ptr );

	    }
	  else // array of doubles
	    {
	      std::vector<double> d( n , 0 );
	      for (int i=0;i<n;i++)
		if ( ! Helper::str2dbl( tok[6+i] , &(d)[i] ) ) 
		  Helper::halt( "format problem, expecting double:\n" + line );
	      
	      ss.insert_interval( a, b , tok[0] , d , channel_ptr , level_ptr );
	      
	    }
	  
	}
      
      // next row of input
    }
    
  std::cerr << "indexing... ";
    
  ss.index();

  ss.commit();

  // all      :   ID CH LVL             N VALUE(S)
  // epoch    :   ID CH LVL E           N VALUE(S)
  // interval :   ID CH LVL START STOP  N VALUE(S)
  
  std::cerr << "done\n";

  std::exit(0);
}
