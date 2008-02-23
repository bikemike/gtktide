// $Id: RGBGraph.cc 2641 2007-09-02 21:31:02Z flaterco $

/*  RGBGraph  Graph implemented as raw RGB image.

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
#include "glyphs.hh"
#include "Graph.hh"
#include "RGBGraph.hh"


RGBGraph::RGBGraph (unsigned xSize, unsigned ySize, GraphStyle style):
  Graph (xSize, ySize, style) {
  assert (xSize >= Global::minGraphWidth && ySize >= Global::minGraphHeight);
  rgb.resize (xSize * ySize * 3);
  for (unsigned a=0; a<Colors::numColors; ++a)
    Colors::parseColor (Global::settings[Colors::colorarg[a]].s,
                        cmap[a][0],
                        cmap[a][1],
                        cmap[a][2]);
}


const unsigned RGBGraph::stringWidth (const Dstr &s) const { 
  return glyphWidth * s.length();
}


const unsigned RGBGraph::fontHeight() const {
  return glyphHeight;
}


void RGBGraph::setPixel (int x, int y, Colors::Colorchoice c) {
  assert (c < (int)Colors::numColors);
  if (x < 0 || x >= (int)_xSize || y < 0 || y >= (int)_ySize)
    return;
  SafeVector<unsigned char>::iterator it = rgb.begin() + (y * _xSize + x) * 3;
  *it     = cmap[c][0];
  *(++it) = cmap[c][1];
  *(++it) = cmap[c][2];
}


void RGBGraph::setPixel (int x,
                         int y,
                         Colors::Colorchoice c,
                         double saturation) {
  assert (c < (int)Colors::numColors);
  if (x < 0 || x >= (int)_xSize || y < 0 || y >= (int)_ySize)
    return;
  SafeVector<unsigned char>::iterator it = rgb.begin() + (y * _xSize + x) * 3;
  *it = linterp (*it, cmap[c][0], saturation);
  ++it;
  *it = linterp (*it, cmap[c][1], saturation);
  ++it;
  *it = linterp (*it, cmap[c][2], saturation);
}


void RGBGraph::drawString (int x, int y, const Dstr &s) {
  for (unsigned a=0; a<s.length(); ++a) {
    int base = s[a];
    if (base < 0)
      base += 256;
    assert (base >= 0 && base < 256);
    base *= glyphWidth;
    for (unsigned int yl=0; yl<fontHeight(); ++yl)
      for (unsigned int xl=0; xl<glyphWidth; ++xl)
        if (glyphs[yl][base+xl])
	  setPixel (x+xl, y+yl, Colors::foreground);
    x += glyphWidth;
  }
}


// This is a hacked-down version of write_png in example.c in libpng-0.99c
void RGBGraph::writeAsPNG (png_rw_ptr write_data_fn) {

  png_structp png_ptr;
  png_infop info_ptr;

  /* Create and initialize the png_struct with the desired error handler
   * functions.  If you want to use the default stderr and longjump method,
   * you can supply NULL for the last three parameters.  We also check that
   * the library version is compatible with the one used at compile time,
   * in case we are using dynamically linked libraries.  REQUIRED.
   */
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
    NULL, NULL, NULL);

  if (png_ptr == NULL)
    Global::barf (Error::PNG_WRITE_FAILURE);

  /* Allocate/initialize the image information data.  REQUIRED */
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL)
    Global::barf (Error::PNG_WRITE_FAILURE);

  /* Set error handling.  REQUIRED if you aren't supplying your own
   * error handling functions in the png_create_write_struct() call.
   */
  if (setjmp(png_ptr->jmpbuf))
    Global::barf (Error::PNG_WRITE_FAILURE);

  /* set up the output control if you are using standard C streams */
  // The user_io_ptr feature of libpng doesn't seem to work.
  png_set_write_fn(png_ptr, NULL, write_data_fn, NULL);

  /* Set the image information here.  Width and height are up to 2^31,
   * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
   * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
   * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
   * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
   * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
   * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
   */
  png_set_IHDR(png_ptr, info_ptr, _xSize, _ySize, 8, PNG_COLOR_TYPE_RGB,
     PNG_INTERLACE_ADAM7, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  /* Write the file header information.  REQUIRED */
  png_write_info(png_ptr, info_ptr);

  // Convert contiguous memory to what libpng wants.
  SafeVector<png_byte*> row_pointers (_ySize);
  for (unsigned i=0; i<_ySize; ++i)
    row_pointers[i] = &(rgb[i * _xSize * 3]);

  /**** write out the entire image data in one call ***/
  png_write_image(png_ptr, &(row_pointers[0]));

  /* It is REQUIRED to call this to finish writing the rest of the file */
  png_write_end(png_ptr, info_ptr);

  /* clean up after the write, and free any memory allocated */
  png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
}

// Cleanup2006 Done
