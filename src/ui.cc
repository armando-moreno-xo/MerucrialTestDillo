/*
 * File: ui.cc
 *
 * Copyright (C) 2005-2007 Jorge Arellano Cid <jcid@dillo.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

// UI for Dillo

#include <unistd.h>
#include <stdio.h>

#include "keys.hh"
#include "ui.hh"
#include "msg.h"
#include "timeout.hh"
#include "utf8.hh"

#include <FL/Fl.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Box.H>

// Include image data
#include "pixmaps.h"
#include "uicmd.hh"

struct iconset {
   Fl_Image *ImgMeterOK, *ImgMeterBug,
            *ImgHome, *ImgReload, *ImgSave, *ImgBook, *ImgTools,
            *ImgClear,*ImgSearch, *ImgHelp, *ImgLeft, *ImgLeftIn,
            *ImgRight, *ImgRightIn, *ImgStop, *ImgStopIn;
};

static struct iconset standard_icons = {
   new Fl_Pixmap(mini_ok_xpm),
   new Fl_Pixmap(mini_bug_xpm),
   new Fl_Pixmap(home_xpm),
   new Fl_Pixmap(reload_xpm),
   new Fl_Pixmap(save_xpm),
   new Fl_Pixmap(bm_xpm),
   new Fl_Pixmap(tools_xpm),
   new Fl_Pixmap(new_s_xpm),
   new Fl_Pixmap(search_xpm),
   new Fl_Pixmap(help_xpm),
   new Fl_Pixmap(left_xpm),
   new Fl_Pixmap(left_i_xpm),
   new Fl_Pixmap(right_xpm),
   new Fl_Pixmap(right_i_xpm),
   new Fl_Pixmap(stop_xpm),
   new Fl_Pixmap(stop_i_xpm),
};

static struct iconset small_icons = {
   standard_icons.ImgMeterOK,
   standard_icons.ImgMeterBug,
   new Fl_Pixmap(home_s_xpm),
   new Fl_Pixmap(reload_s_xpm),
   new Fl_Pixmap(save_s_xpm),
   new Fl_Pixmap(bm_s_xpm),
   new Fl_Pixmap(tools_s_xpm),
   new Fl_Pixmap(new_s_xpm),
   standard_icons.ImgSearch,
   standard_icons.ImgHelp,
   new Fl_Pixmap(left_s_xpm),
   new Fl_Pixmap(left_si_xpm),
   new Fl_Pixmap(right_i_xpm),
   new Fl_Pixmap(right_si_xpm),
   new Fl_Pixmap(stop_s_xpm),
   new Fl_Pixmap(stop_si_xpm),
};


static struct iconset *icons = &standard_icons;

/*
 * Local sub classes
 */

//----------------------------------------------------------------------------

/*
 * (Used to avoid certain shortcuts in the location bar)
 */
class CustInput : public Fl_Input {
public:
   CustInput (int x, int y, int w, int h, const char* l=0) :
      Fl_Input(x,y,w,h,l) {};
   int handle(int e);
};

/*
 * Disable keys: Up, Down, Page_Up, Page_Down and
 * CTRL+{o,r,Home,End}
 */
int CustInput::handle(int e)
{
   int k = Fl::event_key();

   _MSG("CustInput::handle event=%d\n", e);

   // We're only interested in some flags
   unsigned modifier = Fl::event_state() & (FL_SHIFT | FL_CTRL | FL_ALT);

   // Don't focus with arrow keys
   if (e == FL_FOCUS &&
       (k == FL_Up || k == FL_Down || k == FL_Left || k == FL_Right)) {
      return 0;
   } else if (e == FL_KEYBOARD) {
      if (modifier == FL_CTRL) {
         if (k == 'l') {
            // Make text selected when already focused.
            position(size(), 0);
            return 1;
         } else if (k == 'o' || k == 'r' || k == FL_Home || k == FL_End)
            return 0;
      } else if (modifier == FL_SHIFT) {
         if (k == FL_Left || k == FL_Right) {
            _MSG(" CustInput::handle > FL_SHIFT+FL_Right\n");
            a_UIcmd_send_event_to_tabs_by_wid(e, this);
            return 1;
         }
      }
   }
   _MSG("\n");

   return Fl_Input::handle(e);
}

//----------------------------------------------------------------------------

/*
 * Used to handle "paste" within the toolbar's Clear button.
 */
class CustButton : public Fl_Button {
public:
   CustButton(int x, int y, int w, int h, const char *l=0) :
      Fl_Button(x,y,w,h,l) {};
   int handle(int e);
};

int CustButton::handle(int e)
{
   if (e == FL_PASTE) {
      const char* t = Fl::event_text();
      if (t && *t) {
         a_UIcmd_set_location_text(a_UIcmd_get_bw_by_widget(this), t);
         a_UIcmd_open_urlstr(a_UIcmd_get_bw_by_widget(this), t);
         return 1;
      }
   }
   return Fl_Button::handle(e);
}

