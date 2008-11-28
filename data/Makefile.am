xtidedata_DATA = disclaimer.txt

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

icon_dir = hildon
icons_64x54dir = ${datadir}/icons/hicolor/scalable/${icon_dir}
icons_64x54_DATA = \
	icons/64x54/xtide.png

else
desktopdir = ${datadir}/applications
desktop_src_files = gtktide.desktop.in-desktop
gtktide.desktop.in: gtktide.desktop.in-desktop
	cp ${srcdir}/gtktide.desktop.in-desktop gtktide.desktop.in

icon_dir = apps

endif

desktop_in_files = gtktide.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

icons_22x22dir = ${datadir}/icons/hicolor/22x22/${icon_dir}
icons_24x24dir = ${datadir}/icons/hicolor/24x24/${icon_dir}
icons_26x26dir = ${datadir}/icons/hicolor/26x26/${icon_dir}
icons_32x32dir = ${datadir}/icons/hicolor/32x32/${icon_dir}
icons_48x48dir = ${datadir}/icons/hicolor/48x48/${icon_dir}
icons_64x64dir = ${datadir}/icons/hicolor/64x64/${icon_dir}
icons_128x128dir = ${datadir}/icons/hicolor/128x128/${icon_dir}

icons_22x22_DATA = \
	icons/22x22/xtide.png

icons_24x24_DATA = \
	icons/24x24/xtide.png

icons_26x26_DATA = \
	icons/26x26/xtide.png

icons_32x32_DATA = \
	icons/32x32/xtide.png

icons_48x48_DATA = \
	icons/48x48/xtide.png

icons_64x64_DATA = \
	icons/64x64/xtide.png

icons_128x128_DATA = \
	icons/128x128/xtide.png


EXTRA_DIST = \
	${xtidedata_DATA} \
	${harmonics_DATA} \
	${desktop_src_files} \
	${desktop_DATA} \
	${service_DATA} \
	${icons_22x22_DATA} \
	${icons_24x24_DATA} \
	${icons_26x26_DATA} \
	${icons_32x32_DATA} \
	${icons_48x48_DATA} \
	${icons_54x54_DATA} \
	${icons_64x54_DATA} \
	${icons_64x64_DATA} \
	${icons_128x128_DATA}

