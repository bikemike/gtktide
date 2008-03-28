// $Id: Graph.cc 2641 2007-09-02 21:31:02Z flaterco $

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

#include "common.hh"
#include "Graph.hh"
#include "Skycal.hh"

using namespace Shape;

class edge
{
public:
	int x;
	int ymin;
	int ymax;
	double s;
	bool operator<(const edge& other) const
	{
		return ymin < other.ymin || (ymin == other.ymin && x < other.x);
	}
};

// Margin left at top and bottom of tide graphs when scaling tides;
// how much "water" at lowest tide; how much "sky" at highest tide.
// This is a scaling factor for the graph height.
const double Graph::vertGraphMargin (0.0673);

// In drawing of line graphs, slope at which to abandon the thick line
// drawing algorithm.
static const double slopeLimit (5.0);

Graph::Graph (GraphStyle style):
	_style(style),
	_xSize(0),
	_ySize(0),
	m_pStation(NULL)
{
	m_tsStartTime = Timestamp(time(NULL));
}

Graph::Graph (unsigned xSize, unsigned ySize, GraphStyle style):
  _style(style),
  _xSize(xSize),
  _ySize(ySize) {}


Graph::~Graph() {}


static void findNextSunEvent (TideEventsIterator &it,
                              TideEventsOrganizer &organizer,
                              Timestamp now,
                              Timestamp endTime,
                              Timestamp &nextSunEventTime_out) {
  while (it != organizer.end()) {
    TideEvent &te = it->second;
    if (te.eventTime > now &&
         (te.eventType == TideEvent::sunrise ||
          te.eventType == TideEvent::sunset)) {
      nextSunEventTime_out = te.eventTime;
      return;
    }
    ++it;
  }
  nextSunEventTime_out = endTime + Global::day;
}


const double Graph::linterp (double lo, double hi, double saturation) const {
  return lo + saturation * (hi - lo);
}


const unsigned char Graph::linterp (unsigned char lo,
				    unsigned char hi,
				    double saturation) const {
  return (unsigned char) linterp ((double)lo, (double)hi, saturation);
}


const unsigned short Graph::linterp (unsigned short lo,
				     unsigned short hi,
				     double saturation) const {
  return (unsigned short) linterp ((double)lo, (double)hi, saturation);
}


// Translate a tide depth to a y-coordinate.
#define xlate(y) linterp (ymax, \
                          ymin, \
                          (((y) - valmin) / (valmax - valmin)))


void Graph::checkDepth (double ymax,
                        double ymin,
                        double valmax,
                        double valmin,
			unsigned lineStep,
                        int &minDepth_out,
                        int &maxDepth_out) const {
  minDepth_out=INT_MAX;
  maxDepth_out=INT_MIN;
  int depth;
  const double valmax10(valmax*10), valmin10(valmin*10);
  double ytide;
  for (depth = 0; depth <= valmax10; depth += lineStep) {
    ytide = xlate(0.1*depth);
    // Leave room for 3 lines of text at top, 3 lines of text plus
    // tick marks at bottom.
    if (ytide - fontHeight()/2 - depthLineVerticalMargin() <=
                                      (fontHeight()+fontVerticalMargin()) * 3)
      break;
    if (ytide + fontHeight()/2 + depthLineVerticalMargin() >=
             _ySize - (fontHeight()+fontVerticalMargin()) * 3 - hourTickLen())
      continue;
    maxDepth_out = depth;
    if (depth < minDepth_out) // In case one loop is never executed.
      minDepth_out = depth;
  }
  for (depth = -lineStep; depth >= valmin10; depth -= lineStep) {
    ytide = xlate(0.1*depth);
    // Leave room for 3 lines of text at top, 3 lines of text plus
    // tick marks at bottom.
    if (ytide - fontHeight()/2 - depthLineVerticalMargin() <=
                                      (fontHeight()+fontVerticalMargin()) * 3)
      continue;
    if (ytide + fontHeight()/2 + depthLineVerticalMargin() >=
             _ySize - (fontHeight()+fontVerticalMargin()) * 3 - hourTickLen())
      break;
    minDepth_out = depth;
    if (depth > maxDepth_out) // In case one loop is never executed.
      maxDepth_out = depth;
  }
}


void Graph::drawDepth (double ymax,
		double ymin,
		double valmax,
		double valmin,
		unsigned lineStep,
		unsigned labelWidth,
		int minDepth,
		int maxDepth,
		const Region& clipRegion) 
{
	for (int depth=minDepth; depth<=maxDepth; depth+=lineStep)
	{
		int yval = Global::iround(xlate(0.1*depth));

		int xmin = labelWidth;
		int xmax = _xSize;

		if (clipRegion.intersects(Rectangle(xmin, yval, xmax - xmin, 1)))
		{
			drawHorizontalLine (labelWidth,
				_xSize-1,
				xlate(0.1*depth),
				Colors::foreground);
		}
	}
}

void Graph::drawFunkyLine (double prevytide,
                           double ytide,
                           double nextytide,
                           int x,
                           Colors::Colorchoice c) {
  double dy, yleft, yright;
  double slw (Global::settings["lw"].d);

  // The fix for line slope breaks down when the slope gets nasty, so
  // switch to a more conservative strategy when that happens.  Line
  // width becomes 1 no matter what.

#define dohalfline(yy) {                                          \
  double lw;                                                      \
  if (fabs(dy) < slopeLimit)                                      \
    lw = (1.0 + (M_SQRT2 - 1.0) * fabs(dy)) * slw / 2.0;          \
  else                                                            \
    lw = (fabs(dy) + slw) / 2.0;                                  \
  if (dy < 0.0)                                                   \
    lw = -lw;                                                     \
  yy = ytide - lw;                                                \
}

  dy = ytide - prevytide;
  dohalfline (yleft);
  dy = ytide - nextytide;
  dohalfline (yright);

  // Fix degenerate cases.
  if (ytide > yleft && ytide > yright) {
    if (yleft > yright)
      yleft = ytide + slw / 2.0;
    else
      yright = ytide + slw / 2.0;
  } else if (ytide < yleft && ytide < yright) {
    if (yleft < yright)
      yleft = ytide - slw / 2.0;
    else
      yright = ytide - slw / 2.0;
  }
  drawVerticalLine (x, yleft, yright, c);
}


