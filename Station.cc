// $Id: Station.cc 2946 2008-01-18 23:12:25Z flaterco $

/*  Station  A tide station.

    Station has a subclass SubordinateStation.  The superclass is used
    for reference stations and that rare subordinate station where the
    offsets can be reduced to simple corrections to the constituents
    and datum.  After such corrections are made, there is no
    operational difference between that and a reference station.

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
#include "Graph.hh"
#include "TTYGraph.hh"
#include "Banner.hh"
#include "RGBGraph.hh"
#include "Skycal.hh"

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif


#ifndef HAVE_LLROUND
// Round to nearest integer, away from zero.
// (Interval_rep_t is what we want; long long int is possibly longer.)
static interval_rep_t llround (double x) {
  interval_rep_t ret;
  if (x < 0)
    ret = x - .5;
  else
    ret = x + .5;
  return ret;
}
#endif


Station::Station (const Dstr &name_,
                  const StationRef &stationRef,
                  const ConstituentSet &constituents,
                  const Dstr &note_,
                  CurrentBearing minCurrentBearing_,
                  CurrentBearing maxCurrentBearing_,
                  const MetaFields &metadata):
  name(name_),
  coordinates(stationRef.coordinates),
  timezone(stationRef.timezone),
  minCurrentBearing(minCurrentBearing_),
  maxCurrentBearing(maxCurrentBearing_),
  note(note_),
  isCurrent(Units::isCurrent(constituents.predictUnits())),
  aspect(Global::settings["ga"].d),
  step(Global::hour),
  _stationRef(stationRef),
  _constituents(constituents),
  _metadata(metadata) {}


Station::~Station() {}


Station * const Station::clone() const {
  return new Station (*this);
}


Station * const Station::reload() const {
  Station *s = _stationRef.load();
  s->markLevel = markLevel;
  if (!markLevel.isNull())
    if (markLevel.Units() != s->predictUnits())
      s->markLevel.Units (s->predictUnits());
  s->step = step;
  return s;
}


const PredictionValue Station::minLevel() const {
  return _constituents.datum() - _constituents.maxAmplitude();
}


const PredictionValue Station::maxLevel() const {
  return _constituents.datum() + _constituents.maxAmplitude();
}


const bool Station::isSubordinateStation() {
  return false;
}


const bool Station::haveFloodBegins() {
  return true;
}


const bool Station::haveEbbBegins() {
  return true;
}


// The following block of methods is slightly revised from the code
// delivered by Geoffrey T. Dairiki for XTide 1.  Jeff's original
// comments (modulo a few global replacements) are shown in C-style,
// while mine are in C++ style.  As usual, see also Station.hh.

/*************************************************************************
 *
 * Geoffrey T. Dairiki Fri Jul 19 15:44:21 PDT 1996
 *
 ************************************************************************/



/*
 *   We are guaranteed to find all high and low tides as long as their
 *   spacing is greater than Global::eventPrecision.
 */


const PredictionValue Station::maxMinZeroFn (Timestamp t,
					     unsigned deriv,
                                     PredictionValue marklev unusedParameter) {
  return _constituents.tideDerivative (t, deriv+1);
}


const PredictionValue Station::markZeroFn (Timestamp t,
					   unsigned deriv,
					   PredictionValue marklev) {
  PredictionValue pv_out = _constituents.tideDerivative (t, deriv);
  if (deriv == 0)
    pv_out -= marklev;
  return pv_out;
}


/* findZero (time_t t1, time_t t2, double (*f)(time_t t, int deriv))
 *   Find a zero of the function f, which is bracketed by t1 and t2.
 *   Returns a value which is either an exact zero of f, or slightly
 *   past the zero of f.
 */

/*
 * Here's a root finder based upon a modified Newton-Raphson method.
 */

