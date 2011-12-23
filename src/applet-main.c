/*
A small wrapper utility to load indicators and put them as menu items
into the mate-panel using it's applet interface.

Copyright 2009-2010 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License version 3, as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranties of
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <glib/gi18n.h>
#include <mate-panel-applet.h>
#include <gdk/gdkkeysyms.h>

#include "libindicator/indicator-object.h"
#include "tomboykeybinder.h"

static gchar * indicator_order[] = {
	"libapplication.so",
	"libsoundmenu.so",
	"libmessaging.so",
	"libdatetime.so",
	"libme.so",
	"libsession.so",
	NULL
};

static GtkPackDirection packdirection;
static MatePanelAppletOrient orient;

#define  MENU_DATA_INDICATOR_OBJECT  "indicator-object"
#define  MENU_DATA_INDICATOR_ENTRY   "indicator-entry"

#define  IO_DATA_ORDER_NUMBER        "indicator-order-number"

static gboolean     applet_fill_cb (MatePanelApplet * applet, const gchar * iid, gpointer data);

static void cw_panel_background_changed (MatePanelApplet               *applet,
                                         MatePanelAppletBackgroundType  type,
                        				         GdkColor                  *colour,
                        				         GdkPixmap                 *pixmap,
                                         GtkWidget                 *menubar);
static void update_accessible_desc (IndicatorObjectEntry * entry, GtkWidget * menuitem);

/*************
 * main
 * ***********/

#ifdef INDICATOR_APPLET
MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("IndicatorAppletFactory",
               PANEL_TYPE_APPLET,
               "indicator-applet",
               applet_fill_cb, NULL);
#endif
#ifdef INDICATOR_APPLET_SESSION
MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("FastUserSwitchAppletFactory",
               PANEL_TYPE_APPLET,
               "indicator-applet-session",
               applet_fill_cb, NULL);
#endif
#ifdef INDICATOR_APPLET_COMPLETE
MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("IndicatorAppletCompleteFactory",
               PANEL_TYPE_APPLET,
               "indicator-applet-complete",
               applet_fill_cb, NULL);
#endif
#ifdef INDICATOR_APPLET_APPMENU
MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("IndicatorAppletAppmenuFactory",
               PANEL_TYPE_APPLET,
               "indicator-applet-appmenu",
               applet_fill_cb, NULL);
#endif

/*************
 * log files
 * ***********/
#ifdef INDICATOR_APPLET
#define LOG_FILE_NAME  "indicator-applet.log"
#endif
#ifdef INDICATOR_APPLET_SESSION
#define LOG_FILE_NAME  "indicator-applet-session.log"
#endif
#ifdef INDICATOR_APPLET_COMPLETE
#define LOG_FILE_NAME  "indicator-applet-complete.log"
#endif
#ifdef INDICATOR_APPLET_APPMENU
#define LOG_FILE_NAME  "indicator-applet-appmenu.log"
#endif
GOutputStream * log_file = NULL;

/*****************
 * Hotkey support 
 * **************/
#ifdef INDICATOR_APPLET
gchar * hotkey_keycode = "<Super>M";
#endif
#ifdef INDICATOR_APPLET_SESSION
gchar * hotkey_keycode = "<Super>S";
#endif
#ifdef INDICATOR_APPLET_COMPLETE
gchar * hotkey_keycode = "<Super>S";
#endif
#ifdef INDICATOR_APPLET_APPMENU
gchar * hotkey_keycode = "<Super>F1";
#endif

/********************
 * Environment Names
 * *******************/
#ifdef INDICATOR_APPLET
#define INDICATOR_SPECIFIC_ENV  "indicator-applet-original"
#endif
#ifdef INDICATOR_APPLET_SESSION
#define INDICATOR_SPECIFIC_ENV  "indicator-applet-session"
#endif
#ifdef INDICATOR_APPLET_COMPLETE
#define INDICATOR_SPECIFIC_ENV  "indicator-applet-complete"
#endif
#ifdef INDICATOR_APPLET_APPMENU
#define INDICATOR_SPECIFIC_ENV  "indicator-applet-appmenu"
#endif

static const gchar * indicator_env[] = {
	"indicator-applet",
	INDICATOR_SPECIFIC_ENV,
	NULL
};

/*************
 * init function
 * ***********/

static gint
name2order (const gchar * name) {
	int i;

	for (i = 0; indicator_order[i] != NULL; i++) {
		if (g_strcmp0(name, indicator_order[i]) == 0) {
			return i;
		}
	}

	return -1;
}

typedef struct _incoming_position_t incoming_position_t;
struct _incoming_position_t {
	gint objposition;
	gint entryposition;
	gint menupos;
	gboolean found;
};

