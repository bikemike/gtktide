// $Id: TTYGraph.cc 2946 2008-01-18 23:12:25Z flaterco $

/*  TTYGraph  Graph implemented on dumb terminal

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
#include "Graph.hh"
#include "TTYGraph.hh"


// Fudge factor to correct aspect ratio on TTY
// Correct 80x24 of VT100 to match 4/3 aspect
static const double TTYaspectfudge = 2.5;


TTYGraph::TTYGraph (unsigned xSize, unsigned ySize, GraphStyle style):
  Graph (xSize, ySize, style) {
  assert (xSize >= Global::minTTYwidth && ySize >= Global::minTTYheight);
  tty.resize (xSize * ySize);
}


const unsigned TTYGraph::stringWidth (const Dstr &s) const {
  return s.length();
}


const unsigned TTYGraph::fontHeight() const {
  return 1;
}


const unsigned TTYGraph::hourTickLen() const {
  return 1;
}


const unsigned TTYGraph::depthLabelLeftMargin() const {
  return 0;
}


const unsigned TTYGraph::depthLabelRightMargin() const {
  return 1;
}


const unsigned TTYGraph::depthLineVerticalMargin() const {
  return 0;
}


const double TTYGraph::aspectFudgeFactor() const {
  return TTYaspectfudge;
}


const unsigned TTYGraph::startPosition (unsigned labelWidth) const {
  return labelWidth;
}


const int TTYGraph::blurbMargin() const {
  return 0;
}


void TTYGraph::setPixel (int x, int y, Colors::Colorchoice c) {
  assert (c < (int)Colors::numColors);
  char color_analog;
  switch (c) {
  case Colors::daytime:
    color_analog = ' ';
    break;
  case Colors::nighttime:
    color_analog = '·'; // 0xB7, middle dot
    break;
  default:
    color_analog = '*';
  }
  setPixel (x, y, color_analog);
}


void TTYGraph::setPixel (int x, int y, char c) {
  if (x < 0 || x >= (int)_xSize || y < 0 || y >= (int)_ySize)
    return;
  tty[y * _xSize + x] = c;
}


void TTYGraph::drawHorizontalLine (int xlo,
                                   int xhi,
                                   int y,
 			           Colors::Colorchoice c unusedParameter) {
  for (int i=xlo; i<=xhi; ++i)
    setPixel (i, y, '-');
}


void TTYGraph::drawHourTick (int x, Colors::Colorchoice c unusedParameter) {
  setPixel (x, _ySize-1, '|');
}


void TTYGraph::drawString (int x, int y, const Dstr &s) {
  for (unsigned a=0; a<s.length(); ++a)
    setPixel (x+a, y, s[a]);
}


void TTYGraph::print (Dstr &text_out) {
  text_out = (char *)NULL;
  SafeVector<char> lineBuf (_xSize+2);
  lineBuf[_xSize]   = '\n';
  lineBuf[_xSize+1] = '\0';
  for (SafeVector<char>::const_iterator it (tty.begin());
       it != tty.end();
       it += _xSize) {
    std::copy (it, it+_xSize, lineBuf.begin());
    text_out += &(lineBuf[0]);
  }
}


void TTYGraph::drawX (int x, double y) {
  setPixel (x, Global::iround(y), '+');
}

// Cleanup2006 Done
