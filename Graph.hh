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
namespace Shape
{
class Point
{
public:
	Point(int _x, int _y) : x(_x), y(_y) {}
	int x;
	int y;

	bool operator==(const Point& other) const
	{
		return x == other.x && y == other.y;
	}
	bool operator!=(const Point& other) const { return !operator==(other); }
};

class Rectangle
{
public:
	Rectangle(int _x, int _y, int w, int h) : x(_x), y(_y), width(w), height(h){} 
	Rectangle() : x(0), y(0), width(0), height(0){}
	bool intersects(const Rectangle& other, Rectangle* intersectRect = NULL) const
	{
		Rectangle tmpRect;
		Rectangle* pRect = &tmpRect; 

		if (NULL != intersectRect)
		{
			pRect = intersectRect;
		}

		pRect->x = std::max(x, other.x);
		pRect->y = std::max(y, other.y);
		pRect->width = std::min(x + width, other.x + other.width) - pRect->x;
		pRect->height = std::min(y + height, other.y + other.height) - pRect->y;

		return pRect->isValid();
	}

	bool isValid() const
	{
		return (0 < width && 0 < height);
	}

	int getLeft(){return x;}
	int getRight(){return x + width;}
	int getTop(){return y;}
	int getBottom(){return y+height;}

	int x;
	int y;
	int width;
	int height;
};

// a region contains zero or more rectangles
class Region
{
public:
	Region(){}
	Region(const Rectangle& r)
	{
		addRectangle(r);
	}

	Region(int x, int y, int w, int h)
	{
		addRectangle(Rectangle(x,y,w,h));
	}

	void addRectangle(const Rectangle& r)
	{
		if (r.isValid())
		{
			m_vectRectangles.push_back(r);
		}
	}

	bool intersects(const Rectangle& r) const
	{
		bool bIntersects = false;
		RectVect::const_iterator itr;
		for (itr = m_vectRectangles.begin(); 
			m_vectRectangles.end() != itr && !bIntersects; ++itr)
		{
			bIntersects = itr->intersects(r);
		}
		return bIntersects;
	}

	Region intersect (const Rectangle& rect) const
	{
		return intersect(Region(rect));
	}

	Region intersect (const Region& reg) const
	{
		Region r;
		RectVect::const_iterator itr;
		RectVect::const_iterator itr2;
		for (itr = m_vectRectangles.begin(); 
			m_vectRectangles.end() != itr; ++itr)
		{
			for (itr2 = reg.m_vectRectangles.begin(); 
				reg.m_vectRectangles.end() != itr2; ++itr2)
			{
				Rectangle rect;
				if (itr->intersects(*itr2, &rect))
				{
					r.addRectangle(rect);
				}
			}
		}
		return r;
	}

	Region subtract (const Rectangle& rect) const
	{
		return subtract(Region(rect));
	}
	Region subtract( const Region& reg) const
	{
		Region newReg = *this;

		RectVect::iterator itr;
		RectVect::const_iterator itr2;
		for (itr = newReg.m_vectRectangles.begin(); 
			newReg.m_vectRectangles.end() != itr;) // ++itr done at end of loop
		{
			bool bIntersects = false;

			for (itr2 = reg.m_vectRectangles.begin(); 
				reg.m_vectRectangles.end() != itr2; ++itr2)
			{
				Rectangle intersect;
				if (itr2->intersects(*itr, &intersect))
				{
					int nRects = 4;
					Rectangle r[nRects];

					r[0].x      = itr->x;
					r[0].width  = intersect.x - itr->x; 
					r[0].y      = itr->y;
					r[0].height = itr->height;

					r[1].x      = intersect.x;
					r[1].width  = intersect.width; 
					r[1].y      = itr->y;
					r[1].height = intersect.y - itr->y;

					r[2].x      = intersect.x;
					r[2].width  = intersect.width; 
					r[2].y      = intersect.y + intersect.height;
					r[2].height = (itr->y + itr->height) - r[2].y;

					r[3].x      = intersect.x + intersect.width;
					r[3].width  = (itr->x + itr->width) - r[3].x; 
					r[3].y      = itr->y;
					r[3].height = itr->height;

					// erase the intersecting rect
					newReg.m_vectRectangles.erase(itr);
					for (int i = 0 ; i < nRects; ++i)
					{
						// addRectangle will only
						// add the rect if it's valid
						newReg.addRectangle(r[i]);
					}
					bIntersects = true;

					// now break and start over to make sure
					// none of the other rects intersect
					// with the newly added ones
					itr = newReg.m_vectRectangles.begin();
					break;
				}
			}

			if (!bIntersects)
			{
				++itr;
			}
		}
		return newReg;
	}

	bool isEmpty() const
	{
		return m_vectRectangles.empty();
	}

	Rectangle getBoundingRect() const
	{
		int minx, miny;
		int maxx, maxy;
		bool bFirst = true;
		RectVect::const_iterator itr;
		for (itr = m_vectRectangles.begin(); 
			m_vectRectangles.end() != itr; ++itr)
		{
			if (bFirst)
			{
				bFirst = false;
				minx = itr->x;
				miny = itr->y;
				maxx = itr->x + itr->width;
				maxy = itr->y + itr->height;
			}
			else
			{
				minx = std::min(minx, itr->x);
				miny = std::min(miny, itr->y);
				maxx = std::max(maxx, itr->x + itr->width);
				maxy = std::max(maxy, itr->y + itr->height);
			}
		}
		return Rectangle(minx, miny, maxx - minx, maxy - miny);
	}