const Timestamp Station::findZero (Timestamp tl,
				   Timestamp tr,
                 const PredictionValue (Station::*f) (Timestamp t,
						      unsigned deriv,
                                                      PredictionValue marklev),
				   PredictionValue marklev)  {
  PredictionValue fl = (this->*f)(tl, 0, marklev);
  PredictionValue fr = (this->*f)(tr, 0, marklev);
  double scale = 1.0;
  Interval dt;
  Timestamp t;
  PredictionValue fp, ft, f_thresh;

  assert (fl.val() != 0.0 && fr.val() != 0.0);
  assert (tl < tr);
  if (fl.val() > 0) {
    scale = -1.0;
    fl = -fl;
    fr = -fr;
  }
  assert (fl.val() < 0.0 && fr.val() > 0.0);

  while (tr - tl > Global::eventPrecision) {
    if (t.isNull())
      dt = Global::zeroInterval; // Force bisection on first step
    else if (abs(ft) > f_thresh   /* not decreasing fast enough */
        || (ft.val() > 0.0 ?    /* newton step would go outside bracket */
            (fp <=  ft / (t - tl).s()) :
            (fp <= -ft / (tr - t).s())))
      dt = Global::zeroInterval; /* Force bisection */
    else {
      /* Attempt a newton step */
      assert (fp.val() != 0.0);
      // Here I actually do want to round away from zero.
      dt = Interval(llround(-ft/fp));

      /* Since our goal specifically is to reduce our bracket size
         as quickly as possible (rather than getting as close to
         the zero as possible) we should ensure that we don't take
         steps which are too small.  (We'd much rather step over
         the root than take a series of steps that approach the
         root rapidly but from only one side.) */
      if (abs(dt) < Global::eventPrecision)
        dt = (ft.val() < 0.0 ? Global::eventPrecision
 	                     : -Global::eventPrecision);

      t += dt;
      if (t >= tr || t <= tl)
	dt = Global::zeroInterval;  /* Force bisection if outside bracket */
      f_thresh = abs(ft) / 2.0;
    }

    if (dt == Global::zeroInterval) {
      /* Newton step failed, do bisection */
      t = tl + (tr - tl) / 2;
      f_thresh = fr > -fl ? fr : -fl;
    }

    if ((ft = scale * (this->*f)(t,0,marklev)).val() == 0.0)
        return t;             /* Exact zero */
    else if (ft.val() > 0.0)
        tr = t, fr = ft;
    else
        tl = t, fl = ft;

    fp = scale * (this->*f)(t,1,marklev);
  }

  return tr;
}


/* next_zero(time_t t, double (*f)(), double max_fp, double max_fpp)
 *   Find the next zero of the function f which occurs after time t.
 *   The arguments max_fp and max_fpp give the maximum possible magnitudes
 *   that the first and second derivative of f can achieve.
 *
 *   Algorithm:  Our goal here is to bracket the next zero of f ---
 *     then we can use findZero() to quickly refine the root.
 *     So, we will step forward in time until the sign of f changes,
 *     at which point we know we have bracketed a root.
 *     The trick is to use large steps in our search, making
 *     sure the steps are not so large that we inadvertently
 *     step over more than one root.
 *
 *     The big trick, is that since the tides (and derivatives of
 *     the tides) are all just harmonic series's, it is easy to place
 *     absolute bounds on their values.
 */

// This method is only used in one place and is only used for finding
// maxima and minima, so I renamed it to nextMaxMin, got rid of the
// max_fp and max_fpp parameters, and installed a more convenient out
// parameter.

// Since by definition the tide cannot change direction between maxima
// and minima, there is at most one crossing of a given mark level
// between min/max points.  Therefore, we already have a bracket of
// the mark level to give to findZero, and there is no need for a
// function like this to find the next mark crossing.

void Station::nextMaxMin (Timestamp t, TideEvent &tideEvent_out) {

  const Amplitude max_fp (_constituents.tideDerivativeMax(2));
  const Amplitude max_fpp (_constituents.tideDerivativeMax(3));

  Timestamp t_left, t_right;
  Interval step, step1, step2;
  PredictionValue f_left, df_left, f_right, df_right, junk;
  double scale = 1.0;

  t_left = t;

  /* If we start at a zero, step forward until we're past it. */
  while ((f_left = maxMinZeroFn(t_left,0,junk)).val() == 0.0)
    t_left += Global::eventPrecision;

  if (f_left.val() < 0.0) {
    tideEvent_out.eventType = TideEvent::min;
  } else {
    tideEvent_out.eventType = TideEvent::max;
    scale = -1.0;
    f_left = -f_left;
  }

  while (true) {

    /* Minimum time to next zero: */
    step1 = Interval((interval_rep_t)(abs(f_left) / max_fp));

    /* Minimum time to next turning point: */
    df_left = scale * maxMinZeroFn(t_left,1,junk);
    step2 = Interval((interval_rep_t)(abs(df_left) / max_fpp));

    if (df_left.val() < 0.0)
      /* Derivative is in the wrong direction. */
      step = step1 + step2;
    else
      step = step1 > step2 ? step1 : step2;

    if (step < Global::eventPrecision)
	step = Global::eventPrecision; /* No ridiculously small steps */

    t_right = t_left + step;

    /*
     * If we hit upon an exact zero, step right until we're off the
     * zero.  If the sign has changed, we are bracketing a desired
     * root.  If the sign hasn't changed, then the zero was at an
     * inflection point (i.e. a double-zero to within
     * Global::eventPrecision) and we want to ignore it.
     */
    while ((f_right = scale * maxMinZeroFn(t_right,0,junk)).val() == 0.0)
      t_right += Global::eventPrecision;

    if (f_right.val() > 0.0) {  /* Found a bracket */
      tideEvent_out.eventTime = findZero (t_left,
                                          t_right,
                                          &Station::maxMinZeroFn,
                                          junk);
      return;
    }

    t_left = t_right, f_left = f_right;
  }
}


