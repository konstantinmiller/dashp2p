/****************************************************************************
 * MpdWrapper.h                                                             *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 11, 2012                                                 *
 * Authors: Konstantin Miller <konstantin.miller@tu-berlin.de>              *
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

#ifndef MPDWRAPPER_H_
#define MPDWRAPPER_H_


//#include "Dashp2pTypes.h"
#include "mpd/model.h"
#include <cassert>
#include <map>
#include <vector>
#include <string>

using std::pair;
using std::map;
using std::vector;
using std::string;

namespace dashp2p {

class MpdId;
class PeriodId;
class AdaptationSetId;
class RepresentationId;
class SegmentId;

class MpdWrapper
{
/* Public methods */
public:
	/**
	 * @param p     Pointer to memory holding the MPD file. Will be deleted within this function.
	 * @param size  Size of the MPD file.
	 */
    static void init(char* p, int size);
    static void cleanup() {delete mpd;}
    static bool hasMpd() {return mpd != nullptr;}

    /**********************************************************************
     * Properties of the MPD **********************************************
     **********************************************************************/

    /**
     * Returns the ID of the MPD or an empty string if the ID is not set.
     */
    static string getMpdId() {if(mpd->id.isSet()) return mpd->id.get(); else return string();}
    static int64_t getVideoDuration();

    /**********************************************************************
     * Properties of a Period *********************************************
     **********************************************************************/

    /**********************************************************************
     * Properties of an Adaptation Set ************************************
     **********************************************************************/

    static int getNumRepresentations(const AdaptationSetId& adaptationSetId);
    static int getNumRepresentations(int periodIndex, int adaptationSetIndex);
    static int getNumRepresentations(int periodIndex, int adaptationSetIndex, int width, int height);

    /**
     * Retrievs all representation of a given adaptation set.
     */
    static vector<RepresentationId> getRepresentations(const AdaptationSetId& adaptationSetId);

    /**
     * Retrievs all representation of a given adaptation set that match the specified spatial resolution width x height.
     * @param width   Horizontal resolution. Must be strictly greater than zero.
     * @param height  Vertical resolution. Must be strictly greater than zero.
     */
    static vector<RepresentationId> getRepresentations(const AdaptationSetId& adaptationSetId, int width, int height);
    static RepresentationId getRepresentationIdByBitrate(const AdaptationSetId& adaptationSetId, int bitRate);
    static vector<int> getBitrates(int periodIndex, int adaptationSetIndex, int width, int height);
    static vector<pair<int, int> > getSpatialResolutions(int periodIndex, int adaptationSetIndex);
    static pair<int,int> getLowestSpatialResolution(int periodIndex, int adaptationSetIndex);
    static pair<int,int> getHighestSpatialResolution(int periodIndex, int adaptationSetIndex);
    static string printSpatialResolutions(int periodIndex, int adaptationSetIndex);

    /**********************************************************************
     * Properties of a Representation *************************************
     **********************************************************************/

    static int getNumSegments(int periodIndex, int adaptationSetIndex, int representationIndex);
    static int getNumSegments(const dashp2p::mpd::Representation& rep);
    static int getNumSegments(const RepresentationId& representationId);
    static vector<SegmentId> getSegments(const RepresentationId& representationId);
    static int64_t getNominalSegmentDuration(int periodIndex, int adaptationSetIndex, int representationIndex);
    static int64_t getNominalSegmentDuration(const dashp2p::mpd::Representation& rep);
    static string getInitSegmentURL(const dashp2p::mpd::Representation& rep);
    static dashp2p::URL getInitSegmentUrl(const RepresentationId& representationId);
    static ContentIdSegment getNextSegment(const ContentIdSegment& segId);
    static pair<int, int> getSpatialResolution(const RepresentationId& representationId);
    static int getBitrate(const RepresentationId& representationId);
    static int getWidth(const RepresentationId& representationId);
    static int getHeight(const RepresentationId& representationId);

    /**********************************************************************
     * Properties of a Segment ********************************************
     **********************************************************************/

    static int64_t getSegmentDuration(const ContentIdSegment& setId);
    static int64_t getSegmentDuration(const dashp2p::mpd::Representation& rep, int segmentIndex);
    static int64_t getSegmentDuration(int periodIndex, int adaptationSetIndex, int representationIndex, int segmentIndex);
    static int64_t getPosition(const ContentIdSegment& segId, int64_t byte, int64_t segmentSize);
    static int64_t getStartTime(const ContentIdSegment& segId);
    static int64_t getEndTime(const ContentIdSegment& segId);

    /**
     * Returns the URL of a segment.
     */
    static string getSegmentURL(const ContentIdSegment& segId);
    static dashp2p::URL getSegmentUrl(const SegmentId& segmentId);





    //std::vector<int64_t> getSwitchingPoints(int64_t streamPosition, int num);
    static void outputVideoStatistics(const string& fileName);

/* Private methods */
private:
    MpdWrapper(){}
    virtual ~MpdWrapper(){}
    static const dashp2p::mpd::Period& getPeriod(int periodIndex);
    static const dashp2p::mpd::Period& getPeriod(const PeriodId& periodId);
    static const dashp2p::mpd::AdaptationSet& getAdaptationSet(int periodIndex, int adaptationSetIndex);
    static const dashp2p::mpd::AdaptationSet& getAdaptationSet(const AdaptationSetId& adaptationSetId);
    static const dashp2p::mpd::Representation& getRepresentation(const RepresentationId& representationId);
    static const dashp2p::mpd::Representation& getRepresentation(int periodIndex, int adaptationSetIndex, int representationIndex);
    static const dashp2p::mpd::Representation& getRepresentationByBitrate(int periodIndex, int adaptationSetIndex, int bitRate);

/* Private members */
private:
    static dashp2p::mpd::MediaPresentationDescription* mpd;
};


/********************************************************************
 * ID classes *******************************************************
 ********************************************************************/

class MpdId {
public:
	bool operator<(const MpdId& other) const {return (mpdId.compare(other.mpdId) < 0);}
	string mpdId;
};

class PeriodId: public MpdId {
public:
	PeriodId(int periodIndex): periodIndex(periodIndex) {}
	bool operator<(const PeriodId& other) const;
	int periodIndex;
};

class AdaptationSetId: public PeriodId {
public:
	AdaptationSetId(int periodIndex, int adaptationSetIndex): PeriodId(periodIndex), adaptationSetIndex(adaptationSetIndex) {}
	bool operator<(const AdaptationSetId& other) const;
	int adaptationSetIndex;
};

class RepresentationId: public AdaptationSetId {
public:
	RepresentationId(const AdaptationSetId& adaptationSetId, int representationIndex): AdaptationSetId(adaptationSetId), representationIndex(representationIndex) {}
	bool operator<(const RepresentationId& other) const;
	int representationIndex;
};

class SegmentId: public RepresentationId {
public:
	SegmentId(const RepresentationId& representationId, int segmentIndex): RepresentationId(representationId), segmentIndex(segmentIndex) {}
	bool operator<(const SegmentId& other) const;
	bool isInitSegment() const {return (segmentIndex == 0);}
	int segmentIndex;
};

}

#endif /* MPDWRAPPER_H_ */