/* This function helps by determining where in the menu list
   this new entry should be placed.  It compares the objects
   that they're on, and then the individual entries.  Each
   is progressively more expensive. */
static void
place_in_menu (GtkWidget * widget, gpointer user_data)
{
	incoming_position_t * position = (incoming_position_t *)user_data;
	if (position->found) {
		/* We've already been placed, just finish the foreach */
		return;
	}

	IndicatorObject * io = INDICATOR_OBJECT(g_object_get_data(G_OBJECT(widget), MENU_DATA_INDICATOR_OBJECT));
	g_assert(io != NULL);

	gint objposition = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(io), IO_DATA_ORDER_NUMBER));
	/* We've already passed it, well, then this is where
	   we should be be.  Stop! */
	if (objposition > position->objposition) {
		position->found = TRUE;
		return;
	}

	/* The objects don't match yet, keep looking */
	if (objposition < position->objposition) {
		position->menupos++;
		return;
	}

	/* The objects are the same, let's start looking at entries. */
	IndicatorObjectEntry * entry = (IndicatorObjectEntry *)g_object_get_data(G_OBJECT(widget), MENU_DATA_INDICATOR_ENTRY);
	gint entryposition = indicator_object_get_location(io, entry);

	if (entryposition > position->entryposition) {
		position->found = TRUE;
		return;
	}

	if (entryposition < position->entryposition) {
		position->menupos++;
		return;
	}

	/* We've got the same object and the same entry.  Well,
	   let's just put it right here then. */
	position->found = TRUE;
	return;
}

static void
something_shown (GtkWidget * widget, gpointer user_data)
{
	GtkWidget * menuitem = GTK_WIDGET(user_data);
	gtk_widget_show(menuitem);
}

static void
something_hidden (GtkWidget * widget, gpointer user_data)
{
	GtkWidget * menuitem = GTK_WIDGET(user_data);
	gtk_widget_hide(menuitem);
}

static void
sensitive_cb (GObject * obj, GParamSpec * pspec, gpointer user_data)
{
	g_return_if_fail(GTK_IS_WIDGET(obj));
	g_return_if_fail(GTK_IS_WIDGET(user_data));

	gtk_widget_set_sensitive(GTK_WIDGET(user_data), gtk_widget_get_sensitive(GTK_WIDGET(obj)));
	return;
}

static void
entry_activated (GtkWidget * widget, gpointer user_data)
{
	g_return_if_fail(GTK_IS_WIDGET(widget));
	gpointer pio = g_object_get_data(G_OBJECT(widget), "indicator");
	g_return_if_fail(INDICATOR_IS_OBJECT(pio));
	IndicatorObject * io = INDICATOR_OBJECT(pio);

	return indicator_object_entry_activate(io, (IndicatorObjectEntry *)user_data, gtk_get_current_event_time());
}

static gboolean
entry_scrolled (GtkWidget *menuitem, GdkEventScroll *event, gpointer data)
{
	IndicatorObject *io = g_object_get_data (G_OBJECT (menuitem), MENU_DATA_INDICATOR_OBJECT);
	IndicatorObjectEntry *entry = g_object_get_data (G_OBJECT (menuitem), MENU_DATA_INDICATOR_ENTRY);

	g_return_val_if_fail(INDICATOR_IS_OBJECT(io), FALSE);

	g_signal_emit_by_name (io, "scroll", 1, event->direction);
	g_signal_emit_by_name (io, "scroll-entry", entry, 1, event->direction);

	return FALSE;
}

static void
accessible_desc_update_cb (GtkWidget * widget, gpointer userdata)
{
	gpointer data = g_object_get_data(G_OBJECT(widget), MENU_DATA_INDICATOR_ENTRY);

	if (data != userdata) {
		return;
	}

	IndicatorObjectEntry * entry = (IndicatorObjectEntry *)data;
	update_accessible_desc(entry, widget);
}

static void
accessible_desc_update (IndicatorObject * io, IndicatorObjectEntry * entry, GtkWidget * menubar)
{
	gtk_container_foreach(GTK_CONTAINER(menubar), accessible_desc_update_cb, entry);
	return;
}

