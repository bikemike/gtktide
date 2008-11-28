#include <config.h>

#include <stdio.h>
#include <gtk/gtk.h>

#include "common.hh"

#include <sys/time.h>

#include "Graph.hh"

#include "SubordinateStation.hh"



#ifdef MAEMO
#ifdef HAVE_HILDON_1
#include <hildon/hildon-program.h>
#else
#include <hildon-widgets/hildon-program.h>
#endif
#include <libosso.h>
#endif
#include <map>
#include <string>

using namespace Shape;

using namespace std;

class GtkRect : public Rectangle
{
public:
	GtkRect(const GdkRectangle& r)
	{
		x = r.x;
		y = r.y;
		width = r.width;
		height = r.height;
	}
};


class ParsedColor
{
public:
	GdkColor gdk_color;
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

static ParsedColor cachedColors[Colors::numColors]; // Wants to be [Colorchoice]

class GtkTideWindowImpl;
class gtkGraph;
class DisclaimerDlg;

class GtkTideWindow
{
public:
	GtkTideWindow();
	~GtkTideWindow();

private:
	GtkTideWindowImpl* m_pGtkTideWindowImpl;
};

typedef std::map<Dstr, StationRef*> MapLocationStation;
typedef std::map<Dstr, MapLocationStation > MapAreaLocation;
typedef std::map<Dstr, MapAreaLocation > MapCountryArea;

class GtkTideWindowImpl
{
public:
	GtkTideWindowImpl();
	~GtkTideWindowImpl();

	void StartTimeoutSmoothScroll();
	void StopTimeoutSmoothScroll();
	void StopTimeoutSmoothScrollSlowdown();
	void LoadStations();

	void SetStationRef(StationRef* stationRef);

	MapCountryArea m_mapStationRefCache;

	GtkWidget*       m_pWindow;
	GtkWidget*       m_pMenubar;
	GtkWidget*       m_pToolbar;
	GtkWidget*       m_pDrawingArea;
	GtkWidget*       m_pVBox;
	GtkWidget*       m_pComboRecent;
	gulong           m_ulComboHandlerID;
	GtkToolItem*     m_pToolItemStations;

	GtkUIManager*    m_pUIManager;
	GThreadPool*     m_pThreadPool;


	//gui actions
	static GtkActionEntry action_entries[];
	static GtkToggleActionEntry action_entries_toggle[];


	gint             m_iLastX;
	gint             m_iLastY;
	gint             m_iPressX;
	gint             m_iPressY;

	bool             m_bMouseMoved;
	gint             m_ScrollToOffset;

	bool             m_bStationsLoaded;

	guint            timeout_id_smooth_scroll;
	guint            timeout_id_smooth_scroll_slowdown;

	GList*           velocity_time_list;
	struct timeval   last_motion_time;
#ifdef MAEMO
	osso_context_t*  osso_context;
#endif

	Graph*           m_pGraph;
	StationRef*      m_pStationRef;

	
};


class DisclaimerDlg
{
public:
	DisclaimerDlg(GtkTideWindowImpl* parent) : 
		m_pParent(parent), m_bTextLoaded(false)
	{
		m_bLoaded = false;
		m_pDialog = gtk_dialog_new_with_buttons ("Disclaimer",
			GTK_WINDOW(m_pParent->m_pWindow),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			"Dismiss",
			GTK_RESPONSE_OK,
			"Don't Show Again",
			GTK_RESPONSE_CANCEL,
			NULL);
		gtk_window_set_default_size (GTK_WINDOW(m_pDialog),575,350);
		m_pTextView     = gtk_text_view_new();
		gtk_text_view_set_editable (GTK_TEXT_VIEW(m_pTextView), false);

		GtkTextBuffer* gtktxtbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (m_pTextView));

		FILE* f = fopen (XTIDE_DATADIR"/disclaimer.txt","rb");
		if (NULL != f)
		{
			unsigned long lSize;
			char * buffer = NULL;
			size_t result;

			fseek (f , 0 , SEEK_END);
			lSize = ftell (f);
			rewind (f);

			buffer = (char*) malloc (sizeof(char)*(lSize+1));
			if (buffer != NULL)
			{
				result = fread (buffer,1,lSize,f);
				buffer[lSize] = '\0';
				if (result == lSize)
				{
					gtk_text_buffer_set_text (gtktxtbuffer,buffer, -1);
					m_bTextLoaded = true;
				}
				free (buffer);
			}
			fclose (f);
		}

		/*
		g_signal_connect_swapped (m_pDialog,
			"response", 
			G_CALLBACK (gtk_widget_hide),
			m_pDialog);
			*/

		GtkWidget* sw = gtk_scrolled_window_new(NULL,NULL);
		gtk_widget_show(sw);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
		gtk_container_add (GTK_CONTAINER (sw), m_pTextView);
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG(m_pDialog)->vbox), sw);
		gtk_widget_show(m_pTextView);

		// start by selecting the old station
		m_bLoaded = true;
	}
	~DisclaimerDlg()
	{
		gtk_widget_destroy(m_pDialog);
	}

	bool Run()
	{
		m_bDisable = false;
		if (m_bTextLoaded)
		{
			gint result =
				gtk_dialog_run (GTK_DIALOG(m_pDialog));
			if (GTK_RESPONSE_CANCEL == result)
			{
				Global::disableDisclaimer();
				m_bDisable = true;
			}
		}
		return m_bDisable;
	}

	GtkWidget* m_pDialog;
	GtkWidget* m_pTextView;
	GtkTideWindowImpl* m_pParent;
	bool m_bDisable;
	bool m_bLoaded;
	bool m_bTextLoaded;

};

class DateSelectDlg
{
public:
	DateSelectDlg(GtkTideWindowImpl* parent) : 
		m_pParent(parent)
	{
		m_bLoaded = false;
		m_pDialog = gtk_dialog_new_with_buttons ("Choose Date",
			GTK_WINDOW(m_pParent->m_pWindow),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_OK,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL,
			NULL);
		m_pCalendar     = gtk_calendar_new();

		gtk_container_add (GTK_CONTAINER (GTK_DIALOG(m_pDialog)->vbox), m_pCalendar);
		gtk_widget_show(m_pCalendar);

		// start by selecting the old station
		m_bLoaded = true;
	}
	~DateSelectDlg()
	{
		gtk_widget_destroy(m_pDialog);
	}

	bool Run()
	{
		m_bNewDate = false;
		gint result =
			gtk_dialog_run (GTK_DIALOG(m_pDialog));
		if (GTK_RESPONSE_OK == result)
		{
			m_bNewDate = true;
		}
		return m_bNewDate;
	}

	void SetDate(unsigned int year, unsigned int month, unsigned int day)
	{
		gtk_calendar_select_month(GTK_CALENDAR(m_pCalendar), month, year);
		gtk_calendar_select_day(GTK_CALENDAR(m_pCalendar), day);
	}

	void GetDate(unsigned int& year, unsigned int& month, unsigned int& day)
	{
		gtk_calendar_get_date(GTK_CALENDAR(m_pCalendar),&year, &month, &day);
	}

	GtkWidget* m_pDialog;
	GtkWidget* m_pCalendar;
	GtkTideWindowImpl* m_pParent;
	bool m_bNewDate;
	bool m_bLoaded;

};




