
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


#include "luna.h"

globals global;

writer_t writer;

logger_t logger( "+++ luna" );

std::set<std::string>              cmd_t::commands;
std::string                        cmd_t::input = "";
std::string                        cmd_t::cmdline_cmds = "";
std::string                        cmd_t::stout_file = "";
std::string                        cmd_t::stout_template = "";
bool                               cmd_t::append_stout_file = false;
bool                               cmd_t::has_indiv_wildcard = false;

bool                               cmd_t::plaintext_mode = false;
std::string                        cmd_t::plaintext_root = ".";

std::map<std::string,std::string>  cmd_t::vars;
std::map<std::string,std::map<std::string,std::string> >  cmd_t::ivars;
std::map<std::string,std::string>  cmd_t::idmapper;

std::set<std::string>              cmd_t::specials;

std::set<std::string>              cmd_t::signallist;
std::map<std::string,std::string>  cmd_t::label_aliases;
std::map<std::string,std::vector<std::string> >  cmd_t::primary_alias;
std::map<std::string,std::string>  cmd_t::primary_upper2orig;