void Graph::clearGraph (Timestamp startTime,
		Timestamp endTime,
		Interval increment,
		Station *station,
		TideEventsOrganizer &organizer,
		const Region& clipRegion)
{
	assert (station);

	// True if event mask is set to suppress sunrises *or* sunsets
	bool ns (Global::settings["em"].s.contains("s"));

	// Clear the graph by laying down a background of days and nights.
	bool sunIsUp = true;
	if (!(station->coordinates.isNull()) && !ns)
		sunIsUp = Skycal::sunIsUp (startTime, station->coordinates);

	Rectangle clipRect = clipRegion.getBoundingRect();
	unsigned min = clipRect.x;
	unsigned max = clipRect.x + clipRect.width;

	Timestamp loopTime (startTime);
	loopTime += increment * min;
	Timestamp nextSunEventTime;
	TideEventsIterator it (organizer.begin());
	findNextSunEvent (it, organizer, loopTime, endTime, nextSunEventTime);

	if (it != organizer.end()) 
		sunIsUp = it->second.eventType == TideEvent::sunrise ? false : true;

	Rectangle rect;
	rect.x = min;

	rect.y = clipRect.y;
	rect.height = clipRect.height;

	
	for (unsigned x = min; x <= max; ++x, loopTime += increment) 
	{
		if (loopTime >= nextSunEventTime && !ns) 
		{
			Colors::Colorchoice c = (sunIsUp ? Colors::daytime : Colors::nighttime);

			rect.width = x - rect.x;
			drawRect(rect, c, true);

			rect.x = x;

			findNextSunEvent (it, organizer, loopTime, endTime, nextSunEventTime);
			assert (loopTime < nextSunEventTime);
			if (it != organizer.end()) 
			{
				switch (it->second.eventType) 
				{
					case TideEvent::sunrise:
						sunIsUp = false;
						break;
					case TideEvent::sunset:
						sunIsUp = true;
						break;
					default:
						assert (false);
				}
			} 
			else
			{
				sunIsUp = !sunIsUp;
			}
		}
		Colors::Colorchoice c = (sunIsUp ? Colors::daytime : Colors::nighttime);
		//drawVerticalLine (x, 0, _ySize-1, c);
	}
	rect.width = max - rect.x;
	Colors::Colorchoice c = (sunIsUp ? Colors::daytime : Colors::nighttime);
	drawRect(rect, c, true);
}


void Graph::drawLine(const Point& p1, const Point& p2, Colors::Colorchoice c) 
{
	bool bXpos = p2.x > p1.x;
	bool bYpos = p2.y > p1.y;
	int width  = std::abs(p2.x - p1.x);
	int height = std::abs(p2.y - p1.y);

	if (width > height)
	{
		for (int i = p1.x; i != ((bXpos) ? p2.x+1 : p2.x-1); 
			(bXpos ? ++i : --i))
		{
			double dy = std::abs(i - p1.x) / (double)(width);
			int yheight =  (int)(dy * (height) + .5);
			int y = p1.y + ((bYpos) ? yheight : -yheight);
			setPixel(i,y,c);
		}
	}
	else
	{
		for (int i = p1.y; i != (bYpos ? p2.y+1 : p2.y-1); 
			(bYpos ? ++i : --i))
		{
			double dx = std::abs(i - p1.y) / (double)(height);
			int xwidth =  (int)(dx * (width) + .5);
			int x = p1.x + ((bXpos) ? xwidth : 0 -xwidth);
			setPixel(x,i,c);
		}
	}


}

void Graph::drawPolygon(const std::vector<Point>& points, Colors::Colorchoice c, bool filled /* = false */ )
{
	std::vector<Point> new_points;
	const std::vector<Point>* ppoints; 
	ppoints = &points;

	if (filled)
	{
		if (2 <= points.size())
		{
			if (points.front() != points.back())
			{
				new_points = points;
				new_points.push_back(points.front());
				ppoints = &new_points;
			}
		}

		std::vector<edge> edges;
		if (2 <= ppoints->size())
		{
			for (int i = 1; i < ppoints->size(); ++i)
			{
				edge e;
				e.x = (*ppoints)[i-1].x;
				e.ymin = std::min((*ppoints)[i-1].y, (*ppoints)[i].y);
				e.ymax = std::max((*ppoints)[i-1].y, (*ppoints)[i].y);
				e.s = ((*ppoints)[i].y - (*ppoints)[i-1].y ) /
					(double)((*ppoints)[i].x - (*ppoints)[i-1].x );
				edges.push_back(e);
			}
		}

		std::vector<edge> gedges;
		std::vector<edge>::iterator itr;
		for (itr = edges.begin(); edges.end() != itr; ++itr)
		{
			if (0.0 == itr->s)
			{
				continue;
			}
			else
			{
				gedges.push_back(*itr);
			}
		}
		std::sort(gedges.begin(), gedges.end());

		int scanline = gedges.front().ymin;
		while (true)
		{
			bool bParity = true;

			std::vector<edge> active_edges;
			for (itr = gedges.begin() ; gedges.end () != itr; ++itr)
			{
				if (itr->ymin == scanline)
				{
					active_edges.push_back(*itr);
				}
				else
				{
					break;
				}
			}

			if (0 == active_edges.size())
				break;

			int startx = 0;
			for (itr = active_edges.begin() ; active_edges.end () != itr; ++itr)
			{
				if (!bParity)
				{
					drawHorizontalLine(startx, itr->x, scanline, c);
				}
				startx = itr->x;
				bParity = !bParity;
			}

			++scanline;

			for (itr = gedges.begin() ; gedges.end() != itr; )
			{
				if (itr->ymin == scanline-1)
				{
					itr->x = itr->x + (int)(1/itr->s);
					++itr->ymin;
				}

				if (itr->ymax <= scanline)
				{
					itr = gedges.erase(itr);
				}
				else
				{
					++itr;
				}
			}
			std::sort(gedges.begin(), gedges.end());
		}
	}
	else
	{
		if (2 <= ppoints->size())
		{
			for (int i = 1; i < ppoints->size(); ++i)
			{
				drawLine((*ppoints)[i-1], (*ppoints)[i], c);
			}
		}
	}

}	