const Timestamp Station::findMarkCrossing_Dairiki (Timestamp t1,
						   Timestamp t2,
						   PredictionValue marklev,
						   bool &isRising_out) {
  if (t1 > t2)
    std::swap (t1, t2);

  PredictionValue f1 (markZeroFn(t1,0,marklev));
  PredictionValue f2 (markZeroFn(t2,0,marklev));

  // Fail gently on rotten brackets.  (This used to be an assertion.)
  if (f1 == f2)
    return Timestamp(); // return null timestamp

  // We need || instead of && to set isRising_out correctly in the
  // case where there's a zero exactly at t1 or t2.
  if (!(isRising_out = (f1.val() < 0.0 || f2.val() > 0.0))) {
     f1 = -f1;
     f2 = -f2;
  }

  // Since f1 != f2, we can't get two zeros, so it doesn't matter which
  // one we check first.
  if (f1.val() == 0.0)
    return t1;
  else if (f2.val() == 0.0)
    return t2;

  if (f1.val() < 0.0 && f2.val() > 0.0)
    return findZero (t1, t2, &Station::markZeroFn, marklev);

  return Timestamp(); // Don't have a bracket, return null timestamp.
}


/*************************************************************************/


const Timestamp Station::findSimpleMarkCrossing (Timestamp t1,
						 Timestamp t2,
						 PredictionValue marklev,
						 bool &isRising_out) {

  // marklev must compensate for datum and KnotsSquared.  See markZeroFn.
  // Units should already be comparable to datum.
  marklev -= _constituents.datum();
  // Correct knots / knots squared
  if (_constituents.predictUnits() != marklev.Units())
    marklev.Units (_constituents.predictUnits());

  return findMarkCrossing_Dairiki (t1, t2, marklev, isRising_out);
}


void Station::predictTideEvents (Timestamp startTime,
                                 Timestamp endTime,
                                 TideEventsOrganizer &organizer,
                                 TideEventsFilter filter) {
  assert (Global::eventPrecision > Global::zeroInterval);
  if (startTime >= endTime)
    return;

  // calculate the time for which we need to add events
  TideEventsConstIterator itr_first = organizer.begin();
  TideEventsReverseIterator itr_last = organizer.rbegin();

  typedef std::pair<Timestamp, Timestamp> TimeWindow; 
  std::vector<TimeWindow> vectTimeWindows;

  // FIXME if it is a great distance
  // much greater than end - start
  // clear the organizer first?

  if (organizer.end() != itr_first && organizer.rend() != itr_last)
  {
	  if (itr_first->second.eventTime <= startTime)
	  {
		  if (itr_last->second.eventTime < endTime)
		  {
			  startTime = itr_last->second.eventTime;
			  vectTimeWindows.push_back(TimeWindow(startTime, endTime));
		  }
	  }
	  else
	  {
		  if (endTime <= itr_last->second.eventTime)
		  {
			  endTime = itr_first->second.eventTime;
			  vectTimeWindows.push_back(TimeWindow(startTime, endTime));
		  }
		  else
		  {
			  vectTimeWindows.push_back(TimeWindow(startTime, itr_first->second.eventTime));
			  vectTimeWindows.push_back(TimeWindow(itr_last->second.eventTime, endTime));
		  }
	  }
  }
  else
  {
	  vectTimeWindows.push_back(TimeWindow(startTime, endTime));
  }

  std::vector<TimeWindow>::iterator itr;

  for (itr = vectTimeWindows.begin();
		  vectTimeWindows.end() != itr; ++itr)
  {
	  addSimpleTideEvents (itr->first, itr->second, organizer, filter);

	  if (filter == noFilter)
		addSunMoonEvents (itr->first, itr->second, organizer);
  }
}