//----------------------------------------------------------------------------

/*
 * Used to resize the progress boxes automatically.
 */
class CustProgressBox : public Fl_Box {
   int padding;
public:
   CustProgressBox(int x, int y, int w, int h, const char *l=0) :
      Fl_Box(x,y,w,h,l) { padding = 0; };
   void update_label(const char *lbl) {
      int w,h;
      if (!padding) {
         copy_label("W");
         measure_label(w, h);
         padding = w > 2 ? w/2 : 1;
      }
      copy_label(lbl);
      measure_label(w,h);
      size(w+padding,h);
      redraw_label();
   }
};

//
// Toolbar buttons -----------------------------------------------------------
//
//static const char *button_names[] = {
//   "Back", "Forward", "Home", "Reload", "Save", "Stop", "Bookmarks", "Tools",
//   "Clear", "Search"
//};


//
// Callback functions --------------------------------------------------------
//

/*
 * Callback for the search button.
 */
static void search_cb(Fl_Widget *wid, void *data)
{
   int k = Fl::event_key();

   if (k == 1) {
      a_UIcmd_search_dialog(a_UIcmd_get_bw_by_widget(wid));
   } else if (k == 2) {
      ((UI*)data)->color_change_cb_i();
   } else if (k == 3) {
      ((UI*)data)->panel_cb_i();
   }
}

/*
 * Callback for the help button.
 */
static void help_cb(Fl_Widget *w, void *)
{
   char *path = dStrconcat(DILLO_DOCDIR, "user_help.html", NULL);
   BrowserWindow *bw = a_UIcmd_get_bw_by_widget(w);

   if (access(path, R_OK) == 0) {
      char *urlstr = dStrconcat("file:", path, NULL);
      a_UIcmd_open_urlstr(bw, urlstr);
      dFree(urlstr);
   } else {
      MSG("Can't read local help file at \"%s\"."
          " Getting remote help...\n", path);
      a_UIcmd_open_urlstr(bw, "http://www.dillo.org/dillo2-help.html");
   }
   dFree(path);
}

/*
 * Callback for the File menu button.
 */
static void filemenu_cb(Fl_Widget *wid, void *)
{
   int k = Fl::event_key();
   if (k == 1 || k == 3) {
      a_UIcmd_file_popup(a_UIcmd_get_bw_by_widget(wid), wid);
   }
}

/*
 * Callback for the location's clear-button.
 */
static void clear_cb(Fl_Widget *w, void *data)
{
   UI *ui = (UI*)data;

   int k = Fl::event_key();
   if (k == 1) {
      ui->set_location("");
      ui->focus_location();
   } if (k == 2) {
      ui->paste_url();
   }
}

/*
 * Change the color of the location bar.
 *
static void color_change_cb(Fl_Widget *wid, void *data)
{
   ((UI*)data)->color_change_cb_i();
}
 */


/*
 * Send the browser to the new URL in the location.
 */
static void location_cb(Fl_Widget *wid, void *data)
{
   Fl_Input *i = (Fl_Input*)wid;
   UI *ui = (UI*)data;

   _MSG("location_cb()\n");
   /* This test is necessary because WHEN_ENTER_KEY also includes
    * other events we're not interested in. For instance pressing
    * The Back or Forward, buttons, or the first click on a rendered
    * page. BUG: this must be investigated and reported to FLTK2 team */
   if (Fl::event_key() == FL_Enter) {
      a_UIcmd_open_urlstr(a_UIcmd_get_bw_by_widget(i), i->value());
   }
   if (ui->get_panelmode() == UI_TEMPORARILY_SHOW_PANELS) {
      ui->set_panelmode(UI_HIDDEN);
   }
}


/*
 * Callback handler for button press on the panel
 */
