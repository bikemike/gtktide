// $Id: xxX.cc 2935 2008-01-10 02:15:21Z flaterco $

/*  xxX  Globals for interfacing with X11.

    Copyright (C) 2006  David Flater.

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
#include "icon_48x48_3color.xpm.hh"

const unsigned xxX::minWidthFudgeFactor (15U);

// We've had good luck with 48; that's what the CDE demanded.  If 48
// stops working, we need to call XGetIconSizes to find out what the
// window manager wants.  Example code can be found in xtide-2.8.3.
const unsigned xxX::iconSize (48U);

Display *xxX::display=NULL;
Screen *xxX::screen=NULL;
int xxX::screenNum;
Window xxX::rootWindow=0;
Colormap xxX::colormap=0;
Visual *xxX::visual=NULL,
       *xxX::iconVisual=NULL;
unsigned xxX::colordepth,
         xxX::iconColordepth;
GC xxX::textGC=NULL,
   xxX::backgroundGC=NULL,
   xxX::markGC=NULL,
   xxX::spareGC=NULL,
   xxX::invertGC=NULL,
   xxX::iconTextGC=NULL,
   xxX::iconBackgroundGC=NULL;
bool xxX::displaySucks;
Pixel xxX::pixels[Colors::numColors];
XFontStruct *xxX::smallFontStruct=NULL,
            *xxX::defaultFontStruct=NULL,
            *xxX::tinyFontStruct=NULL;
unsigned xxX::smallFontHeight=12;
Atom xxX::killAtom,
     xxX::protocolAtom;
XtAppContext xxX::appContext;
Pixmap xxX::iconPixmap;


static Colormap iconColormap=0;


// These are only valid if !displaySucks.  They do NOT correspond
// directly to the masks in XVisualInfo.  The way that you get from an
// unsigned short to bits for the Pixel is to mask the unsigned short
// by colorMask and then shift left by colorShift (or right if
// colorShift is negative).  Note that the bits per color may NOT be
// the same, contrary to what is implied by the bits_per_RGB field of
// XVisualInfo.  In 16-bit mode, my PC gives 5 bits red, 6 bits green,
// 5 bits blue.
static unsigned short redMask, greenMask, blueMask;
static int redShift, greenShift, blueShift;


// This converts the XVisualInfo masks to what we want.
static void convertMask (unsigned long vinfomask, unsigned short &mask,
int &shift) {
  assert (vinfomask);

  unsigned short highbit = 1 << (sizeof(unsigned short) * CHAR_BIT - 1);
  shift = 0;

  // USHRT_MAX is in SUSv3 limits.h

  if (vinfomask > USHRT_MAX) {
    // Will need to shift left.
    while (vinfomask > USHRT_MAX) {
      ++shift;
      vinfomask >>= 1;
    }
  } else if (!(vinfomask & highbit)) {
    // Will need to shift right.
    while (!(vinfomask & highbit)) {
      --shift;
      vinfomask <<= 1;
    }
  }

  assert (vinfomask);
  assert (vinfomask <= USHRT_MAX);
  mask = (unsigned short)vinfomask;
}


static void enableTrueColorVisual (const XVisualInfo &vinfo) {
  assert (vinfo.c_class == TrueColor && vinfo.depth >= 15);
  xxX::visual = vinfo.visual;
  xxX::screenNum = vinfo.screen;
  xxX::colordepth = vinfo.depth;
  convertMask (vinfo.red_mask, redMask, redShift);
  convertMask (vinfo.green_mask, greenMask, greenShift);
  convertMask (vinfo.blue_mask, blueMask, blueShift);
  xxX::displaySucks = false;
  require (xxX::colormap = XCreateColormap (xxX::display,
                                            xxX::rootWindow,
                                            xxX::visual,
                                            AllocNone));
}


static const Pixel remap (unsigned short color,
			  unsigned short mask,
			  int shift) {
  Pixel temp = color & mask;
  // Experiments indicated that shifting by negative values does not work.
  if (shift > 0)
    temp <<= shift;
  else
    temp >>= -shift;
  return temp;
}


static const double cdelta (unsigned short a, unsigned short b) {
  double d = (double)a-(double)b;
  return d*d;
}


static const Pixel getColor (unsigned short red,
			     unsigned short green,
			     unsigned short blue,
			     Visual *gcvisual,
			     Colormap gccolormap,
			     bool sucks) {

  // On a TrueColor display, XAllocColor is redundant.
  if (!sucks) {
    Pixel temp = 0;
    temp |= remap (red, redMask, redShift);
    temp |= remap (green, greenMask, greenShift);
    temp |= remap (blue, blueMask, blueShift);
    return temp;
  }

  // On a sucky display we really need to allocate room in the
  // colormap.  This is usable on a TrueColor visual too, but I used
  // to have trouble with fully saturated color values getting
  // truncated and muted.
  XColor cdef;
  cdef.red = red;
  cdef.green = green;
  cdef.blue = blue;
  cdef.flags = DoRed | DoGreen | DoBlue;
  if (!(XAllocColor (xxX::display, gccolormap, &cdef))) {
    XColor tryit, closest;

    /* closest_distance must be initialized to avoid floating */
    /* point exception on DEC Alpha OSF1 V3.0 */
    double distance, closest_distance = 0.0;

    for (int looper=0; looper<gcvisual->map_entries; ++looper) {
      tryit.pixel = (Pixel) looper;
      tryit.flags = DoRed | DoGreen | DoBlue;
      XQueryColor (xxX::display, gccolormap, &tryit);
      distance = cdelta (tryit.red, cdef.red)
               + cdelta (tryit.green, cdef.green)
               + cdelta (tryit.blue, cdef.blue);
      assert (distance >= 0.0);
      if (distance < closest_distance || (!looper)) {
        closest_distance = distance;
        closest = tryit;
      }
    }
    fprintf (stderr, "XTide:  Can't allocate color; using substitute (badness %lu)\n", (unsigned long)(sqrt (closest_distance)));
    if (!(XAllocColor (xxX::display, gccolormap, &closest)))
      fprintf (stderr, "XTide:  ACK!  Can't allocate that either!  Expect color shifting.\n");
    return (closest.pixel);
  }
  return (cdef.pixel);
}


