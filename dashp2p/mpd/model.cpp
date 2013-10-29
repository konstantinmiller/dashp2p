/****************************************************************************
 * model.cpp                                                                *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Feb 13, 2012                                                 *
 * Authors: Marcel Patzlaff                                                 *
 *          Konstantin Miller <konstantin.miller@tu-berlin.de>              *
 *                                                                          *
 * This program is free software: you can redistribute it and/or modify     *
 * it under the terms of the GNU General Public License as published by     *
 * the Free Software Foundation, either version 3 of the License, or        *
 * (at your option) any later version.                                      *
 *                                                                          *
 * This program is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU General Public License        *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.     *
 ****************************************************************************/

#include "mpd/model.h"
#include "util/conversions.h"
#include "DebugAdapter.h"
#include <assert.h>
#include <stdio.h>
#include <limits>
#include <sstream>

namespace dashp2p {
    namespace mpd {
        BaseURL::BaseURL():
            value(),
            serviceLocation(),
            byteRange()
        {}

        ConditionalUint::ConditionalUint(bool value) {
            setValue(value);
        }

        ConditionalUint::ConditionalUint(unsigned int value) {
            setValue(value);
        }

        void ConditionalUint::setValue(bool value) {
            value= value ? 1 : 0;
            _isBool= true;
        }

        void ConditionalUint::setValue(unsigned int value) {
            _value= value;
            _isBool= false;
        }


        ContentComponent::ContentComponent():
            id(),
            lang(),
            contentType(),
            par(),
            accessibilities(),
            roles(),
            ratings(),
            viewPoints()
        {}


        Descriptor::Descriptor():
            schemIdUri(),
            value()
        {}


        MediaPresentationDescription::MediaPresentationDescription():
            id(),
            profiles(),
            type(EPresentation::STATIC),
            availabilityStartTime(),
            availabilityEndTime(),
            mediaPresentationDuration(),
            minimumUpdatePeriod(),
            minBufferTime(),
            timeShiftBufferDepth(),
            suggestedPresentationDelay(),
            maxSegmentDuration(),
            maxSubsegmentDuration(),
            programInformations(),
            baseURLs(),
            locations(),
            periods(),
            metrics()
        {}

#if 0
        std::vector< ::Representation> MediaPresentationDescription::getRepresentations()
        {
            dp2p_assert(getNumPeriods() == 1 && getPeriod(0).getNumAdaptationSets() == 1);
            const int numRepresentations = getAdaptationSet(0,0).getNumRepresentations();
            /* prepare return object */
            std::vector< ::Representation> representations(numRepresentations);
            /* fill the return object */
            for(unsigned i = 0; i < representations.size(); ++i)
            {
                Representation& rep = getRepresentation(0,0,i);
                dp2p_assert(rep.id.isSet() && !rep.id.get().empty());
                representations.at(i).ID = rep.id.get();
                representations.at(i).bandwidth = rep.getBandwidth();
                representations.at(i)._nominalSegmentDuration = rep._getSegmentDurationUsec();
            }
            return representations;
        }

        std::vector< ::Representation> MediaPresentationDescription::getRepresentations(unsigned width, unsigned height)
        {
            dp2p_assert(getNumPeriods() == 1 && getPeriod(0).getNumAdaptationSets() == 1);
            const AdaptationSet& adaSet = getAdaptationSet(0,0);
            const unsigned numRepresentations = adaSet.getNumRepresentations();
            /* prepare return object */
            std::vector< ::Representation> representations;
            /* fill the return object */
            for(unsigned i = 0; i < numRepresentations; ++i)
            {
                Representation& rep = getRepresentation(0,0,i);
                const unsigned repWidth = rep.width.isSet() ? rep.width.get() : adaSet.width.get();
                const unsigned repHeight = rep.height.isSet() ? rep.height.get() : adaSet.height.get();
                dp2p_assert(rep.id.isSet() && !rep.id.get().empty());
                if((width == 0 && height == 0) || (repWidth == width && repHeight == height)) {
                    ::Representation _rep;
                    _rep.ID = rep.id.get();
                    _rep.bandwidth = rep.getBandwidth();
                    _rep._nominalSegmentDuration = rep._getSegmentDurationUsec();
                    representations.push_back(_rep);
                }
            }

            /* sort */
            bool done = representations.empty() ? true : false;
            while(!done) {
                done = true;
                for(unsigned i = 0; i < representations.size() - 1; ++i) {
                    if(representations.at(i).bandwidth > representations.at(i+1).bandwidth) {
                        done = false;
                        ::Representation tmp = representations.at(i);
                        representations.at(i) = representations.at(i+1);
                        representations.at(i+1) = tmp;
                    }
                }
            }

            return representations;
        }