static void b1_cb(Fl_Widget *wid, void *cb_data)
{
   int bn = VOIDP2INT(cb_data);
   int k = Fl::event_button();
   if (k >= FL_LEFT_MOUSE && k <= FL_RIGHT_MOUSE) {
      _MSG("[%s], mouse button %d was pressed\n", button_names[bn], k);
      _MSG("mouse button %d was pressed\n", k);
   }
   switch (bn) {
   case UI_BACK:
      if (k == FL_LEFT_MOUSE) {
         a_UIcmd_back(a_UIcmd_get_bw_by_widget(wid));
      } else if (k == FL_RIGHT_MOUSE) {
         a_UIcmd_back_popup(a_UIcmd_get_bw_by_widget(wid));
      }
      break;
   case UI_FORW:
      if (k == FL_LEFT_MOUSE) {
         a_UIcmd_forw(a_UIcmd_get_bw_by_widget(wid));
      } else if (k == FL_RIGHT_MOUSE) {
         a_UIcmd_forw_popup(a_UIcmd_get_bw_by_widget(wid));
      }
      break;
   case UI_HOME:
      if (k == FL_LEFT_MOUSE) {
         a_UIcmd_home(a_UIcmd_get_bw_by_widget(wid));
      }
      break;
   case UI_RELOAD:
      if (k == FL_LEFT_MOUSE) {
         a_UIcmd_reload(a_UIcmd_get_bw_by_widget(wid));
      }
      break;
   case UI_SAVE:
      if (k == FL_LEFT_MOUSE) {
         a_UIcmd_save(a_UIcmd_get_bw_by_widget(wid));
      }
      break;
   case UI_STOP:
      if (k == FL_LEFT_MOUSE) {
         a_UIcmd_stop(a_UIcmd_get_bw_by_widget(wid));
      }
      break;
   case UI_BOOK:
      if (k == FL_LEFT_MOUSE) {
         a_UIcmd_book(a_UIcmd_get_bw_by_widget(wid));
      }
      break;
   case UI_TOOLS:
      if (k == FL_LEFT_MOUSE || k == FL_RIGHT_MOUSE) {
         a_UIcmd_tools(a_UIcmd_get_bw_by_widget(wid), wid);
      }
      break;
   default:
      break;
   }
}

/*
 * Callback handler for fullscreen button press
 */
//static void fullscreen_cb(Fl_Widget *wid, void *data)
//{
//   /* TODO: do we want to toggle fullscreen or panelmode?
//            maybe we need to add another button?*/
//   ((UI*)data)->panelmode_cb_i();
//}

/*
 * Callback for the bug meter button.
 */
static void bugmeter_cb(Fl_Widget *wid, void *data)
{
   int k = Fl::event_key();
   if (k == 1) {
      a_UIcmd_view_page_bugs(a_UIcmd_get_bw_by_widget(wid));
   } else if (k == 3) {
      a_UIcmd_bugmeter_popup(a_UIcmd_get_bw_by_widget(wid));
   }
}

//////////////////////////////////////////////////////////////////////////////
// UI class methods
//

//----------------------------
// Panel construction methods
//----------------------------

/*
 * Create the archetipic browser buttons
 */
Fl_Pack *UI::make_toolbar(int tw, int th)
{
   Fl_Button *b;
   Fl_Pack *p1=new Fl_Pack(0,0,tw,th);
   p1->begin();
    Back = b = new Fl_Button(xpos, 0, bw, bh, (lbl) ? "Back" : 0);
    b->image(icons->ImgLeft);
    b->deimage(icons->ImgLeftIn);
    b->callback(b1_cb, (void *)UI_BACK);
    b->clear_visible_focus();

    Forw = b = new Fl_Button(xpos, 0, bw, bh, (lbl) ? "Forw" : 0);
    b->image(icons->ImgRight);
    b->deimage(icons->ImgRightIn);
    b->callback(b1_cb, (void *)UI_FORW);
    b->clear_visible_focus();

    Home = b = new Fl_Button(xpos, 0, bw, bh, (lbl) ? "Home" : 0);
    b->image(icons->ImgHome);
    b->callback(b1_cb, (void *)UI_HOME);
    b->clear_visible_focus();

    Reload = b = new Fl_Button(xpos, 0, bw, bh, (lbl) ? "Reload" : 0);
    b->image(icons->ImgReload);
    b->callback(b1_cb, (void *)UI_RELOAD);
    b->clear_visible_focus();

    Save = b = new Fl_Button(xpos, 0, bw, bh, (lbl) ? "Save" : 0);
    b->image(icons->ImgSave);
    b->callback(b1_cb, (void *)UI_SAVE);
    b->clear_visible_focus();

    Stop = b = new Fl_Button(xpos, 0, bw, bh, (lbl) ? "Stop" : 0);
    b->image(icons->ImgStop);
    b->deimage(icons->ImgStopIn);
    b->callback(b1_cb, (void *)UI_STOP);
    b->clear_visible_focus();

    Bookmarks = b = new Fl_Button(xpos, 0, bw, bh, (lbl) ? "Book" : 0);
    b->image(icons->ImgBook);
    b->callback(b1_cb, (void *)UI_BOOK);
    b->clear_visible_focus();

    Tools = b = new Fl_Button(xpos, 0, bw, bh, (lbl) ? "Tools" : 0);
    b->image(icons->ImgTools);
    b->callback(b1_cb, (void *)UI_TOOLS);
    b->clear_visible_focus();

   p1->type(Fl_Pack::HORIZONTAL);
   p1->end();

   if (prefs.show_tooltip) {
      Back->tooltip("Previous page");
      Forw->tooltip("Next page");
      Home->tooltip("Go to the Home page");
      Reload->tooltip("Reload");
      Save->tooltip("Save this page");
      Stop->tooltip("Stop loading");
      Bookmarks->tooltip("View bookmarks");
      Tools->tooltip("Settings");
   }
   return p1;
}