const Pixel xxX::getColor (unsigned short red,
			   unsigned short green,
			   unsigned short blue) {
  return ::getColor (red, green, blue, visual, colormap, displaySucks);
}


static const Pixel getColor (const Dstr &color,
			     Visual *gcvisual,
			     Colormap gccolormap,
			     bool sucks) {
  XColor cdef;
  if (!(XParseColor (xxX::display, gccolormap, color.aschar(), &cdef))) {
    Dstr details ("The offending color spec was ");
    details += color;
    details += ".";
    Global::barf (Error::BADCOLORSPEC, details);
  }
  return ::getColor (cdef.red, cdef.green, cdef.blue, gcvisual, gccolormap,
    sucks);
}


static const Pixel getColor (const Dstr &color) {
  return ::getColor (color, xxX::visual, xxX::colormap, xxX::displaySucks);
}


static const Pixel getIconColor (const Dstr &color) {
  return ::getColor (color, xxX::iconVisual, iconColormap, true);
}


static const Pixmap makePixmap (unsigned width,
				unsigned height,
				unsigned mpdepth) {
  Pixmap temp;
  require (temp = XCreatePixmap (xxX::display, xxX::rootWindow, width, height,
    mpdepth));
  return temp;
}


void xxX::installColors () {

  assert (display);
  assert (colormap);
  assert (iconColormap);
  assert (rootWindow);
  assert (smallFontStruct);
  assert (tinyFontStruct);

  // [0] is icon background.
  // [1] is icon foreground (text).
  static Pixel iconPixels[2];

  static bool first = true;

  if (!first) {
    if (displaySucks)
      XFreeColors (display, colormap, pixels, Colors::numColors, 0);
    XFreeColors (display, iconColormap, iconPixels, 2, 0);
  }

  for (unsigned a=0; a<Colors::numColors; ++a)
    pixels[a] = ::getColor (Global::settings[Colors::colorarg[a]].s);
  iconPixels[0] = ::getIconColor (Global::settings["dc"].s);
  iconPixels[1] = ::getIconColor (Global::settings["fg"].s);

  if (first) {

    // Do not use XtGetGC.  It returns a "sharable, read-only GC."  We
    // need non-shared, modifiable GCs.

    // Oddly, XCreateGC requires a Drawable.  We don't have any realized
    // windows yet that match the main visual, so we'll have to create a
    // temporary Drawable for it.  For the icon we can use the root
    // window.

    XGCValues gcv;

    {
      // Avoid catch-22 on makePixmap (1, 1), which uses backgroundGC.
      Pixmap tempDrawable = ::makePixmap (1, 1, colordepth);

      gcv.foreground = pixels[Colors::foreground];
      textGC = XCreateGC (display, tempDrawable, GCForeground, &gcv);
      gcv.foreground = pixels[Colors::mark];
      markGC = XCreateGC (display, tempDrawable, GCForeground, &gcv);
      gcv.foreground = pixels[Colors::background];
      backgroundGC = XCreateGC (display, tempDrawable, GCForeground, &gcv);
      spareGC = XCreateGC (display, tempDrawable, GCForeground, &gcv);
      gcv.function = GXinvert;
      invertGC = XCreateGC (display, tempDrawable, GCFunction, &gcv);

      XFreePixmap (display, tempDrawable);
    }

    gcv.foreground = iconPixels[0];
    iconBackgroundGC = XCreateGC (display, rootWindow, GCForeground, &gcv);
    gcv.foreground = iconPixels[1];
    iconTextGC = XCreateGC (display, rootWindow, GCForeground, &gcv);

    XSetFont (display, textGC, smallFontStruct->fid);
    XSetFont (display, iconTextGC, tinyFontStruct->fid);

    first = false;

  } else {
    XSetForeground (display, textGC,            pixels[Colors::foreground]);
    XSetForeground (display, markGC,            pixels[Colors::mark]);
    XSetForeground (display, backgroundGC,      pixels[Colors::background]);
    XSetForeground (display, iconBackgroundGC,  iconPixels[0]);
    XSetForeground (display, iconTextGC,        iconPixels[1]);
  }
}


