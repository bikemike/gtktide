// $Id: xxMultiChoice.cc 2641 2007-09-02 21:31:02Z flaterco $

/*  xxMultiChoice  Multiple-choice button widget.

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
#include "xxLocationList.hh"
#include "xxMultiChoice.hh"


const unsigned xxMultiChoice::choice() const {
  return currentChoice;
}


static void buttonCallback (Widget w,
                            XtPointer client_data,
			    XtPointer call_data unusedParameter) {
  assert (client_data);
  ((xxMultiChoice*)client_data)->callback (w);
}


void xxMultiChoice::callback (Widget choiceButton) {
  currentChoice = buttonChoices[choiceButton];
  Arg args[1] = {{XtNlabel, (XtArgVal)(_choices[currentChoice])}};
  XtSetValues (button->widget(), args, 1);
  if (_caller)
    _caller->sortKey();
}


void xxMultiChoice::construct (Widget boxWidget) {
  Arg buttonArgs[4] =  {
    {XtNvisual, (XtArgVal)xxX::visual},
    {XtNcolormap, (XtArgVal)xxX::colormap},
    {XtNbackground, (XtArgVal)xxX::pixels[Colors::button]},
    {XtNforeground, (XtArgVal)xxX::pixels[Colors::foreground]}
  };

  // The menu.
  {
    Arg menuArgs[5] = {
      {XtNvisual, (XtArgVal)xxX::visual},
      {XtNcolormap, (XtArgVal)xxX::colormap},
      {"menuName", (XtArgVal)"menu"},
      {XtNbackground, (XtArgVal)xxX::pixels[Colors::button]},
      {XtNforeground, (XtArgVal)xxX::pixels[Colors::foreground]}
    };
    Widget buttonWidget = XtCreateManagedWidget (_choices[currentChoice],
      menuButtonWidgetClass, boxWidget, menuArgs, 5);
    button = xxX::wrap (buttonWidget);
  }{
    Widget menushell = XtCreatePopupShell ("menu",
      simpleMenuWidgetClass, button->widget(), buttonArgs, 4);
    menu = xxX::wrap (menushell);
  }

  // Buttons on the menu.
  for (unsigned y=0; _choices[y]; ++y) {
    Widget buttonWidget = XtCreateManagedWidget (_choices[y],
						 smeBSBObjectClass,
						 menu->widget(),
						 buttonArgs,
						 4);
    XtAddCallback (buttonWidget, XtNcallback, buttonCallback, (XtPointer)this);
    choiceButtons.push_back (new xxWidget (buttonWidget));
    buttonChoices[buttonWidget] = y;
  }
}


xxMultiChoice::xxMultiChoice (const xxWidget &parent,
			      constString caption,
			      constStringArray choices,
			      unsigned initialChoice):
  _caller(NULL),
  _choices(choices),
  currentChoice(initialChoice) {
  {
    Arg args[3] =  {
      {XtNbackground, (XtArgVal)xxX::pixels[Colors::background]},
      {XtNforeground, (XtArgVal)xxX::pixels[Colors::foreground]},
      {XtNorientation, (XtArgVal)XtorientHorizontal}
    };
    Widget boxWidget = XtCreateManagedWidget ("", boxWidgetClass,
      parent.widget(), args, 3);
    _box = xxX::wrap (boxWidget);
  }{
    Arg args[3] =  {
      {XtNbackground, (XtArgVal)xxX::pixels[Colors::background]},
      {XtNforeground, (XtArgVal)xxX::pixels[Colors::foreground]},
      {XtNborderWidth, (XtArgVal)0}
    };
    Widget labelWidget = XtCreateManagedWidget (caption,
      labelWidgetClass, _box->widget(), args, 3);
    label = xxX::wrap (labelWidget);
  }
  construct (_box->widget());
}


xxMultiChoice::xxMultiChoice (const xxWidget &box,
			      constStringArray choices,
			      unsigned initialChoice):
  _caller(NULL),
  _choices(choices),
  currentChoice(initialChoice) {
  construct (box.widget());
}


xxMultiChoice::xxMultiChoice (const xxWidget &box,
			      constStringArray choices,
			      unsigned initialChoice,
			      xxLocationList &caller):
  _caller(&caller),
  _choices(choices),
  currentChoice(initialChoice) {
  construct (box.widget());
}


xxMultiChoice::~xxMultiChoice () {
  for (unsigned a=0; a<choiceButtons.size(); ++a)
    delete choiceButtons[a];
}


void xxMultiChoice::globalRedraw() {
  Arg buttonArgs[4] =  {
    {XtNvisual, (XtArgVal)xxX::visual},
    {XtNcolormap, (XtArgVal)xxX::colormap},
    {XtNbackground, (XtArgVal)xxX::pixels[Colors::button]},
    {XtNforeground, (XtArgVal)xxX::pixels[Colors::foreground]}
  };
  assert (button.get());
  XtSetValues (button->widget(), buttonArgs, 4);
  assert (menu.get());
  XtSetValues (menu->widget(), buttonArgs, 4);
  for (unsigned a=0; a<choiceButtons.size(); ++a)
    XtSetValues (choiceButtons[a]->widget(), buttonArgs, 4);
  if (_box.get()) {
    Arg args[2] =  {
      {XtNbackground, (XtArgVal)xxX::pixels[Colors::background]},
      {XtNforeground, (XtArgVal)xxX::pixels[Colors::foreground]}
    };
    assert (label.get());
    XtSetValues (label->widget(), args, 2);
    XtSetValues (_box->widget(), args, 2);
  }
}

// Cleanup2006 Done