/*
 * Create the location box (Clear/Input/Search)
 */
Fl_Pack *UI::make_location()
{
   Fl_Button *b;
   Fl_Pack *pg = new Fl_Pack(0,0,0,0);
   pg->begin();
    Clear = b = new CustButton(2,2,16,22,0);
    b->image(icons->ImgClear);
    b->callback(clear_cb, this);
    b->clear_visible_focus();

    Fl_Input *i = Location = new CustInput(0,0,0,0,0);
    i->color(CuteColor);
    i->when(FL_WHEN_ENTER_KEY);
    i->callback(location_cb, this);

    Search = b = new Fl_Button(0,0,16,22,0);
    b->image(icons->ImgSearch);
    b->callback(search_cb, this);
    b->clear_visible_focus();

    Help = b = new Fl_Button(0,0,16,22,0);
    b->image(icons->ImgHelp);
    b->callback(help_cb, this);
    b->clear_visible_focus();

   pg->type(Fl_Pack::HORIZONTAL);
   pg->resizable(i);
   pg->end();

   if (prefs.show_tooltip) {
      Clear->tooltip("Clear the URL box.\nMiddle-click to paste a URL.");
      Location->tooltip("Location");
      Search->tooltip("Search the Web");
      Help->tooltip("Help");
   }
   return pg;
}

/*
 * Create the progress bars
 */
Fl_Pack *UI::make_progress_bars(int wide, int thin_up)
{
   ProgBox = new Fl_Pack(0,0,0,0);
   ProgBox->begin();
    // Images
    IProg = new CustProgressBox(0,0,0,0);
    IProg->box(thin_up ? FL_THIN_UP_BOX : FL_EMBOSSED_BOX);
    IProg->labelcolor(FL_GRAY_RAMP + 2);
    IProg->update_label(wide ? "Images\n0 of 0" : "0 of 0");
    // Page
    PProg = new CustProgressBox(0,0,0,0);
    PProg->box(thin_up ? FL_THIN_UP_BOX : FL_EMBOSSED_BOX);
    PProg->labelcolor(FL_GRAY_RAMP + 2);
    PProg->update_label(wide ? "Page\n0.0KB" : "0.0KB");
   ProgBox->type(Fl_Pack::HORIZONTAL);
   ProgBox->end();

   return ProgBox;
}

/*
 * Create the "File" menu
 * Static function for File menu callbacks.
 */
Fl_Widget *UI::make_filemenu_button()
{
   Fl_Button *btn;
   int w,h, padding;

   FileButton = btn = new Fl_Button(0,0,0,0,"W");
   btn->measure_label(w, h);
   padding = w;
   btn->copy_label(PanelSize == P_tiny ? "&F" : "&File");
   btn->measure_label(w,h);
   if (PanelSize == P_large)
      h = fh;
   btn->size(w+padding,h);
   _MSG("UI::make_filemenu_button w=%d h=%d padding=%d\n", w, h, padding);
   btn->box(PanelSize == P_large ? FL_FLAT_BOX : FL_THIN_UP_BOX);
   btn->callback(filemenu_cb, this);
   if (prefs.show_tooltip)
      btn->tooltip("File menu");
   btn->clear_visible_focus();
   if (!prefs.show_filemenu && PanelSize != P_large)
      btn->hide();
   return btn;
}


/*
 * Create the control panel
 */
