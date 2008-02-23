// $Id: xxFilename.cc 2641 2007-09-02 21:31:02Z flaterco $

/*  xxFilename  Get a file name from the user.  If successful, do
    caller.save (filename).

    Copyright (C) 1998  David Flater.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "xtide.hh"
#include "xxSimplePrompt.hh"
#include "xxFilename.hh"


void xxFilename::callback (const Dstr &value) {
  if (value.strchr ('\n') != -1 ||
      value.strchr ('\r') != -1 ||
      value.strchr (' ') != -1 ||
      value[0] == '-') {
    Dstr details ("Well, it's not that I can't do it, it's that you probably\n\
don't want me to.  The filename that you entered is '");
    details += value;
    details += "'.\n";
    if (value[0] == '-')
      details += "Filenames that begin with a dash are considered harmful.";
    else
      details += "Whitespace in filenames is considered harmful.";
    Global::barf (Error::CANT_OPEN_FILE, details, Error::nonfatal);
  } else
    _caller.save (value);
}


xxFilename::xxFilename (const xxWidget &parent,
			xxPredictionWindow &caller,
			constString initName):
  xxSimplePrompt (parent, caller, "Enter Filename") {
  construct ("Enter filename.", initName);
}

// Cleanup2006 Done