        unsigned MediaPresentationDescription::getNumSegments()
        {
            Representation& rep = getRepresentation(0,0,0);
            return rep.getNumSegments();
        }

        int MediaPresentationDescription::getNumAdaptationSets(const std::string& periodID)
        {
            Period& period = getPeriod(periodID);
            return period.getNumAdaptationSets();
        }

        int MediaPresentationDescription::getNumRepresentations(const std::string& periodID, const std::string& adaptationSetID)
        {
            AdaptationSet& adSet = getAdaptationSet(periodID, adaptationSetID);
            return adSet.getNumRepresentations();
        }

        int MediaPresentationDescription::getNumSegments(const std::string& periodID, const std::string& adaptationSetID, const std::string& repID)
        {
            Representation& rep = getRepresentation(periodID, adaptationSetID, repID);
            return rep.getNumSegments();
        }

        unsigned MediaPresentationDescription::getBitRate(const std::string& periodID, const std::string& adaptationSetID, const std::string& repID)
        {
            Representation& rep = getRepresentation(periodID, adaptationSetID, repID);
            return rep.getBandwidth();
        }

        const std::string& MediaPresentationDescription::getLowestRepresentation(const std::string& periodID, const std::string& adaptationSetID)
        {
            AdaptationSet& adaptationSet = getAdaptationSet(periodID, adaptationSetID);
            const int ind = adaptationSet.getLowestRepresentation();
            return getStringID(periodID, adaptationSetID, ind);
        }

        dash::Usec MediaPresentationDescription::getSegmentDuration()
        {
            const Representation& rep = getRepresentation(0, 0, 0);
            return rep._getSegmentDurationUsec();
        }

        std::pair<std::string, std::string> MediaPresentationDescription::getSegmentUrlWithoutBase(const std::string& periodID, const std::string& adaptationSetID, const std::string& representationID, const std::string& segmentID)
        {
            /* prepare the objects to hold the return value */
            std::string url;
            std::string range;

            /* get the representation */
            Representation& representation = getRepresentation(periodID, adaptationSetID, representationID);

            if(segmentID.compare("0") == 0)
            {
                if(representation.segmentBase.isSet() && representation.segmentBase.get().initialization.isSet()) {
                    URLRange& urlRange = representation.segmentBase.get().initialization.get();
                    url.append(urlRange.sourceURL.get());
                    if(urlRange.range.isSet()) {
                        range.append(urlRange.range.get());
                    }
                } else if(representation.segmentList.get().initialization.isSet()) {
                    URLRange& urlRange = representation.segmentList.get().initialization.get();
                    url.append(urlRange.sourceURL.get());
                    if(urlRange.range.isSet()) {
                        range.append(urlRange.range.get());
                    }
                } else {
                    dp2p_assert(false);
                }
            }
            else
            {
                SegmentURL& segmentURL = this->getSegment(periodID, adaptationSetID, representationID, segmentID);
                url.append(segmentURL.media.get());
                if(segmentURL.mediaRange.isSet()) {
                    range.append(segmentURL.mediaRange.get());
                }
            }

            return std::pair<std::string, std::string>(url, range);
        }

        std::pair<std::string, std::string> MediaPresentationDescription::getSegmentUrl(const std::string& periodID, const std::string& adaptationSetID, const std::string& representationID, const std::string& segmentID, const std::string& hostName)
        {
            /* prepare the objects to hold the return value */
            std::string url;
            std::string range;

            /* get the BaseURL(s) of the MPD */
            dp2p_assert(baseURLs.isSet());
            const std::vector<BaseURL*>& baseURLs = this->baseURLs.get();

            /* assert that they are all absolute and find the one that belongs to the given host name */
            for(unsigned i = 0; i < baseURLs.size(); ++i) {
                const std::string& _url = baseURLs.at(i)->value.get();
                //const std::string& _range = baseURLs.at(i)->byteRange.get();
                dp2p_assert(Utilities::isAbsoluteUrl(_url));
                if(_url.find(hostName) != _url.npos) {
                    url.append(_url);
                    break;
                }
            }
            dp2p_assert(!url.empty());

            /* assert that the period do not contain base URLs */
            Period& period = getPeriod(periodID);
            dp2p_assert(!period.baseURLs.isSet());

            /* assert that the adaptation set does not contain base URLs */
            AdaptationSet& adaptationSet = getAdaptationSet(periodID, adaptationSetID);
            dp2p_assert(!adaptationSet.baseURLs.isSet());

            /* assert that the representation does not contain base URLs */
            Representation& representation = getRepresentation(periodID, adaptationSetID, representationID);
            dp2p_assert(!representation.baseURLs.isSet());

            /* get partial URLs */
            std::pair<std::string, std::string> partialURL = getSegmentUrlWithoutBase(periodID, adaptationSetID, representationID, segmentID);

            /* concatenate */
            url.append(partialURL.first);
            range.append(partialURL.second);

            return std::pair<std::string, std::string>(url, range);
        }

