// $Id: Station.hh 2946 2008-01-18 23:12:25Z flaterco $

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


class Station {
public:

  // Immutable attributes.
  const Dstr name;                          // = StationRef.name + legalese
  const Coordinates &coordinates;
  const Dstr &timezone;
  const CurrentBearing minCurrentBearing;   // Null if N/A
  const CurrentBearing maxCurrentBearing;   // Null if N/A
  const Dstr note;                          // Null if N/A
  const bool isCurrent;

  // Attributes that aren't intrinsic to the station, but must
  // transfer with it when it is copied.  Clients *are* entitled to
  // modify these values.  NOTE:  markLevel must be specified in
  // predictUnit() units.
  NullablePredictionValue markLevel;
  double aspect;              // Aspect for graphing.
  Interval step;              // Step size for rare modes.


  // See HarmonicsFile::getStation.
  Station (const Dstr &name_,
           const StationRef &stationRef,
	   const ConstituentSet &constituents,
           const Dstr &note_,
           CurrentBearing minCurrentBearing_,
           CurrentBearing maxCurrentBearing_,
           const MetaFields &metadata);

  virtual ~Station();


  // Default copy constructor will slice a SubordinateStation.  Use
  // this instead.
  virtual Station * const clone() const;

  // This method is only used when applying settings from the control
  // panel, and then only because toggling constituent inference
  // requires a reload.  Marklevel and step are preserved.  Aspect and
  // units are reset to defaults or whatever the new settings
  // specified.
  Station * const reload() const;


  // General method for generating output that fits into a Dstr (any
  // form except PNG or X-Windows).  Note that list mode is
  // implemented as StationIndex::print.
  void print (Dstr &text_out,
              Timestamp startTime,
              Timestamp endTime,
	      Mode::Mode mode,
              Format::Format form);

  // There is no reason not to access the modes directly if that is
  // more convenient, but these will die with assertion failures if
  // the selected form is invalid, whereas print will issue a helpful
  // error message to the user.
  void aboutMode (Dstr &text_out,
                  Format::Format form,
		  const Dstr &codeset) const;
  void plainMode (Dstr &text_out,
                  Timestamp startTime,
                  Timestamp endTime,
                  Format::Format form);
  void statsMode (Dstr &text_out,
                  Timestamp startTime,
                  Timestamp endTime);
  void calendarMode (Dstr &text_out,
                     Timestamp startTime,
                     Timestamp endTime,
                     Mode::Mode mode,
                     Format::Format form);
  void rareModes (Dstr &text_out,
                  Timestamp startTime,
                  Timestamp endTime,
                  Mode::Mode mode,
                  Format::Format form);
  void bannerMode (Dstr &text_out,
                   Timestamp startTime,
                   Timestamp endTime);
  void graphMode (Dstr &text_out,
                  Timestamp startTime);
  void clockMode (Dstr &text_out);

  // Special case for PNG graphs and clocks (binary out not text out).
  void graphModePNG (FILE *fp, Timestamp startTime);
  void clockModePNG (FILE *fp);


  // Change preferred units of length.
  // Default is as specified by settings.
  // Attempts to set same units are tolerated without complaint.
  // Attempts to set velocity units are punished.
  void setUnits (Units::PredictionUnits units);

  // Return units that prediction methods will return (never knots squared).
  const Units::PredictionUnits predictUnits () const;

  // Mathematical bounds (considerably wider than lowest/highest
  // astronomical tide)---used to set range in graphs.
  virtual const PredictionValue minLevel() const;
  virtual const PredictionValue maxLevel() const;

  // Get heights or velocities.
  virtual const PredictionValue predictTideLevel (Timestamp predictTime);

#ifdef blendingTest
  // For testing only.
  void tideLevelBlendValues (Timestamp predictTime,
			     NullablePredictionValue &firstYear_out,
			     NullablePredictionValue &secondYear_out);
#endif