void Graph::drawRect(const Shape::Rectangle& rect, Colors::Colorchoice c, bool filled /* = false*/)
{
	if (filled)
	{
		for (int i = 0; i < rect.width; i++)
		{
			drawVerticalLine(rect.x + i, rect.y, rect.y + rect.height, c);
		}
	}
	else
	{
		drawHorizontalLine(rect.x, rect.x + rect.width, rect.y, c);
		drawHorizontalLine(rect.x, rect.x + rect.width, rect.y + rect.height-1, c);
		drawVerticalLine(rect.x, rect.y, rect.y + rect.height, c);
		drawVerticalLine(rect.x + rect.width -1, rect.y, rect.y + rect.height, c);
	}
}


void Graph::drawX (int x, double y) {
  drawVerticalLine   (x, y-4, y+4, Colors::foreground);
  drawHorizontalLine (x-4, x+4, y, Colors::foreground);
}


static void makeDepthLabel (int depth,
			    unsigned lineStep,
			    const Dstr &unitsDesc,
			    Dstr &text_out) {
  text_out = "";
  if (depth < 0) {
    text_out += '-';
    depth = -depth;
  }
  text_out += depth / 10;
  if (lineStep < 10) {
    text_out += '.';
    text_out += depth % 10;
  }
  text_out += ' ';
  text_out += unitsDesc;
}


void Graph::invalidateDepthLabelsArea()
{
	// Figure constants.
	const double ymin (vertGraphMargin * (double)_ySize);
	const double ymax ((double)_ySize - ymin);
	const double valmin (m_pStation->minLevel().val());
	const double valmax (m_pStation->maxLevel().val());
	assert (valmin < valmax);

	unsigned lineStep, labelWidth, labelRight;
	int minDepth, maxDepth;
	const Dstr unitsDesc (Units::shortName (m_pStation->predictUnits()));
	figureLabels (ymax, ymin, valmax, valmin, unitsDesc, lineStep, labelWidth,
			labelRight, minDepth, maxDepth);

	// Height axis.
	for (int depth = minDepth; depth <= maxDepth; depth += lineStep) 
	{
		Dstr dlabel;
		makeDepthLabel (depth, lineStep, unitsDesc, dlabel);
		double adj = 0.0;
		if (fontHeight() > 1)
			adj = (double)(fontHeight()) / 2.0;

		Rectangle r(0, (int)(xlate(0.1*depth)-adj), labelWidth, (int)fontHeight());
		invalidateRect(r);
	}

}


Rectangle Graph::getDepthAreaRect()
{
	const double ymin (vertGraphMargin * (double)_ySize);
	const double ymax ((double)_ySize - ymin);
	const double valmin (m_pStation->minLevel().val());
	const double valmax (m_pStation->maxLevel().val());
	assert (valmin < valmax);

	unsigned lineStep, labelWidth, labelRight;
	int minDepth, maxDepth;
	const Dstr unitsDesc (Units::shortName (m_pStation->predictUnits()));
	figureLabels (ymax, ymin, valmax, valmin, unitsDesc, lineStep, labelWidth,
			labelRight, minDepth, maxDepth);

	return Rectangle(0,0, labelWidth, _ySize);
}

Rectangle Graph::getTitleAreaRect()
{
	if (_style == clock) 
	{
		return Rectangle(0,0,_xSize, 3*(fontHeight()+fontVerticalMargin()));
	}
	else
	{
		return Rectangle(0,0,_xSize, fontHeight()+fontVerticalMargin());
	}
}


void Graph::setSize(unsigned xSize, unsigned ySize)
{
	if (_xSize != xSize || _ySize != ySize)
	{
		_xSize = xSize;
		_ySize = ySize;
		invalidateRect();
	}
}

void Graph::setNominalStartTime(Timestamp nominalStartTime)
{
	if (m_tsStartTime != nominalStartTime)
	{
		Interval interval = m_tsStartTime - nominalStartTime;
		m_tsStartTime = nominalStartTime;
		// calculate the xoffset based on the interval

		Interval increment (std::max ((interval_rep_t)1,
			Global::intervalround (Global::aspectMagicNumber /
			(double)_ySize /
			(aspectFudgeFactor() * m_pStation->aspect))));
		
		double ddx = interval / increment;
		int dx;


		if (ddx < 0)
		{
			dx = (int)(ddx - .5);
		}
		else
		{
			dx = (int)(ddx + .5);
		}

		// invalidate the title area
		Rectangle titleRect = getTitleAreaRect();
		Rectangle depthRect = getDepthAreaRect();

		Rectangle moveRect;
		moveRect.x = depthRect.x + depthRect.width;
		moveRect.y = titleRect.y + titleRect.height;
		moveRect.width = _xSize - moveRect.x;
		moveRect.height = _ySize - moveRect.y;

		if (0 > dx)
		{
			moveRect.x -= dx;
			moveRect.width += dx;

		}
		moveGraphRegion(moveRect, dx, 0);
		invalidateRect(titleRect);
		depthRect.width += 1; // add one for an occasional off by one bug
		invalidateRect(depthRect);
		processUpdates();
	}
}