static void
entry_added (IndicatorObject * io, IndicatorObjectEntry * entry, GtkWidget * menubar)
{
	g_debug("Signal: Entry Added");
	gboolean something_visible = FALSE;
	gboolean something_sensitive = FALSE;

	GtkWidget * menuitem = gtk_menu_item_new();
	GtkWidget * box = (packdirection == GTK_PACK_DIRECTION_LTR) ?
			gtk_hbox_new(FALSE, 3) : gtk_vbox_new(FALSE, 3);

	g_object_set_data (G_OBJECT (menuitem), "indicator", io);
	g_object_set_data (G_OBJECT (menuitem), "box", box);

	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(entry_activated), entry);
	g_signal_connect(G_OBJECT(menuitem), "scroll-event", G_CALLBACK(entry_scrolled), entry);

	if (entry->image != NULL) {
		gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(entry->image), FALSE, FALSE, 1);
		if (gtk_widget_get_visible(GTK_WIDGET(entry->image))) {
			something_visible = TRUE;
		}

		if (gtk_widget_get_sensitive(GTK_WIDGET(entry->image))) {
			something_sensitive = TRUE;
		}

		g_signal_connect(G_OBJECT(entry->image), "show", G_CALLBACK(something_shown), menuitem);
		g_signal_connect(G_OBJECT(entry->image), "hide", G_CALLBACK(something_hidden), menuitem);

		g_signal_connect(G_OBJECT(entry->image), "notify::sensitive", G_CALLBACK(sensitive_cb), menuitem);
	}
	if (entry->label != NULL) {
		switch(packdirection) {
			case GTK_PACK_DIRECTION_LTR:
				gtk_label_set_angle(GTK_LABEL(entry->label), 0.0);
				break;
			case GTK_PACK_DIRECTION_TTB:
				gtk_label_set_angle(GTK_LABEL(entry->label),
						(orient == MATE_PANEL_APPLET_ORIENT_LEFT) ? 
						270.0 : 90.0);
				break;
			default:
				break;
		}		
		gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(entry->label), FALSE, FALSE, 1);

		if (gtk_widget_get_visible(GTK_WIDGET(entry->label))) {
			something_visible = TRUE;
		}

		if (gtk_widget_get_sensitive(GTK_WIDGET(entry->label))) {
			something_sensitive = TRUE;
		}

		g_signal_connect(G_OBJECT(entry->label), "show", G_CALLBACK(something_shown), menuitem);
		g_signal_connect(G_OBJECT(entry->label), "hide", G_CALLBACK(something_hidden), menuitem);

		g_signal_connect(G_OBJECT(entry->label), "notify::sensitive", G_CALLBACK(sensitive_cb), menuitem);
	}
	gtk_container_add(GTK_CONTAINER(menuitem), box);
	gtk_widget_show(box);

	if (entry->menu != NULL) {
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), GTK_WIDGET(entry->menu));
	}

	incoming_position_t position;
	position.objposition = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(io), IO_DATA_ORDER_NUMBER));
	position.entryposition = indicator_object_get_location(io, entry);
	position.menupos = 0;
	position.found = FALSE;

	gtk_container_foreach(GTK_CONTAINER(menubar), place_in_menu, &position);

	gtk_menu_shell_insert(GTK_MENU_SHELL(menubar), menuitem, position.menupos);

	if (something_visible) {
		if (entry->accessible_desc != NULL) {
			update_accessible_desc(entry, menuitem);
		}
		gtk_widget_show(menuitem);
	}
	gtk_widget_set_sensitive(menuitem, something_sensitive);

	g_object_set_data(G_OBJECT(menuitem), MENU_DATA_INDICATOR_ENTRY,  entry);
	g_object_set_data(G_OBJECT(menuitem), MENU_DATA_INDICATOR_OBJECT, io);

	return;
}

static void
entry_removed_cb (GtkWidget * widget, gpointer userdata)
{
	gpointer data = g_object_get_data(G_OBJECT(widget), MENU_DATA_INDICATOR_ENTRY);

	if (data != userdata) {
		return;
	}

	IndicatorObjectEntry * entry = (IndicatorObjectEntry *)data;
	if (entry->label != NULL) {
		g_signal_handlers_disconnect_by_func(G_OBJECT(entry->label), G_CALLBACK(something_shown), widget);
		g_signal_handlers_disconnect_by_func(G_OBJECT(entry->label), G_CALLBACK(something_hidden), widget);
		g_signal_handlers_disconnect_by_func(G_OBJECT(entry->label), G_CALLBACK(sensitive_cb), widget);
	}
	if (entry->image != NULL) {
		g_signal_handlers_disconnect_by_func(G_OBJECT(entry->image), G_CALLBACK(something_shown), widget);
		g_signal_handlers_disconnect_by_func(G_OBJECT(entry->image), G_CALLBACK(something_hidden), widget);
		g_signal_handlers_disconnect_by_func(G_OBJECT(entry->image), G_CALLBACK(sensitive_cb), widget);
	}

	gtk_widget_destroy(widget);
	return;
}