void xxX::colorOfPixel (Pixel pixel,
                        unsigned short &red_out,
                        unsigned short &green_out,
                        unsigned short &blue_out) {
  XColor temp;
  temp.pixel = pixel;
  temp.flags = DoRed | DoGreen | DoBlue;
  XQueryColor (display, colormap, &temp);
  red_out = temp.red;
  green_out = temp.green;
  blue_out = temp.blue;
}


static const Pixmap makePixmap (char **data,
				Visual *mpvisual,
				Colormap mpcolormap,
				unsigned mpdepth) {
  assert (data);
  Pixmap temp;
  XpmAttributes xpma;
  xpma.valuemask = XpmColormap | XpmDepth | XpmCloseness | XpmExactColors;
  if (mpvisual)
    xpma.valuemask |= XpmVisual;
  xpma.visual = mpvisual;
  xpma.colormap = mpcolormap;
  xpma.depth = mpdepth;
  xpma.closeness = 1000000;  // Anything worth doing is worth overdoing.
  xpma.exactColors = 0;
  int retval = XCreatePixmapFromData (xxX::display, xxX::rootWindow,
    data, &temp, NULL, &xpma);
  switch (retval) {
  case XpmColorError:
    fprintf (stderr, "Warning:  Non-fatal Xpm color error reported.\n");
  case XpmSuccess:
    return temp;
  case XpmOpenFailed:
    Global::barf (Error::XPM_ERROR, "Xpm: Open Failed.");
  case XpmFileInvalid:
    Global::barf (Error::XPM_ERROR, "Xpm: File Invalid.");
  case XpmNoMemory:
    Global::barf (Error::XPM_ERROR, "Xpm: No Memory.");
  case XpmColorFailed:
    return 0;
  }
  if (retval < 0)
    Global::barf (Error::XPM_ERROR, "Fatal Xpm error of unrecognized type reported.");
  fprintf (stderr, "Warning:  Non-fatal Xpm error of unrecognized type reported.\n");
  return temp;
}