void Graph::setStation(Station* station) 
{ 
	if (station != m_pStation)
	{
		m_pStation = station;
		invalidateRect();
	}
}

void Graph::invalidateRect()
{
	Rectangle r;
	r.x = 0;
	r.y = 0;
	r.width = _xSize;
	r.height = _ySize;
	invalidateRect(r);
}	

void Graph::invalidateRect(int x, int y, int w, int h)
{
	invalidateRect(Rectangle(x,y,w,h));
}


void Graph::figureLabels (double ymax,
			  double ymin,
			  double valmax,
			  double valmin,
                          const Dstr &unitsDesc,
			  unsigned &lineStep_out,
			  unsigned &labelWidth_out,
			  unsigned &labelRight_out,
			  int &minDepth_out,
			  int &maxDepth_out) const {
  assert (valmin < valmax);
  const double yzulu (xlate(0));

  // Tortured logic to figure the increment for depth lines.  We want
  // nice increments like 2, 5, 10, not random numbers.
  if (Global::settings["gt"].c == 'y')
    lineStep_out = 1;
  else
    lineStep_out = 10;
  {
    unsigned prevStep (lineStep_out), prevMult (10);
    while (yzulu - xlate(0.1*lineStep_out) <
	   fontHeight() + fontVerticalMargin()) {
      switch (prevMult) {
      case 10:
	prevMult = 2;
	lineStep_out = prevStep * prevMult;
	break;
      case 2:
	prevMult = 5;
	lineStep_out = prevStep * prevMult;
	break;
      case 5:
	prevMult = 10;
	prevStep = lineStep_out = prevStep * prevMult;
	break;
      default:
	assert (false);
      }
    }
  }

  // More figuring.
  checkDepth (ymax, ymin, valmax, valmin, lineStep_out, minDepth_out,
	      maxDepth_out);
  labelWidth_out = labelRight_out = 0;
  if (minDepth_out <= maxDepth_out) {
    {
      Dstr minLabel;
      makeDepthLabel (minDepth_out, lineStep_out, unitsDesc, minLabel);
      labelWidth_out = stringWidth(minLabel);
    }{
      Dstr maxLabel;
      makeDepthLabel (maxDepth_out, lineStep_out, unitsDesc, maxLabel);
      unsigned maxLabelWidth (stringWidth(maxLabel));
      if (maxLabelWidth > labelWidth_out)
	labelWidth_out = maxLabelWidth;
    }
    labelRight_out = labelWidth_out + depthLabelLeftMargin();
    labelWidth_out = labelRight_out + depthLabelRightMargin();
  }
  // Otherwise, there are no depth lines, but labelWidth (0) will be
  // used for extra lines and the now (+) mark.
}


// Blurbs push and shove their neighbors until everyone has room.

static const bool iterateEventBlurbCollisions (
				       SafeVector<Graph::EventBlurb> &blurbs) {
  bool collision (false);
  for (unsigned long i=1; i<blurbs.size(); ++i) {
    Graph::EventBlurb &prev (blurbs[i-1]);
    Graph::EventBlurb &cur (blurbs[i]);
    if (prev.x > cur.x) // Try to keep them in order at least.
      std::swap (prev.x, cur.x);
    int collisionWidth ((prev.x + prev.deltaRight) -
			(cur.x + cur.deltaLeft) + 1);
    if (collisionWidth > 0) {
      collision = true;
      int leftAdjust (-collisionWidth / 2);
      int rightAdjust (collisionWidth + leftAdjust);
      prev.x += leftAdjust;
      cur.x  += rightAdjust;
    }
  }
  return collision;
}


// With no iteration limit, this would converge eventually.  There is
// a first event and a last event, and those boundary events would
// eventually be pushed far enough out into the void to make room for
// everyone.  However, the distance that events could migrate from
// their original locations would be unbounded.  The optimal iteration
// limit to maintain some semblance of order is anybody's guess.  Even
// when it fails, the end result is better than what we started with.

static void fixEventBlurbCollisions (SafeVector<Graph::EventBlurb> &blurbs) {
  for (unsigned i=0; i<20 && iterateEventBlurbCollisions(blurbs); ++i);
}


void Graph::drawBlurbs (int topLine, SafeVector<EventBlurb> &blurbs, const Region& clipRegion)
{
  fixEventBlurbCollisions (blurbs);
  for (SafeVector<EventBlurb>::iterator blurbit = blurbs.begin();
       blurbit != blurbs.end();
       ++blurbit)
    labelEvent (topLine, *blurbit, clipRegion);
}

Timestamp Graph::getStartTime(Timestamp ts)
{
	return getStartTime(m_pStation, ts);
}

Timestamp Graph::getStartTime()
{
	return getStartTime(m_pStation, m_tsStartTime);
}

Timestamp Graph::getStartTime(Station *station, Timestamp nominalStartTime)
{
  const double ymin (vertGraphMargin * (double)_ySize);
  const double ymax ((double)_ySize - ymin);
  const double valmin (station->minLevel().val());
  const double valmax (station->maxLevel().val());
  assert (valmin < valmax);

  unsigned lineStep, labelWidth, labelRight;
  int minDepth, maxDepth;
  const Dstr unitsDesc (Units::shortName (station->predictUnits()));
  figureLabels (ymax, ymin, valmax, valmin, unitsDesc, lineStep, labelWidth,
		labelRight, minDepth, maxDepth);
 
  Interval startOffset = getIntervalOffsetAtPosition(station, startPosition(labelWidth));
  Timestamp startTime (nominalStartTime - startOffset);
  return startTime;
}

Timestamp Graph::getEndTime()
{
	return getEndTime(m_pStation, m_tsStartTime);
}

Timestamp Graph::getEndTime(Timestamp ts)
{
	return getEndTime(m_pStation, ts);
}

