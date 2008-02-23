// $Id: xxPixmapGraph.cc 2641 2007-09-02 21:31:02Z flaterco $

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

#include "xtide.hh"
#include "Graph.hh"
#include "RGBGraph.hh"
#include "xxPixmapGraph.hh"


xxPixmapGraph::xxPixmapGraph (unsigned xSize,
                              unsigned ySize,
                              GraphStyle style):
  Graph (xSize, ySize, style),
  antiAliasingDisabled (xxX::displaySucks || Global::settings["aa"].c == 'n'),
  pixelCache(NULL) {
  assert (xSize >= Global::minGraphWidth && ySize >= Global::minGraphHeight);
  pixmap = xxX::makePixmap (xSize, ySize);
}


xxPixmapGraph::~xxPixmapGraph() {
  stopPixelCache();
  XFreePixmap (xxX::display, pixmap);
}


const unsigned xxPixmapGraph::stringWidth (const Dstr &s) const {
  return xxX::stringWidth (xxX::smallFontStruct, s);
}


const unsigned xxPixmapGraph::fontHeight() const {
  return xxX::smallFontHeight;
}


void xxPixmapGraph::setPixel (int x, int y, Colors::Colorchoice c) {
  if (x < 0 || x >= (int)_xSize || y < 0 || y >= (int)_ySize)
    return;
  XSetForeground (xxX::display, xxX::spareGC, xxX::pixels[c]);
  XDrawPoint (xxX::display, pixmap, xxX::spareGC, x, y);
}


void xxPixmapGraph::setPixel (int x,
			      int y,
			      Colors::Colorchoice c,
			      double saturation) {
  if (x < 0 || x >= (int)_xSize || y < 0 || y >= (int)_ySize)
    return;
  if (antiAliasingDisabled) {
    if (saturation >= 0.5)
      setPixel (x, y, c);
  } else {

    // We arrive at this sorry crossroads as a result of these
    // unhealthy facts about Xlib:
    //    1.  Apart from doing XGetImage, there is no way to "read"
    //        the value of a pixel in a Pixmap.
    //    2.  The drawing functions XDrawPoint, XDrawLine, and XDrawString
    //        only work on "drawables."  Windows and Pixmaps are drawables;
    //        XImages are not.  So it's not possible to avoid the kludge
    //        by using XImages instead of Pixmaps everywhere.
    // It would be nice to just inherit everything from RGBGraph and then
    // convert the PPM to a Pixmap at the end, but this suddenly gets
    // complicated when your display only supports 256 colors.  It's also
    // intolerably slow.  The only remaining alternative would be to
    // duplicate the effort in RGBGraph to create an XImage at the PutPixel
    // level, sans even XDrawLine to speed things up.  This would probably
    // be even slower than the current implementation (I guess).

    // 2005-12-24  Pixel cache added to speed this up.

    Pixel pixel;
    if (pixelCache)
      pixel = XGetPixel (pixelCache, x, y);
    else {
      XImage *img (XGetImage (xxX::display, pixmap, x, y, 1, 1,
			      AllPlanes, ZPixmap));
      pixel = XGetPixel (img, 0, 0);
      XDestroyImage (img);
    }

    unsigned short r1, g1, b1, r2, g2, b2;
    xxX::colorOfPixel (pixel, r1, g1, b1);
    xxX::colorOfPixel (xxX::pixels[c], r2, g2, b2);
    XSetForeground (xxX::display,
		    xxX::spareGC,
		    xxX::getColor (linterp (r1, r2, saturation),
				   linterp (g1, g2, saturation),
				   linterp (b1, b2, saturation)));
    XDrawPoint (xxX::display, pixmap, xxX::spareGC, x, y);
  }
}


void xxPixmapGraph::drawVerticalLine (int x,
				      int y1,
				      int y2,
				      Colors::Colorchoice c) {
  XSetForeground (xxX::display, xxX::spareGC, xxX::pixels[c]);
  XDrawLine (xxX::display, pixmap, xxX::spareGC, x, y1, x, y2);
}


void xxPixmapGraph::drawHorizontalLine (int xlo,
					int xhi,
					int y,
					Colors::Colorchoice c) {
  XSetForeground (xxX::display, xxX::spareGC, xxX::pixels[c]);
  if (xlo <= xhi) // Constraint per Graph::drawHorizontalLine.
    XDrawLine (xxX::display, pixmap, xxX::spareGC, xlo, y, xhi, y);
}


void xxPixmapGraph::drawString (int x, int y, const Dstr &s) {
  XDrawString (xxX::display, pixmap, xxX::textGC, x, y+fontHeight()-2,
               s.aschar(), s.length());
}


void xxPixmapGraph::startPixelCache() {
  assert (!pixelCache);
  if (!antiAliasingDisabled)
    pixelCache = XGetImage (xxX::display, pixmap, 0, 0, _xSize, _ySize,
			    AllPlanes, ZPixmap);
}


void xxPixmapGraph::stopPixelCache() {
  if (pixelCache) {
    XDestroyImage (pixelCache);
    pixelCache = NULL;
  }
}

// Cleanup2006 Done
