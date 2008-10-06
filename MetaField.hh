// $Id: MetaField.hh 2641 2007-09-02 21:31:02Z flaterco $

/*
    Copyright (C) 2004  David Flater.

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
#include <map>
#include <vector>

typedef std::map<const Dstr, const Dstr>  MetaFieldMap;
typedef std::pair<const Dstr, const Dstr> MetaFieldPair;


struct MetaField : MetaFieldPair {

  // The constructor trims whitespace from value_.
  MetaField (const Dstr &name_, const Dstr &value_);

};

struct MetaFields 
{
	void insert(const MetaField &field);

	MetaFieldMap             m_mapMetaFields;
	std::vector<Dstr>        m_vectSortOrder;
	Dstr                     m_strEmpty;

	unsigned size() const;
	const Dstr &operator [] (unsigned index) const;
	const Dstr &operator [] (const Dstr &name) const;
};

// Cleanup2006 Done