Fl_Group *UI::make_panel(int ww)
{
   Fl_Widget *w;
   Fl_Group *g1, *g2, *g3;
   Fl_Pack *pg;

   if (PanelSize > P_large) {
      PanelSize = P_tiny;
      Small_Icons = !Small_Icons;
   }

   if (Small_Icons)
      icons = &small_icons;
   else
      icons = &standard_icons;

   if (PanelSize == P_tiny) {
      if (Small_Icons)
         xpos = 0, bw = 22, bh = 22, fh = 0, lh = 22, lbl = 0;
      else
         xpos = 0, bw = 28, bh = 28, fh = 0, lh = 28, lbl = 0;
   } else if (PanelSize == P_small) {
      if (Small_Icons)
         xpos = 0, bw = 20, bh = 20, fh = 0, lh = 20, lbl = 0;
      else
         xpos = 0, bw = 28, bh = 28, fh = 0, lh = 28, lbl = 0;
   } else if (PanelSize == P_medium) {
      if (Small_Icons)
         xpos = 0, bw = 42, bh = 36, fh = 0, lh = 22, lbl = 1;
      else
         xpos = 0, bw = 45, bh = 45, fh = 0, lh = 28, lbl = 1;
   } else {   // P_large
      if (Small_Icons)
         xpos = 0, bw = 42, bh = 36, fh = 22, lh = 22, lbl = 1;
      else
         xpos = 0, bw = 45, bh = 45, fh = 24, lh = 28, lbl = 1;
   }

   if (PanelSize == P_tiny) {
      g1 = new Fl_Group(0,0,ww,bh);
       // Toolbar
       pg = make_toolbar(ww,bh);
       pg->box(FL_EMBOSSED_BOX);
       g1->add(pg);
       w = make_filemenu_button();
       pg->add(w);
       w = make_location();
       pg->add(w);
       pg->resizable(w);
       w = make_progress_bars(0,1);
       pg->add(w);

      g1->resizable(pg);

   } else {
      g1 = new Fl_Group(0,0,ww,fh+lh+bh);
      g1->begin();
        // File menu
        if (PanelSize == P_large) {
           g3 = new Fl_Group(0,0,ww,lh);
           g3->box(FL_FLAT_BOX);
           Fl_Widget *bn = make_filemenu_button();
           g3->add(bn);
           g3->add_resizable(*new Fl_Box(bn->w(),0,ww - bn->w(),lh));

           g2 = new Fl_Group(0,fh,ww,lh);
           g2->begin();
           pg = make_location();
           pg->size(ww,lh);
        } else {
           g2 = new Fl_Pack(0,fh,ww,lh);
           g2->type(Fl_Pack::HORIZONTAL);
           g2->begin();
           make_filemenu_button();
           pg = make_location();
        }

       g2->resizable(pg);
       g2->end();

       // Toolbar
       g3 = new Fl_Group(0,fh+lh,ww,bh);
       g3->begin();
        pg = make_toolbar(ww,bh);
        //w = new Fl_Box(0,0,0,0,"i n v i s i b l e");
        w = new Fl_Box(0,0,0,0,0);
        pg->add(w);
        pg->resizable(w);

        if (PanelSize == P_small) {
           w = make_progress_bars(0,0);
        } else {
           w = make_progress_bars(1,0);
        }
        pg->add(w);

       g3->resizable(pg); // Better than 'w3' and it also works
       pg->box(FL_BORDER_FRAME);
       //g3->box(FL_EMBOSSED_BOX);
       g3->end();

      g1->resizable(g3);
      g1->end();
   }

   return g1;
}

/*
 * Create the status panel
 */
Fl_Group *UI::make_status_panel(int ww)
{
   const int s_h = 20, bm_w = 16;
   Fl_Group *g = new Fl_Group(0, 0, ww, s_h, 0);

   // Status box
   StatusOutput = new Fl_Output(0, 0, ww-bm_w, s_h, 0);
   StatusOutput->value("");
   StatusOutput->box(FL_THIN_DOWN_BOX);
   StatusOutput->clear_visible_focus();
   StatusOutput->color(FL_GRAY_RAMP + 18);
   g->add(StatusOutput);
   //StatusOutput->throw_focus();

   // Bug Meter
   BugMeter = new Fl_Button(ww-bm_w,0,bm_w,s_h,0);
   BugMeter->image(icons->ImgMeterOK);
   BugMeter->box(FL_THIN_DOWN_BOX);
   BugMeter->align(FL_ALIGN_INSIDE|FL_ALIGN_CLIP|FL_ALIGN_LEFT);
   if (prefs.show_tooltip)
      BugMeter->tooltip("Show HTML bugs\n(right-click for menu)");
   BugMeter->callback(bugmeter_cb, this);
   BugMeter->clear_visible_focus();
   g->add(BugMeter);

   g->resizable(StatusOutput);
   return g;
}

/*
 * User Interface constructor
 */