void Station::addSimpleTideEvents (Timestamp startTime,
                                   Timestamp endTime,
                                   TideEventsOrganizer &organizer,
                                   TideEventsFilter filter) {
  bool isRising;
  TideEvent te;

  // loopTime is the "internal" timestamp for scanning the reference
  // station.  The timestamps of each event get mangled for sub
  // stations.
  Timestamp loopTime (startTime - maximumTimeOffset);
  Timestamp loopEndTime (endTime - minimumTimeOffset);

  // Patience... range is correctly enforced below.
  while (loopTime <= loopEndTime) {
    Timestamp previousLoopTime (loopTime);

    // Get next max or min.
    nextMaxMin (loopTime, te);
    loopTime = te.eventTime;
    finishTideEvent (te);
    if (te.eventTime >= startTime && te.eventTime < endTime)
      organizer.add (te);

    // Check for slacks, if applicable.  Skip the ones that need
    // interpolation; those are done in
    // SubordinateStation::addInterpolatedSubstationMarkCrossingEvents.
    if (filter != maxMin && isCurrent &&
        ((te.eventType == TideEvent::max && haveFloodBegins()) ||
         (te.eventType == TideEvent::min && haveEbbBegins()))) {
      te.eventTime = findSimpleMarkCrossing (previousLoopTime,
                                             loopTime,
                                          PredictionValue(predictUnits(), 0.0),
                                             isRising);
      if (!(te.eventTime.isNull())) {
	te.eventType = (isRising ? TideEvent::slackrise
 			         : TideEvent::slackfall);
	finishTideEvent (te);
	if (te.eventTime >= startTime && te.eventTime < endTime)
	  organizer.add (te);
      }
    }

    // Check for mark, if applicable.
    if ((!isSubordinateStation()) &&
        (!markLevel.isNull()) &&
        (filter == noFilter)) {
      te.eventTime = findSimpleMarkCrossing (previousLoopTime,
                                             loopTime,
                                             markLevel,
                                             isRising);
      if (!(te.eventTime.isNull())) {
	te.eventType = (isRising ? TideEvent::markrise
			         : TideEvent::markfall);
	finishTideEvent (te);
	if (te.eventTime >= startTime && te.eventTime < endTime)
	  organizer.add (te);
      }
    }
  }
}


// Submethod of predictTideEvents.
void Station::addSunMoonEvents (Timestamp startTime,
                                Timestamp endTime,
				TideEventsOrganizer &organizer) {
  TideEvent te;

  const Dstr &em = Global::settings["em"].s;

  if (!(coordinates.isNull())) {

    bool S (em.strchr('S') != -1);
    bool s (em.strchr('s') != -1);
    bool M (em.strchr('M') != -1);
    bool m (em.strchr('m') != -1);
    bool T (em.strchr('T') != -1);
    bool t (em.strchr('t') != -1);

    // Add twilight
    if (!(S && s)) {    
      te.eventTime = startTime;
      Skycal::findNextRiseOrSet(te.eventTime, 
                                coordinates,
                                Skycal::twilight,
                                te);
      while (te.eventTime < endTime) {
        if ((te.eventType == TideEvent::dawn && !T) ||
          (te.eventType == TideEvent::dusk  && !t)) {
          finishTideEvent (te);
          organizer.add (te);
        }
        Skycal::findNextRiseOrSet(te.eventTime, 
                                  coordinates,
                                  Skycal::twilight,
                                  te);
      }
    }

    // Add sunrises and sunsets.
    if (!(S && s)) {
      te.eventTime = startTime;
      Skycal::findNextRiseOrSet (te.eventTime,
                                 coordinates,
                                 Skycal::solar,
                                 te);
      while (te.eventTime < endTime) {
	if ((te.eventType == TideEvent::sunrise && !S) ||
	    (te.eventType == TideEvent::sunset  && !s)) {
  	  finishTideEvent (te);
	  organizer.add (te);
        }
	Skycal::findNextRiseOrSet (te.eventTime,
                                   coordinates,
                                   Skycal::solar,
                                   te);
      }
    }

    // Add moonrises and moonsets.
    if (startTime.inRangeForLunarRiseSet() && (!(M && m))) {
      te.eventTime = startTime;
      Skycal::findNextRiseOrSet (te.eventTime,
                                 coordinates,
                                 Skycal::lunar,
                                 te);
      while (te.eventTime < endTime) {
        if ((te.eventType == TideEvent::moonrise && !M) ||
            (te.eventType == TideEvent::moonset  && !m)) {
  	  finishTideEvent (te);
	  organizer.add (te);
        }
	if (!(te.eventTime.inRangeForLunarRiseSet()))
	  break;
	Skycal::findNextRiseOrSet (te.eventTime,
                                   coordinates,
                                   Skycal::lunar,
                                   te);
      }
    }
  }

  // Add moon phases.
  if (em.strchr('p') == -1) {
    te.eventTime = startTime;
    Skycal::findNextMoonPhase (te.eventTime, te);
    while (te.eventTime < endTime) {
      finishTideEvent (te);
      organizer.add (te);
      Skycal::findNextMoonPhase (te.eventTime, te);
    }
  }
}