class LocationSelectDlg
{
public:
	LocationSelectDlg(GtkTideWindowImpl* parent) : 
		m_pParent(parent), m_pOldStationRef(parent->m_pStationRef)
	{
		m_bLoaded = false;
		m_pDialog = gtk_dialog_new_with_buttons ("Choose Location",
			GTK_WINDOW(m_pParent->m_pWindow),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_OK,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL,
			NULL);
		m_pComboCountry = gtk_combo_box_new_text();
		m_pComboArea    = gtk_combo_box_new_text();
		m_pComboLoc     = gtk_combo_box_new_text();

		gtk_container_add (GTK_CONTAINER (GTK_DIALOG(m_pDialog)->vbox), m_pComboCountry);
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG(m_pDialog)->vbox), m_pComboArea);
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG(m_pDialog)->vbox), m_pComboLoc);

		g_signal_connect(m_pComboCountry,
			"changed",(GCallback)LocationSelectDlg::combo_changed,this);
		g_signal_connect(m_pComboArea,
			"changed",(GCallback)LocationSelectDlg::combo_changed,this);

		gtk_widget_show(m_pComboCountry);
		gtk_widget_show(m_pComboArea);
		gtk_widget_show(m_pComboLoc);

		// start by selecting the old station


		
		Dstr old_country;
		if (NULL != m_pOldStationRef)
		{
			old_country = m_pOldStationRef->country;
		}
		old_country.utf8();

		gint selectid = 0;
		MapCountryArea::iterator itr1;
		gint count = 0;
		for (itr1 = m_pParent->m_mapStationRefCache.begin();
			m_pParent->m_mapStationRefCache.end() != itr1; ++itr1)
		{
			Dstr country = itr1->first;
			gtk_combo_box_append_text(GTK_COMBO_BOX(m_pComboCountry),
					country.aschar());
			if (country == old_country)
			{
				selectid = count;
			}
			++count;
		}
		gtk_combo_box_set_active(GTK_COMBO_BOX(m_pComboCountry),selectid);
		m_bLoaded = true;
	}
	~LocationSelectDlg()
	{
		gtk_widget_destroy(m_pDialog);
	}

	void Run()
	{
		m_bNewLocation = false;
		gint result =
			gtk_dialog_run (GTK_DIALOG(m_pDialog));
		if (GTK_RESPONSE_OK == result)
		{
			m_bNewLocation = true;
		}
	}

	static void combo_changed (GtkComboBox *widget, gpointer user_data)
	{
		LocationSelectDlg* locDlg = (LocationSelectDlg*)user_data;
		if (widget == GTK_COMBO_BOX(locDlg->m_pComboCountry))
		{
			gtk_list_store_clear(
				GTK_LIST_STORE(
					gtk_combo_box_get_model(
						GTK_COMBO_BOX(locDlg->m_pComboArea))));

			Dstr old_area;
			if (!locDlg->m_bLoaded && NULL != locDlg->m_pOldStationRef)
			{
				string strName = locDlg->m_pOldStationRef->name.aschar();
				string::size_type pos = strName.find_last_of(',');
				if (string::npos != pos && strName.size()-1 != pos)
				{
					string strLoc  = strName.substr(0,pos);
					string strArea = strName.substr(pos+1, strName.size()-1 - pos);

					old_area = strArea.c_str();
					old_area.trim().utf8();
				}
				else
				{
					old_area = "Unknown";
				}
			}

			gchar* country = gtk_combo_box_get_active_text(widget);
			if (NULL != country)
			{
				// update area and loc
				MapAreaLocation::iterator itr;
				gint selectid = 0;
				gint count = 0;
				for (itr = locDlg->m_pParent->m_mapStationRefCache[country].begin();
					locDlg->m_pParent->m_mapStationRefCache[country].end() != itr;
					++itr)
				{
					Dstr area = itr->first; 
					gtk_combo_box_append_text(GTK_COMBO_BOX(locDlg->m_pComboArea),
						area.aschar());

					if (!locDlg->m_bLoaded && old_area == area)
					{
						selectid = count;
					}
					++count;

				}
				gtk_combo_box_set_active(GTK_COMBO_BOX(locDlg->m_pComboArea),selectid);

				g_free(country);
			}
		}
		else if (widget == GTK_COMBO_BOX(locDlg->m_pComboArea))
		{
			gtk_list_store_clear(
				GTK_LIST_STORE(
					gtk_combo_box_get_model(
						GTK_COMBO_BOX(locDlg->m_pComboLoc))));

			Dstr old_loc;
			if (!locDlg->m_bLoaded && NULL != locDlg->m_pOldStationRef)
			{
				string strName = locDlg->m_pOldStationRef->name.aschar();
				string::size_type pos = strName.find_last_of(',');
				if (string::npos != pos && strName.size()-1 != pos)
				{
					string strLoc  = strName.substr(0,pos);
					old_loc = strLoc.c_str();
					old_loc.trim().utf8();
				}
				else
				{
					old_loc = locDlg->m_pOldStationRef->name;
					old_loc.utf8();
				}
			}

			gchar* country = gtk_combo_box_get_active_text(GTK_COMBO_BOX(locDlg->m_pComboCountry));
			gchar* area = gtk_combo_box_get_active_text(widget);
			if (NULL != country && NULL != area)
			{
				// update area and loc
				MapLocationStation::iterator itr;
				gint selectid = 0;
				gint count = 0;
				for (itr = locDlg->m_pParent->m_mapStationRefCache[country][area].begin();
					locDlg->m_pParent->m_mapStationRefCache[country][area].end() != itr;
					++itr)
				{
					Dstr loc = itr->first; 
					gtk_combo_box_append_text(GTK_COMBO_BOX(locDlg->m_pComboLoc),
						loc.aschar());

					if (!locDlg->m_bLoaded && old_loc == loc)
					{
						selectid = count;
					}
					++count;
				}
				gtk_combo_box_set_active(GTK_COMBO_BOX(locDlg->m_pComboLoc),selectid);
			}
			if (country)
				g_free(country);
			if (area)
				g_free(area);
		}
	}

	StationRef* GetLocation()
	{
		if (m_bNewLocation)
		{
			gchar* country = gtk_combo_box_get_active_text(GTK_COMBO_BOX(m_pComboCountry));
			gchar* area = gtk_combo_box_get_active_text(GTK_COMBO_BOX(m_pComboArea));
			gchar* loc = gtk_combo_box_get_active_text(GTK_COMBO_BOX(m_pComboLoc));

			return m_pParent->m_mapStationRefCache[country][area][loc];
		}
		return NULL;
	}

	GtkWidget* m_pDialog;
	GtkWidget* m_pComboCountry;
	GtkWidget* m_pComboArea;
	GtkWidget* m_pComboLoc;
	GtkTideWindowImpl* m_pParent;
	StationRef* m_pOldStationRef;
	bool m_bNewLocation;
	bool m_bLoaded;

};

#define ACTION_QUIT                  "Quit"
#define ACTION_QUIT_2                "Quit_2"
#define ACTION_FULLSCREEN            "FullScreen"
#define ACTION_FULLSCREEN_F11        "FullScreenF11"
#define ACTION_VIEW_TODAY            "ViewToday"
#define ACTION_CHOOSE_LOCATION       "ChooseLocation"
#define ACTION_CHOOSE_DATE           "ChooseDate"
#ifdef MAEMO
#define ACTION_FULLSCREEN_MAEMO      ACTION_FULLSCREEN"_MAEMO"
#endif