UI::UI(int x, int y, int ww, int wh, const char* label, const UI *cur_ui) :
  Fl_Group(x, y, ww, wh, label)
{
   PointerOnLink = FALSE;

   Tabs = NULL;
   TabTooltip = NULL;
   TopGroup = new Fl_Pack(0, 0, ww, wh);
   add(TopGroup);
   resizable(TopGroup);
   clear_flag(SHORTCUT_LABEL);

   if (cur_ui) {
      PanelSize = cur_ui->PanelSize;
      CuteColor = cur_ui->CuteColor;
      Small_Icons = cur_ui->Small_Icons;
      if (cur_ui->Panelmode == UI_HIDDEN ||
          cur_ui->Panelmode == UI_TEMPORARILY_SHOW_PANELS)
         Panelmode = UI_HIDDEN;
      else
         Panelmode = UI_NORMAL;
   } else {
     // Set some default values
     //PanelSize = P_tiny, CuteColor = 26, Small_Icons = 0;
     PanelSize = prefs.panel_size;
     Small_Icons = prefs.small_icons;
     CuteColor = 206;
     Panelmode = (UIPanelmode) prefs.fullwindow_start;
   }

   // Control panel
   Panel = make_panel(ww);
   TopGroup->add(Panel);

   // Render area
   Main = new Fl_Group(0,0,1,1,"Welcome...");
   Main->box(FL_FLAT_BOX);
   Main->color(FL_GRAY_RAMP + 3);
   Main->labelfont(FL_HELVETICA_BOLD_ITALIC);
   Main->labelsize(36);
   Main->labeltype(FL_SHADOW_LABEL);
   Main->labelcolor(FL_WHITE);
   TopGroup->add(Main);
   TopGroup->resizable(Main);
   MainIdx = TopGroup->find(Main);

   // Find text bar
   findbar = new Findbar(ww, 28);
   TopGroup->add(findbar);

   // Status Panel
   StatusPanel = make_status_panel(ww);
   TopGroup->add(StatusPanel);

   // Make the full screen button (to be attached to the viewport later)
   // TODO: attach to the viewport
   //FullScreen = new Fl_Button(0,0,15,15);
   //FullScreen->image(ImgFullScreenOn);
   //FullScreen->tooltip("Hide Controls");
   //FullScreen->callback(fullscreen_cb, this);

   customize(0);

   if (Panelmode) {
      Panel->hide();
      StatusPanel->hide();
   }
}

/*
 * UI destructor
 */
UI::~UI()
{
   _MSG("UI::~UI()\n");
   dFree(TabTooltip);
}

/*
 * FLTK event handler for this window.
 */
int UI::handle(int event)
{
   _MSG("UI::handle event=%d (%d,%d)\n", event, Fl::event_x(), Fl::event_y());
   _MSG("Panel->h()=%d Main->h()=%d\n", Panel->h() , Main->h());

   int ret = 0;

   if (event == FL_KEYBOARD) {
      return 0; // Receive as shortcut
   } else if (event == FL_SHORTCUT) {
      KeysCommand_t cmd = Keys::getKeyCmd();
      if (cmd == KEYS_NOP) {
         // Do nothing
      } else if (cmd == KEYS_SCREEN_UP || cmd == KEYS_SCREEN_DOWN ||
                 cmd == KEYS_LINE_UP || cmd == KEYS_LINE_DOWN ||
                 cmd == KEYS_LEFT || cmd == KEYS_RIGHT ||
                 cmd == KEYS_TOP || cmd == KEYS_BOTTOM) {
         a_UIcmd_scroll(a_UIcmd_get_bw_by_widget(this), cmd);
         ret = 1;
      } else if (cmd == KEYS_BACK) {
         a_UIcmd_back(a_UIcmd_get_bw_by_widget(this));
         ret = 1;
      } else if (cmd == KEYS_FORWARD) {
         a_UIcmd_forw(a_UIcmd_get_bw_by_widget(this));
         ret = 1;
      } else if (cmd == KEYS_BOOKMARKS) {
         a_UIcmd_book(a_UIcmd_get_bw_by_widget(this));
         ret = 1;
      } else if (cmd == KEYS_FIND) {
         set_findbar_visibility(1);
         ret = 1;
      } else if (cmd == KEYS_WEBSEARCH) {
         a_UIcmd_search_dialog(a_UIcmd_get_bw_by_widget(this));
         ret = 1;
      } else if (cmd == KEYS_GOTO) {
         focus_location();
         ret = 1;
      } else if (cmd == KEYS_NEW_TAB) {
         a_UIcmd_open_url_nt(a_UIcmd_get_bw_by_widget(this), NULL, 1);
         ret = 1;
      } else if (cmd == KEYS_CLOSE_TAB) {
         a_UIcmd_close_bw(a_UIcmd_get_bw_by_widget(this));
         ret = 1;
      } else if (cmd == KEYS_HIDE_PANELS &&
                 get_panelmode() == UI_TEMPORARILY_SHOW_PANELS) {
         set_panelmode(UI_HIDDEN);
         ret = 1;
      } else if (cmd == KEYS_NEW_WINDOW) {
         a_UIcmd_browser_window_new(w(),h(),0,a_UIcmd_get_bw_by_widget(this));
         ret = 1;
      } else if (cmd == KEYS_OPEN) {
         a_UIcmd_open_file(a_UIcmd_get_bw_by_widget(this));
         ret = 1;
      } else if (cmd == KEYS_HOME) {
         a_UIcmd_home(a_UIcmd_get_bw_by_widget(this));
         ret = 1;
      } else if (cmd == KEYS_RELOAD) {
         a_UIcmd_reload(a_UIcmd_get_bw_by_widget(this));
         ret = 1;
      } else if (cmd == KEYS_STOP) {
         a_UIcmd_stop(a_UIcmd_get_bw_by_widget(this));
         ret = 1;
      } else if (cmd == KEYS_SAVE) {
         a_UIcmd_save(a_UIcmd_get_bw_by_widget(this));
         ret = 1;
      } else if (cmd == KEYS_FULLSCREEN) {
         panelmode_cb_i();
         ret = 1;
      } else if (cmd == KEYS_FILE_MENU) {
         a_UIcmd_file_popup(a_UIcmd_get_bw_by_widget(this), FileButton);
         ret = 1;
      } else if (cmd == KEYS_CLOSE_ALL) {
         a_Timeout_add(0.0, a_UIcmd_close_all_bw, NULL);
         ret = 1;
      }
   } else if (event == FL_PUSH) {
      if (prefs.middle_click_drags_page == 0 &&
          Fl::event_button() == FL_MIDDLE_MOUSE &&
          !a_UIcmd_pointer_on_link(a_UIcmd_get_bw_by_widget(this))) {
         if (Main->contains(Fl::belowmouse())) {
            /* Offer the event to Main's children (form widgets) */
            /* TODO: Try just offering it to Fl::belowmouse() */
            ret = ((Fl_Group *)Main)->Fl_Group::handle(event);
         }
         if (!ret) {
            /* middle click was not on a link or a form widget */
            paste_url();
            ret = 1;
         }
      }
   }

   if (!ret) {
      ret = Fl_Group::handle(event);
   }

   return ret;
}