        long MediaPresentationDescription::getSegmentSize(const std::string& periodID, const std::string& adSetID, const std::string& repID, const std::string& segID)
        {
            /* get partial URLs */
            std::pair<std::string, std::string> partialURL = getSegmentUrlWithoutBase(periodID, adSetID, repID, segID);

            /* get the range */
            const std::string& byteRange = partialURL.second;

            if(!byteRange.empty()) {
                /* parse the byteRange string */
                long byteRange1 = -1;
                long byteRange2 = -1;
                dp2p_assert(2 == sscanf(byteRange.c_str(), "%lu-%lu", &byteRange1, &byteRange2));
                dp2p_assert(byteRange2 > byteRange1);
                return byteRange2 - byteRange1;
            } else {
                return -1;
            }
        }

        boost::tuple<bool, const std::string&, const std::string&, const std::string&, const std::string&> MediaPresentationDescription::getNextSegment(const std::string& periodID, const std::string& adaptationSetID, const std::string& representationID, const std::string& segmentID)
        {
            Representation& representation = getRepresentation(periodID, adaptationSetID, representationID);
            SegmentList& segmentList = representation.segmentList.get();
            std::vector<SegmentURL*>& segmentURLs = segmentList.segmentURLs.get();

            if(segmentID.compare("0") == 0)
            {
                const std::string& nextSegmentID = getStringID(periodID, adaptationSetID, representationID, 1);
                return boost::tuple<bool, const std::string&, const std::string&, const std::string&, const std::string&>(true, periodID, adaptationSetID, representationID, nextSegmentID);
            }

            for(unsigned i = 0; i < segmentURLs.size(); ++i)
            {
                if(getStringID(periodID, adaptationSetID, representationID, i).compare(segmentID) == 0) {
                    if(i != segmentURLs.size() - 1) {
                        const std::string& nextSegmentID = getStringID(periodID, adaptationSetID, representationID, i + 1);
                        return boost::tuple<bool, const std::string&, const std::string&, const std::string&, const std::string&>(true, periodID, adaptationSetID, representationID, nextSegmentID);
                    } else {
                        /* TODO: add support for multiple periods. here, just go to next period. */
                        return boost::tuple<bool, const std::string&, const std::string&, const std::string&, const std::string&>(false, Utilities::emptyString, Utilities::emptyString, Utilities::emptyString, Utilities::emptyString);
                    }
                }
            }

            dp2p_assert(false);
        }

        Period& MediaPresentationDescription::getPeriod(int i)
        {
            const std::vector<Period*>& periods = this->periods.get();
            return *periods.at(i);
        }

        AdaptationSet& MediaPresentationDescription::getAdaptationSet(int i, int j)
        {
            Period& period = getPeriod(i);
            AdaptationSet& adSet = period.getAdaptationSet(j);
            return adSet;
        }

        Representation& MediaPresentationDescription::getRepresentation(int i, int j, int k)
        {
            AdaptationSet& adSet = getAdaptationSet(i, j);
            Representation& rep = adSet.getRepresentation(k);
            return rep;
        }

        Period& MediaPresentationDescription::getPeriod(const std::string& periodID)
        {
            const std::vector<Period*>& periods = this->periods.get();
            for(unsigned i = 0; i < periods.size(); ++i)
            {
                if(getStringID(i).compare(periodID) == 0) {
                    Period& period = *periods.at(i);
                    return period;
                }
            }
            dp2p_assert(false);
            static Period p;
            return p;
        }

        AdaptationSet& MediaPresentationDescription::getAdaptationSet(const std::string& periodID, const std::string& adaptationSetID)
        {
            Period& period = getPeriod(periodID);
            const std::vector<AdaptationSet*>& adaptationSets = period.adaptationSets.get();
            for(unsigned i = 0; i < adaptationSets.size(); ++i)
            {
                if(getStringID(periodID, i).compare(adaptationSetID) == 0) {
                    AdaptationSet& adaptationSet = *adaptationSets.at(i);
                    return adaptationSet;
                }
            }
            dp2p_assert(false);
            static AdaptationSet as;
            return as;
        }

        Representation& MediaPresentationDescription::getRepresentation(const std::string& periodID, const std::string& adaptationSetID, const std::string& representationID)
        {
            AdaptationSet& adaptationSet = getAdaptationSet(periodID, adaptationSetID);
            const std::vector<Representation*>& representations = adaptationSet.representations.get();
            for(unsigned i = 0; i < representations.size(); ++i)
            {
                if(getStringID(periodID, adaptationSetID, i).compare(representationID) == 0) {
                    Representation& representation = *representations.at(i);
                    return representation;
                }
            }
            dp2p_assert(false);
            static Representation r;
            return r;
        }