static void
entry_removed (IndicatorObject * io G_GNUC_UNUSED, IndicatorObjectEntry * entry,
               gpointer user_data)
{
	g_debug("Signal: Entry Removed");

	gtk_container_foreach(GTK_CONTAINER(user_data), entry_removed_cb, entry);

	return;
}

static void
entry_moved_find_cb (GtkWidget * widget, gpointer userdata)
{
	gpointer * array = (gpointer *)userdata;
	if (array[1] != NULL) {
		return;
	}

	gpointer data = g_object_get_data(G_OBJECT(widget), MENU_DATA_INDICATOR_ENTRY);

	if (data != array[0]) {
		return;
	}

	array[1] = widget;
	return;
}

/* Gets called when an entry for an object was moved. */
static void
entry_moved (IndicatorObject * io, IndicatorObjectEntry * entry,
             gint old G_GNUC_UNUSED, gint new G_GNUC_UNUSED, gpointer user_data)
{
	GtkWidget * menubar = GTK_WIDGET(user_data);

	gpointer array[2];
	array[0] = entry;
	array[1] = NULL;

	gtk_container_foreach(GTK_CONTAINER(menubar), entry_moved_find_cb, array);
	if (array[1] == NULL) {
		g_warning("Moving an entry that isn't in our menus.");
		return;
	}

	GtkWidget * mi = GTK_WIDGET(array[1]);
	g_object_ref(G_OBJECT(mi));
	gtk_container_remove(GTK_CONTAINER(menubar), mi);

	incoming_position_t position;
	position.objposition = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(io), IO_DATA_ORDER_NUMBER));
	position.entryposition = indicator_object_get_location(io, entry);
	position.menupos = 0;
	position.found = FALSE;

	gtk_container_foreach(GTK_CONTAINER(menubar), place_in_menu, &position);

	gtk_menu_shell_insert(GTK_MENU_SHELL(menubar), mi, position.menupos);
	g_object_unref(G_OBJECT(mi));

	return;
}

static void
menu_show (IndicatorObject * io, IndicatorObjectEntry * entry,
           guint32 timestamp, gpointer user_data)
{
	GtkWidget * menubar = GTK_WIDGET(user_data);

	if (entry == NULL) {
		/* Close any open menus instead of opening one */
		GList * entries = indicator_object_get_entries(io);
		GList * entry = NULL;
		for (entry = entries; entry != NULL; entry = g_list_next(entry)) {
			IndicatorObjectEntry * entrydata = (IndicatorObjectEntry *)entry->data;
			gtk_menu_popdown(entrydata->menu);
		}
		g_list_free(entries);

		/* And tell the menubar to exit activation mode too */
		gtk_menu_shell_cancel(GTK_MENU_SHELL(menubar));
		return;
	}

	// TODO: do something sensible here
}

static void
update_accessible_desc(IndicatorObjectEntry * entry, GtkWidget * menuitem)
{
	/* FIXME: We need to deal with the use case where the contents of the
	   label overrides what is found in the atk object's name, or at least
	   orca speaks the label instead of the atk object name.
	 */
	AtkObject * menuitem_obj = gtk_widget_get_accessible(menuitem);
	if (menuitem_obj == NULL) {
		/* Should there be an error printed here? */
		return;
	}

	if (entry->accessible_desc != NULL) {
		atk_object_set_name(menuitem_obj, entry->accessible_desc);
	} else {
		atk_object_set_name(menuitem_obj, "");
	}
	return;
}


static gboolean
load_module (const gchar * name, GtkWidget * menubar)
{
	g_debug("Looking at Module: %s", name);
	g_return_val_if_fail(name != NULL, FALSE);

	if (!g_str_has_suffix(name, G_MODULE_SUFFIX)) {
		return FALSE;
	}

	g_debug("Loading Module: %s", name);

	/* Build the object for the module */
	gchar * fullpath = g_build_filename(INDICATOR_DIR, name, NULL);
	IndicatorObject * io = indicator_object_new_from_file(fullpath);
	g_free(fullpath);

	/* Set the environment it's in */
	indicator_object_set_environment(io, (const GStrv)indicator_env);

	/* Attach the 'name' to the object */
	g_object_set_data(G_OBJECT(io), IO_DATA_ORDER_NUMBER, GINT_TO_POINTER(name2order(name)));

	/* Connect to its signals */
	g_signal_connect(G_OBJECT(io), INDICATOR_OBJECT_SIGNAL_ENTRY_ADDED,   G_CALLBACK(entry_added),    menubar);
	g_signal_connect(G_OBJECT(io), INDICATOR_OBJECT_SIGNAL_ENTRY_REMOVED, G_CALLBACK(entry_removed),  menubar);
	g_signal_connect(G_OBJECT(io), INDICATOR_OBJECT_SIGNAL_ENTRY_MOVED,   G_CALLBACK(entry_moved),    menubar);
	g_signal_connect(G_OBJECT(io), INDICATOR_OBJECT_SIGNAL_MENU_SHOW,     G_CALLBACK(menu_show),      menubar);
	g_signal_connect(G_OBJECT(io), INDICATOR_OBJECT_SIGNAL_ACCESSIBLE_DESC_UPDATE, G_CALLBACK(accessible_desc_update), menubar);

	/* Work on the entries */
	GList * entries = indicator_object_get_entries(io);
	GList * entry = NULL;

	for (entry = entries; entry != NULL; entry = g_list_next(entry)) {
		IndicatorObjectEntry * entrydata = (IndicatorObjectEntry *)entry->data;
		entry_added(io, entrydata, menubar);
	}

	g_list_free(entries);

	return TRUE;
}