//----------------------------
// API for the User Interface
//----------------------------

/*
 * Get the text from the location input-box.
 */
const char *UI::get_location()
{
   return Location->value();
}

/*
 * Set a new URL in the location input-box.
 */
void UI::set_location(const char *str)
{
   if (!str) str = "";
   // This text() call clears fl_pending_callback, avoiding
   // an extra location_cb() call.
   Location->value(str);
   Location->position(strlen(str));
}

/*
 * Focus location entry.
 * If it's not visible, show it until the callback is done.
 */
void UI::focus_location()
{
   if (get_panelmode() == UI_HIDDEN) {
      set_panelmode(UI_TEMPORARILY_SHOW_PANELS);
   }
   Location->take_focus();
   // Make text selected when already focused.
   Location->position(Location->size(), 0);
}

/*
 * Focus Main area.
 */
void UI::focus_main()
{
   Main->take_focus();
}

/*
 * Set a new message in the status bar.
 */
void UI::set_status(const char *str)
{
   StatusOutput->value(str);
}

/*
 * Set the page progress text
 * cmd: 0 Deactivate, 1 Update, 2 Clear
 */
void UI::set_page_prog(size_t nbytes, int cmd)
{
   char str[32];

   if (cmd == 0) {
      PProg->deactivate();
   } else {
      PProg->activate();
      if (cmd == 1) {
         snprintf(str, 32, "%s%.1f KB",
                  (PanelSize == 0) ? "" : "Page\n", nbytes/1024.0);
      } else if (cmd == 2) {
         str[0] = '\0';
      }
      PProg->update_label(str);
   }
}

/*
 * Set the image progress text
 * cmd: 0 Deactivate, 1 Update, 2 Clear
 */
void UI::set_img_prog(int n_img, int t_img, int cmd)
{
   char str[32];

   if (cmd == 0) {
      IProg->deactivate();
   } else {
      IProg->activate();
      if (cmd == 1) {
         snprintf(str, 32, "%s%d of %d",
                  (PanelSize == 0) ? "" : "Images\n", n_img, t_img);
      } else if (cmd == 2) {
         str[0] = '\0';
      }
      IProg->update_label(str);
   }
}

/*
 * Set the bug meter progress text
 */
void UI::set_bug_prog(int n_bug)
{
   char str[32];
   int new_w = 16;

   if (n_bug == 0) {
      BugMeter->image(icons->ImgMeterOK);
      BugMeter->label("");
   } else if (n_bug >= 1) {
      if (n_bug == 1)
         BugMeter->image(icons->ImgMeterBug);
      snprintf(str, 32, "%d", n_bug);
      BugMeter->copy_label(str);
      BugMeter->redraw_label();
      new_w = strlen(str)*8 + 20;
   }
   StatusOutput->resize(0,0,StatusPanel->w()-new_w,StatusOutput->h());
   BugMeter->resize(StatusPanel->w()-new_w, 0, new_w, BugMeter->h());
   StatusPanel->init_sizes();
}

/*
 * Customize the UI's panel (show/hide buttons)
 */