const Pixmap xxX::makePixmap (char **data) {
  return ::makePixmap (data, visual, colormap, colordepth);
}


static const Pixmap makeIconPixmap (char **data) {
  return ::makePixmap (data, xxX::iconVisual, iconColormap,
                       xxX::iconColordepth);
}


const Pixmap xxX::makePixmap (unsigned width, unsigned height) {
  Pixmap temp = ::makePixmap (width, height, colordepth);
  XFillRectangle (display, temp, backgroundGC, 0, 0, width, height);
  return temp;
}


const Pixmap xxX::makeIconPixmap (unsigned width, unsigned height) {
  Pixmap temp = ::makePixmap (width, height, iconColordepth);
  XFillRectangle (display, temp, iconBackgroundGC, 0, 0, width, height);
  return temp;
}


static void loadFonts (Widget rootWidget) {
  assert (xxX::display);
  assert (rootWidget);

  // 2006-06:  Under Gentoo, the schumacher fonts are now optional.
  // Must at least report the name of the missing font.

  constString smallFontName =
                  "-misc-fixed-medium-r-normal--13-100-100-100-c-70-iso8859-1";
  constString tinyFontName =
                        "-schumacher-clean-medium-r-normal-*-10-*-*-*-*-*-*-*";

  if (!(xxX::smallFontStruct = XLoadQueryFont (xxX::display, smallFontName)))
    Global::barf (Error::CANT_LOAD_FONT, smallFontName);
  if (!(xxX::tinyFontStruct = XLoadQueryFont (xxX::display, tinyFontName)))
    Global::barf (Error::CANT_LOAD_FONT, tinyFontName);

  // The shell has no font attribute.  There is probably a better
  // way to do this.
  Widget labelwidget = XtCreateManagedWidget ("", labelWidgetClass,
    rootWidget, NULL, 0);
  Arg fontargs[1] = {
    {XtNfont, (XtArgVal)(&xxX::defaultFontStruct)}
  };
  XtGetValues (labelwidget, fontargs, 1);
  assert (xxX::defaultFontStruct);
  XtDestroyWidget (labelwidget);

  // Unclear what is the best approach for all possible fonts.  For
  // the chosen smallFont, ascent = 11, descent = 2, and the right
  // answer is 12.
  xxX::smallFontHeight = xxX::smallFontStruct->max_bounds.ascent + 1;
}


const unsigned xxX::stringWidth (XFontStruct *fs, const Dstr &s) {
  assert (fs);
  assert (!s.isNull());
  int dir_return, asc_return, desc_return;
  XCharStruct ov_return;
  XTextExtents (fs, s.aschar(), s.length(), &dir_return, &asc_return,
    &desc_return, &ov_return);
  return (unsigned)(ov_return.width);
}


