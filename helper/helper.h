
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

#ifndef __HELPER_H__
#define __HELPER_H__

#include <iostream>

#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <stdint.h>
#include <map>
#include <cmath>

struct interval_t;
struct clocktime_t;
class gzifstream;

namespace Helper 
{

  std::string toupper( const std::string & );  


  // trim from start
  // static inline std::string &ltrim(std::string &s) {
  //   s.erase(s.begin(), std::find_if( s.begin(), s.end(),  std::not1(std::ptr_fun<int, int>(std::isspace))  ));
  //   return s;
  // }

  // trim from end
  // static inline std::string &rtrim( std::string s ) {
  //    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
  //    return s;
  //  }

  // Updated for C++11 using lambda function; trim now do not modify existing string...
  
  // trim from start
  static inline std::string ltrim( std::string s ) {
    s.erase(s.begin(), std::find_if( s.begin(), s.end(),  [](int c) {return !std::isspace(c);} ));
    return s;
  }
 
  // trim from end
  static inline std::string rtrim(std::string s) {
    s.erase(std::find_if( s.rbegin(), s.rend(),  [](int c) {return !std::isspace(c);} ).base(), s.end() );
    return s;
  }
  
  // trim from both ends
  static inline std::string lrtrim( std::string s ) {
    return ltrim(rtrim(s));
  }

  static inline std::string unquote(const std::string &s , const char q2 = '"' ) {
    int a = ( s[0] == '"' || s[0] == q2 ) ? 1 : 0;
    int b = ( s[s.size()-1] == '"' || s[s.size()-1] == q2 ) ? 1 : 0 ;
    return s.substr(a,s.size()-a-b);
  }

  std::string remove_all_quotes(const std::string &s , const char q2 = '"' );
  std::string quote_spaced( const std::string & s );

  std::string quote_if( const std::string & s , char q );
  std::string quote_if( const std::string & s , char q , char p );

  std::string sanitize( const std::string & , const char except );
  std::set<std::string> sanitize( const std::set<std::string> & , const char except  ); 
  std::string sanitize( const std::string & , const std::set<char> * except = NULL );
  std::set<std::string> sanitize( const std::set<std::string> & , const std::set<char> * except = NULL ); 
  
  bool yesno( const std::string & );
  
  std::string search_replace( const std::string & , char a , char b );

  std::string search_replace( std::string s , const std::string & a , const std::string & b , const bool only_root = false );
  
  void swap_in_variables( std::string * , std::map<std::string,std::string> * );

  bool swap_in_includes( std::string * , const std::string & delim = "," );
			
  void expand_numerics( std::string * );

  void process_block_conditionals( std::string * , const std::map<std::string,std::string> & );

  std::string insert_indiv_id( const std::string & id , const std::string & str );

  bool file_extension( const std::string & , const std::string & , bool with_period = true );
  
  bool is_folder( const std::string & f );

  void build_sample_list( const std::vector<std::string> & );

  void merge_EDFs( const std::vector<std::string> & );

  void repath_SL( const std::vector<std::string> & );
  
  void compile_txttabs( const std::string & );

  std::vector<std::string> file2strvector( const std::string & );

  // case insenstive string comparison
  bool iequals(const std::string& a, const std::string& b);

  // case insenstive string-root match
  bool imatch(const std::string& a, const std::string& b , unsigned int min = 0 );

  // case-insensitive any match
  bool contains( const std::string& a, const std::string& b );
  
  // print vector
  template <class T> std::string print( const std::vector<T> & x , const std::string & label , const int l )
    {
      std::stringstream ss;
      if ( label != "" )     
	ss << "--- " << label << " ---\n";
      int n = l > x.size() ? x.size() : l ;
      for (int i=0;i<n;i++) ss << i << " [ " << x[i] << " ]\n";
      return ss.str();
    }

  bool fileExists(const std::string &);
  std::string expand( const std::string & f );
  bool deleteFile( const std::string & );
  
  std::vector<std::string> file2strvector( const std::string & filename );

  std::istream& safe_getline(std::istream& is, std::string& t);
  gzifstream & zsafe_getline( gzifstream & is , std::string& t);

  void halt( const std::string & msg );
  void warn( const std::string & msg );
  void debug( const std::string & msg );
  bool realnum(double d);
  bool similar( double a, double b , double EPS = 1e-6 );
  
  std::string int2str(int n);  
  std::string int2str(long n);
  std::string int2str(uint64_t n);
  std::string dbl2str(double n);  
  std::string dbl2str(double n, int dp);  
  std::string dbl2str_fixed(double n, int ch );
  std::string brief( const std::string & , int l = 40);
/*   std::string stringize( const std::vector<std::string> & ); */
/*   std::string stringize( const std::set<std::string> & , const std::string & delim = "," ); */

  std::string readfile( const std::string & file );

  void ascii7( std::string * s , char repl );
  void ascii7( std::vector<char> * s , char repl );
  