  // Filters for predictTideEvents.
  // noFilter = maxes, mins, slacks, mark crossings, sun and moon
  // knownTideEvents = tide events that can be determined without interpolation
  //                   (maxes, mins, and sometimes slacks)
  // maxMin = maxes and mins
  enum TideEventsFilter {noFilter, knownTideEvents, maxMin};

  // Get all tide events within a range of timestamps and add them to
  // the organizer.  The range is >= startTime and < endTime.  Because
  // predictions are done to plus or minus one minute, invoking this
  // multiple times with adjoining ranges could duplicate or delete
  // tide events falling right on the boundary.  TideEventsOrganizer
  // should suppress the duplicates, but omissions will not be
  // detected.
  //
  // Either settings or the filter arg can suppress sun and moon events.
  virtual void predictTideEvents (Timestamp startTime,
                                  Timestamp endTime,
                                  TideEventsOrganizer &organizer,
                                  TideEventsFilter filter = noFilter);

  // Analogous, for raw readings.
  void predictRawEvents (Timestamp startTime,
                         Timestamp endTime,
                         TideEventsOrganizer &organizer);

  // Direction for extendRange.
  enum Direction {forward, backward};

  // Add events to an organizer to extend its range in the specified
  // direction by the specified interval.  (Number of events is
  // indeterminate.)  A safety margin is used to attempt to prevent
  // tide events from falling through the cracks as discussed above
  // predictTideEvents.
  //
  // Either settings or the filter arg can suppress sun and moon events.
  void extendRange (TideEventsOrganizer &organizer,
                    Direction direction,
		    Interval howMuch,
                    TideEventsFilter filter = noFilter);

  // Analogous, for raw readings.  Specify number of events in howmany.
  void extendRange (TideEventsOrganizer &organizer,
                    Direction direction,
                    unsigned howMany);


  const Dstr&  getMetaValue(const Dstr& name);

protected:

  const StationRef &    _stationRef;
  ConstituentSet        _constituents;
  const MetaFields    _metadata;


  // To get all tide events falling between t1 and t2, you have to
  // scan the interval from t1 - maximumTimeOffset to t2 - minimumTimeOffset.
  // These will remain zero for reference stations.
  Interval minimumTimeOffset; // Most negative, or least positive.
  Interval maximumTimeOffset; // Most positive, or least negative.

  virtual const bool isSubordinateStation();

  // These two return true if the offset is known OR is not needed
  // (i.e., it's a reference station or the offsets are simple).
  virtual const bool haveFloodBegins();
  virtual const bool haveEbbBegins();


  // G. Dairiki code, slightly revised.  See Station.cc for
  // more documentation.

  // Functions to zero out.
  // Option #1 -- find maxima and minima.  Marklev is unused (but
  // maxMinZeroFn and markZeroFn must have the same signature).
  const PredictionValue maxMinZeroFn (Timestamp t,
				      unsigned deriv,
				      PredictionValue marklev);

  // Option #2 -- find mark crossings or slack water.
  // ** Marklev must be made compatible with the tide as returned by
  // tideDerivative, i.e., no datum, no conversion from KnotsSquared.
  const PredictionValue markZeroFn   (Timestamp t,
				      unsigned deriv,
				      PredictionValue marklev);

  // Root finder.
  //   * If tl >= tr, assertion failure.
  //   * If tl and tr do not bracket a root, assertion failure.
  //   * If a root exists exactly at tl or tr, assertion failure.
  const Timestamp findZero (Timestamp tl,
			    Timestamp tr,
                 const PredictionValue (Station::*f) (Timestamp t,
		       				      unsigned deriv,
						      PredictionValue marklev),
			    PredictionValue marklev);