        SegmentURL& MediaPresentationDescription::getSegment(const std::string& periodID, const std::string& adaptationSetID, const std::string& representationID, const std::string& segmentID)
        {
            /* For initialization segments getInitializationSegment must be called */
            dp2p_assert(segmentID.compare("0") != 0);

            Representation& representation = getRepresentation(periodID, adaptationSetID, representationID);
            const SegmentList& segmentList = representation.segmentList.get();
            const std::vector<SegmentURL*>& segmentURLs = segmentList.segmentURLs.get();
            for(unsigned i = 0; i < segmentURLs.size(); ++i)
            {
                const std::string& _segID = getStringID(periodID, adaptationSetID, representationID, i + 1);
                if(_segID.compare(segmentID) == 0) {
                    SegmentURL& segmentURL = *segmentURLs.at(i);
                    return segmentURL;
                }
            }
            dp2p_assert(false);
            static SegmentURL s;
            return s;
        }

        const std::string& resentationDescription::getStringID()
        {
            if(ID.empty()) {
                if(id.isSet()) {
                    ID = id.get();
                } else {
                    char buffer[16] = {0};
                    Utilities::getRandomString(buffer, 16);
                    ID = buffer;
                }
            }
            return ID;
        }

        const std::string& MediaPresentationDescription::getStringID(int periodIndex)
        {
            const std::vector<Period*>& periods = this->periods.get();
            Period& period = *periods.at(periodIndex);
            if(period.ID.empty()) {
                if(period.id.isSet()) {
                    period.ID = period.id.get();
                } else if(period.start.isSet()) {
                    const Duration& startTime = period.start.get();
                    period.ID = util::millisToDurationString(startTime);
                } else {
                    dp2p_assert(periods.size() == 1); //TODO: extend in the future
                    period.ID = "0";
                }
            }
            return period.ID;
        }

        const std::string& MediaPresentationDescription::getStringID(int localPeriodID, int adaptationSetIndex)
        {
            Period& period = getPeriod(localPeriodID);
            const std::vector<AdaptationSet*>& adaptationSets = period.adaptationSets.get();
            AdaptationSet& adaptationSet = *adaptationSets.at(adaptationSetIndex);
            if(adaptationSet.ID.empty()) {
                if(adaptationSet.id.isSet()) {
                    std::stringstream ss;
                    ss << adaptationSet.id.get();
                    adaptationSet.ID = ss.str();
                } else {
                    dp2p_assert(adaptationSets.size() == 1); //TODO: extend in the future
                    adaptationSet.ID = "0";
                }
            }
            return adaptationSet.ID;
        }

        const std::string& MediaPresentationDescription::getStringID(int localPeriodID, int localAdaptationSetID, int representationIndex)
        {
            AdaptationSet& adaptationSet = getAdaptationSet(localPeriodID, localAdaptationSetID);
            const std::vector<Representation*>& representations = adaptationSet.representations.get();
            Representation& representation = *representations.at(representationIndex);
            if(representation.ID.empty()) {
                dp2p_assert(representation.id.isSet());
                representation.ID = representation.id.get();
            }
            return representation.ID;
        }

        const std::string& MediaPresentationDescription::getStringID(int localPeriodID, int localAdaptationSetID, int localRepresentationID, int segmentIndex)
        {
            //Period& period = getPeriod(localPeriodID);
            //AdaptationSet& adaptationSet = getAdaptationSet(localPeriodID, localAdaptationSetID);
            Representation& representation = getRepresentation(localPeriodID, localAdaptationSetID, localRepresentationID);
            if(segmentIndex == 0)
            {
                if(representation.segmentBase.isSet() && representation.segmentBase.get().initialization.isSet()) {
                    representation.segmentBase.get().localInitSegmentStringID = "0";
                    return representation.segmentBase.get().localInitSegmentStringID;
                } else if (representation.segmentList.isSet() && representation.segmentList.get().initialization.isSet()) {
                    representation.segmentList.get().localInitSegmentStringID = "0";
                    return representation.segmentList.get().localInitSegmentStringID;
                }
                dp2p_assert(false);
            }
            else
            {
                SegmentList& segmentList = representation.segmentList.get();
                std::vector<SegmentURL*>& urls = segmentList.segmentURLs.get();
                SegmentURL& url = *urls.at(segmentIndex - 1);
                if(url.localSegmentStringID.empty()) {
                    url.localSegmentStringID = url.media.get();
                    if(url.mediaRange.isSet()) {
                        url.localSegmentStringID.append(":");
                        url.localSegmentStringID.append(url.mediaRange.get());
                    }
                }
                return url.localSegmentStringID;
            }
        }