void UI::customize(int flags)
{
   // flags argument not currently used

   if ( !prefs.show_back )
      Back->hide();
   if ( !prefs.show_forw )
      Forw->hide();
   if ( !prefs.show_home )
      Home->hide();
   if ( !prefs.show_reload )
      Reload->hide();
   if ( !prefs.show_save )
      Save->hide();
   if ( !prefs.show_stop )
      Stop->hide();
   if ( !prefs.show_bookmarks )
      Bookmarks->hide();
   if ( !prefs.show_tools )
      Tools->hide();
   if ( !prefs.show_clear_url )
      Clear->hide();
   if ( !prefs.show_url )
      Location->hide();
   if ( !prefs.show_search )
      Search->hide();
   if ( !prefs.show_help )
      Help->hide();
   if ( !prefs.show_progress_box )
      ProgBox->hide();
}

/*
 * On-the-fly panel style change
 */
void UI::panel_cb_i()
{
   Fl_Group *NewPanel;

   // Create a new Panel
   ++PanelSize;
   NewPanel = make_panel(TopGroup->w());
   TopGroup->remove(Panel);
   delete(Panel);
   TopGroup->add(NewPanel);
   Panel = NewPanel;
   customize(0);

   Location->take_focus();
}

/*
 * On-the-fly color style change
 */
void UI::color_change_cb_i()
{
   const int cols[] = {7,17,26,51,140,156,205,206,215,-1};
   static int ncolor = 0;

   ncolor = (cols[ncolor+1] < 0) ? 0 : ncolor + 1;
   CuteColor = cols[ncolor];
   MSG("Location color %d\n", CuteColor);
   Location->color(CuteColor);
   Location->redraw();
}

/*
 * Set or remove the Panelmode flag and update the UI accordingly
 */
void UI::set_panelmode(UIPanelmode mode)
{
   if (mode == UI_HIDDEN) {
      Panel->hide();
      StatusPanel->hide();
   } else {
      /* UI_NORMAL or UI_TEMPORARILY_SHOW_PANELS */
      Panel->show();
      StatusPanel->show();
   }
   Panelmode = mode;
}

/*
 * Get the value of the panelmode flag
 */
UIPanelmode UI::get_panelmode()
{
   return Panelmode;
}

/*
 * Toggle the Control Panel out of the way
 */
void UI::panelmode_cb_i()
{
   set_panelmode((UIPanelmode) !Panelmode);
}

/*
 * Set 'nw' as the main render area widget
 */
void UI::set_render_layout(Fl_Group &nw)
{
   // BUG: replace() is not working as it should.
   // In our case, replacing the rendering area leaves the vertical
   // scrollbar without events.
   //
   // We'll use a workaround in a_UIcmd_browser_window_new() instead.
   TopGroup->remove(MainIdx);
   delete(Main);
   TopGroup->add(nw);
   Main = &nw;
   //TopGroup->box(FL_DOWN_BOX);
   //TopGroup->box(FL_BORDER_FRAME);
   TopGroup->resizable(TopGroup->child(MainIdx));
}

/*
 * Set the tab title
 */
void UI::set_tab_title(const char *label)
{
   char title[128];

   dReturn_if_fail(label != NULL);

   if (*label) {
      // Make a label for this tab
      size_t tab_chars = 18, label_len = strlen(label);

      if (label_len > tab_chars)
         tab_chars = a_Utf8_end_of_char(label, tab_chars - 1) + 1;
      snprintf(title, tab_chars + 1, "%s", label);
      if (label_len > tab_chars)
         snprintf(title + tab_chars, 4, "...");
      // Avoid unnecessary redraws
      if (strcmp(this->label(), title)) {
         this->copy_label(title);
         this->redraw_label();
      }

      // Disabled because of a bug in fltk::Tabgroup
      //dFree(TabTooltip);
      //TabTooltip = dStrdup(label);
      //this->tooltip(TabTooltip);
   }
}

/*
 * Set button sensitivity (Back/Forw/Stop)
 */
void UI::button_set_sens(UIButton btn, int sens)
{
   switch (btn) {
   case UI_BACK:
      (sens) ? Back->activate() : Back->deactivate();
//    Back->redraw(DAMAGE_HIGHLIGHT);
      break;
   case UI_FORW:
      (sens) ? Forw->activate() : Forw->deactivate();
//    Forw->redraw(DAMAGE_HIGHLIGHT);
      break;
   case UI_STOP:
      (sens) ? Stop->activate() : Stop->deactivate();
//    Stop->redraw(DAMAGE_HIGHLIGHT);
      break;
   default:
      break;
   }
}

/*
 * Paste a middle-click-selection into "Clear" button as URL
 */
void UI::paste_url()
{
   Fl::paste(*Clear, false);
}

/*
 * Shows or hides the findbar of this window
 */
void UI::set_findbar_visibility(bool visible)
{
   if (visible) {
      findbar->show();
   } else {
      findbar->hide();
   }
}