Timestamp Graph::getEndTime(Station *station, Timestamp nominalStartTime)
{
  Interval endOffset = getIntervalOffsetAtPosition(station, _xSize);
  Timestamp startTime = getStartTime(station, nominalStartTime);
  Timestamp endTime = startTime + endOffset;
  return endTime;
}

Interval Graph::getIntervalOffsetAtPosition(unsigned xOffset) 
{
	if (NULL == m_pStation)
		return Interval();

	return getIntervalOffsetAtPosition(m_pStation, xOffset); 
}

Interval Graph::getIntervalOffsetAtPosition(Station *station, unsigned xOffset) {

  Interval increment (std::max ((interval_rep_t)1,
           Global::intervalround (Global::aspectMagicNumber /
                                  (double)_ySize /
  			          (aspectFudgeFactor() * station->aspect))));

  Interval offsetInterval = 
	  increment * xOffset;
  return offsetInterval;

}


void Graph::drawTides (const Region& clipRegion)
{
	//printf("drawing tides\n");
	if (NULL != m_pStation)
		drawTides(m_pStation, m_tsStartTime, clipRegion, NULL);
}

void Graph::drawTides (Station *station,
                       Timestamp nominalStartTime,
                       Angle *angle) 
{
	drawTides(station, nominalStartTime, Region(0,0,_xSize,_ySize), angle);
}

