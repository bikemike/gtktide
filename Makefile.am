# $Id: Makefile.am 2955 2008-01-25 23:17:01Z flaterco $

bin_PROGRAMS  = xtide tide
sbin_PROGRAMS = xttpd
dist_man_MANS = tide.1 xtide.1 xttpd.8
AM_CXXFLAGS   = @X_CFLAGS@
BUILT_SOURCES = xml_y.h
AM_YFLAGS     = -d
CLEANFILES    = INSTALL README ChangeLog NEWS AUTHORS README-QUICK
EXTRA_DIST    = CONFIGURE-HELP CodingConventions.txt README-QUICK \
		scripts/xtide.conf.sample \
		scripts/Fedora/rc.xttpd \
		scripts/Fedora/xttpd-wrapper.sh \
		scripts/Fedora/xttpd.conf \
		iconsrc/Palette.ppm \
		iconsrc/icon_16x16_3color.ico \
		iconsrc/icon_16x16_3color.png \
		iconsrc/icon_16x16_orig.png \
		iconsrc/icon_48x48_3color.png \
		iconsrc/icon_48x48_orig.png


COMMONSRCS = \
  Amplitude.cc Amplitude.hh Angle.cc Angle.hh Banner.cc Banner.hh        \
  BetterMap.hh Calendar.cc Calendar.hh CalendarFormC.cc CalendarFormC.hh \
  CalendarFormH.cc CalendarFormH.hh CalendarFormL.cc CalendarFormL.hh    \
  CalendarFormNotC.cc CalendarFormNotC.hh CalendarFormT.cc               \
  CalendarFormT.hh Colors.cc Colors.hh Configurable.hh Constituent.cc    \
  Constituent.hh ConstituentSet.cc ConstituentSet.hh Coordinates.cc      \
  Coordinates.hh CurrentBearing.cc CurrentBearing.hh Date.cc Date.hh     \
  Errors.hh Global.cc Global.hh Graph.cc Graph.hh HarmonicsFile.cc       \
  HarmonicsFile.hh HarmonicsPath.cc HarmonicsPath.hh Interval.cc         \
  Interval.hh MetaField.cc MetaField.hh ModeFormat.hh Nullable.cc        \
  Nullable.hh NullableInterval.cc NullableInterval.hh                    \
  NullablePredictionValue.cc NullablePredictionValue.hh Offsets.cc       \
  Offsets.hh PredictionValue.cc PredictionValue.hh RGBGraph.cc           \
  RGBGraph.hh SafeVector.hh Settings.cc Settings.hh Skycal.cc            \
  Skycal.hh Speed.cc Speed.hh Station.cc Station.hh StationIndex.cc      \
  StationIndex.hh StationRef.cc StationRef.hh SubordinateStation.cc      \
  SubordinateStation.hh TTYGraph.cc TTYGraph.hh TideEvent.cc             \
  TideEvent.hh TideEventsOrganizer.cc TideEventsOrganizer.hh             \
  Timestamp.cc Timestamp.hh Units.cc Units.hh Year.cc Year.hh            \
  common.hh config.hh glyphs.hh rgb.hh xml_l.ll xml_y.yy xmlparser.hh

if NO_LIBDSTR
COMMONSRCS += Dstr.cc Dstr.hh
endif


xtide_SOURCES = $(COMMONSRCS) \
  errorExclaim.xpm.hh errorSunk.xpm.hh icon_48x48_3color.xpm.hh        \
  wvsrtv.cc xtide.cc xtide.hh xxAspect.cc xxAspect.hh xxClock.cc       \
  xxClock.hh xxDisclaimer.cc xxDisclaimer.hh xxErrorBox.cc             \
  xxErrorBox.hh xxFilename.cc xxFilename.hh xxGlobal.cc xxGlobal.hh    \
  xxGlobe.cc xxGlobe.hh xxGraphMode.cc xxGraphMode.hh xxHelpBox.cc     \
  xxHelpBox.hh xxHorizDialog.cc xxHorizDialog.hh xxLocationList.cc     \
  xxLocationList.hh xxMap.cc xxMap.hh xxMarkLevel.cc xxMarkLevel.hh    \
  xxMouseWheelViewport.cc xxMouseWheelViewport.hh xxMultiChoice.cc     \
  xxMultiChoice.hh xxPixmapGraph.cc xxPixmapGraph.hh                   \
  xxPredictionWindow.cc xxPredictionWindow.hh xxRareModeSavePrompts.cc \
  xxRareModeSavePrompts.hh xxRedrawable.cc xxRedrawable.hh xxRoot.cc   \
  xxRoot.hh xxSimplePrompt.cc xxSimplePrompt.hh xxStep.cc xxStep.hh    \
  xxTextMode.cc xxTextMode.hh xxTimestamp.cc xxTimestamp.hh	       \
  xxTimestampDialog.cc xxTimestampDialog.hh xxUnsignedChooser.cc       \
  xxUnsignedChooser.hh xxWidget.cc xxWidget.hh xxWindow.cc xxWindow.hh \
  xxX.cc xxX.hh

xtide_LDFLAGS = @X_LIBS@
xtide_LDADD = -lXaw -lXmu -lXt -lSM -lICE -lXext -lX11 -lXpm @X_EXTRA_LIBS@


tide_SOURCES =  $(COMMONSRCS) tide.cc


xttpd_SOURCES = $(COMMONSRCS) ZoneIndex.cc ZoneIndex.hh xttpd.cc
# xttpd needs -lsocket -lnsl on Solaris too.
xttpd_LDADD = @X_EXTRA_LIBS@


xmlclean:
	rm -f xml_l.cc xml_y.cc xml_y.h


docs:   INSTALL README ChangeLog NEWS AUTHORS README-QUICK
.PHONY: INSTALL README ChangeLog NEWS AUTHORS README-QUICK docs


# "Generic" INSTALL moved to CONFIGURE-HELP so that something more helpful
# can go here.
INSTALL:
	lynx -dump "http://www.flaterco.com/xtide/installation.html" > INSTALL


README:
	lynx -dump "http://www.flaterco.com/xtide/xtide.html" > README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/disclaimer.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/introduction.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/sysreq.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/installation.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/ports.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/modes.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/interactive.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/advanced.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/tty.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/xttpd.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/settings.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/harmonics.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/bugs.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/faq.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/design.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/bibliography.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/time_t.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/mincurrents.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/pound_to_fit.html" >> README
	echo "" >> README
	echo "################################################################" >> README
	echo "" >> README
	lynx -dump "http://www.flaterco.com/xtide/xtide1diff.html" >> README
	echo "" >> README


ChangeLog:
	lynx -dump "http://www.flaterco.com/xtide/changelog.html" > ChangeLog


NEWS:
	lynx -dump "http://www.flaterco.com/xtide/news.html" > NEWS


AUTHORS:
	lynx -dump "http://www.flaterco.com/xtide/credits.html" > AUTHORS


README-QUICK:
	lynx -dump "http://www.flaterco.com/xtide/quickinst.html" > README-QUICK


# Cleanup2006 Done