  // Find the marklev crossing in this bracket.  Used for both
  // markLevel and slacks.
  //   * Doesn't matter which of t1 and t2 is greater.
  //   * If t1 == t2, returns null.
  //   * If t1 and t2 do not bracket a mark crossing, returns null.
  //   * If mark crossing is exactly at t1 or t2, returns that.
  const Timestamp findMarkCrossing_Dairiki (Timestamp t1,
					    Timestamp t2,
					    PredictionValue marklev,
					    bool &isRising_out);

  // Find the next maximum or minimum.
  // eventTime and eventType are set to the next event (uncorrected time).
  // Nothing else in tideEvent_out is changed.
  // finishTideEvent must be called afterward.
  void nextMaxMin (Timestamp t, TideEvent &tideEvent_out);


  // Wrapper for findMarkCrossing_Dairiki that does necessary
  // compensations for datum, KnotsSquared, and units.  Used for both
  // markLevel and slacks.
  const Timestamp findSimpleMarkCrossing (Timestamp t1,
					  Timestamp t2,
					  PredictionValue marklev,
					  bool &isRising_out);

  // Submethods of predictTideEvents.
  void addSimpleTideEvents (Timestamp startTime,
                            Timestamp endTime,
                            TideEventsOrganizer &organizer,
                            TideEventsFilter filter);

  void addSunMoonEvents (Timestamp startTime,
                         Timestamp endTime,
                         TideEventsOrganizer &organizer);

  // Given eventTime and eventType, fill in other fields and possibly
  // apply corrections.
  virtual void finishTideEvent (TideEvent &te);

  // Given PredictionValue from ConstituentSet::tideDerivative, fix up
  // hydraulic current units and apply datum.
  const PredictionValue finishPredictionValue (PredictionValue pv);


  // Modes helper.
  void textBoilerplate (Dstr &text_out, Format::Format form) const;


private:
  // Prohibited operation not implemented.
  // (Default copy constructor is used inside of clone().)
  Station &operator= (const Station &);
};


// Meta Field Names
#define METAFIELD_NAME                   "Name"
#define METAFIELD_IN_FILE                "In file"
#define METAFIELD_LEGALESE               "Legalese"
#define METAFIELD_STATION_ID_CONTEXT     "Station ID context"
#define METAFIELD_STATION_ID             "Station ID"
#define METAFIELD_DATE_IMPORTED          "Date imported"
#define METAFIELD_COORDINATES            "Coordinates"
#define METAFIELD_COUNTRY                "Country"
#define METAFIELD_TIME_ZONE              "Time zone"
#define METAFIELD_NATIVE_UNITS           "Native units"
#define METAFIELD_FLOOD_DIRECTION        "Flood direction"
#define METAFIELD_EBB_DIRECTION          "Ebb direction"
#define METAFIELD_SOURCE                 "Source"
#define METAFIELD_RESTRICTION            "Restriction"
#define METAFIELD_COMMENTS               "Comments"
#define METAFIELD_NOTES                  "Notes"
#define METAFIELD_TYPE                   "Type"
#define METAFIELD_MERIDIAN               "Meridian"
#define METAFIELD_DATUM                  "Datum"
#define METAFIELD_MONTHS_ON_STATION      "Months on station"
#define METAFIELD_LAST_DATE_ON_STATION   "Last date on station"
#define METAFIELD_EXPIRATION             "Expiration"
#define METAFIELD_CONFIDENCE             "Confidence"
#define METAFIELD_REFERENCE              "Reference"
#define METAFIELD_MIN_TIME_ADD           "Min time add"
#define METAFIELD_MIN_LEVEL_ADD          "Min level add"
#define METAFIELD_MIN_LEVEL_MULT         "Min level mult"
#define METAFIELD_MAX_TIME_ADD           "Max time add"
#define METAFIELD_MAX_LEVEL_ADD          "Max level add"
#define METAFIELD_MAX_LEVEL_MULT         "Max level mult"
#define METAFIELD_FLOOD_BEGINS           "Flood begins"
#define METAFIELD_EBB_BEGINS             "Ebb begins"




// Cleanup2006 Done