static const char* sz_base_ui =
"<ui>"
"	<menubar name='MenubarMain'>"
#ifdef MAEMO
"		<menu action='MenuMain'>"
#endif
"		<menu action='MenuFile'>"
"			<separator/>"
"			<menuitem action='"ACTION_QUIT"'/>"
"		</menu>"
"		<menu action='MenuView'>"
"			<menuitem action='"ACTION_CHOOSE_LOCATION"'/>"
"			<menuitem action='"ACTION_CHOOSE_DATE"'/>"
"			<separator/>"
"			<menuitem action='"ACTION_FULLSCREEN"'/>"
"			<separator/>"
"			<menuitem action='"ACTION_VIEW_TODAY"'/>"
"			<separator/>"
"		</menu>"
#ifdef MAEMO
"		</menu>"
#endif
"	</menubar>"
"	<toolbar name='ToolbarMain'>"
"		<toolitem action='"ACTION_CHOOSE_LOCATION"'/>"
"		<toolitem action='"ACTION_CHOOSE_DATE"'/>"
"		<separator/>"
"		<toolitem action='"ACTION_VIEW_TODAY"'/>"
"		<separator/>"
"		<toolitem action='"ACTION_FULLSCREEN"'/>"
"		<separator/>"
"	</toolbar>"
"	<popup name='ContextMenu'>"
"	</popup>"
"	<accelerator action='"ACTION_FULLSCREEN_F11"'/>"
#ifdef MAEMO
"	<accelerator action='"ACTION_FULLSCREEN_MAEMO"'/>"
#endif
"	<accelerator action='"ACTION_QUIT_2"'/>"
"</ui>";


static void action_handler_cb(GtkAction *action, gpointer data);

GtkToggleActionEntry GtkTideWindowImpl::action_entries_toggle[] = {
	{ ACTION_FULLSCREEN, GTK_STOCK_FULLSCREEN , ("_Full Screen"), "f", ("Toggle Full Screen Mode"), G_CALLBACK(action_handler_cb),FALSE},
};


GtkActionEntry GtkTideWindowImpl::action_entries[] = {
	
#ifdef MAEMO
	{ "MenuMain", NULL, ("_Main") },
#endif
	{ "MenuFile", NULL, ("_File") },
	{ "MenuView", NULL, ("_View") },
	{ ACTION_QUIT, "", ("_Quit"), "<Control>w", ("Quit"), G_CALLBACK(action_handler_cb)},
	{ ACTION_QUIT_2, "", ("_Quit"), "q", ("Quit"), G_CALLBACK(action_handler_cb)},
	{ ACTION_VIEW_TODAY, GTK_STOCK_HOME, "View _Today", "<Control>t", "View today's date", G_CALLBACK(action_handler_cb)},
	{ ACTION_CHOOSE_LOCATION, GTK_STOCK_INDEX, "Choose _Location", "<Control>l", "Choose the Tide/Current Location", G_CALLBACK(action_handler_cb)},
	{ ACTION_CHOOSE_DATE, GTK_STOCK_JUMP_TO, "Choose _Date", "<Control>d", "Choose the Date to Show", G_CALLBACK(action_handler_cb)},
	{ ACTION_FULLSCREEN_F11, "", ("_Full Screen"), "F11", ("Toggle Full Screen Mode"), G_CALLBACK(action_handler_cb)},
#ifdef MAEMO
	{ ACTION_FULLSCREEN_MAEMO, "", ("_Full Screen"), "F6", ("Toggle Full Screen Mode"), G_CALLBACK(action_handler_cb)},
#endif

	
};