  template<typename T> 
    std::string stringize( const T & t , const std::string & delim = "," )
    {
      std::stringstream ss;
      
      typename T::const_iterator tt = t.begin();
      while ( tt != t.end() )
	{
	  if ( tt != t.begin() ) ss << delim;
	  ss << *tt;
	  ++tt;
	}
      return ss.str();
    }
  
  
  std::string trim( const std::string & s , const char c = ' ' , const char d = ' ' );
  std::string format( const std::string & , int indent = 10 , int width = 60, bool no_initial_indent = true );

  
  bool str2dbl(const std::string & , double * ); 
  bool str2int(const std::string & , int * ); 
  bool str2int64(const std::string & , uint64_t * ); 

  template <class T>
    bool from_string(T& t,
		     const std::string& s,
		     std::ios_base& (*f)(std::ios_base&))
    {
      std::istringstream iss(s);
      return !(iss >> f >> t).fail();
    }
  
  uint64_t sec2tp( double );
  double tp2sec( uint64_t );


  template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
  }

  // vector --> string
  template <class T> std::set<T> vec2set( const std::vector<T> & x )
  {
    std::set<T> s;
    for (int i=0; i<x.size(); i++) s.insert(x[i]);
    return s;
  }

  std::vector<std::string> parse(const std::string & item, const std::string & s = " \t\n" , bool empty = false );
  std::vector<std::string> parse(const std::string & item, const char s , bool empty = false );
  
  std::vector<std::string> quoted_parse(const std::string & item , const std::string & s , const char q = '"' , const char q2 = '\'' , bool empty = false );
  std::vector<std::string> quoted_parse(const std::string & item , const char s , const char q = '"' , const char q2 = '\'' , bool empty = false );

  std::vector<std::string> char_split( const std::string & s , const char c , bool empty );
  std::vector<std::string> char_split( const std::string & s , const char c , const char c2 , bool empty );
  std::vector<std::string> char_split( const std::string & s , const char c , const char c2 , const char c3 , bool empty );

  std::vector<std::string> quoted_char_split( const std::string & s , const char c , const char q , const char q2, bool empty );
  std::vector<std::string> quoted_char_split( const std::string & s , const char c , const char c2 , const char q , const char q2, bool empty );
  std::vector<std::string> quoted_char_split( const std::string & s , const char c , const char c2 , const char c3 , const char q , const char q2, bool empty );


  // time-string
  std::string timestring( uint64_t msec , char delim = '.' , bool fractional = true ); 
  std::string timestring( int h , int m , double s , char delim = '.' , bool fractional = false );
  std::string timestring( const std::string & , const interval_t & , char delim = '.' , const std::string & delim2 = " - " );

  double position( uint64_t a , uint64_t tot , int * h , int * m , double *s);  
  bool timestring( const std::string & , int * h, int *m , double *s );
  bool add_clocktime( int *h , int *m , double *s , uint64_t a );

  // given a clock time and an interval, get an output-friendly timestamp (x2)
  bool hhmmss( const clocktime_t & ct , const interval_t & interval , std::string * t1 , std::string * t2 , const int dp = 4 );

  
}


struct clocktime_t
{
  
  // default (midnight)
  clocktime_t() 
  {
    valid = true;
    d=h=m=0;
    s=0.0;
  }

  // convert time-string to internal 
  clocktime_t( const std::string & t );
  
  // assume from hours, fractional
  //  clocktime_t( double ); 
  
  clocktime_t( int h, int m, double s ) 
  : valid(true) , d(0) , h(h), m(m), s(s)
  { 
    if ( h < 0 || m < 0 || s < 0 ) valid = false;
    if ( h > 23 || m > 59 || s >= 60.0 ) valid = false;
  } 

  // with day specified
  clocktime_t( int d , int h, int m, double s )
  : valid(true) , d(d) , h(h), m(m), s(s) 
  { 
    if ( d < 0 ) valid = false;
    if ( h < 0 || m < 0 || s < 0 ) valid = false;
    if ( h > 23 || m > 59 || s >= 60.0 ) valid = false;
  } 

  clocktime_t( const clocktime_t & t1 ) { copy(t1); } 

  clocktime_t & operator= (const clocktime_t & t1 ) { copy(t1); return *this; }
    
  void copy( const clocktime_t & t1 )
  {
    d = t1.d;
    h = t1.h;
    m = t1.m;
    s = t1.s;
    valid = t1.valid;
  }
  
  void reset() 
  {
    valid = true;
    d = 0; h = 0; m=0; s=0.0;
  }

  bool valid;
  int d;
  int h;
  int m;
  double s;

  std::string as_string( const char tchar = '.' , bool fractional = false ) 
  {
    if ( ! valid ) return "NA";
    return Helper::timestring( h,m,s, tchar , fractional );
  }

  std::string as_numeric_string() 
  {
    if ( ! valid ) return "NA";
    return Helper::dbl2str( hours() );
  }

  // calculate mid-point between two times
  bool midpoint( const clocktime_t & t1 , const clocktime_t & t2 );
  
  double minutes() const 
  {
    return h*60 + m + s/(double)60; 
  }
  
  double hours() const
  {
    return h + m/(double)60 + s/(double)(60*60);
  }
  
  double seconds() const
  {
    return h*60*60 + m*60 + s ;
  }

