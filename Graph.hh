// $Id: Graph.hh 2641 2007-09-02 21:31:02Z flaterco $

/*  Graph  Abstract superclass for all graphs.

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

class Graph {
public:
  enum GraphStyle {normal, clock};
  Graph (unsigned xSize, unsigned ySize, GraphStyle style = normal);
  virtual ~Graph();

  // angle is a kludge to help out the tide clock icon.
  void drawTides (Station *station,
                  Timestamp nominalStartTime,
                  Angle *angle = NULL);

  // Data type that needs to be public for the sake of some hidden
  // functions.
  struct EventBlurb {
    int x;           // Nominal position.
    int deltaLeft;   // x + deltaLeft = lowest used pixel.
    int deltaRight;  // x + deltaRight = highest used pixel.
    Dstr line1;      // Upper line of text.
    Dstr line2;      // Lower line of text.
  };

protected:

  const GraphStyle _style;
  unsigned _xSize, _ySize;

  // Margin left at top and bottom of tide graphs when scaling tides;
  // how much "water" at lowest tide; how much "sky" at highest tide.
  // This is a scaling factor for the graph height.
  static const double vertGraphMargin;


  // The complex tangle of methods below is to minimize the amount of
  // duplicated code between RGBGraph, xxPixmapGraph, TTYGraph, and
  // Banner.

  virtual const bool isBanner() const;

  // Determine which depth lines will be drawn.
  // lineStep, minDepth and maxDepth are in tenths of a unit.
  // If no lines at all, minDepth_out=INT_MAX and maxDepth_out=INT_MIN.
  void checkDepth (double ymax,
                   double ymin,
                   double valmax,
                   double valmin,
                   unsigned lineStep,
                   int &minDepth_out,
                   int &maxDepth_out) const;

  // Determine lineStep, labelWidth, labelRight, minDepth, and maxDepth.
  // lineStep:  depth line increment in tenths of a unit.
  // labelWidth:  space allowed for depth labels, including margins.
  // labelRight:  position to which depth labels should be right-aligned.
  // minDepth and maxDepth are as provided by checkDepth.
  void figureLabels (double ymax,
		     double ymin,
		     double valmax,
		     double valmin,
		     const Dstr &unitsDesc,
		     unsigned &lineStep_out,
		     unsigned &labelWidth_out,
		     unsigned &labelRight_out,
		     int &minDepth_out,
		     int &maxDepth_out) const;

  // Draw depth lines.
  // lineStep, minDepth and maxDepth are in tenths of a unit.
  void drawDepth (double ymax,
                  double ymin,
                  double valmax,
                  double valmin,
                  unsigned lineStep,
                  unsigned labelWidth,
                  int minDepth,
                  int maxDepth);

  // This fills in the background, which indicates sunrise/sunset.
  void clearGraph (Timestamp startTime,
                   Timestamp endTime,
                   Interval increment,
                   Station *station,
                   TideEventsOrganizer &organizer);

  // Mark current time.
  virtual void drawX (int x, double y);

  // Horrible logic for line graphs.
  void drawFunkyLine (double prevytide,
                      double ytide,
                      double nextytide,
                      int x,
                      Colors::Colorchoice c);

  // Horizontal font metrics.
  virtual const unsigned stringWidth (const Dstr &s) const = 0;

  // Vertical font metrics.
  virtual const unsigned fontHeight() const = 0;
  // Extra vertical space between lines of text.
  virtual const unsigned fontVerticalMargin() const;

  // Event blurb metrics.  Sets the values of deltaLeft and deltaRight
  // given line1 and line2.  (Blurbs are not intrinsically vertical or
  // horizontal as they are flipped in banner mode.)
  virtual void measureBlurb (EventBlurb &blurb) const;
  virtual const int blurbMargin() const;

  // Lots of other things that are adjustable by subclasses.
  virtual const unsigned depthLabelLeftMargin()    const;
  virtual const unsigned depthLabelRightMargin()   const;
  // Extra vertical space between depth lines and timestamps or hour ticks.
  virtual const unsigned depthLineVerticalMargin() const;
  virtual const unsigned hourTickLen()             const;
  virtual const double   aspectFudgeFactor()       const;
  virtual const unsigned startPosition (unsigned labelWidth) const;

  // All coordinates use northwest gravity.  Integer coordinates range
  // between 0 and size-1; double coordinates range between 0 and
  // size.  A double coordinate with integer value x refers to the top
  // of a pixel.  The bottom would effectively be x+1.

  // int coordinates are used instead of unsigned so that things can
  // be drawn correctly even when they are only partially visible.

  // Doubles are used for anti-aliasing in truecolor pixmaps.  Others
  // will just round off.

  // Ordering of y1 and y2 is irrelevant.
  void drawVerticalLine (int x,
                         double y1,
                         double y2,
                         Colors::Colorchoice c);

  // No line will be drawn if xlo > xhi.
  virtual void drawHorizontalLine (int xlo,
                                   int xhi,
                                   int y,
				   Colors::Colorchoice c);

  // For methods taking int line:  from the top it's line 0, 1; from
  // the bottom it's -1, -2.

  virtual void drawHourTick  (int x, Colors::Colorchoice c);
  virtual void labelHourTick (int x, const Dstr &label);
  virtual void drawTitleLine (const Dstr &title);
  virtual void labelEvent    (int topLine, const EventBlurb &blurb);
  void drawBlurbs (int topLine, SafeVector<EventBlurb> &blurbs);

  // This is the "simple" low level line drawer, where y-coordinates
  // behave in the usual way.
  virtual void drawVerticalLine (int x,
                                 int y1,
                                 int y2,
                                 Colors::Colorchoice c);

  // Likewise, the simple low level pixel setter.
  virtual void setPixel (int x,
                         int y,
                         Colors::Colorchoice c) = 0;

  // By default, this just calls the regular setPixel if saturation >= 0.5.
  // Override to do anti-aliasing.
  virtual void setPixel (int x,
                         int y,
                         Colors::Colorchoice c,
                         double saturation); // Saturation ranges from 0 to 1

  // That anti-aliased setPixel turned out to be a performance killer
  // for X because there's no efficient way to read the current
  // contents of a single pixel.  The following methods are used to
  // give hints to xxPixmapGraph so it can cache the image and avoid
  // whaling on that inefficient operation.
  virtual void startPixelCache();
  virtual void stopPixelCache();

  // Strings should be drawn downwards from the y coordinate provided.
  virtual void drawString (int x, int y, const Dstr &s) = 0;
  void centerString       (int x, int y, const Dstr &s);
  void centerStringOnLine (int x, int line, const Dstr &s);

  // These won't anti-alias, they will round the y coordinate.
  void drawHorizontalLine (int xlo, int xhi, double y, Colors::Colorchoice c);
  void drawString         (int x, double y, const Dstr &s);
  void rightJustifyString (int x, double y, const Dstr &s);

  // These are used in Graph and subclasses.
  // "lo" is what you get with saturation 0.
  // "hi" is what you get with saturation 1.
  // "lo" does not need to be less than "hi".
  const double linterp (double lo,
			double hi,
			double saturation) const;
  const unsigned char linterp (unsigned char lo,
			       unsigned char hi,
			       double saturation) const;
  const unsigned short linterp (unsigned short lo,
				unsigned short hi,
				double saturation) const;

private:
  // Prohibited operations not implemented.
  Graph (const Graph &);
  Graph &operator= (const Graph &);
};

// Cleanup2006 Done