const Widget xxX::connect (int &argc, char **argv) {
  static bool onlyonce (true);
  assert (onlyonce);
  onlyonce = false;

  // 2008-01  Workaround for an extreme example of code rot (breakage
  // inflicted by backward-incompatible changes occurring in the
  // environment).  The Composite extension throws ARGB visuals at
  // unsuspecting legacy applications.  ARGB visuals are reported as
  // TrueColor visuals, which is a lie.  They aren't substitutable.  The
  // alpha channel defaults to transparent, so an application that doesn't
  // know it's there will NOT work correctly.  There isn't even a proper way
  // to determine if an ARGB visual is what you got!
  static char env_string[] = "XLIB_SKIP_ARGB_VISUALS=1";
  require (putenv (env_string) == 0);

  Widget widget;

  // See if we can get a nice true color visual (code fragment 11-5, X
  // Toolkit Cookbook).  Somebody can set the default to 8 plane
  // PseudoColor even when 24 bit color is supported, so you have to
  // explicitly ask for a TrueColor visual to get it.

  XtToolkitInitialize();
  appContext = XtCreateApplicationContext();

  // Special handling of -geometry.
  // Concatenation of the argument is not supported here.
  // It's not supported by standard X11 programs either.
  {
    for (int a=0; a<argc; ++a)
      if (!strncmp (argv[a], "-ge", 3) || !strcmp (argv[a], "-g"))
	strcpy (argv[a], "-X");
  }

  display = XtOpenDisplay (appContext, NULL, "XTide", "XTide", NULL, 0,
                           &argc, argv);
  if (!display)
    Global::barf (Error::CANTOPENDISPLAY);

  screenNum = DefaultScreen (display);
  screen = ScreenOfDisplay (display, screenNum);
  rootWindow = RootWindow (display, screenNum);
  protocolAtom = XInternAtom (display, "WM_PROTOCOLS", False);
  killAtom = XInternAtom (display, "WM_DELETE_WINDOW", False);
  // Must use default visual and default colormap for icon.
  iconVisual = DefaultVisual (display, screenNum);
  iconColormap = DefaultColormap (display, screenNum);
  iconColordepth =  DefaultDepth (display, screenNum);
  iconPixmap = ::makeIconPixmap (icon_48x48_3color);

  // Find best available TrueColor visual
  XVisualInfo vinfo;
  colordepth = 0;
  {
    XVisualInfo vinfo_template;
    vinfo_template.c_class = TrueColor;
    vinfo_template.screen = screenNum;
    int nitems;
    XVisualInfo *vinfo_out = XGetVisualInfo (display,
      VisualClassMask|VisualScreenMask, &vinfo_template, &nitems);
    for (int l = 0; l < nitems; ++l)
      if ((vinfo_out[l].depth >= 15) &&
	  ((unsigned)vinfo_out[l].depth > colordepth)) {
	colordepth = vinfo_out[l].depth;
	vinfo = vinfo_out[l];
      }
    XFree (vinfo_out);
  }

  displaySucks = (colordepth == 0);
  // Undocumented switch for testing default visual and simple protocol.
  if (argc == 2)
    if (!strcmp (argv[1], "-suck"))
      displaySucks = true;

  if (!displaySucks) {
    enableTrueColorVisual (vinfo);
    Arg args[5] = {
      {XtNdepth, (XtArgVal)colordepth},
      {XtNvisual, (XtArgVal)visual},
      {XtNcolormap, (XtArgVal)colormap},
      {XtNborderColor, (XtArgVal)0},
      {XtNbackground, (XtArgVal)0}
    };
    widget = XtAppCreateShell ("XTide", "XTide",
      applicationShellWidgetClass, display, args, 5);

  } else {
    widget = XtAppCreateShell ("XTide", "XTide",
      applicationShellWidgetClass, display, NULL, 0);
    Arg args[3] = {
      {XtNcolormap, (XtArgVal)(&colormap)},
      {XtNvisual, (XtArgVal)(&visual)},
      {XtNdepth, (XtArgVal)(&colordepth)}
    };
    XtGetValues (widget, args, 3);
  }

  if (!visual)
    visual = DefaultVisual (display, screenNum);
  loadFonts (widget);
  return widget;
}


std::auto_ptr<xxWidget> xxX::wrap (Widget widget) {
  return std::auto_ptr<xxWidget> (new xxWidget (widget));
}

// Cleanup2006 Done