static void
hotkey_filter (char * keystring G_GNUC_UNUSED, gpointer data)
{
	g_return_if_fail(GTK_IS_MENU_SHELL(data));

	/* Oh, wow, it's us! */
	GList * children = gtk_container_get_children(GTK_CONTAINER(data));
	if (children == NULL) {
		g_debug("Menubar has no children");
		return;
	}

	if (!GTK_MENU_SHELL(data)->active) {
		gtk_grab_add (GTK_WIDGET(data));
		GTK_MENU_SHELL(data)->have_grab = TRUE;
		GTK_MENU_SHELL(data)->active = TRUE;
	}

	gtk_menu_shell_select_item(GTK_MENU_SHELL(data), GTK_WIDGET(g_list_last(children)->data));
	g_list_free(children);
	return;
}

static gboolean
menubar_press (GtkWidget * widget,
                    GdkEventButton *event,
                    gpointer data G_GNUC_UNUSED)
{
	if (event->button != 1) {
		g_signal_stop_emission_by_name(widget, "button-press-event");
	}

	return FALSE;
}

static gboolean
menubar_on_expose (GtkWidget * widget,
                    GdkEventExpose *event G_GNUC_UNUSED,
                    GtkWidget * menubar)
{
	if (GTK_WIDGET_HAS_FOCUS(menubar))
		gtk_paint_focus(widget->style, widget->window, GTK_WIDGET_STATE(menubar),
		                NULL, widget, "menubar-applet", 0, 0, -1, -1);

	return FALSE;
}

static void
about_cb (GtkAction *action G_GNUC_UNUSED,
          gpointer   data G_GNUC_UNUSED)
{
	static const gchar *authors[] = {
		"Ted Gould <ted@canonical.com>",
		NULL
	};

	static gchar *license[] = {
        N_("This program is free software: you can redistribute it and/or modify it "
           "under the terms of the GNU General Public License version 3, as published "
           "by the Free Software Foundation."),
        N_("This program is distributed in the hope that it will be useful, but "
           "WITHOUT ANY WARRANTY; without even the implied warranties of "
           "MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR "
           "PURPOSE.  See the GNU General Public License for more details."),
        N_("You should have received a copy of the GNU General Public License along "
           "with this program.  If not, see <http://www.gnu.org/licenses/>."),
		NULL
	};
	gchar *license_i18n;

	license_i18n = g_strconcat (_(license[0]), "\n\n", _(license[1]), "\n\n", _(license[2]), NULL);

	gtk_show_about_dialog(NULL,
		"version", VERSION,
		"copyright", "Copyright \xc2\xa9 2009-2010 Canonical, Ltd.",
#ifdef INDICATOR_APPLET_SESSION
		"comments", _("A place to adjust your status, change users or exit your session."),
#else
#ifdef INDICATOR_APPLET_APPMENU
		"comments", _("An applet to hold your application menus."),
#endif
		"comments", _("An applet to hold all of the system indicators."),
#endif
		"authors", authors,
		"license", license_i18n,
		"wrap-license", TRUE,
		"translator-credits", _("translator-credits"),
		"logo-icon-name", "indicator-applet",
		"icon-name", "indicator-applet",
		"website", "http://launchpad.net/indicator-applet",
		"website-label", _("Indicator Applet Website"),
		NULL
	);

	g_free (license_i18n);

	return;
}