        int MediaPresentationDescription::getID()
        {
            if(mpdID == -1) {
                mpdID = IDManager::translate(getStringID());
            }
            return mpdID;
        }

        int MediaPresentationDescription::getID(int periodIndex)
        {
            const std::vector<Period*>& periods = this->periods.get();
            Period& period = *periods.at(periodIndex);
            if(period.localPeriodID == -1) {
                dp2p_assert(!ID.empty());
                boost::tuple<int,int> periodID = IDManager::translate(ID, getStringID(periodIndex));
                period.localPeriodID = periodID.get<1>();
            }
            return period.localPeriodID;
        }

        int MediaPresentationDescription::getID(int localPeriodID, int adaptationSetIndex)
        {
            Period& period = getPeriod(localPeriodID);
            const std::vector<AdaptationSet*>& adaptationSets = period.adaptationSets.get();
            AdaptationSet& adaptationSet = *adaptationSets.at(adaptationSetIndex);
            if(adaptationSet.localAdaptationSetID == -1) {
                dp2p_assert(!ID.empty() && !period.ID.empty());
                boost::tuple<int,int,int> adaptationSetID = IDManager::translate(ID, period.ID, getStringID(localPeriodID, adaptationSetIndex));
                adaptationSet.localAdaptationSetID = adaptationSetID.get<2>();
            }
            return adaptationSet.localAdaptationSetID;
        }

        int MediaPresentationDescription::getID(int localPeriodID, int localAdaptationSetID, int representationIndex)
        {
            Period& period = getPeriod(localPeriodID);
            AdaptationSet& adaptationSet = getAdaptationSet(localPeriodID, localAdaptationSetID);
            const std::vector<Representation*>& representations = adaptationSet.representations.get();
            Representation& representation = *representations.at(representationIndex);
            if(representation.localRepresentationID == -1) {
                dp2p_assert(!ID.empty() && !period.ID.empty() && !adaptationSet.ID.empty());
                boost::tuple<int,int,int,int> representationID = IDManager::translate(ID, period.ID, adaptationSet.ID, getStringID(localPeriodID, localAdaptationSetID, representationIndex));
                representation.localRepresentationID = representationID.get<3>();
            }
            return representation.localRepresentationID;
        }

        int MediaPresentationDescription::getID(int localPeriodID, int localAdaptationSetID, int localRepresentationID, int segmentIndex)
        {
            Period& period = getPeriod(localPeriodID);
            AdaptationSet& adaptationSet = getAdaptationSet(localPeriodID, localAdaptationSetID);
            Representation& representation = getRepresentation(localPeriodID, localAdaptationSetID, localRepresentationID);
            if(segmentIndex == 0) {
                /* initialization segment */
                return 0;
            } else {
                /* regular segment */
                SegmentList& segmentList = representation.segmentList.get();
                std::vector<SegmentURL*>& urls = segmentList.segmentURLs.get();
                SegmentURL& url = *urls.at(segmentIndex - 1);
                if(url.localSegmentID == -1) {
                    dp2p_assert(!ID.empty() && !period.ID.empty() && !adaptationSet.ID.empty() && !representation.ID.empty());
                    boost::tuple<int,int,int,int,int> segmentID = IDManager::translate(ID, period.ID, adaptationSet.ID, representation.ID, getStringID(localPeriodID, localAdaptationSetID, localRepresentationID, segmentIndex));
                    url.localSegmentID = segmentID.get<4>();
                }
                return url.localSegmentID;
            }
        }

        Period& MediaPresentationDescription::getPeriod(int localPeriodID)
        {
            const std::vector<Period*>& periods = this->periods.get();
            for(unsigned i = 0; i < periods.size(); ++i)
            {
                if(getID(i) == localPeriodID) {
                    Period& period = *periods.at(i);
                    return period;
                }
            }
            dp2p_assert(false);
        }

        AdaptationSet& MediaPresentationDescription::getAdaptationSet(int localPeriodID, int localAdaptationSetID)
        {
            Period& period = getPeriod(localPeriodID);
            const std::vector<AdaptationSet*>& adaptationSets = period.adaptationSets.get();
            for(unsigned i = 0; i < adaptationSets.size(); ++i)
            {
                if(getID(localPeriodID, i) == localAdaptationSetID) {
                    AdaptationSet& adaptationSet = *adaptationSets.at(i);
                    return adaptationSet;
                }
            }
            dp2p_assert(false);
        }

        Representation& MediaPresentationDescription::getRepresentation(int localPeriodID, int localAdaptationSetID, int localRepresentationID)
        {
            AdaptationSet& adaptationSet = getAdaptationSet(localPeriodID, localAdaptationSetID);
            const std::vector<Representation*>& representations = adaptationSet.representations.get();
            for(unsigned i = 0; i < representations.size(); ++i)
            {
                if(getID(localPeriodID, localAdaptationSetID, i) == localRepresentationID) {
                    Representation& representation = *representations.at(i);
                    return representation;
                }
            }
            dp2p_assert(false);
        }