static guint32 pixbuf_get_pixel(GdkPixbuf* pixbuf, gint x, gint y)
{
	int width, height, rowstride, n_channels;
	guchar *pixels, *p;

	n_channels = gdk_pixbuf_get_n_channels (pixbuf);

	g_assert (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
	g_assert (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	g_assert (x >= 0 && x < width);
	g_assert (y >= 0 && y < height);

	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);

	p = pixels + y * rowstride + x * n_channels;

	guint32 pixel = 0;
	pixel |= (p[0] << 16);
	pixel |= (p[1] << 8);
	pixel |= (p[2]);
	if (gdk_pixbuf_get_has_alpha (pixbuf))
	{
		pixel |= (p[3] << 24);
	}
	return pixel;
}

class gtkGraph : public Graph 

{
public:
	gtkGraph (GdkDrawable *drawable, PangoContext* context, GraphStyle style = normal);
	//gtkGraph (GdkDrawable *drawable, PangoContext* context, GraphStyle style = clock);
	~gtkGraph();

	void setDrawable(GdkDrawable* d)
	{
		if (NULL != gc)
			g_object_unref(gc);
		drawable = d;
		gc = gdk_gc_new(drawable);
	}

	virtual void invalidateRect(const Rectangle& rect)
	{
		GdkRectangle r;
		r.x = rect.x;
		r.y = rect.y;
		r.width = rect.width;
		r.height = rect.height;
		gdk_window_invalidate_rect(drawable, &r, FALSE);
	}

	void moveGraphRegion(const Rectangle& rect, int dx, int dy)
	{
		GdkRectangle r;
		r.x = rect.x;
		r.y = rect.y;
		r.width = rect.width;
		r.height = rect.height;

		GdkRegion* region = gdk_region_rectangle(&r);
		gdk_window_move_region(drawable, region, dx, dy);
		gdk_region_destroy(region);
	}

	virtual void drawRect(const Rectangle& r, Colors::Colorchoice c, bool bFilled = false)
	{
		ParsedColor parsedColor = cachedColors[c];
		gdk_gc_set_rgb_fg_color(gc,&parsedColor.gdk_color);

		gdk_draw_rectangle(drawable, gc, bFilled ? TRUE : FALSE, r.x, r.y, r.width, r.height);
	}

	virtual void drawLine(const Point& p1, const Point& p2, Colors::Colorchoice c)
	{
		ParsedColor parsedColor = cachedColors[c];
		gdk_gc_set_rgb_fg_color(gc,&parsedColor.gdk_color);
		gdk_draw_line(drawable, gc, p1.x, p1.y, p2.x, p2.y);
	}

	void drawLines(const std::vector<Shape::Point>& points, Colors::Colorchoice c, double thickness = 1.0 )
	{
		ParsedColor parsedColor = cachedColors[c];
		GdkColor* fc = &parsedColor.gdk_color;

		cairo_t* context = gdk_cairo_create(drawable);
		if (1 < points.size())
		{
			cairo_move_to(context, points[0].x, points[0].y);
		}
		for (int i = 1; i < points.size(); ++i)
		{
			cairo_line_to(context, points[i].x, points[i].y);
		}
		//cairo_clip_preserve(context);
		cairo_set_source_rgb (context, 0, 0, 0);
		gdk_cairo_set_source_color(context,&parsedColor.gdk_color);
		cairo_stroke(context);

		cairo_destroy(context);
	}
	virtual void drawPolygon(const std::vector<Shape::Point>& points, Colors::Colorchoice c, bool filled = false)
	{
		bool hq = true;
		if (hq)
		{

			cairo_t* context = gdk_cairo_create(drawable);

			Rectangle hourRect = getHourAreaRect();
			// create a gradient 0,
			cairo_pattern_t* pattern = cairo_pattern_create_linear(0,0,0,_ySize);

			ParsedColor parsedColor = cachedColors[c];
			GdkColor* fc = &parsedColor.gdk_color;

			double r,g,b;
			r = fc->red / 65535.;
			g = fc->green / 65535.;
			b = fc->blue / 65535.;
			cairo_pattern_add_color_stop_rgba( pattern, 0.,r,g,b,1.);
			cairo_pattern_add_color_stop_rgba( pattern, hourRect.y/(double)_ySize,r,g,b,0.4);
			cairo_pattern_add_color_stop_rgba( pattern, 1., r,g,b,.2);

			cairo_set_line_join(context,CAIRO_LINE_JOIN_ROUND);
			for (unsigned int i = 0; i < points.size(); ++i)
			{
				cairo_line_to(context, points[i].x, points[i].y);
			}
			cairo_close_path(context);
			cairo_clip_preserve(context);
			cairo_set_source(context, pattern);

			if (filled)
				cairo_fill(context);

			cairo_destroy(context);
		}
		else
		{
			ParsedColor parsedColor = cachedColors[c];
			GdkColor* fc = &parsedColor.gdk_color;

			gdk_gc_set_rgb_fg_color(gc,&parsedColor.gdk_color);

			GdkPoint* gdk_points = new GdkPoint[points.size()];
			for (unsigned int i = 0; i < points.size(); ++i)
			{
				gdk_points[i].x = points[i].x;
				gdk_points[i].y = points[i].y;
			}

			gdk_draw_polygon(drawable, gc, FALSE, gdk_points, points.size());
			if (filled)
				gdk_draw_polygon(drawable, gc, TRUE, gdk_points, points.size());

			delete [] gdk_points;
		}
	}

	void processUpdates()
	{
		gdk_window_process_updates(drawable, FALSE);
	}

protected:
	GdkGC*              gc;
	const bool          antiAliasingDisabled;
	GdkDrawable*        drawable;
	PangoContext*       pango_context;
	PangoLayout*        pango_layout;

	GdkPixbuf*          pixbufCache;


	mutable std::map<std::string,int>    m_mapCacheFontWidth;
	mutable int m_iFontHeight;
	

	void                startPixelCache();
	void                stopPixelCache();

	const unsigned      stringWidth (const Dstr &s) const;
	const unsigned      fontHeight() const;

	void                drawString (int x, int y, const Dstr &s);

	                    // These override perfectly good versions in Graph in order to use
                    	// the available X11 drawing functions.
	void                drawVerticalLine (int x, int y1, int y2, Colors::Colorchoice c);
	void                drawHorizontalLine (int xlo, int xhi, int y, Colors::Colorchoice c);

	void                setPixel (int x, int y, Colors::Colorchoice c);
	void                setPixel (int x, int y, Colors::Colorchoice c, double saturation);


};


gtkGraph::gtkGraph (GdkDrawable* drawable, PangoContext* context,
                              GraphStyle style) :
	Graph (style),
	antiAliasingDisabled (Global::settings["aa"].c == 'n'),
	drawable(drawable),
	pango_context(context),
	pango_layout(NULL),
	pixbufCache(NULL),
	m_iFontHeight(-1)

{
	//assert (xSize >= Global::minGraphWidth && ySize >= Global::minGraphHeight);
	if (NULL != pango_context)
	{
		pango_layout = pango_layout_new(pango_context);
	}

	if (NULL != drawable)
	{
		gc = gdk_gc_new(drawable);
	}
}


gtkGraph::~gtkGraph() 
{
	stopPixelCache();
	if (NULL != pango_layout)
	{
		g_object_unref(pango_layout);
		pango_layout = NULL;
	}

	g_object_unref(gc);
}


const unsigned gtkGraph::stringWidth (const Dstr &s) const 
{
	//return xxX::stringWidth (xxX::smallFontStruct, s);
	pango_layout_set_text(pango_layout, s.aschar(), s.length());

	int width;
	std::map<std::string, int>::iterator itr;
	itr = m_mapCacheFontWidth.find(std::string(s.aschar()));
	if (m_mapCacheFontWidth.end() != itr)
	{
		width = itr->second;
	}
	else
	{
		pango_layout_set_text(pango_layout, s.aschar(), s.length());
		pango_layout_get_pixel_size(pango_layout, &width, NULL);
		m_mapCacheFontWidth.insert(std::pair<std::string, int>(s.aschar(),width));
	}
	return width;
}


const unsigned gtkGraph::fontHeight() const 
{
	if (-1 == m_iFontHeight)
	{
		pango_layout_get_pixel_size(pango_layout, NULL, &m_iFontHeight);
	}
	return m_iFontHeight;
}


void gtkGraph::setPixel (int x, int y, Colors::Colorchoice c) 
{
	if (x < 0 || x >= (int)_xSize || y < 0 || y >= (int)_ySize)
		return;
	ParsedColor parsedColor = cachedColors[c];

	gdk_gc_set_rgb_fg_color(gc,&parsedColor.gdk_color);
	gdk_draw_point(drawable, gc, x, y);
}


void gtkGraph::setPixel (int x,
			      int y,
			      Colors::Colorchoice c,
			      double saturation) 
{
	if (x < 0 || x >= (int)_xSize || y < 0 || y >= (int)_ySize)
		return;

	if (antiAliasingDisabled) 
	{
		if (saturation >= 0.5)
			setPixel (x, y, c);
	} 
	else 
	{
		return;
		guint32 pixel;

		if (NULL != pixbufCache)
		{
			pixel = pixbuf_get_pixel(pixbufCache, x, y);
		}
		else
		{
			GdkPixbuf* pixbuf = gdk_pixbuf_get_from_drawable(NULL, drawable, NULL, x, y, 0,0, 1, 1);
			pixel = pixbuf_get_pixel(pixbuf, 0, 0);
			g_object_unref(pixbuf);
		}

		uint8_t r1, g1, b1;
		r1 = (pixel >> 16) & 0xFF;
		g1 = (pixel >> 8) & 0xFF;
		b1 = (pixel) & 0xFF;

		ParsedColor parsedColor = cachedColors[c];

		GdkColor c2;
		c2.red = linterp (r1, parsedColor.r, saturation) * 0xff;
		c2.green = linterp (g1, parsedColor.g, saturation) * 0xff;
		c2.blue = linterp (b1, parsedColor.b, saturation) * 0xff;

		gdk_gc_set_rgb_fg_color(gc,&c2);
		gdk_draw_point(drawable, gc, x, y);
	}
}


void gtkGraph::drawVerticalLine (int x,
				      int y1,
				      int y2,
				      Colors::Colorchoice c) 
{
	ParsedColor parsedColor = cachedColors[c];

	gdk_gc_set_rgb_fg_color(gc, &parsedColor.gdk_color);

	gdk_draw_line(drawable,gc,x, y1, x, y2);
}


void gtkGraph::drawHorizontalLine (int xlo,
					int xhi,
					int y,
					Colors::Colorchoice c) 
{
	ParsedColor parsedColor = cachedColors[c];
	gdk_gc_set_rgb_fg_color(gc, &parsedColor.gdk_color);

	gdk_gc_set_rgb_fg_color(gc, &parsedColor.gdk_color);

	gdk_draw_line(drawable,gc, xlo, y, xhi, y);
}


void gtkGraph::drawString (int x, int y, const Dstr &s) 
{

	ParsedColor parsedColor = cachedColors[Colors::foreground];
	gdk_gc_set_rgb_fg_color(gc, &parsedColor.gdk_color);

	pango_layout_set_text(pango_layout, s.aschar(), s.length());

	gdk_draw_layout(drawable, gc, x, y, pango_layout);

	/*
  XDrawString (xxX::display, pixmap, xxX::textGC, x, y+fontHeight()-2,
               s.aschar(), s.length());
	*/
}


void gtkGraph::startPixelCache() 
{
	if (NULL != pixbufCache)
	{
		g_object_unref(pixbufCache);
	}

	//pixbufCache = gdk_pixbuf_get_from_drawable(NULL, drawable, NULL, 0, 0, 0, 0, _xSize, _ySize);
}


void gtkGraph::stopPixelCache() 
{
	if (NULL != pixbufCache)
	{
		g_object_unref(pixbufCache);
		pixbufCache = NULL;
	}
}

typedef struct _VelocityTimeStruct 
{
	gint hvelocity;
	gint vvelocity;
	gdouble time;
} VelocityTimeStruct;



static gboolean
timeout_smooth_scroll_slowdown(gpointer data)
{
	GtkTideWindowImpl* pTideWindowImpl = (GtkTideWindowImpl*)data;

	gdouble divider = 1.02; 	
	gdouble timeout_secs = 35 / 1000.;

	gboolean hdone = FALSE;
	gboolean vdone = FALSE;

	gdk_threads_enter();
	
	GList* list_itr = g_list_first(pTideWindowImpl->velocity_time_list);
	gint hvelocity_avg = 0;
	gint vvelocity_avg = 0;
	gdouble total_time = 0; 
	if (NULL != list_itr)
	{
		do
		{
			VelocityTimeStruct* vt = (VelocityTimeStruct*)list_itr->data;
			hvelocity_avg += (gint)(vt->hvelocity * vt->time);
			vvelocity_avg += (gint)(vt->vvelocity * vt->time);
			total_time += vt->time;
			list_itr = g_list_next(list_itr);
		} while (NULL !=list_itr);
	}
	
	hvelocity_avg = (gint)(hvelocity_avg/total_time);
	vvelocity_avg = (gint)(vvelocity_avg/total_time);


	if (1 != g_list_length(pTideWindowImpl->velocity_time_list) )
	{
		g_list_foreach(pTideWindowImpl->velocity_time_list, (GFunc)g_free, NULL);
		g_list_free(pTideWindowImpl->velocity_time_list);
		pTideWindowImpl->velocity_time_list = NULL;
		
		VelocityTimeStruct* vt = (VelocityTimeStruct*)g_malloc(sizeof(VelocityTimeStruct));
		vt->hvelocity = hvelocity_avg;
		vt->vvelocity = vvelocity_avg;
		vt->time = total_time;
		
		pTideWindowImpl->velocity_time_list = 
			g_list_append(pTideWindowImpl->velocity_time_list,vt);
	}
	GList* first = g_list_first(pTideWindowImpl->velocity_time_list);
	
	VelocityTimeStruct* vt = (VelocityTimeStruct*)first->data;
	vt->hvelocity = (gint)(hvelocity_avg/divider);
	vt->vvelocity = (gint)(vvelocity_avg/divider);
	
	gint hdistance = (gint)(timeout_secs * hvelocity_avg);
	
	if (0 == hdistance || 0 == hvelocity_avg)
	{
		hdone = TRUE;
	}
	else
	{
		gint diff = hdistance;
		unsigned ival;

		if (0 > diff)
		{
			ival = (unsigned)(-1*diff);
		}
		else
		{
			ival = diff;
		}

		Interval interval = pTideWindowImpl->m_pGraph->getIntervalOffsetAtPosition(ival); 

		Timestamp timestamp;
		if (0 > diff)
		{
			timestamp = pTideWindowImpl->m_pGraph->getNominalStartTime() + interval;
		} 
		else
		{
			timestamp = pTideWindowImpl->m_pGraph->getNominalStartTime() - interval;
		}

		pTideWindowImpl->m_pGraph->setNominalStartTime(timestamp);
	}
	
	gint vdistance = (gint)(timeout_secs * vvelocity_avg);

	if (0 == vdistance || 0 == vvelocity_avg)
	{
		vdone = TRUE;
	}
	else
	{
		vdone = TRUE;
	
		/*
		gint old_vadjust = (guint)gtk_adjustment_get_value(vadjustment);
		gint vadjust = old_vadjust;
		vadjust = MAX (0,vadjust - vdistance);
		vadjust = MIN (vadjust,vadjustment->upper - vadjustment->page_size);
		if (old_vadjust == vadjust)
		{
			vdone = TRUE;
		}
		else
		{
			gtk_adjustment_set_value(vadjustment,vadjust);
		}
		*/
	}
	
	
	if (hdone && vdone)
	{
		pTideWindowImpl->timeout_id_smooth_scroll_slowdown = 0;
	}
	
	gdk_threads_leave();
	
	return !(hdone && vdone);
}

static void  combo_changed (GtkComboBox *widget, gpointer data)
{
	GtkTideWindowImpl* pTideWindowImpl = (GtkTideWindowImpl*)data;

	if (widget == GTK_COMBO_BOX(pTideWindowImpl->m_pComboRecent))
	{
		gchar* str_selected = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));

		StationIndex &stations = Global::stationIndex();
		StationRef* ref = stations.getStationRefByName(str_selected);
		pTideWindowImpl->SetStationRef(ref);
		g_free(str_selected);
	}
	
}