// Analogous to predictTideEvents for raw readings.
void Station::predictRawEvents (Timestamp startTime,
                                Timestamp endTime,
                                TideEventsOrganizer &organizer) {
  assert (step > Global::zeroInterval);
  assert (startTime <= endTime);
  TideEvent te;
  te.eventType = TideEvent::rawreading;
  for (Timestamp t = startTime; t < endTime; t += step) {
    te.eventTime = t;
    finishTideEvent (te);
    organizer.add (te);
  }
}


void Station::extendRange (TideEventsOrganizer &organizer,
                           Direction direction,
                           Interval howMuch,
                           TideEventsFilter filter) {
  assert (howMuch > Global::zeroInterval);
  Timestamp startTime, endTime;
  if (direction == forward) {
    TideEventsReverseIterator it = organizer.rbegin();
    assert (it != organizer.rend());
    startTime = it->second.eventTime;
    endTime = startTime + howMuch;
    startTime -= Global::eventSafetyMargin;
  } else {
    TideEventsConstIterator it = organizer.begin();
    assert (it != organizer.end());
    endTime = it->second.eventTime;
    startTime = endTime - howMuch;
    endTime += Global::eventSafetyMargin;
  }
  predictTideEvents (startTime, endTime, organizer, filter);
}


void Station::extendRange (TideEventsOrganizer &organizer,
                           Direction direction,
                           unsigned howMany) {
  assert (howMany);
  assert (step > Global::zeroInterval);
  Timestamp startTime, endTime;
  if (direction == forward) {
    TideEventsReverseIterator it = organizer.rbegin();
    assert (it != organizer.rend());
    startTime = it->second.eventTime + step;
    endTime = startTime + step * howMany;
  } else {
    TideEventsConstIterator it = organizer.begin();
    assert (it != organizer.end());
    endTime = it->second.eventTime;
    startTime = endTime - step * howMany;
  }
  predictRawEvents (startTime, endTime, organizer);
}


const Dstr& Station::getMetaValue(const Dstr& name) {
  return _metadata[name];
}


const PredictionValue Station::finishPredictionValue (PredictionValue pv) {
  if (Units::isHydraulicCurrent (pv.Units()))
    pv.Units (Units::flatten (pv.Units()));
  pv += _constituents.datum();
  return pv;
}


const PredictionValue Station::predictTideLevel (Timestamp predictTime) {
  return finishPredictionValue (_constituents.tideDerivative (predictTime, 0));
}


#ifdef blendingTest
void Station::tideLevelBlendValues (Timestamp predictTime,
				    NullablePredictionValue &firstYear_out,
				    NullablePredictionValue &secondYear_out) {
  assert (!isSubordinateStation());
  _constituents.tideDerivativeBlendValues (predictTime,
					   0,
					   firstYear_out,
					   secondYear_out);
  if (!firstYear_out.isNull())
    firstYear_out = finishPredictionValue (firstYear_out);
  if (!secondYear_out.isNull())
    secondYear_out = finishPredictionValue (secondYear_out);
}
#endif


const Units::PredictionUnits Station::predictUnits () const {
  return Units::flatten (_constituents.predictUnits());
}


void Station::setUnits (Units::PredictionUnits units) {
  if (!isCurrent) {
    _constituents.setUnits (units);
    if (!markLevel.isNull())
      if (markLevel.Units() != units)
	markLevel.Units (units);
  }
}


void Station::aboutMode (Dstr &text_out,
			 Format::Format form,
			 const Dstr &codeset) const {
  unsigned maximumNameLength = 0;
  assert (form == Format::text || form == Format::HTML);
  if (form == Format::HTML)
    text_out = "<table>\n";
  else {
    for (int i = 0; i < _metadata.size(); ++i) {
      int len = _metadata[i].length();
      if (len > maximumNameLength)
        maximumNameLength = len;
    }
  }
  for (int i = 0; i < _metadata.size(); ++i) {
    if (form == Format::HTML) {
      text_out += "<tr><td valign=top>";
      text_out += _metadata[i];
      text_out += "</td><td valign=top><pre>";
      text_out += _metadata[_metadata[i]];
      text_out += "</pre></td></td>\n";
    } else {
      Dstr tmp1 (_metadata[i]), tmp2 (_metadata[_metadata[i]]), tmp3;
      tmp1.pad (maximumNameLength+2);
      tmp2.getline (tmp3);
      tmp1 += tmp3;
      tmp1 += '\n';
      while (tmp2.length()) {
        tmp3 = "";
        tmp3.pad (maximumNameLength+2);
        tmp1 += tmp3;
        tmp2.getline (tmp3);
        tmp1 += tmp3;
        tmp1 += '\n';
      }
      text_out += tmp1;
    }
  }
  if (form == Format::HTML)
    text_out += "</table>\n";
  if (codeset == "UTF-8")
    text_out.utf8();
}