        SegmentURL& MediaPresentationDescription::getSegment(int localPeriodID, int localAdaptationSetID, int localRepresentationID, int localSegmentID)
        {
            /* For initialization segments getInitializationSegment must be called */
            dp2p_assert(localSegmentID != 0);

            Representation& representation = getRepresentation(localPeriodID, localAdaptationSetID, localRepresentationID);
            const SegmentList& segmentList = representation.segmentList.get();
            const std::vector<SegmentURL*>& segmentURLs = segmentList.segmentURLs.get();
            for(unsigned i = 0; i < segmentURLs.size(); ++i)
            {
                if(getID(localPeriodID, localAdaptationSetID, localRepresentationID, i) == localSegmentID) {
                    SegmentURL& segmentURL = *segmentURLs.at(i);
                    return segmentURL;
                }
            }
            dp2p_assert(false);
        }

        int MediaPresentationDescription::getNumAdaptationSets(int localPeriodID)
        {
            Period& period = getPeriod(localPeriodID);
            return period.getNumAdaptationSets();
        }

        int MediaPresentationDescription::getLowestRepresentation(int localPeriodID, int localAdaptationSetID)
        {
            AdaptationSet& adaptationSet = getAdaptationSet(localPeriodID, localAdaptationSetID);
            int localRepresentationID = adaptationSet.getLowestRepresentation();
            return localRepresentationID;
        }

        std::pair<std::string*, std::string*> MediaPresentationDescription::getSegmentUrl(int mpdID, int localPeriodID, int localAdaptationSetID, int localRepresentationID, int localSegmentID, const std::string& hostName)
        {
            dp2p_assert(this->mpdID == mpdID);

            /* prepare the objects to hold the return value */
            std::string* url = new std::string;
            std::string* range = NULL;

            /* get the BaseURL(s) of the MPD */
            dp2p_assert(baseURLs.isSet());
            const std::vector<BaseURL*>& baseURLs = this->baseURLs.get();

            /* assert that they are all absolute and find the one that belongs to the given host name */
            for(unsigned i = 0; i < baseURLs.size(); ++i) {
                const std::string& _url = baseURLs.at(i)->value.get();
                //const std::string& _range = baseURLs.at(i)->byteRange.get();
                dp2p_assert(Utilities::isAbsoluteUrl(_url));
                if(_url.find(hostName) != _url.npos) {
                    url->append(_url);
                    break;
                }
            }
            dp2p_assert(!url->empty());

            /* assert that the period do not contain base URLs */
            Period& period = getPeriod(localPeriodID);
            dp2p_assert(!period.baseURLs.isSet());

            /* assert that the adaptation set does not contain base URLs */
            AdaptationSet& adaptationSet = getAdaptationSet(localPeriodID, localAdaptationSetID);
            dp2p_assert(!adaptationSet.baseURLs.isSet());

            /* assert that the representation does not contain base URLs */
            Representation& representation = getRepresentation(localPeriodID, localAdaptationSetID, localRepresentationID);
            dp2p_assert(!representation.baseURLs.isSet());

            if(localSegmentID == 0)
            {
                if(representation.segmentBase.isSet() && representation.segmentBase.get().initialization.isSet()) {
                    URLRange& urlRange = representation.segmentBase.get().initialization.get();
                    url->append(urlRange.sourceURL.get());
                    if(urlRange.range.isSet()) {
                        range = new std::string(urlRange.range.get());
                    }
                } else if(representation.segmentList.get().initialization.isSet()) {
                    URLRange& urlRange = representation.segmentList.get().initialization.get();
                    url->append(urlRange.sourceURL.get());
                    if(urlRange.range.isSet()) {
                        range = new std::string(urlRange.range.get());
                    }
                } else {
                    dp2p_assert(false);
                }
            }
            else
            {
                SegmentURL& segmentURL = this->getSegment(localPeriodID, localAdaptationSetID, localRepresentationID, localSegmentID);
                url->append(segmentURL.media.get());
                if(segmentURL.mediaRange.isSet()) {
                    range = new std::string(segmentURL.mediaRange.get());
                }
            }

            return std::pair<std::string*, std::string*>(url, range);
        }

        const Period& MediaPresentationDescription::getPeriod(Duration position) const
        {
            const std::vector<Period*>& periods = this->periods.get();
            for(unsigned i = 0; i < periods.size(); ++i)
            {
                const Period& period = *periods.at(i);
                const Duration& start = period.start.get();
                Duration end = 0;
                if( period.duration.isSet() ) {
                    end = start + period.duration.get();
                } else {
                    dp2p_assert( i == periods.size() - 1 );
                    end = mediaPresentationDuration.get();
                }
                if( start <= position && position <= end ) {
                    return period;
                }

            }
            dp2p_assert( false );
        }

