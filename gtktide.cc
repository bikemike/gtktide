#include <config.h>

#include <stdio.h>
#include <gtk/gtk.h>

#include "common.hh"

#include <sys/time.h>

#include "Graph.hh"



#ifdef MAEMO
#ifdef HAVE_HILDON_1
#include <hildon/hildon-program.h>
#else
#include <hildon-widgets/hildon-program.h>
#endif
#include <libosso.h>
#endif

using namespace Shape;

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

class GtkTideWindow
{
public:
	GtkTideWindow();
	~GtkTideWindow();

private:
	GtkTideWindowImpl* m_pGtkTideWindowImpl;
};

class GtkTideWindowImpl
{
public:
	GtkTideWindowImpl();
	~GtkTideWindowImpl();

	void StartTimeoutSmoothScroll();
	void StopTimeoutSmoothScroll();
	void StopTimeoutSmoothScrollSlowdown();

	GtkWidget*       m_pWindow;
	GtkWidget*       m_pMenubar;
	GtkWidget*       m_pToolbar;
	GtkWidget*       m_pDrawingArea;
	GtkWidget*       m_pVBox;
	GtkWidget*       m_pComboStations;
	GtkToolItem*     m_pToolItemStations;

	GtkUIManager*    m_pUIManager;
	//gui actions
	static GtkActionEntry action_entries[];
	static GtkToggleActionEntry action_entries_toggle[];


	gint             m_iLastX;
	gint             m_iLastY;
	gint             m_iPressX;
	gint             m_iPressY;

	bool             m_bMouseMoved;
	gint             m_ScrollToOffset;

	guint            timeout_id_smooth_scroll;
	guint            timeout_id_smooth_scroll_slowdown;

	GList*           velocity_time_list;
	struct timeval   last_motion_time;
#ifdef MAEMO
	osso_context_t*  osso_context;
#endif

	Graph*        m_pGraph;
	
};

#define ACTION_QUIT                  "Quit"
#define ACTION_QUIT_2                "Quit_2"
#define ACTION_FULLSCREEN            "FullScreen"
#define ACTION_FULLSCREEN_F11        "FullScreenF11"
#define ACTION_VIEW_TODAY            "ViewToday"
#ifdef MAEMO
#define ACTION_FULLSCREEN_MAEMO      ACTION_FULLSCREEN"_MAEMO"
#endif


static char* sz_base_ui =
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
		printf("draw line\n");
	}

	virtual void drawPolygon(const std::vector<Shape::Point>& points, Colors::Colorchoice c, bool filled = false)
	{
		GdkPoint* gdk_points = new GdkPoint[points.size()];
		for (int i = 0; i < points.size(); ++i)
		{
			gdk_points[i].x = points[i].x;
			gdk_points[i].y = points[i].y;
		}
		ParsedColor parsedColor = cachedColors[c];
		gdk_gc_set_rgb_fg_color(gc,&parsedColor.gdk_color);
		gdk_draw_polygon(drawable, gc, FALSE, gdk_points, points.size());
		if (filled)
			gdk_draw_polygon(drawable, gc, TRUE, gdk_points, points.size());
		delete [] gdk_points;
	}

	void processUpdates()
	{
		gdk_window_process_updates(drawable, FALSE);
	}

protected:
	GdkGC*              gc;
	PangoContext*       pango_context;
	PangoLayout*        pango_layout;

	const bool          antiAliasingDisabled;
	GdkDrawable*        drawable;
	GdkPixbuf*          pixbufCache;


	

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
	pixbufCache(NULL)

{
	//assert (xSize >= Global::minGraphWidth && ySize >= Global::minGraphHeight);
	if (NULL != pango_context)
	{
		pango_layout = pango_layout_new(pango_context);
	}

	if (NULL != drawable)
		gc = gdk_gc_new(drawable);
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
	pango_layout_get_pixel_size(pango_layout, &width, NULL);
	return width;
}


const unsigned gtkGraph::fontHeight() const 
{
	int height;
	pango_layout_get_pixel_size(pango_layout, NULL, &height);
	return height;
}


void gtkGraph::setPixel (int x, int y, Colors::Colorchoice c) 
{
	if (x < 0 || x >= (int)_xSize || y < 0 || y >= (int)_ySize)
		return;
	uint8_t r,g,b;
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
	//printf("set pixel!\n");

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

	GtkWidget* widget = pTideWindowImpl->m_pDrawingArea;

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

	if (widget == GTK_COMBO_BOX(pTideWindowImpl->m_pComboStations))
	{
		gchar* str_selected = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));

		StationIndex &stations = Global::stationIndex();
		for (unsigned long a=0; a<stations.size(); ++a)
		{
			Dstr name = stations[a]->name;

			if ( 0 == strcmp(str_selected, name.aschar()))
			{
				Station* pStation = stations[a]->load();
				pStation->setUnits(Units::meters);
				pTideWindowImpl->m_pGraph->setStation(pStation);
			}

		}
		g_free(str_selected);
	}
	
}



