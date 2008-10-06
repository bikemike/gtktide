data_DATA = disclaimer.txt

harmonics_DATA = \
	harmonics/harmonics-free.tcd \
	harmonics/harmonics-nonfree.tcd


if MAEMO
desktopdir = ${datadir}/applications/hildon
servicedir = ${datadir}/dbus-1/services
service_DATA = org.yi.mike.gtktide.service

desktop_src_files = gtktide.desktop.in-maemo
gtktide.desktop.in: gtktide.desktop.in-maemo
	cp ${srcdir}/gtktide.desktop.in-maemo gtktide.desktop.in

else
desktopdir = ${datadir}/applications
desktop_src_files = gtktide.desktop.in-desktop
gtktide.desktop.in: gtktide.desktop.in-desktop
	cp ${srcdir}/gtktide.desktop.in-desktop gtktide.desktop.in
endif

desktop_in_files = gtktide.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	${data_DATA} \
	${harmonics_DATA} \
	${desktop_src_files} \
	${desktop_DATA} \
	${service_DATA}
