// $Id: xxPixmapGraph.hh 2641 2007-09-02 21:31:02Z flaterco $

/*  xxPixmapGraph  Graph implemented as Pixmap.

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

class xxPixmapGraph: public Graph {
public:
  xxPixmapGraph (unsigned xSize, unsigned ySize, GraphStyle style = normal);
  ~xxPixmapGraph();

  Pixmap pixmap;

protected:

  const bool antiAliasingDisabled;
  XImage *pixelCache;

  void startPixelCache();
  void stopPixelCache();

  const unsigned stringWidth (const Dstr &s) const;
  const unsigned fontHeight() const;

  void drawString (int x, int y, const Dstr &s);

  // These override perfectly good versions in Graph in order to use
  // the available X11 drawing functions.
  void drawVerticalLine (int x, int y1, int y2, Colors::Colorchoice c);
  void drawHorizontalLine (int xlo, int xhi, int y, Colors::Colorchoice c);

  void setPixel (int x, int y, Colors::Colorchoice c);
  void setPixel (int x, int y, Colors::Colorchoice c, double saturation);
};

// Cleanup2006 Done