void Graph::drawTides (Station *station,
                       Timestamp nominalStartTime,
					   const Region& clipRegion,
                       Angle *angle) 
{
	assert (station);
	assert (station->aspect > 0.0);

	// Figure constants.
	const double ymin (vertGraphMargin * (double)_ySize);
	const double ymax ((double)_ySize - ymin);
	const double valmin (station->minLevel().val());
	const double valmax (station->maxLevel().val());
	assert (valmin < valmax);

	unsigned lineStep, labelWidth, labelRight;
	int minDepth, maxDepth;
	const Dstr unitsDesc (Units::shortName (station->predictUnits()));
	figureLabels (ymax, ymin, valmax, valmin, unitsDesc, lineStep, labelWidth,
			labelRight, minDepth, maxDepth);

	char tl (Global::settings["tl"].c);
	char nf (Global::settings["nf"].c);
	char el (Global::settings["el"].c);
	Interval increment (std::max ((interval_rep_t)1,
				Global::intervalround (Global::aspectMagicNumber /
					(double)_ySize /
					(aspectFudgeFactor() * station->aspect))));
	Timestamp startTime(getStartTime(station, nominalStartTime));
	Timestamp endTime (getEndTime(station, nominalStartTime));
	Timestamp currentTime ((time_t)time(NULL));

	// First get a list of the relevant tide events.  Need some extra on
	// either side since text pertaining to events occurring beyond the
	// margins can still be visible.  We also need to make sure
	// *something* shows up so that extendRange can work below.

	TideEventsOrganizer organizer;
	Interval delta;
	for (delta = Global::day; organizer.empty(); delta *= 2U)
		station->predictTideEvents (startTime - delta, endTime + delta, organizer);

	// Need at least one following max and min for clock mode.
	TideEvent nextMax, nextMin;
	if (_style == clock) 
	{
		bool doneMax = false, doneMin = false;
		delta = Global::day;
		while (!(doneMax && doneMin)) 
		{
			TideEventsIterator it = organizer.upper_bound(currentTime);
			while (it != organizer.end() && !(doneMax && doneMin)) 
			{
				TideEvent &te = it->second;
				if (!doneMax && te.eventType == TideEvent::max) 
				{
					doneMax = true;
					nextMax = te;
				} 
				else if (!doneMin && te.eventType == TideEvent::min) 
				{
					doneMin = true;
					nextMin = te;
				}
				++it;
			}
			if (!(doneMax && doneMin)) 
			{
				station->extendRange (organizer, Station::forward, delta);
				delta *= 2U;
			}
		}

		// Need a max/min pair bracketing current time for tide clock icon
		// angle kludge.
		if (angle) 
		{
			TideEvent nextMaxOrMin;
			if (nextMax.eventTime < nextMin.eventTime)
				nextMaxOrMin = nextMax;
			else
				nextMaxOrMin = nextMin;
			TideEvent previousMaxOrMin;

			{
				bool done = false;
				delta = Global::day;
				while (!done) 
				{
					TideEventsIterator it = organizer.upper_bound(currentTime);
					assert (it != organizer.end());
					while (it != organizer.begin() && !done)
						if ((--it)->second.isMaxMinEvent()) 
						{
							done = true;
							previousMaxOrMin = it->second;
						}
					if (!done) 
					{
						station->extendRange (organizer, Station::backward, delta);
						delta *= 2U;
					}
				}
			}

			// This could blow up on pathological subordinate stations.
			// Better to let it slide.  (The clock will do something weird
			// but won't die.)
			// assert (previousMaxOrMin.eventType != nextMaxOrMin.eventType);

			assert (previousMaxOrMin.eventTime <= currentTime &&
					nextMaxOrMin.eventTime > currentTime);
			assert (previousMaxOrMin.isMaxMinEvent());
			assert (nextMaxOrMin.isMaxMinEvent());

			double temp ((currentTime - previousMaxOrMin.eventTime) /
					(nextMaxOrMin.eventTime - previousMaxOrMin.eventTime));
			temp *= 180.0;
			if (previousMaxOrMin.eventType == TideEvent::min)
				temp += 180.0;
			(*angle) = Angle (Units::degrees, temp);
		}
	}

	//printf("clearing graph\n");
	clearGraph (startTime, endTime, increment, station, organizer, clipRegion);

	// Draw depth lines now?
	if (tl == 'n')
	{
		//printf("drawing depth\n");
		drawDepth (ymax, ymin, valmax, valmin, lineStep, labelWidth, minDepth,
				maxDepth, clipRegion);
	}

	Rectangle clipRect = clipRegion.getBoundingRect();
	unsigned minx = clipRect.x;
	unsigned maxx = clipRect.x + clipRect.width;
	unsigned miny = clipRect.y;
	unsigned maxy = clipRect.y + clipRect.height;

	Timestamp loopt;
	loopt = startTime + increment*minx;

	// Prepare to draw the actual tides.
	double prevval, prevytide;
	double val (station->predictTideLevel(loopt-increment).val());
	double ytide = xlate(val);

	// clamp to cliprect
	const double yzulu  = std::min((xlate(0)), (double)maxy);

	double nextval (station->predictTideLevel(loopt).val());
	double nextytide (xlate (nextval));
	int x;
	startPixelCache();

	std::vector<Point> points;
	//std::vector<Point> poly;
	std::vector< std::pair<Colors::Colorchoice, std::vector<Point> > > polys;

	//polys[polys.size()-1].second.push_back(Point(x,y));

	// loopt is actually 1 step ahead of x.
	std::list<Point> Points;
	for (x=minx, loopt+= increment;
			x <= maxx;
			++x, loopt += increment) 
	{
		//printf("looping through tide incs\n");
		prevval = val;
		prevytide = ytide;
		val = nextval;
		ytide = nextytide;
		nextval = station->predictTideLevel(loopt).val();

		nextytide = xlate(nextval);

		// Coloration is determined from the predicted heights, not from
		// the eventTypes of surrounding tide events.  Ideally the two
		// would never disagree, but for pathological sub stations they
		// can.

		// clamp to cliprect
		//bool bTop = miny < ytide;
		//ytide = std::max(ytide, (double)miny);
		if (station->isCurrent) 
		{
			Colors::Colorchoice c = (val > 0.0 ? Colors::flood : Colors::ebb);
			if (nf == 'n')
			{
				if (ytide > yzulu)
				{
					// out of clipRect range
					continue;
				}
				//drawVerticalLine (x, yzulu, ytide, c);
				points.push_back(Point(x,(int)ytide));

				if ( 0 == polys.size() || polys[polys.size() -1].first != c )
				{
					polys.push_back(std::pair<Colors::Colorchoice, std::vector<Point> >(c, std::vector<Point>()));
				}
				
				if (polys.back().second.size() == 0)
				{
					if (1 < polys.size())
					{
						polys[polys.size()-2].second.push_back(Point(x-1, (int)yzulu));
					}
					polys.back().second.push_back(Point(x, (int)yzulu));
				}
				polys.back().second.push_back(Point(x, (int)ytide));
			}
			else
				drawFunkyLine (prevytide, ytide, nextytide, x, c);

		}
		else 
		{
			Colors::Colorchoice c = (prevval < val ? Colors::flood : Colors::ebb);
			
			if (nf == 'n')
			{
				// clamp to cliprect
				unsigned bottom = std::min(_ySize, maxy);
				if (ytide > bottom)
				{
					// out of clipRect range
					continue;
				}
				//drawVerticalLine (x, bottom, ytide, c);
				points.push_back(Point(x,(int)ytide));

				if ( 0 == polys.size() || polys[polys.size() -1].first != c )
				{
					polys.push_back(std::pair<Colors::Colorchoice, std::vector<Point> >(c, std::vector<Point>()));
				}
				
				if (polys.back().second.size() == 0)
				{
					if (1 < polys.size())
					{
						polys[polys.size()-2].second.push_back(Point(x-1, (int)_ySize));
					}
					polys.back().second.push_back(Point(x, (int)_ySize));
				}
				polys.back().second.push_back(Point(x, (int)ytide));
			}
			else
				drawFunkyLine (prevytide, ytide, nextytide, x, c);
		}

#ifdef blendingTest
		NullablePredictionValue firstYear, secondYear;
		station->tideLevelBlendValues (loopt-increment, firstYear, secondYear);
		if (!firstYear.isNull())
			setPixel (x, (int)xlate(firstYear.val()), Colors::mark);
		if (!secondYear.isNull())
			setPixel (x, (int)xlate(secondYear.val()), Colors::msl);
#endif
	}
	
	if (0 < polys.size())
	{
		if (station->isCurrent) 
			polys[polys.size()-1].second.push_back(Point(x-1, (int)yzulu));
		else
			polys[polys.size()-1].second.push_back(Point(x-1, (int)_ySize));
	}

	//drawPolygon(points, Colors::background);

	std::vector< std::pair<Colors::Colorchoice, std::vector<Point> > >::iterator itr;
	for (itr = polys.begin(); polys.end() != itr; ++itr)
	{
		drawPolygon(itr->second, itr->first, true);
	}	


	stopPixelCache();

	// Draw depth lines later?
	if (tl != 'n')
		drawDepth (ymax, ymin, valmax, valmin, lineStep, labelWidth, minDepth,
				maxDepth, clipRect);

	// Height axis.
	for (int depth = minDepth; depth <= maxDepth; depth += lineStep) 
	{
		Dstr dlabel;
		makeDepthLabel (depth, lineStep, unitsDesc, dlabel);
		double adj = 0.0;
		if (fontHeight() > 1)
			adj = (double)(fontHeight()) / 2.0;
		rightJustifyString (labelRight, xlate(0.1*depth)-adj, dlabel);
	}

	// Relatively straightforward logic to figure the increment for the
	// time axis.  We stop at 1 day.
	unsigned timeStep = 1;
	unsigned doubleOughtWidth = stringWidth("00");
	if ((Global::hour * timeStep) / increment < doubleOughtWidth)
		timeStep = 2;
	if ((Global::hour * timeStep) / increment < doubleOughtWidth)
		timeStep = 3;
	if ((Global::hour * timeStep) / increment < doubleOughtWidth)
		timeStep = 4;
	if ((Global::hour * timeStep) / increment < doubleOughtWidth)
		timeStep = 6;
	if ((Global::hour * timeStep) / increment < doubleOughtWidth)
		timeStep = 12;
	if ((Global::hour * timeStep) / increment < doubleOughtWidth)
		timeStep = 24;

	// Do time axis.
	const Timestamp timeAxisStopTime (endTime + Global::hour * timeStep);
	loopt = startTime;
	for (loopt.floorHour(station->timezone);
			loopt < timeAxisStopTime;
			loopt.nextHour(station->timezone)) 
	{
		if (loopt.tmStruct(station->timezone).tm_hour % timeStep == 0) 
		{
			x = Global::iround ((loopt - startTime) / increment);
			drawHourTick (x, Colors::foreground, clipRect);
			Dstr ts;
			loopt.printHour (ts, station->timezone);
			labelHourTick (x, ts, clipRect);
		}
	}

	/* Make tick marks for day boundaries thicker. */
	/* They are not guaranteed to coincide with hour transitions! */
	loopt = startTime;
	for (loopt.floorDay(station->timezone);
			loopt < timeAxisStopTime;
			loopt.nextDay(station->timezone)) 
	{
		x = Global::iround ((loopt - startTime) / increment);
		drawHourTick (x-1, Colors::foreground, clipRect);
		drawHourTick (x, Colors::foreground, clipRect);
		drawHourTick (x+1, Colors::foreground, clipRect);
	}

	if (_style == clock) 
	{

		if (clipRect.intersects(getTitleAreaRect()))
		{
			// Write current time
			Dstr ts;
			currentTime.printTime (ts, station->timezone);
			centerStringOnLine (_xSize/2, 0, ts);

			// Write next max
			centerStringOnLine (_xSize/2, 1, nextMax.longDescription());
			nextMax.eventTime.printTime (ts, station->timezone);
			centerStringOnLine (_xSize/2, 2, ts);

			// Write next min
			centerStringOnLine (_xSize/2, -3, nextMin.longDescription());
			nextMin.eventTime.printTime (ts, station->timezone);
			centerStringOnLine (_xSize/2, -2, ts);
		}

	}
	else 
	{

		if (clipRect.intersects(getTitleAreaRect()))
		{
			drawTitleLine (station->name);
		}

		// Put timestamps for timestampable events.
#ifdef DumbTideClockDemo
		Timestamp firstMaxMinTime;
#endif
		SafeVector<EventBlurb> topBlurbs, bottomBlurbs;
		EventBlurb tempBlurb;
		for (TideEventsIterator it = organizer.begin();
				it != organizer.end();
				++it) 
		{
			TideEvent &te = it->second;
#ifdef DumbTideClockDemo
			if (firstMaxMinTime.isNull() &&
					te.eventTime >= startTime &&
					te.isMaxMinEvent())
				firstMaxMinTime = te.eventTime;
#endif
			tempBlurb.x = Global::iround ((te.eventTime - startTime) / increment);
			switch (te.eventType) 
			{
				case TideEvent::max:
				case TideEvent::min:
					te.eventTime.printDate (tempBlurb.line1, station->timezone);
					te.eventTime.printTime (tempBlurb.line2, station->timezone);
					measureBlurb (tempBlurb);
					topBlurbs.push_back (tempBlurb);
					break;
				case TideEvent::moonrise:
				case TideEvent::moonset:
				case TideEvent::slackrise:
				case TideEvent::slackfall:
				case TideEvent::markrise:
				case TideEvent::markfall:
				case TideEvent::newmoon:
				case TideEvent::firstquarter:
				case TideEvent::fullmoon:
				case TideEvent::lastquarter:
					drawHourTick (tempBlurb.x, Colors::mark, clipRect);
					te.eventTime.printTime (tempBlurb.line2, station->timezone);
					tempBlurb.line1 = te.longDescription();
					if (!isBanner())
						if (stringWidth(tempBlurb.line1) > stringWidth(tempBlurb.line2))
							tempBlurb.line1 = te.shortDescription();
					measureBlurb (tempBlurb);
					bottomBlurbs.push_back (tempBlurb);
					break;
				default:
					break;
			}
		}

		drawBlurbs (1, topBlurbs, clipRect);
		drawBlurbs (-3, bottomBlurbs, clipRect);

#ifdef DumbTideClockDemo
		if (!firstMaxMinTime.isNull()) 
		{
			for (loopt = firstMaxMinTime;
					loopt < endTime;
					loopt += Global::halfCycle) 
			{
				x = (int) round ((loopt - startTime) / increment);
				drawVerticalLine (x, (int)_ySize-1, 0, Colors::msl);
			}
		}
#endif
	}

	// Extra lines.
	if (!station->markLevel.isNull()) 
	{
		ytide = xlate(station->markLevel.val());
		drawHorizontalLine (labelWidth, _xSize-1, ytide, Colors::mark);
	}
	if (el != 'n') 
	{
		drawHorizontalLine (labelWidth, _xSize-1, yzulu, Colors::datum);
		ytide = (ymax + ymin) / 2.0;
		drawHorizontalLine (labelWidth, _xSize-1, ytide, Colors::msl);
	}

	// X marks the current time.
	if (currentTime >= startTime && currentTime < endTime) 
	{
		x = Global::iround ((currentTime - startTime) / increment);
		ytide = xlate(station->predictTideLevel(currentTime).val());
		drawX (x, ytide);
	}
}