static gboolean 
timeout_smooth_scroll(gpointer data)
{
	GtkTideWindowImpl* pTideWindowImpl = (GtkTideWindowImpl*)data;
	GtkWidget* widget = pTideWindowImpl->m_pDrawingArea;

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

	pTideWindowImpl->m_pGraph->setNominalStartTime(ts);

	if (0 == pTideWindowImpl->m_ScrollToOffset)
	{
		pTideWindowImpl->timeout_id_smooth_scroll = 0;
		return FALSE;
	}
	return TRUE;
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
		unsigned ival;

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

	Region region;

	GdkRectangle *rects = NULL;
	gint n_rects = 0;
	gdk_region_get_rectangles(event->region, &rects, &n_rects);
	//printf("rects: %d\n", n_rects);
	for (int i = 0; i < n_rects; ++i)
	{
		//printf("clip area: %d %d %d %d\n", r.x, r.y, r.width, r.height);
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
	//printf("setting size: %d %d\n", widget->allocation.width, widget->allocation.height);
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
	m_pGraph(NULL)
{
	m_iLastX = 0;
	m_iLastY = 0;
	m_iPressX = 0;
	m_iPressY = 0;


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
	StationIndex &stations = Global::stationIndex();

	for (int i = 0; i < Colors::numColors; ++i)
	{

		Colors::parseColor (Global::settings[Colors::colorarg[(Colors::Colorchoice)i]].s,
			cachedColors[i].r,cachedColors[i].g,cachedColors[i].b);
		cachedColors[i].gdk_color.red = cachedColors[i].r * 0xFF;
		cachedColors[i].gdk_color.green = cachedColors[i].g * 0xff;
		cachedColors[i].gdk_color.blue = cachedColors[i].b * 0xff;
	}

	m_pComboStations = gtk_combo_box_new_text();

	Station* pStation = NULL;
	for (unsigned long a=0; a<stations.size(); ++a)
	{
		

		//printf("stn: %s %s\n", stations[a]->timezone.aschar(), stations[a]->name.aschar());
		//if (NULL == station && 0 == strcmp("Victoria, British Columbia", stations[a]->name.aschar()))
		//if (stations[a]->name.aschar()[0] == 'P')
		//printf("Station Details:\n%s\n", stations[a]->name.aschar());

		if (0 == strcmp("Puerto Vallarta, Jalisco, Mexico", stations[a]->name.aschar()))
		{
			Dstr name = stations[a]->name;
			gtk_combo_box_append_text(GTK_COMBO_BOX(m_pComboStations), name.utf8().aschar());
		}

		if (NULL == pStation && 0 == strcmp("Point No Point, British Columbia", stations[a]->name.aschar()))
		{
			Dstr name = stations[a]->name;
			gtk_combo_box_append_text(GTK_COMBO_BOX(m_pComboStations), name.utf8().aschar());
			pStation = stations[a]->load();
			pStation->setUnits(Units::meters);
			Dstr about;
			pStation->aboutMode (about, Format::text, Global::codeset);
			
			//gtk_combo_box_set_active(GTK_COMBO_BOX(m_pComboStations), a);
			gtk_combo_box_set_active(GTK_COMBO_BOX(m_pComboStations), 0);
			//printf("Station Details:\n%s\n", about.aschar());
		}

	}

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
		gtk_widget_set_size_request(m_pComboStations, 250, -1); 
		gtk_container_add(GTK_CONTAINER(alignment), m_pComboStations);
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
	//gtk_widget_set_double_buffered(m_pDrawingArea, false);
	
	gtk_widget_add_events(m_pDrawingArea, (GdkEventMask)
			( GDK_BUTTON1_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
			  GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK)
			);

	g_signal_connect(m_pComboStations,
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

	gtk_widget_show_all(m_pWindow);

	m_pGraph = new gtkGraph(m_pDrawingArea->window, context);
	m_pGraph->setSize(m_pDrawingArea->allocation.width, m_pDrawingArea->allocation.height);
	((gtkGraph*)m_pGraph)->setDrawable(m_pDrawingArea->window);
	m_pGraph->setStation(pStation);
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

int main (int argc, char* argv[])
{
	gtk_init(&argc, &argv);

	GtkTideWindow myTideWindow;

	gtk_main();

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