	typedef std::vector<Rectangle> RectVect;
	RectVect getRectangles() const
	{
		return m_vectRectangles;
	}

private:
	RectVect m_vectRectangles;
};
}

class Graph {
public:
  enum GraphStyle {normal, clock};
  Graph (unsigned xSize, unsigned ySize, GraphStyle style = normal);
  virtual ~Graph();

	Graph(GraphStyle style = normal);

	void setSize(unsigned xSize, unsigned ySize);

	void setNominalStartTime(Timestamp nominalStartTime);

	void setDrawTitle(bool bDrawTitle);
	void setDrawTitleOverGraph(bool bOver);
	void setDrawDepthOverGraph(bool bOver);
	void setDrawHoursOverGraph(bool bOver);

	Shape::Rectangle getTitleAreaRect();
	Shape::Rectangle getDepthAreaRect();

	void setStation(Station* station);
	Station* getStation(){return m_pStation;}

	void drawTides(const Shape::Region& clipRegion);

	void drawTides (Station *station,
		Timestamp nominalStartTime,
		const Shape::Region& clipRegion,
		Angle *angle = NULL);

	void drawTitleArea(Station* station,
	   Timestamp nominalStartTime,
	   const Shape::Region& clipRegion);

	void drawTidesFull (Station *station,
		Timestamp nominalStartTime,
		const Shape::Region& clipRegion,
		Angle *angle = NULL);

  Timestamp getNominalStartTime() { return m_tsStartTime; }

  Timestamp getStartTime();
  Timestamp getStartTime(Timestamp nominalStartTime);
  Timestamp getStartTime(Station *station, Timestamp nominalStartTime);

  Timestamp getEndTime();
  Timestamp getEndTime(Timestamp nominalStartTime);
  Timestamp getEndTime(Station *station, Timestamp nominalStartTime);

  // get the Timestamp offset for the time at offset x in the graph
  Interval getIntervalOffsetAtPosition(unsigned xOffset);
  Interval getIntervalOffsetAtPosition(Station *station, unsigned xOffset);

  void clearTideEventsOrganizer(){m_TideEventOrganizer.clear();}

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

  bool m_bDrawTitle;
  bool m_bDrawTitleOverGraph;
  bool m_bDrawHoursOverGraph;
  bool m_bDrawDepthOverGraph;
  Timestamp m_tsStartTime;
  Station* m_pStation;

  TideEventsOrganizer m_TideEventOrganizer;

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
                  int maxDepth,
                  const Shape::Region& clipRegion);

  // This fills in the background, which indicates sunrise/sunset.
  void clearGraph (Timestamp startTime,
                   Timestamp endTime,
                   Interval increment,
                   Station *station,
                   TideEventsOrganizer &organizer,
				   const Shape::Region& clipRect);

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

	virtual void drawLine(const Shape::Point& p1, const Shape::Point& p2, Colors::Colorchoice c); 
	virtual void drawPolygon(const std::vector<Shape::Point>& points, Colors::Colorchoice, bool filled = false); 
	virtual void drawRect(const Shape::Rectangle& rect, Colors::Colorchoice, bool filled = false);

	virtual void moveGraphRegion(const Shape::Rectangle& area, int dx, int dy){}

	void invalidateRect();
	void invalidateRect(int x, int y, int w, int h);
	virtual void invalidateRect(const Shape::Rectangle& rect){}

	void invalidateDepthLabelsArea();

	virtual void processUpdates(){}

  // Ordering of y1 and y2 is irrelevant.
  void drawVerticalLine (int x,
                         double y1,
                         double y2,
                         Colors::Colorchoice c,
						 const Shape::Region& clipRegion);
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

  virtual void drawHourTick  (int x, Colors::Colorchoice c, const Shape::Region& clipRegion);
  virtual void drawHourTick  (int x, Colors::Colorchoice c);
  virtual void labelHourTick (int x, const Dstr &label, const Shape::Region& clipRegion);
  virtual void labelHourTick (int x, const Dstr &label);

  virtual void drawTitleLine (const Dstr &title);
  virtual void labelEvent    (int topLine, const EventBlurb &blurb);
  virtual void labelEvent    (int topLine, const EventBlurb &blurb, const Shape::Region& clipRegion);
  void drawBlurbs (int topLine, SafeVector<EventBlurb> &blurbs, const Shape::Region& clipRegion);

  // This is the "simple" low level line drawer, where y-coordinates
  // behave in the usual way.
  void drawVerticalLine (int x,
                         int y1,
                         int y2,
                         Colors::Colorchoice c,
						 const Shape::Region& clipRegion);
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
  Shape::Rectangle getCenterStringRect(int x, int y, const Dstr& s);
  Shape::Rectangle getCenterStringOnLineRect(int x, int line, const Dstr& s);
  

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
