/*
   Copyright 2019 Equinor ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#include <opm/io/eclipse/ERst.hpp>

#include <algorithm>
#include <cstring>
#include <exception>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

#include <boost/regex.hpp>

namespace {
    int seqnumFromSeparateFilename(const std::string& filename)
    {
        const auto re = boost::regex {
            R"~(\.[FX]([0-9]{4})$)~"
        };

        auto match = boost::smatch{};
        if (boost::regex_search(filename, match, re)) {
            return std::stoi(match[1]);
        }

        throw std::invalid_argument {
            "Unable to Determine Report Step Sequence Number "
            "From Restart Filename \"" + filename + '"'
        };
    }
}

namespace Opm { namespace EclIO {

ERst::ERst(const std::string& filename)
    : EclFile(filename)
{
    if (this->hasKey("SEQNUM")) {
        this->initUnified();
    }
    else {
        this->initSeparate(seqnumFromSeparateFilename(filename));
    }
}


bool ERst::hasReportStepNumber(int number) const
{
    auto search = arrIndexRange.find(number);
    return search != arrIndexRange.end();
}


void ERst::loadReportStepNumber(int number)
{
    if (!hasReportStepNumber(number)) {
        std::string message="Trying to load non existing report step number " + std::to_string(number);
        OPM_THROW(std::invalid_argument, message);
    }

    std::vector<int> arrayIndexList;
    arrayIndexList.reserve(arrIndexRange[number].second - arrIndexRange[number].first + 1);

    for (int i = arrIndexRange[number].first; i < arrIndexRange[number].second; i++) {
        arrayIndexList.push_back(i);
    }

    loadData(arrayIndexList);

    reportLoaded[number] = true;
}


std::vector<EclFile::EclEntry> ERst::listOfRstArrays(int reportStepNumber)
{
    std::vector<EclEntry> list;

    if (!hasReportStepNumber(reportStepNumber)) {
        std::string message = "Trying to get list of arrays from non existing report step number  " + std::to_string(reportStepNumber);
        OPM_THROW(std::invalid_argument, message);
    }

    const auto& rng = this->arrIndexRange[reportStepNumber];
    list.reserve(rng.second - rng.first);

    for (int i = rng.first;  i < rng.second; i++) {
        list.emplace_back(array_name[i], array_type[i], array_size[i]);
    }

    return list;
}

void ERst::initUnified()
{
    loadData("SEQNUM");

    std::vector<int> firstIndex;

    for (size_t i = 0;  i < array_name.size(); i++) {
        if (array_name[i] == "SEQNUM") {
            auto seqn = get<int>(i);
            seqnum.push_back(seqn[0]);
            firstIndex.push_back(i);
        }
    }


    for (size_t i = 0; i < seqnum.size(); i++) {
        std::pair<int,int> range;
        range.first = firstIndex[i];

        if (i != seqnum.size() - 1) {
            range.second = firstIndex[i+1];
        } else {
            range.second = array_name.size();
        }

        arrIndexRange[seqnum[i]] = range;
    }

    nReports = seqnum.size();

    for (int i = 0; i < nReports; i++) {
        reportLoaded[seqnum[i]] = false;
    }
}

void ERst::initSeparate(const int number)
{
    auto& range = this->arrIndexRange[number];
    range.first = 0;
    range.second = static_cast<int>(this->array_name.size());

    this->seqnum.assign(1, number);
    this->nReports = 1;
    this->reportLoaded[number] = false;
}

int ERst::getArrayIndex(const std::string& name, int number) const
{
    if (!hasReportStepNumber(number)) {
        std::string message = "Trying to get vector " + name + " from non existing sequence " + std::to_string(number);
        OPM_THROW(std::invalid_argument, message);
    }

    auto search = reportLoaded.find(number);

    if (!search->second) {
        std::string message = "Data not loaded for sequence " + std::to_string(number);
        OPM_THROW(std::runtime_error, message);
    }

    auto range_it = arrIndexRange.find(number);

    std::pair<int,int> indexRange = range_it->second;

    auto it = std::find(array_name.begin() + indexRange.first,
                        array_name.begin() + indexRange.second, name);

    if (std::distance(array_name.begin(),it) == indexRange.second) {
        std::string message = "Array " + name + " not found in sequence " + std::to_string(number);
        OPM_THROW(std::runtime_error, message);
    }

    return std::distance(array_name.begin(), it);
}

std::streampos
ERst::restartStepWritePosition(const int seqnumValue) const
{
    auto pos = this->arrIndexRange.lower_bound(seqnumValue);

    return (pos == this->arrIndexRange.end())
        ? std::streampos(std::streamoff(-1))
        : this->seekPosition(pos->second.first);
}

template<>
const std::vector<int>& ERst::getRst<int>(const std::string& name, int reportStepNumber)
{
    int ind = getArrayIndex(name, reportStepNumber);
    return getImpl(ind, INTE, inte_array, "integer");
}


template<>
const std::vector<float>& ERst::getRst<float>(const std::string& name, int reportStepNumber)
{
    int ind = getArrayIndex(name, reportStepNumber);
    return getImpl(ind, REAL, real_array, "float");
}


template<>
const std::vector<double>& ERst::getRst<double>(const std::string& name, int reportStepNumber)
{
    int ind = getArrayIndex(name, reportStepNumber);
    return getImpl(ind, DOUB, doub_array, "double");
}


template<>
const std::vector<bool>& ERst::getRst<bool>(const std::string& name, int reportStepNumber)
{
    int ind = getArrayIndex(name, reportStepNumber);
    return getImpl(ind, LOGI, logi_array, "bool");
}

template<>
const std::vector<std::string>& ERst::getRst<std::string>(const std::string& name, int reportStepNumber)
{
    int ind = getArrayIndex(name, reportStepNumber);
    return getImpl(ind, CHAR, char_array, "string");
}

}} // namespace Opm::ecl
