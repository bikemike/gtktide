// $Id: xxHorizDialog.cc 2641 2007-09-02 21:31:02Z flaterco $

/*  xxHorizDialog  More compact replacement for dialogWidgetClass.

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
#include "xxHorizDialog.hh"


constString xxHorizDialog::val() const {
  return buf;
}


xxHorizDialog::xxHorizDialog (const xxWidget &parent,
			      constString caption,
			      constString init) {
  {
    Arg args[3] =  {
      {XtNbackground, (XtArgVal)xxX::pixels[Colors::background]},
      {XtNforeground, (XtArgVal)xxX::pixels[Colors::foreground]},
      {XtNorientation, (XtArgVal)XtorientHorizontal}
    };
    Widget boxWidget = XtCreateManagedWidget ("", boxWidgetClass,
      parent.widget(), args, 3);
    box = xxX::wrap (boxWidget);
  }{
    Arg args[3] =  {
      {XtNbackground, (XtArgVal)xxX::pixels[Colors::background]},
      {XtNforeground, (XtArgVal)xxX::pixels[Colors::foreground]},
      {XtNborderWidth, (XtArgVal)0}
    };
    Widget labelWidget = XtCreateManagedWidget (caption,
      labelWidgetClass, box->widget(), args, 3);
    label = xxX::wrap (labelWidget);
  }
  assert (strlen (init) < bufSize);
  strcpy (buf, init);
  {
    Arg args[6] =  {
      {XtNbackground, (XtArgVal)xxX::pixels[Colors::background]},
      {XtNforeground, (XtArgVal)xxX::pixels[Colors::foreground]},
      {XtNstring, (XtArgVal)buf},
      {XtNuseStringInPlace, (XtArgVal)1},
      {XtNlength, (XtArgVal)(bufSize-1)},
      {XtNeditType, (XtArgVal)XawtextEdit}
    };
    Widget textWidget = XtCreateManagedWidget ("", asciiTextWidgetClass,
      box->widget(), args, 6);
    text = xxX::wrap (textWidget);
  }
}


void xxHorizDialog::globalRedraw() {
  Arg args[2] =  {
    {XtNbackground, (XtArgVal)xxX::pixels[Colors::background]},
    {XtNforeground, (XtArgVal)xxX::pixels[Colors::foreground]}
  };
  assert (label.get());
  XtSetValues (label->widget(), args, 2);
  assert (text.get());
  XtSetValues (text->widget(), args, 2);
  assert (box.get());
  XtSetValues (box->widget(), args, 2);
}

// Cleanup2006 Done
