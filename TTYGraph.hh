// $Id: TTYGraph.hh 2946 2008-01-18 23:12:25Z flaterco $

/*  TTYGraph  Graph implemented on dumb terminal.

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

class TTYGraph: public Graph {
public:
  TTYGraph (unsigned xSize, unsigned ySize, GraphStyle style = normal);

  void print (Dstr &text_out);

protected:

  // xSize * ySize characters, row major, starting at upper left.
  SafeVector<char> tty;

  // Overridden virtual methods.
  const unsigned stringWidth (const Dstr &s) const;
  const unsigned fontHeight()                const;
  const unsigned hourTickLen()               const;
  const unsigned depthLabelLeftMargin()      const;
  const unsigned depthLabelRightMargin()     const;
  const unsigned depthLineVerticalMargin()   const;
  const int blurbMargin()                    const;
  const double aspectFudgeFactor()           const;
  const unsigned startPosition (unsigned labelWidth) const;
  void drawString (int x, int y, const Dstr &s);

  // Overridden virtual setPixel draws asterisks by default.
  void setPixel (int x, int y, Colors::Colorchoice c);

  // New setPixel draws the specified character.
  void setPixel (int x, int y, char c);

  // For line drawing, use -, |, and + instead of asterisks.
  void drawHorizontalLine (int xlo,
                           int xhi,
                           int y,
			   Colors::Colorchoice c);
  void drawHourTick (int x, Colors::Colorchoice c);
  void drawX (int x, double y);
};

// Cleanup2006 Done
