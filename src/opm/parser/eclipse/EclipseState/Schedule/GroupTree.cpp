/*
  Copyright 2013 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <stdexcept>
#include <iostream>

#include <opm/parser/eclipse/EclipseState/Schedule/GroupTree.hpp>

namespace Opm {

void GroupTree::update( const std::string& name) {
    this->update( name, "FIELD");
}

/*
 * Insertions are only done via the update method which maintains the
 * underlying group vector sorted on group names. This requires group names to
 * be unique, but simplifies the implementation greatly and emphasises that
 * this grouptree class is just meta data for the actual group objects (stored
 * and represented elsewhere)
 */

const std::map<std::string , size_t>& GroupTree::nameSeqIndMap() const {
    return m_nameSeqIndMap;
}

const std::map<size_t, std::string >& GroupTree::seqIndNameMap() const {
    return m_seqIndNameMap;
}

void GroupTree::update( const std::string& name, const std::string& other_parent) {
    if( name == "FIELD" )
        throw std::invalid_argument( "The FIELD group name is reserved." );

    if( other_parent.empty() )
        throw std::invalid_argument( "Parent group must have a name." );

    auto root = this->find( other_parent );
    if( root == this->groups.end() || root->name != other_parent )
        this->groups.insert( root, 1, group { other_parent, "FIELD" } );

    auto node = this->find( name );
    if( node == this->groups.end() || node->name != name ) {
        this->groups.insert( node, 1, group { name, other_parent } );
        return;
    }

    node->parent = other_parent;
}

void GroupTree::updateSeqIndex( const std::string& name, const std::string& other_parent) {
    if( name == "FIELD" )
        throw std::invalid_argument( "The FIELD group name is reserved." );

    if( other_parent.empty() )
        throw std::invalid_argument( "Parent group must have a name." );

    // add code to set an index that determine the sequence of the groups
    // defined in the group tree
    size_t index = this->m_nameSeqIndMap.size();
    auto name_itr = this->m_nameSeqIndMap.find(name);
    if (name_itr == this->m_nameSeqIndMap.end()) {
        this->m_nameSeqIndMap.insert(std::make_pair(name, index));
        this->m_seqIndNameMap.insert(std::make_pair(index, name));
        index +=1;
    }
    auto parent_itr = this->m_nameSeqIndMap.find(other_parent);
    if (parent_itr == this->m_nameSeqIndMap.end()) {
        this->m_nameSeqIndMap.insert(std::make_pair(other_parent, index));
        this->m_seqIndNameMap.insert(std::make_pair(index, other_parent));
    }
}


bool GroupTree::exists( const std::string& name ) const {
    return std::binary_search( this->groups.begin(),
                               this->groups.end(),
                               name );
}

const std::string& GroupTree::parent( const std::string& name ) const {
    auto node = std::find( this->groups.begin(), this->groups.end(), name );

    if( node == this->groups.end() )
        throw std::out_of_range( "No such group: '" + name + "'." );

    return node->parent;
}

std::vector< std::string > GroupTree::children( const std::string& other_parent ) const {
    if( !this->exists( other_parent ) )
        throw std::out_of_range( "Node '" + other_parent + "' does not exist." );

    std::vector< std::string > kids;
    for( auto it = this->groups.begin(); it != this->groups.end(); it++ ) {
        if( (*it).parent != other_parent ) continue;
        kids.push_back( (*it).name );
    }

    return kids;
}

bool GroupTree::operator==( const GroupTree& rhs ) const {
    return this->groups.size() == rhs.groups.size()
        && std::equal( this->groups.begin(),
                       this->groups.end(),
                       rhs.groups.begin() );
}

bool GroupTree::operator!=( const GroupTree& rhs ) const {
    return !( *this == rhs );
}

bool GroupTree::group::operator==( const std::string& rhs ) const {
    return this->name == rhs;
}

bool GroupTree::group::operator!=( const std::string& rhs ) const {
    return !( *this == rhs );
}

bool GroupTree::group::operator==( const GroupTree::group& rhs ) const {
    return this->name == rhs.name && this->parent == rhs.parent;
}

bool GroupTree::group::operator!=( const GroupTree::group& rhs ) const {
    return !( *this == rhs );
}

bool GroupTree::group::operator<( const GroupTree::group& rhs ) const {
    return this->name < rhs.name;
}

bool GroupTree::group::operator<( const std::string& rhs ) const {
    return this->name < rhs;
}

bool operator<( const std::string& lhs, const GroupTree::group& rhs ) {
    return lhs < rhs.name;
}

std::vector< GroupTree::group >::iterator GroupTree::find( const std::string& name ) {
    return std::lower_bound( this->groups.begin(),
                             this->groups.end(),
                             name );
}

std::vector<GroupTree::group>::const_iterator GroupTree::begin() const {
    return this->groups.begin();
}

std::vector<GroupTree::group>::const_iterator GroupTree::end() const {
    return this->groups.end();
}

std::ostream& operator<<(std::ostream& stream, const GroupTree& gt) {
    for (const auto& group_pair : gt)
        stream << group_pair.parent << " -> " << group_pair.name << std::endl;

    stream << std::endl << std::endl;
    return stream;
}

}
