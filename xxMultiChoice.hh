// $Id: xxMultiChoice.hh 2641 2007-09-02 21:31:02Z flaterco $

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

class xxLocationList;

class xxMultiChoice: public xxRedrawable {
public:

  // Create box with caption and everything.  (Used by xxRoot.)
  // Choices is a NULL-terminated list.
  xxMultiChoice (const xxWidget &parent,
                 constString caption,
                 constStringArray choices,
                 unsigned initialChoice);

  // Put chooser in existing box, no caption.
  xxMultiChoice (const xxWidget &box,
		 constStringArray choices,
		 unsigned initialChoice);

  // Same as previous, but invoke caller.sortKey after changes.
  xxMultiChoice (const xxWidget &box,
		 constStringArray choices,
		 unsigned initialChoice,
		 xxLocationList &caller);

  ~xxMultiChoice();

  void callback (Widget choiceButton);
  void globalRedraw();                // See xxRedrawable.
  const unsigned choice() const;      // Accessor (get current choice).

protected:
  xxLocationList *_caller;
  std::auto_ptr<xxWidget> _box, label, button, menu;
  constStringArray _choices;
  SafeVector<xxWidget*> choiceButtons;
  BetterMap<const Widget, unsigned> buttonChoices;
  unsigned currentChoice;

  void construct (Widget boxWidget); // Common code.
};

// Cleanup2006 Done