static gboolean
swap_orient_cb (GtkWidget *item, gpointer data)
{
	GtkWidget *from = (GtkWidget *) data;
	GtkWidget *to = (GtkWidget *) g_object_get_data(G_OBJECT(from), "to");
	g_object_ref(G_OBJECT(item));
	gtk_container_remove(GTK_CONTAINER(from), item);
	if (GTK_IS_LABEL(item)) {
			switch(packdirection) {
			case GTK_PACK_DIRECTION_LTR:
				gtk_label_set_angle(GTK_LABEL(item), 0.0);
				break;
			case GTK_PACK_DIRECTION_TTB:
				gtk_label_set_angle(GTK_LABEL(item),
						(orient == MATE_PANEL_APPLET_ORIENT_LEFT) ? 
						270.0 : 90.0);
				break;
			default:
				break;
		}		
	}
	gtk_box_pack_start(GTK_BOX(to), item, FALSE, FALSE, 0);
	return TRUE;
}

static gboolean
reorient_box_cb (GtkWidget *menuitem, gpointer data)
{
	GtkWidget *from = g_object_get_data(G_OBJECT(menuitem), "box");
	GtkWidget *to = (packdirection == GTK_PACK_DIRECTION_LTR) ?
			gtk_hbox_new(FALSE, 0) : gtk_vbox_new(FALSE, 0);
	g_object_set_data(G_OBJECT(from), "to", to);
	gtk_container_foreach(GTK_CONTAINER(from), (GtkCallback)swap_orient_cb,
			from);
	gtk_container_remove(GTK_CONTAINER(menuitem), from);
	gtk_container_add(GTK_CONTAINER(menuitem), to);
	g_object_set_data(G_OBJECT(menuitem), "box", to);
	gtk_widget_show_all(menuitem);
	return TRUE;
}

static gboolean
matepanelapplet_reorient_cb (GtkWidget *applet, MatePanelAppletOrient neworient,
		gpointer data)
{
	GtkWidget *menubar = (GtkWidget *)data;
	if ((((neworient == MATE_PANEL_APPLET_ORIENT_UP) || 
			(neworient == MATE_PANEL_APPLET_ORIENT_DOWN)) && 
			((orient == MATE_PANEL_APPLET_ORIENT_LEFT) || 
			(orient == MATE_PANEL_APPLET_ORIENT_RIGHT))) || 
			(((neworient == MATE_PANEL_APPLET_ORIENT_LEFT) || 
			(neworient == MATE_PANEL_APPLET_ORIENT_RIGHT)) && 
			((orient == MATE_PANEL_APPLET_ORIENT_UP) ||
			(orient == MATE_PANEL_APPLET_ORIENT_DOWN)))) {
		packdirection = (packdirection == GTK_PACK_DIRECTION_LTR) ?
				GTK_PACK_DIRECTION_TTB : GTK_PACK_DIRECTION_LTR;
		gtk_menu_bar_set_pack_direction(GTK_MENU_BAR(menubar),
				packdirection);
		orient = neworient;
		gtk_container_foreach(GTK_CONTAINER(menubar),
				(GtkCallback)reorient_box_cb, NULL);
	}
	orient = neworient;
	return FALSE;
}

#ifdef N_
#undef N_
#endif
#define N_(x) x

static void
log_to_file_cb (GObject * source_obj G_GNUC_UNUSED,
                GAsyncResult * result G_GNUC_UNUSED, gpointer user_data)
{
	g_free(user_data);
	return;
}