  int rounded_seconds() const
  {
    int si = floor(s);
    if ( s - si > 0.5 ) ++si;
    return h*60*60 + m*60 + si ;
  }
  
  void advance_1second()
  {
    ++s;
    // check we don't wrap mins or hours
    if ( s >= 60 ) 
      {
	++m;
	s -= 60.0;
	if ( m == 60 ) 
	  {
	    ++h;
	    m = 0;
	    if ( h == 24 ) h = 0;
	  }	
      }
    
  }

  bool convert( double hrs ) 
  {
    valid = true;
    if ( hrs < 0 ) valid = false;
    if ( hrs > 24 ) valid = false;
    if ( ! valid ) return false;
    
    double t_hours = hrs;
    double t_mins  = hrs * 60.0;
    double t_secs  = hrs * 3600.0;

    t_mins -= floor(t_hours) * 60 ;
    t_secs -= floor(t_hours) * 3600 + floor(t_mins) * 60;
    
    h = floor(t_hours);
    m = floor(t_mins);
    s = t_secs;
    
    return true;

  }
  

  bool convert_seconds( double sec ) 
  {
    valid = true;
    if ( sec < 0 ) valid = false;
    if ( sec > 86400 ) valid = false;
    if ( ! valid ) return false;
    
    double t_hours = floor( sec / 3600.0 );
    sec -= t_hours * 3600.0;
    
    double t_mins  = floor( sec / 60.0 );
    sec -= t_mins * 60.0;
    
    h = t_hours;
    m = t_mins;
    s = sec;
     
    return true;

  }

  void advance_tp( uint64_t tp );

  void advance_hrs( double hrs ) 
  { 
    double t_hrs = hours(); 
    
    t_hrs += hrs; 
    
    // need to wrap?  
    while ( 1 ) 
      { 
   	if ( t_hrs >= 0 && t_hrs < 24 ) break; 
   	if ( t_hrs < 0 ) t_hrs += 24.0; 
   	else if ( t_hrs >= 24 ) t_hrs -= 24.0; 
      }     
    
    // update this time back to usual format 
    convert( t_hrs ); 
  } 
  
  void advance_seconds( double secs )
  {

    double t_sec = seconds();

    t_sec += secs;
    
    // need to wrap? (86400 seconds in a day)
    while ( 1 )
      {
	if ( t_sec >= 0 && t_sec < 86400 ) break;
	if ( t_sec < 0 ) t_sec += 86400.0;
	else if ( t_sec >= 86400.0 ) t_sec -= 86400.0;
      }    
    
    // update this time back to usual format
    convert_seconds( t_sec );
  }

  //
  void advance( const clocktime_t & t ) 
  {

    if ( ! t.valid ) { valid = false; return; }

    double secs = seconds();
    double secs2 = t.seconds();

    //advance
    secs += secs2;

    // need to wrap? 
    // seconds in the day = 0 .. 86400

    while ( 1 )
      {
	if ( secs >= 86400 ) secs -= 86400;
	else break;
      }

    // convert back to h/m/s

    double t_secs   = secs;
    double t_mins   = secs / 60.0;
    double t_hours  = secs / 3600.0;
    
    t_mins -= floor(t_hours) * 60 ;
    t_secs -= floor(t_hours) * 3600 + floor(t_mins) * 60;
 
    h = floor(t_hours);
    m = floor(t_mins);
    s = t_secs;
        
  }

  // this *tests* whether t1 comes before t2 (return 1 or 2 for which comes first)
  //  based on smallest difference going from t1 --> t2   or t1 --> 2
  //    i.e. all these would have t1 first 
  //       09:00 09:10
  //       22:00 02:00    ( as 4 < 20 ) 
  static int earlier( const clocktime_t & t1 , const clocktime_t & t2 )
  {
    double d1 = difference_seconds( t1 , t2 );
    double d2 = difference_seconds( t2 , t1 );
    if ( d1 < d2 ) return 1;
    if ( d2 < d1 ) return 2;
    return 0;
  }
    
  // this *assumes* that t1 comes before t2
  static double difference_hours( const clocktime_t & t1 , const clocktime_t & t2 )
  {
    // we assume t1 happens before t2
    // thus  22 8   means from 22 to 8
    // (not 8 to 22)
    // and assume time wrap is always within 1 day

    double t1h = t1.hours();
    double t2h = t2.hours();
    
    if ( t2h < t1h ) // e.g. 22 to 8 , means it wraps
      {
	return 24.0 - t1h + t2h;
      }
    else // no overnight wrap, 
      {
	return t2h - t1h;
      }
    
    return 0;
  }


  static double difference_seconds( const clocktime_t & t1 , const clocktime_t & t2 )
  {
    // we assume t1 happens before t2
    // thus  22 8   means from 22 to 8
    // (not 8 to 22)
    // and assume time wrap is always within 1 day

    double t1s = t1.seconds();
    double t2s = t2.seconds();
    
    if ( t2s < t1s ) 
      {
	return 86400.0 - t1s + t2s;
      }
    else // no overnight wrap, 
      {
	return t2s - t1s;
      }
    
    return 0;
  }

};




#endif