void Station::finishTideEvent (TideEvent &te) {
  te.isCurrent = isCurrent;
  te.uncorrectedEventTime.makeNull();
  te.uncorrectedEventLevel.makeNull();
  if (te.isSunMoonEvent())
    te.eventLevel.makeNull();
  else
    te.eventLevel = predictTideLevel (te.eventTime);
}


// Legal forms are c, h, i, l, or t, but c and l do nothing.
void Station::textBoilerplate (Dstr &text_out, Format::Format form) const {
  text_out = (char *)NULL;
  if (form == Format::CSV || form == Format::LaTeX)
    return;
  assert (form == Format::HTML ||
          form == Format::iCalendar ||
          form == Format::text);

  if (form == Format::iCalendar) {

    // RFC2445 doesn't allow putting very much outside of the VEVENTs.
    // This makes sense considering that a calendaring tool is only
    // equipped to display metadata corresponding to specific events.

    // RFC2445 does clearly specify CRLF (CRNL) line discipline.

    text_out += "BEGIN:VCALENDAR\r\n\
VERSION:2.0\r\n\
PRODID:";
    // ISO 9070 compliance not mandatory.
    Dstr ver;
    Global::versionString (ver);
    text_out += ver;
    text_out += "\r\n\
CALSCALE:GREGORIAN\r\n\
METHOD:PUBLISH\r\n";
  } else {

    if (form == Format::HTML)
      text_out += "<h3>";
    text_out += name;
    if (form == Format::HTML)
      text_out += "<br>";
    text_out += '\n';
    if (coordinates.isNull())
      text_out += "Coordinates unknown\n";
    else {
      Dstr t;
      coordinates.print (t);
      text_out += t;
      text_out += '\n';
    }

    // When known, append the direction of currents.  (The offending
    // attributes should be null if it's not a current station.)
    if (!(maxCurrentBearing.isNull())) {
      if (form == Format::HTML)
	text_out += "<br>";
      text_out += "Flood direction ";
      Dstr tmpbuf;
      maxCurrentBearing.print (tmpbuf);
      text_out += tmpbuf;
      text_out += '\n';
    }
    if (!(minCurrentBearing.isNull())) {
      if (form == Format::HTML)
	text_out += "<br>";
      text_out += "Ebb direction ";
      Dstr tmpbuf;
      minCurrentBearing.print (tmpbuf);
      text_out += tmpbuf;
      text_out += '\n';
    }

    // Similarly for notes
    if (!(note.isNull())) {
      if (form == Format::HTML)
	text_out += "<br>Note:&nbsp; ";
      else
	text_out += "Note:  ";
      text_out += note;
      text_out += '\n';
    }

    if (form == Format::HTML)
      text_out += "</h3>";
    text_out += '\n';

    if (Global::codeset == "UTF-8")
      text_out.utf8();
  }
}


// iCalendar format output is actually produced by plainMode.  From
// an engineering perspective this makes perfect sense.  But from a
// usability perspective, iCalendar output is a calendar and ought
// to appear in calendar mode.  So calendarMode falls through to
// plainMode when i format is chosen.

void Station::plainMode (Dstr &text_out,
                         Timestamp startTime,
                         Timestamp endTime,
			 Format::Format form) {
  textBoilerplate (text_out, form);
  TideEventsOrganizer organizer;
  predictTideEvents (startTime, endTime, organizer);
  TideEventsConstIterator it = organizer.begin();
  while (it != organizer.end()) {
    Dstr line;
    it->second.print (line, Mode::plain, form, *this);
    text_out += line;
    text_out += '\n';
    ++it;
  }
  if (form == Format::iCalendar)
    text_out += "END:VCALENDAR\r\n";
}