static gboolean 
timeout_smooth_scroll(gpointer data)
{
	GtkTideWindowImpl* pTideWindowImpl = (GtkTideWindowImpl*)data;
	double half = pTideWindowImpl->m_ScrollToOffset/2.;

	gint dist;
	if (half < 0)
	{
		dist = (int)(half - .5);
	}
	else
	{
		dist = (int)(half + .5);
	}
	pTideWindowImpl->m_ScrollToOffset -= dist;

	Interval pixel_interval = pTideWindowImpl->m_pGraph->getIntervalOffsetAtPosition(1); 
	Interval ioffset = pixel_interval * (double)dist;

	Timestamp ts = pTideWindowImpl->m_pGraph->getNominalStartTime() + ioffset;

	gdk_threads_enter();
	pTideWindowImpl->m_pGraph->setNominalStartTime(ts);
	gdk_threads_leave();

	if (0 == pTideWindowImpl->m_ScrollToOffset)
	{
		pTideWindowImpl->timeout_id_smooth_scroll = 0;
		return FALSE;
	}
	return TRUE;
}

static void thread_pool_func(gpointer data, gpointer user_data)
{
	void (*function)(gpointer) = (void (*)(gpointer))data;
	if (NULL != function)
	{
		(*function)(user_data);
	}
}

static void 
load_stations(gpointer user_data)
{
	GtkTideWindowImpl* pTideWindowImpl = (GtkTideWindowImpl*)user_data;
	pTideWindowImpl->LoadStations();
}

gboolean
button_release_event_callback (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkTideWindowImpl* pTideWindowImpl = (GtkTideWindowImpl*)data;
	if (!pTideWindowImpl->m_bMouseMoved)
	{
		// adjust the graph
		pTideWindowImpl->m_iLastX = (gint)event->x;
		pTideWindowImpl->m_iLastY = (gint)event->y;

		gint diff = pTideWindowImpl->m_iLastX - widget->allocation.width/2;

		pTideWindowImpl->m_ScrollToOffset = diff;

		pTideWindowImpl->StartTimeoutSmoothScroll();

	}
	else
	{
		struct timeval new_motion_time = {0};
		gettimeofday(&new_motion_time,NULL);

		gdouble old_time = (gdouble)pTideWindowImpl->last_motion_time.tv_sec + ((gdouble)pTideWindowImpl->last_motion_time.tv_usec)/1000000;
		gdouble new_time = (gdouble)new_motion_time.tv_sec + ((gdouble)new_motion_time.tv_usec)/1000000;
				
		if ( 3 == g_list_length(pTideWindowImpl->velocity_time_list) &&
			(0.1 > new_time - old_time) )
		{
			pTideWindowImpl->StopTimeoutSmoothScroll();
			pTideWindowImpl->StopTimeoutSmoothScrollSlowdown();

			pTideWindowImpl->timeout_id_smooth_scroll_slowdown = 
				g_timeout_add(35,timeout_smooth_scroll_slowdown,pTideWindowImpl);
		}

	}


	pTideWindowImpl->m_bMouseMoved = false;

	if (GDK_BUTTON_PRESS == event->type)
	{
		pTideWindowImpl->m_iLastX = (gint)event->x;
	}
	return TRUE;
}