void Graph::drawHorizontalLine (int xlo,
                                int xhi,
                                double y,
				Colors::Colorchoice c) {
  drawHorizontalLine (xlo, xhi, Global::iround(y), c);
}


void Graph::drawString (int x, double y, const Dstr &s) {
  drawString (x, Global::iround(y), s);
}


Rectangle Graph::getCenterStringRect(int x, int y, const Dstr& s)
{
	unsigned w = stringWidth(s);
	return Rectangle(x-(int)w/2,y, w, fontHeight());
}

void Graph::centerString (int x, int y, const Dstr &s) {
  Rectangle r = getCenterStringRect(x,y,s);
  drawString (r.x, r.y, s);
}


// This must agree with whatever centerString does about odd numbers.
// However, it doesn't hurt to fudge by an even number to create a
// margin on both sides.
void Graph::measureBlurb (EventBlurb &blurb) const {
  int width (std::max (stringWidth (blurb.line1), stringWidth (blurb.line2))
	     + blurbMargin());
  blurb.deltaLeft = -width / 2;
  blurb.deltaRight = width + blurb.deltaLeft - 1;
}


const int Graph::blurbMargin() const {
  return 2;
}


void Graph::rightJustifyString (int x, double y, const Dstr &s) {
  drawString (x-(int)stringWidth(s), y, s);
}