static void
log_to_file (const gchar * domain G_GNUC_UNUSED,
             GLogLevelFlags level G_GNUC_UNUSED,
             const gchar * message,
             gpointer data G_GNUC_UNUSED)
{
	if (log_file == NULL) {
		GError * error = NULL;
		gchar * filename = g_build_filename(g_get_user_cache_dir(), LOG_FILE_NAME, NULL);
		GFile * file = g_file_new_for_path(filename);
		g_free(filename);

		if (!g_file_test(g_get_user_cache_dir(), G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
			GFile * cachedir = g_file_new_for_path(g_get_user_cache_dir());
			g_file_make_directory_with_parents(cachedir, NULL, &error);

			if (error != NULL) {
				g_error("Unable to make directory '%s' for log file: %s", g_get_user_cache_dir(), error->message);
				return;
			}
		}

		g_file_delete(file, NULL, NULL);

		GFileIOStream * io = g_file_create_readwrite(file,
		                          G_FILE_CREATE_REPLACE_DESTINATION, /* flags */
		                          NULL, /* cancelable */
		                          &error); /* error */
		if (error != NULL) {
			g_error("Unable to replace file: %s", error->message);
			return;
		}

		log_file = g_io_stream_get_output_stream(G_IO_STREAM(io));
	}

	gchar * outputstring = g_strdup_printf("%s\n", message);
	g_output_stream_write_async(log_file,
	                            outputstring, /* data */
	                            strlen(outputstring), /* length */
	                            G_PRIORITY_LOW, /* priority */
	                            NULL, /* cancelable */
	                            log_to_file_cb, /* callback */
	                            outputstring); /* data */

	return;
}

static gboolean
applet_fill_cb (MatePanelApplet * applet, const gchar * iid G_GNUC_UNUSED,
                gpointer data G_GNUC_UNUSED)
{
	static const GtkActionEntry menu_actions[] = {
		{"About", GTK_STOCK_ABOUT, N_("_About"), NULL, NULL, G_CALLBACK(about_cb)}
	};
	static const gchar *menu_xml = "<menuitem name=\"About\" action=\"About\"/>";

	static gboolean first_time = FALSE;
	GtkWidget *menubar;
	gint indicators_loaded = 0;
	GtkActionGroup *action_group;

#ifdef INDICATOR_APPLET_SESSION
	/* check if we are running stracciatella session */
	if (g_strcmp0(g_getenv("MDMSESSION"), "mate-stracciatella") == 0) {
		g_debug("Running stracciatella MATE session, disabling myself");
		return TRUE;
	}
#endif

	if (!first_time)
	{
		first_time = TRUE;
#ifdef INDICATOR_APPLET
		g_set_application_name(_("Indicator Applet"));
#endif
#ifdef INDICATOR_APPLET_SESSION
		g_set_application_name(_("Indicator Applet Session"));
#endif
#ifdef INDICATOR_APPLET_COMPLETE
		g_set_application_name(_("Indicator Applet Complete"));
#endif
#ifdef INDICATOR_APPLET_APPMENU
		g_set_application_name(_("Indicator Applet Application Menu"));
#endif
		
		g_log_set_default_handler(log_to_file, NULL);

		tomboy_keybinder_init();
	}

	/* Set panel options */
	gtk_container_set_border_width(GTK_CONTAINER (applet), 0);
	mate_panel_applet_set_flags(applet, MATE_PANEL_APPLET_EXPAND_MINOR);
	menubar = gtk_menu_bar_new();
	action_group = gtk_action_group_new ("Indicator Applet Actions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (action_group, menu_actions,
	                              G_N_ELEMENTS (menu_actions),
	                              menubar);
	mate_panel_applet_setup_menu(applet, menu_xml, action_group);
	g_object_unref(action_group);
#ifdef INDICATOR_APPLET
	atk_object_set_name (gtk_widget_get_accessible (GTK_WIDGET (applet)),
	                     "indicator-applet");
#endif
#ifdef INDICATOR_APPLET_SESSION
	atk_object_set_name (gtk_widget_get_accessible (GTK_WIDGET (applet)),
	                     "indicator-applet-session");
#endif
#ifdef INDICATOR_APPLET_COMPLETE
	atk_object_set_name (gtk_widget_get_accessible (GTK_WIDGET (applet)),
	                     "indicator-applet-complete");
#endif
#ifdef INDICATOR_APPLET_APPMENU
	atk_object_set_name (gtk_widget_get_accessible (GTK_WIDGET (applet)),
	                     "indicator-applet-appmenu");
#endif

	/* Init some theme/icon stuff */
	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(),
	                                  INDICATOR_ICONS_DIR);
	/* g_debug("Icons directory: %s", INDICATOR_ICONS_DIR); */
	gtk_rc_parse_string (
	    "style \"indicator-applet-style\"\n"
        "{\n"
        "    GtkMenuBar::shadow-type = none\n"
        "    GtkMenuBar::internal-padding = 0\n"
        "    GtkWidget::focus-line-width = 0\n"
        "    GtkWidget::focus-padding = 0\n"
        "}\n"
	    "style \"indicator-applet-menubar-style\"\n"
        "{\n"
        "    GtkMenuBar::shadow-type = none\n"
        "    GtkMenuBar::internal-padding = 0\n"
        "    GtkWidget::focus-line-width = 0\n"
        "    GtkWidget::focus-padding = 0\n"
        "    GtkMenuItem::horizontal-padding = 0\n"
        "}\n"
	    "style \"indicator-applet-menuitem-style\"\n"
        "{\n"
        "    GtkWidget::focus-line-width = 0\n"
        "    GtkWidget::focus-padding = 0\n"
        "    GtkMenuItem::horizontal-padding = 0\n"
        "}\n"
        "widget \"*.fast-user-switch-applet\" style \"indicator-applet-style\""
        "widget \"*.fast-user-switch-menuitem\" style \"indicator-applet-menuitem-style\""
        "widget \"*.fast-user-switch-menubar\" style \"indicator-applet-menubar-style\"");
	//gtk_widget_set_name(GTK_WIDGET (applet), "indicator-applet-menubar");
	gtk_widget_set_name(GTK_WIDGET (applet), "fast-user-switch-applet");

	/* Build menubar */
	orient = (mate_panel_applet_get_orient(applet));
	packdirection = ((orient == MATE_PANEL_APPLET_ORIENT_UP) ||
			(orient == MATE_PANEL_APPLET_ORIENT_DOWN)) ? 
			GTK_PACK_DIRECTION_LTR : GTK_PACK_DIRECTION_TTB;
	gtk_menu_bar_set_pack_direction(GTK_MENU_BAR(menubar),
			packdirection);
	GTK_WIDGET_SET_FLAGS (menubar, GTK_WIDGET_FLAGS(menubar) | GTK_CAN_FOCUS);
	gtk_widget_set_name(GTK_WIDGET (menubar), "fast-user-switch-menubar");
	g_signal_connect(menubar, "button-press-event", G_CALLBACK(menubar_press), NULL);
	g_signal_connect_after(menubar, "expose-event", G_CALLBACK(menubar_on_expose), menubar);
	g_signal_connect(applet, "change-orient", 
			G_CALLBACK(matepanelapplet_reorient_cb), menubar);
	gtk_container_set_border_width(GTK_CONTAINER(menubar), 0);

	/* Add in filter func */
	tomboy_keybinder_bind(hotkey_keycode, hotkey_filter, menubar);

	/* load 'em */
	if (g_file_test(INDICATOR_DIR, (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))) {
		GDir * dir = g_dir_open(INDICATOR_DIR, 0, NULL);

		const gchar * name;
		while ((name = g_dir_read_name(dir)) != NULL) {
#ifdef INDICATOR_APPLET_APPMENU
			if (g_strcmp0(name, "libappmenu.so")) {
				continue;
			}
#else
			if (!g_strcmp0(name, "libappmenu.so")) {
				continue;
			}
#endif
#ifdef INDICATOR_APPLET
			if (!g_strcmp0(name, "libsession.so")) {
				continue;
			}
			if (!g_strcmp0(name, "libme.so")) {
				continue;
			}
			if (!g_strcmp0(name, "libdatetime.so")) {
				continue;
			}
#endif
#ifdef INDICATOR_APPLET_SESSION
			if (g_strcmp0(name, "libsession.so") && g_strcmp0(name, "libme.so")) {
				continue;
			}
#endif
			if (load_module(name, menubar)) {
				indicators_loaded++;
			}
		}
		g_dir_close (dir);
	}

	if (indicators_loaded == 0) {
		/* A label to allow for click through */
		GtkWidget * item = gtk_label_new(_("No Indicators"));
		gtk_container_add(GTK_CONTAINER(applet), item);
		gtk_widget_show(item);
	} else {
		gtk_container_add(GTK_CONTAINER(applet), menubar);
		mate_panel_applet_set_background_widget(applet, menubar);
		gtk_widget_show(menubar);
	}

	/* Background of applet */
	g_signal_connect(applet, "change-background",
			  G_CALLBACK(cw_panel_background_changed), menubar);

	gtk_widget_show(GTK_WIDGET(applet));

	return TRUE;

}