gboolean
button_press_event_callback (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkTideWindowImpl* pTideWindowImpl = (GtkTideWindowImpl*)data;
	pTideWindowImpl->m_bMouseMoved = false;
	if (GDK_BUTTON_PRESS == event->type)
	{
		pTideWindowImpl->m_iLastX = (gint)event->x;
		pTideWindowImpl->m_iLastY = (gint)event->y;

		pTideWindowImpl->m_iPressX = (gint)event->x;
		pTideWindowImpl->m_iPressY = (gint)event->y;
	}

	pTideWindowImpl->StopTimeoutSmoothScrollSlowdown();

	gettimeofday(&pTideWindowImpl->last_motion_time,NULL);

	g_list_foreach(pTideWindowImpl->velocity_time_list, (GFunc)g_free, NULL);
	g_list_free(pTideWindowImpl->velocity_time_list);
	pTideWindowImpl->velocity_time_list = NULL;
	
	return TRUE;
}

gboolean
motion_notify_event_callback (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
	GtkTideWindowImpl* pTideWindowImpl = (GtkTideWindowImpl*)data;

	GdkModifierType state;
	gint x =0, y=0;

	if (event->is_hint)
	{
		gdk_window_get_pointer (event->window, &x, &y, &state);
	}		
	else
	{
		x = (gint)event->x;
		y = (gint)event->y;
		state = (GdkModifierType)event->state;
		
	}

	// if mouse has moved more than 20px since button press
	// consider it a mouse move
	if ( 20 < abs(pTideWindowImpl->m_iPressX - x)
		|| 20 < abs(pTideWindowImpl->m_iPressY - y) )
	{
		pTideWindowImpl->m_bMouseMoved = true;
	}

	gint diff = pTideWindowImpl->m_iLastX - x;
	unsigned ival;

	if (0 > diff)
	{
		ival = (unsigned)(-1*diff);
	}
	else
	{
		ival = diff;
	}

	Interval interval = pTideWindowImpl->m_pGraph->getIntervalOffsetAtPosition(ival); 

	Timestamp ts = pTideWindowImpl->m_pGraph->getNominalStartTime();
	

	if (0 > diff)
	{
		ts -= interval;
	}
	else
	{
		ts += interval;
	}

	pTideWindowImpl->m_pGraph->setNominalStartTime(ts);
	
	
	if (TRUE) // smooth drag slowdown
	{
				
		struct timeval new_motion_time = {0};
		gettimeofday(&new_motion_time,NULL);
		gdouble old_time = (gdouble)pTideWindowImpl->last_motion_time.tv_sec + ((gdouble)pTideWindowImpl->last_motion_time.tv_usec)/1000000;
		gdouble new_time = (gdouble)new_motion_time.tv_sec + ((gdouble)new_motion_time.tv_usec)/1000000;
		
		// velocity pixels per second
		// max velocity should be around +/-12000 pps
#define MAX_VELOCITY 12000
		VelocityTimeStruct* vt = (VelocityTimeStruct*)g_malloc(sizeof(VelocityTimeStruct));
		
		vt->time = new_time - old_time;
		vt->hvelocity = (gint)((x - pTideWindowImpl->m_iLastX) / vt->time);
		vt->hvelocity = MIN(MAX_VELOCITY,vt->hvelocity);
		vt->hvelocity = MAX(-MAX_VELOCITY,vt->hvelocity);
		
		vt->vvelocity =  (gint)((y - pTideWindowImpl->m_iLastY) / vt->time);
		vt->vvelocity = MIN(MAX_VELOCITY,vt->vvelocity);
		vt->vvelocity = MAX(-MAX_VELOCITY,vt->vvelocity);		

		if ( 3 == g_list_length(pTideWindowImpl->velocity_time_list) )
		{
			GList* last = g_list_last(pTideWindowImpl->velocity_time_list);
			pTideWindowImpl->velocity_time_list = 
				g_list_remove_link(pTideWindowImpl->velocity_time_list,last);	
		}
		pTideWindowImpl->velocity_time_list = 
			g_list_prepend(pTideWindowImpl->velocity_time_list, vt);
		
		pTideWindowImpl->last_motion_time = new_motion_time;
	}


	pTideWindowImpl->m_iLastX = x;
	pTideWindowImpl->m_iLastY = y;

	return TRUE;

}

static gboolean event_delete(GtkWidget *widget, GdkEvent *event, gpointer data )
{
	gboolean return_value = FALSE;
	
    return return_value;
}


static void event_destroy( GtkWidget *widget, gpointer   data )
{
	gtk_main_quit();
}


 

gboolean
expose_event_callback (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	GtkTideWindowImpl* pTideWindowImpl = (GtkTideWindowImpl*)data;
	if (NULL == pTideWindowImpl->m_pGraph)
		return TRUE;

	Shape::Region region;

	GdkRectangle *rects = NULL;
	gint n_rects = 0;
	gdk_region_get_rectangles(event->region, &rects, &n_rects);
	for (int i = 0; i < n_rects; ++i)
	{
		region.addRectangle( GtkRect (rects[i]) );
	}
	g_free(rects);


	pTideWindowImpl->m_pGraph->drawTides(region);

	return TRUE;
}

void size_allocate_callback (GtkWidget *widget, GtkAllocation *allocation, gpointer data)
{
	GtkTideWindowImpl* pTideWindowImpl = (GtkTideWindowImpl*)data;
	if (NULL == pTideWindowImpl->m_pGraph)
		return;
	pTideWindowImpl->m_pGraph->setSize(widget->allocation.width, widget->allocation.height);
}

GtkTideWindow::GtkTideWindow()
{
	m_pGtkTideWindowImpl = new GtkTideWindowImpl();
}

GtkTideWindow::~GtkTideWindow()
{
	delete m_pGtkTideWindowImpl;
}