Rectangle Graph::getCenterStringOnLineRect(int x, int line, const Dstr& s)
{
  int y;
  if (line >= 0)
    y = line * (fontHeight()+fontVerticalMargin());
  else
    y = _ySize+(fontHeight()+fontVerticalMargin())*line-hourTickLen();

  return getCenterStringRect(x, y, s);
}

void Graph::centerStringOnLine (int x, int line, const Dstr &s) {
  Rectangle r = getCenterStringOnLineRect(x,line,s);
  drawString (r.x, r.y, s);
}


const unsigned Graph::fontVerticalMargin() const {
  return 0;
}


const unsigned Graph::depthLabelLeftMargin() const {
  return 2;
}


const unsigned Graph::depthLabelRightMargin() const {
  return 2;
}


const unsigned Graph::depthLineVerticalMargin() const {
  return 2;
}


const unsigned Graph::hourTickLen() const {
  return 8;
}


const double Graph::aspectFudgeFactor() const {
  return 1.0;
}


const unsigned Graph::startPosition (unsigned labelWidth) const {
  return labelWidth + 8;
}


void Graph::setPixel (int x,
                      int y,
                      Colors::Colorchoice c,
		      double saturation) {
  assert (c < (int)Colors::numColors);
  if (saturation >= 0.5)
    setPixel (x, y, c);
}


void Graph::drawHourTick (int x, Colors::Colorchoice c) {
	drawHourTick(x, c, Rectangle(0,0,_xSize, _ySize));
}

void Graph::drawHourTick (int x, Colors::Colorchoice c, const Region& clipRegion) {
  Rectangle tickRect(x,(int)_ySize-hourTickLen(), 1, hourTickLen());
  if (clipRegion.intersects(tickRect))
  {
    drawVerticalLine (tickRect.x, tickRect.y, tickRect.y + tickRect.height, c);
  }
}


void Graph::drawVerticalLine (int x, int y1, int y2, Colors::Colorchoice c) {
  int ylo, yhi;
  if (y1 < y2) {
    ylo = y1; yhi = y2;
  } else {
    ylo = y2; yhi = y1;
  }
  for (int i=ylo; i<=yhi; ++i)
    setPixel (x, i, c);
}


void Graph::drawVerticalLine (int x,
                              double y1,
                              double y2,
                              Colors::Colorchoice c) {
  double ylo, yhi;
  if (y1 < y2) {
    ylo = y1; yhi = y2;
  } else {
    ylo = y2; yhi = y1;
  }
  int ylo2 ((int) ceil (ylo));
  int yhi2 ((int) floor (yhi));
  if (ylo2 < yhi2)
    drawVerticalLine (x, ylo2, yhi2-1, c);
  // What if they both fall within the same pixel:  ylo2 > yhi2
  if (ylo2 > yhi2) {
    assert (yhi2 == ylo2 - 1);
    double saturation (yhi - ylo);
    setPixel (x, yhi2, c, saturation);
  } else {
    // The normal case.
    if (ylo < (double)ylo2) {
      double saturation ((double)ylo2 - ylo);
      setPixel (x, ylo2-1, c, saturation);
    }
    if (yhi > (double)yhi2) {
      double saturation (yhi - (double)yhi2);
      setPixel (x, yhi2, c, saturation);
    }
  }
}


void Graph::drawHorizontalLine (int xlo,
                                int xhi,
                                int y,
			        Colors::Colorchoice c) {
  for (int i=xlo; i<=xhi; ++i)
    setPixel (i, y, c);
}


void Graph::labelHourTick (int x, const Dstr &label) {
	labelHourTick(x, label, Rectangle(0,0,_xSize, _ySize));
}
void Graph::labelHourTick (int x, const Dstr &label, const Region& clipRegion) {
  if (clipRegion.intersects(getCenterStringOnLineRect(x,-1, label)))
  {
    centerStringOnLine (x, -1, label);
  }
}


void Graph::drawTitleLine (const Dstr &title) {
  centerStringOnLine (_xSize/2, 0, title);
}


void Graph::labelEvent (int topLine, const EventBlurb &blurb)
{
	labelEvent(topLine, blurb, Rectangle(0,0, _xSize, _ySize));
}

void Graph::labelEvent (int topLine, const EventBlurb &blurb, const Region& clipRegion)
{
  Rectangle blurb1Rect = getCenterStringOnLineRect(blurb.x, topLine, blurb.line1);
  Rectangle blurb2Rect = getCenterStringOnLineRect(blurb.x, topLine+1, blurb.line2);

  // This needs to get overridden in Banner.
  if (clipRegion.intersects(blurb1Rect))
  {
	  centerStringOnLine (blurb.x, topLine, blurb.line1);
  }
  if (clipRegion.intersects(blurb2Rect))
  {
	  centerStringOnLine (blurb.x, topLine+1, blurb.line2);
  }
}


const bool Graph::isBanner() const {
  return false;
}


void Graph::startPixelCache() {
}


void Graph::stopPixelCache() {
}

// Cleanup2006 LongMethod(drawTides) Done