static void
cw_panel_background_changed (MatePanelApplet               *applet,
                             MatePanelAppletBackgroundType  type,
                             GdkColor                  *colour,
                             GdkPixmap                 *pixmap,
                             GtkWidget                 *menubar)
{
	GtkRcStyle *rc_style;
	GtkStyle *style;

	/* reset style */
	gtk_widget_set_style(GTK_WIDGET (applet), NULL);
 	gtk_widget_set_style(menubar, NULL);
	rc_style = gtk_rc_style_new ();
	gtk_widget_modify_style(GTK_WIDGET (applet), rc_style);
	gtk_widget_modify_style(menubar, rc_style);
	gtk_rc_style_unref(rc_style);

	switch (type)
	{
		case PANEL_NO_BACKGROUND:
			break;
		case PANEL_COLOR_BACKGROUND:
			gtk_widget_modify_bg(GTK_WIDGET (applet), GTK_STATE_NORMAL, colour);
			gtk_widget_modify_bg(menubar, GTK_STATE_NORMAL, colour);
			break;

		case PANEL_PIXMAP_BACKGROUND:
			style = gtk_style_copy(GTK_WIDGET (applet)->style);
			if (style->bg_pixmap[GTK_STATE_NORMAL])
				g_object_unref(style->bg_pixmap[GTK_STATE_NORMAL]);
			style->bg_pixmap[GTK_STATE_NORMAL] = g_object_ref (pixmap);
			gtk_widget_set_style(GTK_WIDGET (applet), style);
			gtk_widget_set_style(GTK_WIDGET (menubar), style);
			g_object_unref(style);
			break;
  }
}