GtkTideWindowImpl::GtkTideWindowImpl() :
	m_bStationsLoaded(false),
	m_pGraph(NULL), 
	m_pStationRef(NULL)
{
	m_iLastX = 0;
	m_iLastY = 0;
	m_iPressX = 0;
	m_iPressY = 0;

	m_pThreadPool = g_thread_pool_new(thread_pool_func, this, 4, FALSE, NULL);

#ifdef MAEMO
	/* Initialize maemo application */
	osso_context = osso_initialize("org.yi.mike.gtktide", PACKAGE_VERSION, TRUE, NULL);
#endif

	m_bMouseMoved = false;

	timeout_id_smooth_scroll = 0;
	timeout_id_smooth_scroll_slowdown = 0;

	velocity_time_list = NULL;

	srand (time (NULL));
	Global::initCodeset();
	Global::settings["tf"].s = "%T %Z";
	Global::settings["hf"].s = "%H";
	Global::settings["fb"].c = 'y';

	for (unsigned int i = 0; i < Colors::numColors; ++i)
	{

		Colors::parseColor (Global::settings[Colors::colorarg[(Colors::Colorchoice)i]].s,
			cachedColors[i].r,cachedColors[i].g,cachedColors[i].b);
		cachedColors[i].gdk_color.red = cachedColors[i].r * 0xFF;
		cachedColors[i].gdk_color.green = cachedColors[i].g * 0xff;
		cachedColors[i].gdk_color.blue = cachedColors[i].b * 0xff;
	}

	m_pComboRecent = gtk_combo_box_new_text();

#ifndef MAEMO
	m_pWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#else
	HildonProgram* program;
	program = HILDON_PROGRAM(hildon_program_get_instance());

	g_set_application_name("gtktide");

	m_pWindow = hildon_window_new();
	hildon_program_add_window(program, HILDON_WINDOW(m_pWindow));
	
#ifdef HAVE_HILDON_1
	GtkSettings* settings = gtk_settings_get_default();
	g_object_set(settings, "gtk-enable-accels",TRUE,NULL);
#endif
#endif

	gtk_window_set_default_icon_name ("xtide");	

	/* set up GtkUIManager */
	GError *tmp_error;
	tmp_error = NULL;

	m_pUIManager = gtk_ui_manager_new();
#ifndef MAEMO
	gtk_ui_manager_set_add_tearoffs (m_pUIManager,TRUE);
#endif

	gtk_ui_manager_add_ui_from_string(m_pUIManager,
			sz_base_ui,
			strlen(sz_base_ui),
			&tmp_error);
	

	guint n_entries = G_N_ELEMENTS (action_entries);

	
	GtkActionGroup* actions = gtk_action_group_new ("GlobalActions");
	
	gtk_action_group_add_actions(actions, action_entries, n_entries, this);
                                 
	gtk_action_group_add_toggle_actions(actions,
										action_entries_toggle, 
										G_N_ELEMENTS (action_entries_toggle),
										this);

	gtk_ui_manager_insert_action_group (m_pUIManager,actions,0);

#ifndef MAEMO
	m_pMenubar = gtk_ui_manager_get_widget(m_pUIManager,"/ui/MenubarMain/");
#else
	m_pMenubar = gtk_menu_item_get_submenu (GTK_MENU_ITEM (gtk_ui_manager_get_widget (m_pUIManager, "/ui/MenubarMain/MenuMain")));
#endif
	m_pToolbar = gtk_ui_manager_get_widget(m_pUIManager,"/ui/ToolbarMain/");
	if (NULL != m_pToolbar)
	{
		gtk_toolbar_set_style(GTK_TOOLBAR(m_pToolbar),GTK_TOOLBAR_ICONS);
#ifdef MAEMO
		gtk_toolbar_set_tooltips(GTK_TOOLBAR(m_pToolbar),FALSE);
#endif
		m_pToolItemStations = gtk_tool_item_new();
		GtkWidget* alignment = gtk_alignment_new(1.,5.,0.,1.);
		gtk_widget_set_size_request(m_pComboRecent, 250, -1); 
		gtk_container_add(GTK_CONTAINER(alignment), m_pComboRecent);
		gtk_container_add(GTK_CONTAINER(m_pToolItemStations), alignment);
		gtk_widget_show_all(GTK_WIDGET(m_pToolItemStations));
		gtk_tool_item_set_expand(m_pToolItemStations, TRUE);
		gtk_toolbar_insert(GTK_TOOLBAR(m_pToolbar),m_pToolItemStations,-1);
	}

	GtkWidget* m_pVBox = gtk_vbox_new(FALSE, 0);
#ifdef MAEMO
	g_object_ref_sink(m_pMenubar);
	hildon_program_set_common_menu (program, GTK_MENU(m_pMenubar));
	g_object_unref(m_pMenubar);
	hildon_program_set_common_toolbar (program, GTK_TOOLBAR(m_pToolbar));
#else
	gtk_box_pack_start (GTK_BOX (m_pVBox), m_pMenubar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (m_pVBox), m_pToolbar, FALSE, FALSE, 0);
#endif

	gtk_window_add_accel_group (GTK_WINDOW(m_pWindow),
								gtk_ui_manager_get_accel_group(m_pUIManager));

	/* end setup GtkUIManager */

	m_pDrawingArea = gtk_drawing_area_new();
	//GTK_WIDGET_SET_FLAGS(m_pDrawingArea, GTK_CAN_FOCUS);
	//gtk_widget_set_double_buffered(m_pDrawingArea, false);
	
	gtk_widget_add_events(m_pDrawingArea, (GdkEventMask)
			( GDK_BUTTON1_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
			  GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK)
			);

	m_ulComboHandlerID = g_signal_connect(m_pComboRecent,
		"changed",(GCallback)combo_changed,this);

    g_signal_connect (G_OBJECT (m_pWindow), "delete_event",
    			G_CALLBACK (event_delete), this);

	g_signal_connect (G_OBJECT (m_pWindow), "destroy",
				G_CALLBACK (event_destroy), this);
	
	
	g_signal_connect (G_OBJECT (m_pDrawingArea), "expose_event",  
		G_CALLBACK (expose_event_callback), this);

	g_signal_connect (G_OBJECT (m_pDrawingArea), "size_allocate",  
		G_CALLBACK (size_allocate_callback), this);

	g_signal_connect (G_OBJECT (m_pDrawingArea), "motion_notify_event",  
		G_CALLBACK (motion_notify_event_callback), this);

	g_signal_connect (G_OBJECT (m_pDrawingArea), "button_press_event",  
		G_CALLBACK (button_press_event_callback), this);

	g_signal_connect (G_OBJECT (m_pDrawingArea), "button_release_event",  
		G_CALLBACK (button_release_event_callback), this);
	//gtk_widget_set_size_request (m_pDrawingArea, 600, 400);

	gtk_box_pack_start (GTK_BOX(m_pVBox), m_pDrawingArea, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(m_pWindow),m_pVBox);

	gtk_window_set_default_size (GTK_WINDOW(m_pWindow),600,400);


	PangoContext* context = gtk_widget_get_pango_context(m_pDrawingArea);

	gtk_widget_grab_focus(m_pDrawingArea);
	gtk_widget_show_all(m_pWindow);

	m_pGraph = new gtkGraph(m_pDrawingArea->window, context);
	//m_pGraph->setDrawTitle(false);
	m_pGraph->setDrawTitleOverGraph(false);
	m_pGraph->setDrawDepthOverGraph(false);
	m_pGraph->setDrawHoursOverGraph(false);
	m_pGraph->setSize(m_pDrawingArea->allocation.width, m_pDrawingArea->allocation.height);
	((gtkGraph*)m_pGraph)->setDrawable(m_pDrawingArea->window);
	//m_pGraph->setStation(pStation);
	//

	g_thread_pool_push(m_pThreadPool, (gpointer)load_stations, NULL);

	bool bDisclaimerDisabled = Global::disclaimerDisabled();
	if (!bDisclaimerDisabled)
	{
		DisclaimerDlg disclaimerDlg(this);
		disclaimerDlg.Run();
	}

}

GtkTideWindowImpl::~GtkTideWindowImpl()
{
	StopTimeoutSmoothScroll();
	StopTimeoutSmoothScrollSlowdown();

	g_list_foreach(velocity_time_list, (GFunc)g_free, NULL);
	g_list_free(velocity_time_list);
	velocity_time_list = NULL;
#ifdef MAEMO
	if (NULL != osso_context)
	{
		osso_deinitialize(osso_context);
	}
#endif

	g_thread_pool_free(m_pThreadPool, TRUE, TRUE);
}

void GtkTideWindowImpl::StartTimeoutSmoothScroll()
{
	if (0 == timeout_id_smooth_scroll)
	{
		timeout_id_smooth_scroll = 
			g_timeout_add(35, timeout_smooth_scroll,this);
	}
}

void GtkTideWindowImpl::StopTimeoutSmoothScroll()
{
	if (0 != timeout_id_smooth_scroll)
	{
		g_source_remove(timeout_id_smooth_scroll);
		timeout_id_smooth_scroll = 0;
	}
}

void GtkTideWindowImpl::StopTimeoutSmoothScrollSlowdown()
{
	if (0 != timeout_id_smooth_scroll_slowdown)
	{
		g_source_remove(timeout_id_smooth_scroll_slowdown);
		timeout_id_smooth_scroll_slowdown = 0;
	}
}

