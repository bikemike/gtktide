// $Id: CalendarFormL.cc 2641 2007-09-02 21:31:02Z flaterco $

/*  Calendar  Manage construction, organization, and printing of calendars.

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

#include "common.hh"
#include "Calendar.hh"
#include "CalendarFormNotC.hh"
#include "CalendarFormL.hh"


// All lengths are in mm.
// Magic number 25.4 mm is hardcoded in LaTeX (1 inch margins).
// We care about the order of initialization this time.
CalendarFormL::CalendarFormL (Station &station,
                              Timestamp startTime,
			      Timestamp endTime,
			      Mode::Mode mode):
  CalendarFormNotC (station, startTime, endTime, mode, Format::LaTeX),
  pageHeight(Global::settings["ph"].d),
  pageWidth(Global::settings["pw"].d),
  pageMargin(Global::settings["pm"].d),
  textHeight(pageHeight - 2 * pageMargin),
  textWidth(pageWidth - 2 * pageMargin),
  topMargin((pageHeight - textHeight) / 2.0 - 25.4),
  sideMargin((pageWidth - textWidth) / 2.0 - 25.4) {}


void CalendarFormL::printPV (const PredictionValue &pv, Dstr &buf) {
  Dstr temp;
  pv.printnp (temp);
  temp.repstr (" ", "$~");
  temp *= '$';
  buf += temp;
}


void CalendarFormL::monthBanner (Dstr &buf, Date date) {
  Dstr temp (_station.name);
  temp.LaTeX_mangle();
  buf += "{\\Large\\bf \\begin{tabularx}{";
  buf += textWidth;
  buf += "mm}{Lr}\n";
  buf += temp;
  buf += " & \\hspace{5mm}";
  if (_station.coordinates.isNull())
    buf += "Coordinates unknown";
  else {
    _station.coordinates.print (temp);
    buf += temp;
  }
  buf += "\\\\\n\\end{tabularx}}\n\n";

  if (isCurrent) {
    buf += "{\\large Flood direction ";
    if (_station.maxCurrentBearing.isNull())
      buf += "unspecified";
    else {
      _station.maxCurrentBearing.print (temp);
      buf += temp;
    }
    buf += " \\hfill Ebb direction ";
    if (_station.minCurrentBearing.isNull())
      buf += "unspecified";
    else {
      _station.minCurrentBearing.print (temp);
      buf += temp;
    }
    buf += "}\n\n";
  }

  if (!(_station.note.isNull())) {
    buf += "Note:  ";
    temp = _station.note;
    temp.LaTeX_mangle();
    buf += temp;
    buf += "\n\n";
  }

  date.printCalendarHeading (temp);
  buf += "\\vspace{5mm}\\centerline{\\LARGE\\bf ";
  buf += temp;
  buf += "}\n\n";
}


void CalendarFormL::hardLineBreak (Dstr &buf) {
  buf += "\n\n"; // Normal LaTeX paragraph break
}


void CalendarFormL::startDoc (Dstr &buf) {
  buf += "% LaTeX incantation by XTide.\n\
% Run this file through pdflatex to get a PDF.\n\
\\documentclass{article}\n\
\\nofiles\n\
\\pagestyle{empty}\n\
\\usepackage{tabularx}\n\
\\renewcommand{\\tabularxcolumn}[1]{m{#1}}\n\
\\newcolumntype{C}{>{\\centering\\arraybackslash}X}\n\
\\newcolumntype{L}{>{\\raggedright\\arraybackslash}X}\n\
\\usepackage[latin1]{inputenc}\n\
\\frenchspacing\n\
\n\
\\oddsidemargin ";
  buf += sideMargin;
  buf += "mm\n\\topmargin ";
  buf += topMargin;
  buf += "mm\n\\headheight 0mm\n\
\\headsep 0mm\n\
\\textheight ";
  buf += textHeight;
  buf += "mm\n\\textwidth ";
  buf += textWidth;
  buf += "mm\n\\parindent 0mm\n\
\\linespread{0.8}\n\
\\tabcolsep 0mm\n\
\n\
\\pdfpagewidth ";
  buf += pageWidth;
  buf += "mm\n\\pdfpageheight ";
  buf += pageHeight;
  buf += "mm\n\\pdfcompresslevel 9\n\
\n\
\\begin{document}\n";
}


void CalendarFormL::endDoc (Dstr &buf) {
  buf += "\\end{document}\n";
}


void CalendarFormL::pageBreak (Dstr &buf) {
  buf += "\\newpage\n";
}


void CalendarFormL::startTable (Dstr &buf, unsigned numcols) {
  buf += "\\begin{tabularx}{";
  buf += textWidth;
  buf += "mm}{|";
  buf += (_mode == Mode::altCalendar ? 'C' : 'c');
  buf += '|';
  for (unsigned a=1; a<numcols; ++a)
    buf += "C|";
  buf += "}\\hline\n";
}


void CalendarFormL::endTable (Dstr &buf) {
  buf += "\\end{tabularx}\n\n";
}


void CalendarFormL::startRow (Dstr &buf,
                              HeadersBool headers) {
  if (headers == isHeader)
    buf += "\\textbf{";
}


void CalendarFormL::endRow (Dstr &buf, HeadersBool headers) {
  if (headers == isHeader)
    buf += '}';
  buf += "\\\\\\hline\n";
}


void CalendarFormL::cellSep (Dstr &buf, HeadersBool headers) {
  if (headers == isHeader)
    buf += "} & \\textbf{";
  else
    buf += " & ";
}


void CalendarFormL::startBold (Dstr &buf) {
  buf += "\\textbf{";
}


void CalendarFormL::endBold (Dstr &buf) {
  buf += '}';
}


void CalendarFormL::startHigh (Dstr &buf) {
  buf += "\\raisebox{5mm}{";
}


void CalendarFormL::endHigh (Dstr &buf) {
  buf += '}';
}


void CalendarFormL::startLow (Dstr &buf) {
  buf += "\\raisebox{-5mm}{";
}


void CalendarFormL::endLow (Dstr &buf) {
  buf += '}';
}

// Cleanup2006 Done
