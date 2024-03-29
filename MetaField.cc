// $Id: MetaField.cc 2641 2007-09-02 21:31:02Z flaterco $

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

#include "common.hh"


MetaField::MetaField (const Dstr &name_, const Dstr &value_) :
	MetaFieldPair(name_, Dstr(value_).trim())
{}


void MetaFields::insert(const MetaField &field)
{
	m_vectSortOrder.push_back(field.first);
	m_mapMetaFields.insert(field);


}

unsigned MetaFields::size() const
{
	return m_vectSortOrder.size();
}

const Dstr &MetaFields::operator [] (unsigned index) const
{
	assert (index < m_vectSortOrder.size());
	return m_vectSortOrder[index];
}

const Dstr &MetaFields::operator [] (const Dstr &name) const
{
	MetaFieldMap::const_iterator itr = m_mapMetaFields.find(name);
	if (m_mapMetaFields.end() != itr)
	{
		return itr->second;
	}
	return m_strEmpty;
}


// Cleanup2006 Done