void GtkTideWindowImpl::LoadStations()
{
	StationIndex &stations = Global::stationIndex();
	for (unsigned long i=0; i < stations.size(); ++i)
	{
		StationRef* stationRef = stations[i];
		string strName = stationRef->name.aschar();
		string::size_type pos = strName.find_last_of(',');

		Dstr country = stationRef->country;

		if (string::npos != pos && strName.size()-1 != pos)
		{
			string strLoc  = strName.substr(0,pos);
			string strArea = strName.substr(pos+1, strName.size()-1 - pos);

			Dstr loc = strLoc.c_str();
			loc.trim();
			Dstr area = strArea.c_str();
			area.trim();

			m_mapStationRefCache[country.utf8()][area.utf8()][loc.utf8()]= stationRef;
		}
		else
		{
			Dstr name = stationRef->name;
			m_mapStationRefCache[country]["Unknown"][name.utf8()] = stationRef;
		}
	}

	// possibly set default location
	StationRef *defaultRef =
		stations.getStationRefByName (Global::settings["dl"].s);

	gdk_threads_enter();
	SetStationRef(defaultRef);
	gdk_threads_leave();

	m_bStationsLoaded = true;
}

void GtkTideWindowImpl::SetStationRef(StationRef* stationRef)
{
	Configurable &c = Global::settings["rl"];
	if (NULL != stationRef)
	{
		m_pStationRef = stationRef;

		// FIXME hardcoded to meters
		Station* pStation = m_pStationRef->load();
		pStation->setUnits(Units::meters);
		m_pGraph->setStation(pStation);

		Dstr name = "gtktide - ";
		name += m_pStationRef->name;
		gtk_window_set_title (GTK_WINDOW(m_pWindow), name.utf8().aschar());

		Global::settings["dl"].s = m_pStationRef->name;
		Global::settings["dl"].isNull = false;

		// update recent location list
		while (9 < c.v.size())
		{
			c.v.pop_back();
		}
		// remove dups
		c.v.erase(
			remove(c.v.begin(), c.v.end(), m_pStationRef->name),
			c.v.end());

		// inserting:
		c.v.insert(
			c.v.begin(), m_pStationRef->name);
		c.isNull = false;


	}

	// update the recent combo
	g_signal_handler_block(m_pComboRecent, m_ulComboHandlerID);
	gtk_list_store_clear(
		GTK_LIST_STORE(
			gtk_combo_box_get_model(
				GTK_COMBO_BOX(m_pComboRecent))));
	for (unsigned int i = 0 ; i < c.v.size(); ++i)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(m_pComboRecent),
				c.v[i].aschar());
		if (NULL != m_pStationRef &&
				m_pStationRef->name == c.v[i])
		{
			// dont signal a change 
			gtk_combo_box_set_active(GTK_COMBO_BOX(m_pComboRecent),i);
		}
	}
	g_signal_handler_unblock(m_pComboRecent, m_ulComboHandlerID);
}


int main (int argc, char* argv[])
{
	g_thread_init(NULL);
	gdk_threads_init();

	gtk_init(&argc, &argv);

	//Global::settings.applyXResources (getResource);
	Global::settings.applyUserDefaults();
	Global::settings.applyCommandLine (argc, argv);
	Global::settings.fixUpDeprecatedSettings();

	//printf("location %s\n", Global::settings["l"].v[0].aschar());
	// const StationRef *sr (Global::stationIndex().getStationRefByName(name));
	
	gdk_threads_enter();
	GtkTideWindow myTideWindow;
	gtk_main();
	gdk_threads_leave();

	Global::settings.save();

	return 0;
}



static void action_handler_cb(GtkAction *action, gpointer data)
{
	GtkTideWindowImpl *pTideWindowImpl = (GtkTideWindowImpl*)data;
	
	const gchar * szAction = gtk_action_get_name(action);

	if (0 == strcmp(szAction,ACTION_FULLSCREEN) 
		|| 0 == strcmp(szAction,ACTION_FULLSCREEN_F11)
#ifdef MAEMO
		|| 0 == strcmp(szAction,ACTION_FULLSCREEN_MAEMO) 
#endif
	   )
	{
		if (GDK_WINDOW_STATE_FULLSCREEN & gdk_window_get_state(pTideWindowImpl->m_pWindow->window))
		{
			gtk_widget_show(pTideWindowImpl->m_pToolbar);
			gtk_widget_show(pTideWindowImpl->m_pMenubar);
			gtk_window_unfullscreen(GTK_WINDOW(pTideWindowImpl->m_pWindow));
		}
		else
		{
			gtk_widget_hide(pTideWindowImpl->m_pToolbar);
			gtk_widget_hide(pTideWindowImpl->m_pMenubar);
			gtk_window_fullscreen(GTK_WINDOW(pTideWindowImpl->m_pWindow));
		}
		
	}
	else if (0 == strcmp(szAction,ACTION_CHOOSE_LOCATION) )
	{
		LocationSelectDlg dlg(pTideWindowImpl);
		dlg.Run();
		StationRef* newStation = dlg.GetLocation();
		if (NULL != newStation)
		{
			pTideWindowImpl->SetStationRef(newStation);
		}
	}
	else if (0 == strcmp(szAction,ACTION_CHOOSE_DATE) )
	{
		DateSelectDlg dlg(pTideWindowImpl);
		Timestamp ts_current = pTideWindowImpl->m_pGraph->getNominalStartTime();
		time_t timet = ts_current.timet();
		struct tm tm_cur = *localtime(&timet);
		
		dlg.SetDate(tm_cur.tm_year+1900, tm_cur.tm_mon, tm_cur.tm_mday);
		if ( dlg.Run() )
		{
			unsigned int year, month, day;
			dlg.GetDate(year, month, day);
			time_t date;

			tm tm_time = {0};
			tm_time.tm_year = year -1900;
			tm_time.tm_mon = month;
			tm_time.tm_mday = day;
			tm_time.tm_isdst = -1;
			
			date = mktime(&tm_time);

			Timestamp ts(date);

			Interval diff = abs(ts - ts_current);
			// 5 days = 5 * 24 * 60 *60

			if (diff.s() > 5*24*60*60) 
			{
				pTideWindowImpl->m_pGraph->clearTideEventsOrganizer();
			}

			pTideWindowImpl->m_pGraph->setNominalStartTime(Timestamp(date));
		}
	}
	else if (0 == strcmp(szAction,ACTION_VIEW_TODAY) )
	{
		Timestamp tsNominal(time(NULL));
		Timestamp tsStartTime = 
			pTideWindowImpl->m_pGraph->getStartTime(tsNominal);

		Interval nominalOffset = tsNominal - tsStartTime;
		Interval centerOffset  = 
			pTideWindowImpl->m_pGraph->getIntervalOffsetAtPosition(
					pTideWindowImpl->m_pDrawingArea->allocation.width/2); 

		Timestamp tsCenter = tsNominal;// - centerOffset + nominalOffset;

		// stop timeouts
		pTideWindowImpl->StopTimeoutSmoothScroll();
		pTideWindowImpl->StopTimeoutSmoothScrollSlowdown();
		Interval diff = tsCenter - pTideWindowImpl->m_pGraph->getNominalStartTime();
		Interval increment = pTideWindowImpl->m_pGraph->getIntervalOffsetAtPosition(1); 
		double offset = diff / increment;
		if (offset < 0)
		{
			pTideWindowImpl->m_ScrollToOffset = (int)(offset - .5);
		}
		else
		{
			pTideWindowImpl->m_ScrollToOffset = (int)(offset + .5);
		}

		pTideWindowImpl->StartTimeoutSmoothScroll();
	}
	else if (0 == strcmp(szAction,ACTION_QUIT) || 
		0 == strcmp(szAction, ACTION_QUIT_2) )
	{
		gtk_main_quit();
	}
}	