void Station::statsMode (Dstr &text_out,
                         Timestamp startTime,
                         Timestamp endTime) {

  textBoilerplate (text_out, Format::text);
  PredictionValue maxl = maxLevel();
  PredictionValue minl = minLevel();
  assert (minl < maxl);
  PredictionValue meanl = (maxl + minl) / 2.0;
  Dstr temp;
  text_out += "Mathematical upper bound: ";
  maxl.print (temp);
  text_out += temp;
  text_out += "\nMathematical lower bound: ";
  minl.print (temp);
  text_out += temp;
  text_out += "\nMathematical mean, assuming symmetry: ";
  meanl.print (temp);
  text_out += temp;
  text_out += "\n\n";

  bool first (true);
  TideEventsOrganizer organizer;
  predictTideEvents (startTime, endTime, organizer, maxMin);
  Timestamp maxt, mint, lastTidalDay (startTime);
  PredictionValue sumLevels, sumLLW;
  NullablePredictionValue LLW;
  unsigned long numberOfSamples (0), numberOfMLLWSamples (0);

  for (TideEventsConstIterator it = organizer.begin();
       it != organizer.end();
       ++it) {
    const TideEvent &te = it->second;
    assert (!te.isSunMoonEvent());

    if (!isCurrent) {
      // MLLW estimation uses the lowest low tide in each tidal day.
      while (te.eventTime - lastTidalDay >= Global::tidalDay) {
	if (!LLW.isNull()) {
	  sumLLW += LLW;
	  ++numberOfMLLWSamples;
          LLW.makeNull();
	}
        lastTidalDay += Global::tidalDay;
      }
      if (te.eventType == TideEvent::min) {
	if (LLW.isNull())
	  LLW = te.eventLevel;
	else if (te.eventLevel < LLW)
	  LLW = te.eventLevel;
      }
    }

    sumLevels += te.eventLevel;
    ++numberOfSamples;
    if (first || (te.eventLevel < minl)) {
      mint = te.eventTime;
      minl = te.eventLevel;
    }
    if (first || (te.eventLevel > maxl)) {
      maxt = te.eventTime;
      maxl = te.eventLevel;
    }
    first = false;
  }
  if (!isCurrent)
    if (endTime - lastTidalDay >= Global::tidalDay && !LLW.isNull()) {
      sumLLW += LLW;
      ++numberOfMLLWSamples;
    }

  text_out += "Searched interval from ";
  startTime.print (temp, timezone);
  text_out += temp;
  text_out += " to ";
  endTime.print (temp, timezone);
  text_out += temp;
  text_out += "\n";
  if (!first) {
    text_out += "Maximum was ";
    maxl.print (temp);
    text_out += temp;
    text_out += " at ";
    maxt.print (temp, timezone);
    text_out += temp;
    text_out += '\n';

    text_out += "Minimum was ";
    minl.print (temp);
    text_out += temp;
    text_out += " at ";
    mint.print (temp, timezone);
    text_out += temp;
    text_out += '\n';

    sumLevels /= numberOfSamples;
    text_out += "Mean of maxima and minima was ";
    sumLevels.print (temp);
    text_out += temp;
    text_out += '\n';

    if (!isCurrent) {
      if (numberOfMLLWSamples) {
	sumLLW /= numberOfMLLWSamples;
	text_out += "Estimated MLLW: ";
	sumLLW.print (temp);
	text_out += temp;
	text_out += '\n';
      } else
	text_out += "Insufficient data to estimate MLLW.\n";
    }
  } else {
    text_out += "Found no tide events.\n";
  }

#ifdef HAVE_SYS_RESOURCE_H
  rusage r;
  require (getrusage (RUSAGE_SELF, &r) == 0);
  text_out += "\nCPU time used:  ";
  text_out += r.ru_utime.tv_sec + r.ru_utime.tv_usec / 1000000.0;
  text_out += " s\n";
#endif
}


void Station::calendarMode (Dstr &text_out,
                            Timestamp startTime,
                            Timestamp endTime,
			    Mode::Mode mode,
                            Format::Format form) {
  assert (mode == Mode::calendar || mode == Mode::altCalendar);
  assert ((form == Format::CSV && mode == Mode::calendar) ||
          form == Format::HTML ||
          form == Format::iCalendar ||
          form == Format::LaTeX ||
          form == Format::text);

  if (form == Format::iCalendar)
    plainMode (text_out, startTime, endTime, form);
  else {
    textBoilerplate (text_out, form);
    std::auto_ptr<Calendar> cal (Calendar::factory (*this,
						    startTime,
						    endTime,
						    mode,
						    form));
    Dstr temp;
    cal->print (temp);
    text_out += temp;
  }
}