        const std::pair<const std::string&, const std::string&> MediaPresentationDescription::getSegmentURL(const data::GlobalSegmentID& globalSegmentID) const
        {
            const data::LocalSegmentID& localSegmentID = globalSegmentID.second;
            const data::GlobalRepresentationID& globalRepresentationID = globalSegmentID.first;
            const data::LocalRepresentationID& localRepresentationID = globalRepresentationID.second;
            const data::GlobalAdaptationSetID& globalAdaptationSetID = globalRepresentationID.first;
            const data::LocalAdaptationSetID& localAdaptationSetID = globalAdaptationSetID.second;
            const data::PeriodID& periodID = globalAdaptationSetID.first;

            /* find the period */
            const Period& period = getPeriod(periodID);
            return period.getSegmentURL(localAdaptationSetID, localRepresentationID, localSegmentID);
        }
#endif

        Metrics::Metrics() :
            metrics(),
            reporting(),
            range()
        {}


        Period::Period():
            href(),
            actuate(EActuate::ON_REQUEST),
            id(),
            start(),
            duration(),
            bitstreamSwitching(false),
            baseURLs(),
            segmentBase(),
            segmentList(),
            segmentTemplate(),
            adaptationSets(),
            subSets()
        {}

#if 0
        data::PeriodID Period::getID() const
        {
            if(id.isSet())
            {
                return id.get();
            }
            else if(start.isSet())
            {
                const Duration& startTime = start.get();
                return util::millisToDurationString(startTime);
            }
            else
            {
                dp2p_assert(false);
            }
        }

        const AdaptationSet& Period::getAdaptationSet(int i) const
        {
            const std::vector<AdaptationSet*>& adaptationSets = this->adaptationSets.get();
            return *adaptationSets.at(i);
        }

        const std::pair<const std::string&, const std::string&> getSegmentURL(const data::LocalAdaptationSetID& localAdaptationSetID,
                const data::LocalRepresentationID& localRepresentationID, const data::LocalSegmentID& localSegmentID) const
        {
            const AdaptationSet& adaptationSet = getAdaptationSet(localAdaptationSetID);
            return adaptationSet.getSegmentURL(localRepresentationID, localSegmentID);
        }
#endif

        ProgramInformation::ProgramInformation():
            lang(),
            moreInformationURL(),
            title(),
            source(),
            copyright()
        {}


        Range::Range():
            starttime(),
            duration()
        {}


        RepresentationBase::RepresentationBase():
            profiles(),
            width(),
            height(),
            sar(),
            frameRate(),
            audioSamplingRate(),
            mimeType(),
            segmentProfiles(),
            codecs(),
            maximumSAPPeriod(),
            startWithSAP(),
            maxPlayoutRate(),
            codingDependency(),
            scanType(),
            framePackagings(),
            audioChannelConfigurations(),
            contentProtections()
        {}


        AdaptationSet::AdaptationSet():
            RepresentationBase(),
            href(),
            actuate(EActuate::ON_REQUEST),
            id(),
            group(),
            lang(),
            contentType(),
            par(),
            minBandwith(),
            maxBandwith(),
            minWidth(),
            maxWidth(),
            minHeight(),
            maxHeight(),
            minFrameRate(),
            maxFrameRate(),
            segmentAlignment(ConditionalUint(false)),
            subsegmentAlignment(ConditionalUint(false)),
            subsegmentStartsWithSAP(0),
            bitstreamSwitching(),
            accessibilities(),
            roles(),
            ratings(),
            viewPoints(),
            contentComponents(),
            baseURLs(),
            segmentBase(),
            segmentList(),
            segmentTemplate(),
            representations()
        {}

#if 0
        int AdaptationSet::getNumRepresentations() const
        {
            const std::vector<Representation*>& representations = this->representations.get();
            return representations.size();
        }

        Representation& AdaptationSet::getRepresentation(int i)
        {
            return *representations.get().at(i);
        }

        Representation& AdaptationSet::getRepresentation(const std::string& repID)
        {
            for(unsigned i = 0; i < representations.get().size(); ++i)
            {
                if(repID.compare(representations.get().at(i)->id.get()) == 0) {
                    return *representations.get().at(i);
                }
            }
            dp2p_assert(false);
            return *new Representation;
        }

