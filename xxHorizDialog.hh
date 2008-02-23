// $Id: xxHorizDialog.hh 2641 2007-09-02 21:31:02Z flaterco $

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

class xxHorizDialog: public xxRedrawable {

public:
  xxHorizDialog (const xxWidget &parent,
		 constString caption,
		 constString init);

  constString val() const;
  void globalRedraw();                // See xxRedrawable.

protected:
  std::auto_ptr<xxWidget> box, label, text;
  static const unsigned bufSize = 80U;
  char buf[bufSize];
};

// Cleanup2006 Done
