// $Id: Banner.hh 2641 2007-09-02 21:31:02Z flaterco $

/*  Banner  Graph printed sideways on tractor feed dot matrix printer

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

class Banner: public TTYGraph {
public:

  static Banner * const factory (const Station &station,
				 unsigned xSize,
				 Timestamp startTime,
				 Timestamp endTime);

  void print (Dstr &text_out);

protected:
  Banner (unsigned xSize, unsigned ySize);

  // Overridden virtual methods.
  const bool isBanner() const;
  const double aspectFudgeFactor() const;
  void drawHorizontalLine (int xlo, int xhi, int y, Colors::Colorchoice c);
  void drawHourTick  (int x, Colors::Colorchoice c);
  void labelHourTick (int x, const Dstr &label);
  void labelEvent    (int topLine, const EventBlurb &blurb);
  void drawTitleLine (const Dstr &title);
  void measureBlurb  (EventBlurb &blurb) const;

  // New method.
  void drawStringSideways (int x, int y, const Dstr &s);
};

// Cleanup2006 Done