        int AdaptationSet::getLowestRepresentation() const
        {
            const std::vector<Representation*>& representations = this->representations.get();
            unsigned minBandwidth = std::numeric_limits<unsigned int>::max();
            unsigned minInd = representations.size();
            for(unsigned i = 0; i < representations.size(); ++i)
            {
                const unsigned& bandwidth = representations.at(i)->bandwidth.get();
                if(bandwidth < minBandwidth) {
                    minBandwidth = bandwidth;
                    minInd = i;
                }
            }
            dp2p_assert(minInd < representations.size());
            return minInd;
        }

        int AdaptationSet::getLowestRepresentation() const
        {
            const std::vector<Representation*>& representations = this->representations.get();
            unsigned minBandwidth = std::numeric_limits<unsigned int>::max();
            unsigned minInd = representations.size();
            for(unsigned i = 0; i < representations.size(); ++i)
            {
                const unsigned& bandwidth = representations.at(i)->bandwidth.get();
                if(bandwidth < minBandwidth) {
                    minBandwidth = bandwidth;
                    minInd = i;
                }
            }
            dp2p_assert(minInd < representations.size());
            dp2p_assert(representations.at(minInd)->localRepresentationID != -1);
            return representations.at(minInd)->localRepresentationID;
        }

        int AdaptationSet::getID() const
        {
            if(id.isSet()) {
                return id.get();
            } else {
                /* TODO: This should happen only if there is only one AdaptationSet. The caller function should assert that. */
                return 0;
            }
        }

        const std::pair<const std::string&, const std::string&> getSegmentURL(
                const LocalRepresentationID& localRepresentationID, const LocalSegmentID& localSegmentID) const
        {
            const Representation& representation = getRepresentation(localRepresentationID);
            return representation.getSegmentURL(localSegmentID);
        }
#endif


        Representation::Representation() :
            RepresentationBase(),
            id(),
            bandwidth(),
            qualityRanking(),
            dependencyId(),
            mediaStreamStructureId(),
            baseURLs(),
            subRepresentations(),
            segmentBase(),
            segmentList(),
            segmentTemplate()
        {}

#if 0
        unsigned Representation::getBandwidth() const
        {
            return bandwidth.get();
        }

        int Representation::getNumSegments() const
        {
            const std::vector<SegmentURL*>& segURLs = segmentList.get().segmentURLs.get();
            return 1 + segURLs.size();
        }

        dash::Usec Representation::_getSegmentDurationUsec() const
        {
            const unsigned timescale = segmentList.get().timescale.isSet() ? segmentList.get().timescale.get() : 1;
            const unsigned duration = segmentList.get().duration.get();
            dp2p_assert(duration % timescale == 0);
            return ((dash::Usec)1000000 * (dash::Usec)duration) / (dash::Usec)timescale;
        }

        const std::pair<const std::string&, const std::string&> getSegmentURL(const data::LocalSegmentID& localSegmentID) const
        {
            if(localSegmentID == 0) {
                const SegmentBase& segmentBase = this->segmentBase.get();
                return segmentBase.getInitializationSegmentURL();
            }
        }

        data::LocalRepresentationID Representation::getID() const
        {
            dp2p_assert(id.isSet());
            return id.get();
        }
#endif

        SubRepresentation::SubRepresentation() :
            RepresentationBase(),
            level(),
            dependencyLevel(),
            bandwidth(),
            contentComponent()
        {}


        SegmentBase::SegmentBase():
            timescale(),
            presentationTimeOffset(),
            indexRange(),
            indexRangeExact(false),
            initialization(),
            representationIndex()
        {}


#if 0
        const std::pair<const std::string&, const std::string&> SegmentBase::getInitializationSegmentURL() const
        {
            if(initialization.isSet()) {
                return initialization.getURL();
            } else {
                return std::pair<std::string&, std::string&>(dash::Utilities::emptyString, dash::Utilities::emptyString);
            }
        }
#endif

        MultipleSegmentBase::MultipleSegmentBase() :
            SegmentBase(),
            duration(),
            startNumber(),
            segmentTimeline(),
            bitstreamSwitching()
        {}


        SegmentList::SegmentList():
            MultipleSegmentBase(),
            href(),
            actuate(EActuate::ON_REQUEST),
            segmentURLs()
        {}


        SegmentTemplate::SegmentTemplate():
            MultipleSegmentBase(),
            media(),
            index(),
            initialization(),
            bitstreamSwitching()
        {}


        SegmentTimeline::SegmentTimeline():
            s()
        {}


        SegmentTimeline::S::S():
            t(),
            d(),
            r(0)
        {}


        SegmentURL::SegmentURL():
            media(),
            mediaRange(),
            index(),
            indexRange()
        {}


        Subset::Subset():
            contains()
        {}


        URLRange::URLRange():
            sourceURL(),
            range()
        {}


        const ModelFactory& ModelFactory::DEFAULT_FACTORY= ModelFactory();
    }
}