void Station::rareModes (Dstr &text_out,
                         Timestamp startTime,
                         Timestamp endTime,
                         Mode::Mode mode,
                         Format::Format form) {
  assert (form == Format::text || form == Format::CSV);
  assert (mode == Mode::raw || mode == Mode::mediumRare);
  text_out = (char *)NULL;

  TideEventsOrganizer organizer;
  predictRawEvents (startTime, endTime, organizer);
  TideEventsConstIterator it = organizer.begin();
  while (it != organizer.end()) {
    Dstr line;
    it->second.print (line, mode, form, *this);
    text_out += line;
    text_out += '\n';
    ++it;
  }
}


void Station::bannerMode (Dstr &text_out,
                          Timestamp startTime,
                          Timestamp endTime) {
  textBoilerplate (text_out, Format::text);
  std::auto_ptr<Banner> banner (Banner::factory (*this,
						 Global::settings["tw"].u,
						 startTime,
						 endTime));
  Dstr temp;
  banner->drawTides (this, startTime);
  banner->print (temp);
  text_out += temp;
}


void Station::graphMode (Dstr &text_out,
                         Timestamp startTime) {
  TTYGraph g (Global::settings["tw"].u, Global::settings["th"].u);
  g.drawTides (this, startTime);
  g.print (text_out);
}


void Station::clockMode (Dstr &text_out) {
  TTYGraph g (Global::settings["tw"].u,
	      Global::settings["th"].u,
	      Graph::clock);
  g.drawTides (this, (time_t)time(NULL));
  g.print (text_out);
}


void Station::graphModePNG (FILE *fp, Timestamp startTime) {
  RGBGraph g (Global::settings["gw"].u, Global::settings["gh"].u);
  g.drawTides (this, startTime);
  Global::PNGFile = fp;
  g.writeAsPNG (Global::writePNGToFile);
}


void Station::clockModePNG (FILE *fp) {
  RGBGraph g (Global::settings["cw"].u,
	      Global::settings["gh"].u,
	      Graph::clock);
  g.drawTides (this, (time_t)time(NULL));
  Global::PNGFile = fp;
  g.writeAsPNG (Global::writePNGToFile);
}


void Station::print (Dstr &text_out,
                     Timestamp startTime,
                     Timestamp endTime,
		     Mode::Mode mode,
                     Format::Format form) {
  switch (mode) {
  case Mode::about:
    if (form != Format::text && form != Format::HTML)
      Global::formatBarf (mode, form);
    aboutMode (text_out, form, Global::codeset); // Timestamps are redundant
    break;

  case Mode::plain:
    if (form != Format::text && form != Format::CSV)
      Global::formatBarf (mode, form);
    plainMode (text_out, startTime, endTime, form);
    break;

  case Mode::stats:
    if (form != Format::text)
      Global::formatBarf (mode, form);
    statsMode (text_out, startTime, endTime);
    break;

  case Mode::calendar:
    switch (form) {
    case Format::CSV:
    case Format::HTML:
    case Format::iCalendar:
    case Format::LaTeX:
    case Format::text:
      calendarMode (text_out, startTime, endTime, mode, form);
      break;
    default:
      Global::formatBarf (mode, form);
    }
    break;

  case Mode::altCalendar:
    switch (form) {
    case Format::HTML:
    case Format::iCalendar:
    case Format::LaTeX:
    case Format::text:
      calendarMode (text_out, startTime, endTime, mode, form);
      break;
    default:
      Global::formatBarf (mode, form);
    }
    break;

  case Mode::raw:
  case Mode::mediumRare:
    if (form != Format::text && form != Format::CSV)
      Global::formatBarf (mode, form);
    rareModes (text_out, startTime, endTime, mode, form);
    break;

  case Mode::banner:
    if (form != Format::text)
      Global::formatBarf (mode, form);
    bannerMode (text_out, startTime, endTime);
    break;

  case Mode::graph:
    switch (form) {
    case Format::PNG:
      Global::log ("Can't happen:  Station::print called for graph mode, PNG form:  use graphModePNG instead\n", LOG_ERR);
      assert (false);
    case Format::text:
      graphMode (text_out, startTime);
      break;
    default:
      Global::formatBarf (mode, form);
    }
    break;

  case Mode::clock:
    switch (form) {
    case Format::PNG:
      Global::log ("Can't happen:  Station::print called for clock mode, PNG form:  use graphModePNG instead\n", LOG_ERR);
      assert (false);
    case Format::text:
      clockMode (text_out);
      break;
    default:
      Global::formatBarf (mode, form);
    }
    break;

  default:
    {
      Dstr details ("Unsupported mode: ");
      details += (char)mode;
      Global::barf (Error::BAD_MODE, details);
    }
  }
}

// Cleanup2006 Done
