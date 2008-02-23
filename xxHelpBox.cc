// $Id: xxHelpBox.cc 2641 2007-09-02 21:31:02Z flaterco $

/*  xxHelpBox  Plain old, non-scrollable, window with text in it.

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

#include "xtide.hh"
#include "xxHelpBox.hh"


static void dismissCallback (Widget w unusedParameter,
                             XtPointer client_data,
                             XtPointer call_data unusedParameter) {
  assert (client_data);
  delete ((xxHelpBox *)client_data)->dismiss();
}


xxHelpBox::xxHelpBox (const xxWidget &parent, const Dstr &help):
  xxWindow (parent, boxContainer) {

  setTitle ("About...");
  Arg bgArgs[2] =  {
    {XtNbackground, (XtArgVal)xxX::pixels[Colors::background]},
    {XtNforeground, (XtArgVal)xxX::pixels[Colors::foreground]}
  };
  {
    Widget labelWidget = XtCreateManagedWidget (help.aschar(),
      labelWidgetClass, container->widget(), bgArgs, 2);
    label = xxX::wrap (labelWidget);
  }

  {
    Arg buttonArgs[2] =  {
      {XtNbackground, (XtArgVal)xxX::pixels[Colors::button]},
      {XtNforeground, (XtArgVal)xxX::pixels[Colors::foreground]}
    };
    Widget buttonwidget = XtCreateManagedWidget ("Dismiss", commandWidgetClass,
      container->widget(), buttonArgs, 2);
    XtAddCallback (buttonwidget, XtNcallback, dismissCallback,
     (XtPointer)this);
    dismissButton = xxX::wrap (buttonwidget);
  }

  realize();
  fixSize();
}


xxHelpBox::~xxHelpBox() {
  unrealize();
}


void xxHelpBox::globalRedraw() {
  xxWindow::globalRedraw();
  Arg buttonArgs[2] =  {
    {XtNbackground, (XtArgVal)xxX::pixels[Colors::button]},
    {XtNforeground, (XtArgVal)xxX::pixels[Colors::foreground]}
  };
  assert (dismissButton.get());
  XtSetValues (dismissButton->widget(), buttonArgs, 2);
  Arg args[2] =  {
    {XtNbackground, (XtArgVal)xxX::pixels[Colors::background]},
    {XtNforeground, (XtArgVal)xxX::pixels[Colors::foreground]}
  };
  assert (label.get());
  XtSetValues (label->widget(), args, 2);
}

// Cleanup2006 Done
