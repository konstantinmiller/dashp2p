/****************************************************************************
 * MpdWrapper.cpp                                                           *
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "MpdWrapper.h"
#include "XmlAdapter.h"
#include "DebugAdapter.h"
#include "SegmentStorage.h"
//#include <cinttypes>
#include <limits>

namespace dashp2p {

dashp2p::mpd::MediaPresentationDescription* MpdWrapper::mpd = nullptr;

//void MpdWrapper::init(char* p, int size)
void MpdWrapper::init(const ContentIdMpd& contentIdMpd)
{
    assert(!mpd);

    const int64_t s = SegmentStorage::get(contentIdMpd).getTotalSize();
    char* p = new char[s];
    dp2p_assert(SegmentStorage::get(contentIdMpd).getData(0, p, s) == s);

    mpd = XmlAdapter::parseMpd(p, s);
#if 0
    /* caching for faster/easier access later */
    vector<pair<unsigned,unsigned> > resolutions = mpd->getSpatialResolutions();
    for(unsigned i = 0; i < resolutions.size(); ++i) {
        pair<int, int> res(resolutions.at(i).first, resolutions.at(i).second);
        dp2p_assert(reps.insert(pair<pair<int, int>, vector<Representation> >(res, mpd->getRepresentations(res.first, res.second))).second == true);
    }

    repsAll = mpd->getRepresentations(0, 0);

    /* Duration sanity check */
    if(getVideoDuration() - (getNumSegments() - 2) * getNominalSegmentDuration() < 0) {
        ERRMSG("Total number of segments: %d, nominal segment duration: %"PRId64" us, total video duration: %"PRId64".", getNumSegments(), getNominalSegmentDuration(), getVideoDuration());
        ERRMSG("Duration of last segment: %"PRId64" - %d * %"PRId64" = %"PRId64" < =! Bad MPD?", getVideoDuration(), getNumSegments(), getNominalSegmentDuration(), getVideoDuration() - (getNumSegments() - 2) * getNominalSegmentDuration());
        dp2p_assert(0);
    }
#endif
}

int MpdWrapper::getNumRepresentations(const AdaptationSetId& adaptationSetId)
{
	return getNumRepresentations(adaptationSetId.periodIndex, adaptationSetId.adaptationSetIndex);
}

int MpdWrapper::getNumRepresentations(int periodIndex, int adaptationSetIndex)
{
	const dashp2p::mpd::AdaptationSet& adaptationSet = getAdaptationSet(periodIndex, adaptationSetIndex);
	return adaptationSet.representations.get().size();
}

int MpdWrapper::getNumRepresentations(int periodIndex, int adaptationSetIndex, int width, int height)
{
	if(width == 0 || height == 0) {
		dp2p_assert(width == height);
		return getNumRepresentations(periodIndex, adaptationSetIndex);
	}

	dp2p_assert(width > 0 && height > 0);

	const dashp2p::mpd::AdaptationSet& adaptationSet = getAdaptationSet(periodIndex, adaptationSetIndex);

	int ret = 0;
	for(size_t i = 0; i < adaptationSet.representations.get().size(); ++i)
	{
		const dashp2p::mpd::Representation& rep = *(adaptationSet.representations.get().at(i));
		if(rep.width.isSet() || rep.height.isSet()) {
			if((int)rep.width.get() == width && (int)rep.height.get() == height) {
				++ret;
			}
		} else if((int)adaptationSet.width.get() == width && (int)adaptationSet.height.get() == height) {
			++ret;
		}
	}

	return ret;
}

vector<RepresentationId> MpdWrapper::getRepresentations(const AdaptationSetId& adaptationSetId)
{
	vector<RepresentationId> ret;

	for(int i = 0; i < getNumRepresentations(adaptationSetId); ++i)
		ret.push_back(RepresentationId(adaptationSetId, i));

	return ret;
}

vector<RepresentationId> MpdWrapper::getRepresentations(const AdaptationSetId& adaptationSetId, int width, int height)
{
	vector<RepresentationId> ret;

	dp2p_assert(width > 0 && height > 0);

	for(int i = 0; i < getNumRepresentations(adaptationSetId); ++i)
	{
		const RepresentationId repId(adaptationSetId, i);
		const pair<int, int> spRes = getSpatialResolution(repId);
		if(spRes.first == width && spRes.second == height)
			ret.push_back(repId);
	}

	return ret;
}

RepresentationId MpdWrapper::getRepresentationIdByBitrate(const AdaptationSetId& adaptationSetId, int bitRate)
{
	vector<RepresentationId> representations = getRepresentations(adaptationSetId);
	for(size_t i = 0; i < representations.size(); ++i)
	{
		if(getBitrate(representations.at(i)) == bitRate)
			return representations.at(i);
	}
	dp2p_assert(false);
	exit(1);
}

int MpdWrapper::getNumSegments(int periodIndex, int adaptationSetIndex, int representationIndex)
{
	const dashp2p::mpd::Representation& rep = getRepresentation(periodIndex, adaptationSetIndex, representationIndex);
	return getNumSegments(rep);
}

int MpdWrapper::getNumSegments(const dashp2p::mpd::Representation& rep)
{
	return 1 + rep.segmentList.get().segmentURLs.get().size();
}

int MpdWrapper::getNumSegments(const RepresentationId& representationId)
{
	return getNumSegments(representationId.periodIndex, representationId.adaptationSetIndex, representationId.representationIndex);
}

vector<SegmentId> MpdWrapper::getSegments(const RepresentationId& representationId)
{
	vector<SegmentId> ret;
	for(int i = 0; i < getNumSegments(representationId); ++i)
	{
		ret.push_back(SegmentId(representationId, i));
	}
	return ret;
}

int64_t MpdWrapper::getVideoDuration()
{
	return (int64_t)1000 * (int64_t)mpd->mediaPresentationDuration.get();
}

int64_t MpdWrapper::getSegmentDuration(const ContentIdSegment& segId)
{
    const dashp2p::mpd::Representation& rep = getRepresentationByBitrate(segId.periodIndex(), segId.adaptationSetIndex(), segId.bitRate());
    return getSegmentDuration(rep, segId.segmentIndex());
}

int64_t MpdWrapper::getSegmentDuration(const dashp2p::mpd::Representation& rep, int segmentIndex)
{
    if(segmentIndex == 0) {
        return 0;
    } else if(segmentIndex == getNumSegments(rep) - 1) {
        const int64_t lastSegDuration = std::max<int64_t>(1, getVideoDuration() - (getNumSegments(rep) - 2) * getNominalSegmentDuration(rep));
        return lastSegDuration;
    } else {
        return getNominalSegmentDuration(rep);
    }
}

int64_t MpdWrapper::getSegmentDuration(int periodIndex, int adaptationSetIndex, int representationIndex, int segmentIndex)
{
    const dashp2p::mpd::Representation& rep = getRepresentation(periodIndex, adaptationSetIndex, representationIndex);
    return getSegmentDuration(rep, segmentIndex);
}

int64_t MpdWrapper::getPosition(const ContentIdSegment& segId, int64_t byte, int64_t segmentSize)
{
	dp2p_assert(segId.valid());

    /* Init segment has no duration */
    if(segId.segmentIndex() == 0)
        return 0;

    const dashp2p::mpd::Representation& rep = getRepresentationByBitrate(segId.periodIndex(), segId.adaptationSetIndex(), segId.bitRate());

    const int64_t nominalDuration = getNominalSegmentDuration(rep);
    const int64_t durationCurrentSegment = getSegmentDuration(rep, segId.segmentIndex());
    return (segId.segmentIndex() - 1) * nominalDuration + (byte * durationCurrentSegment) / segmentSize;
}

int64_t MpdWrapper::getStartTime(const ContentIdSegment& segId)
{
    if(segId.segmentIndex() == 0) {
        return 0;
    } else {
    	/* Get the representation. */
    	const dashp2p::mpd::Representation& rep = getRepresentationByBitrate(segId.periodIndex(), segId.adaptationSetIndex(), segId.bitRate());

        return (segId.segmentIndex() - 1) * getNominalSegmentDuration(rep);
    }
}

int64_t MpdWrapper::getEndTime(const ContentIdSegment& segId)
{
	/* Get the representation. */
	const dashp2p::mpd::Representation& rep = getRepresentationByBitrate(segId.periodIndex(), segId.adaptationSetIndex(), segId.bitRate());

    if(segId.segmentIndex() < getNumSegments(rep) - 1) {
        return segId.segmentIndex() * getNominalSegmentDuration(rep);
    } else {
        return getVideoDuration();
    }
}

int64_t MpdWrapper::getNominalSegmentDuration(int periodIndex, int adaptationSetIndex, int representationIndex)
{
	const dashp2p::mpd::Representation& rep = getRepresentation(periodIndex, adaptationSetIndex, representationIndex);
	return getNominalSegmentDuration(rep);
}

int64_t MpdWrapper::getNominalSegmentDuration(const dashp2p::mpd::Representation& rep)
{
	const unsigned timescale = rep.segmentList.get().timescale.isSet() ? rep.segmentList.get().timescale.get() : 1;
	const unsigned duration = rep.segmentList.get().duration.get();
	dp2p_assert(duration % timescale == 0);
	return ((int64_t)1000000 * (int64_t)duration) / (int64_t)timescale;
}

vector<pair<int, int> > MpdWrapper::getSpatialResolutions(int periodIndex, int adaptationSetIndex)
{
	/* prepare return object */
	vector<pair<int, int> > resolutions;

	/* fill the return object */
	const dashp2p::mpd::AdaptationSet& adaSet = getAdaptationSet(periodIndex, adaptationSetIndex);
	for(size_t i = 0; i < adaSet.representations.get().size(); ++i)
	{
		const dashp2p::mpd::Representation& rep = *(adaSet.representations.get().at(i));
		const int width = rep.width.isSet() ? rep.width.get() : adaSet.width.get();
		const int height = rep.height.isSet() ? rep.height.get() : adaSet.height.get();
		if(resolutions.empty()) {
			resolutions.push_back(pair<int, int>(width, height));
			continue;
		}
		for(size_t j = 0; j < resolutions.size(); ++j)
		{
			if(resolutions.at(j).first == width && resolutions.at(j).second == height) {
				break;
			} else if(j == resolutions.size() - 1) {
				resolutions.push_back(pair<int, int>(width, height));
			}
		}
	}

	/* bubble sort */
	for(size_t i = 0; i < resolutions.size(); ++i) {
		for(size_t j = 0; j < resolutions.size() - 1; ++j) {
			if(resolutions.at(j).first > resolutions.at(j+1).first) {
				std::pair<int, int> temp = resolutions.at(j);
				resolutions.at(j) = resolutions.at(j + 1);
				resolutions.at(j + 1) = temp;
			}
		}
	}

	return resolutions;
}

pair<int,int> MpdWrapper::getLowestSpatialResolution(int periodIndex, int adaptationSetIndex)
{
	pair<int,int> ret(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());

	const dashp2p::mpd::AdaptationSet& adaptationSet = getAdaptationSet(periodIndex, adaptationSetIndex);

	for(size_t i = 0; i < adaptationSet.representations.get().size(); ++i)
	{
		const dashp2p::mpd::Representation& rep = *(adaptationSet.representations.get().at(i));
		if(rep.width.isSet() && rep.height.isSet()) {
			if((int)rep.width.get() * (int)rep.height.get() < ret.first * ret.second)
				ret = std::pair<int, int>(rep.width.get(), rep.height.get());
		} else if((int)adaptationSet.width.get() * (int)adaptationSet.height.get() < ret.first * ret.second) {
			ret = std::pair<int, int>(adaptationSet.width.get(), adaptationSet.height.get());
		}
	}

	return ret;
}

pair<int,int> MpdWrapper::getHighestSpatialResolution(int periodIndex, int adaptationSetIndex)
{
	pair<int,int> ret(std::numeric_limits<int>::min(), std::numeric_limits<int>::min());

	const dashp2p::mpd::AdaptationSet& adaptationSet = getAdaptationSet(periodIndex, adaptationSetIndex);

	for(size_t i = 0; i < adaptationSet.representations.get().size(); ++i)
	{
		const dashp2p::mpd::Representation& rep = *(adaptationSet.representations.get().at(i));
		if(rep.width.isSet() && rep.height.isSet()) {
			if((int)rep.width.get() * (int)rep.height.get() > ret.first * ret.second)
				ret = std::pair<int, int>(rep.width.get(), rep.height.get());
		} else if((int)adaptationSet.width.get() * (int)adaptationSet.height.get() > ret.first * ret.second) {
			ret = std::pair<int, int>(adaptationSet.width.get(), adaptationSet.height.get());
		}
	}

	return ret;
}

string MpdWrapper::printSpatialResolutions(int periodIndex, int adaptationSetIndex)
{
	vector<pair<int, int> > resolutions = getSpatialResolutions(periodIndex, adaptationSetIndex);

	const unsigned buflen = 1048576;
	char* tmp = new char[buflen];
	tmp[0] = 0;
	unsigned pos = 0;
	for(unsigned i = 0; i < resolutions.size(); ++i)
	{
		const vector<int> reps = getBitrates(periodIndex, adaptationSetIndex, resolutions.at(i).first, resolutions.at(i).second);

		pos += sprintf(tmp + pos, "%5u x %5u: ", resolutions.at(i).first, resolutions.at(i).second);
		for(unsigned j = 0; j < reps.size(); ++j) {
			if(j == reps.size() - 1 && i == resolutions.size() - 1)
				pos += sprintf(tmp + pos, "%.6f", reps.at(j) / 1e6);
			else if(j == reps.size() - 1)
				pos += sprintf(tmp + pos, "%.6f\n", reps.at(j) / 1e6);
			else
				pos += sprintf(tmp + pos, "%.6f, ", reps.at(j) / 1e6);
		}
		dp2p_assert(pos < buflen);
	}
	string ret(tmp);
	delete[] tmp;
	return ret;
}

vector<int> MpdWrapper::getBitrates(int periodIndex, int adaptationSetIndex, int width, int height)
{
	dp2p_assert((width == 0 && height == 0) || (width > 0 && height > 0));

	vector<int> ret;

	const dashp2p::mpd::AdaptationSet& adaptationSet = getAdaptationSet(periodIndex, adaptationSetIndex);

	for(size_t i = 0; i < adaptationSet.representations.get().size(); ++i)
	{
		const dashp2p::mpd::Representation& rep = *(adaptationSet.representations.get().at(i));
		bool tmp = false;
		if(width == 0 && height == 0) {
			tmp = true;
		} else if (rep.width.isSet() || rep.height.isSet()) {
			if(width == (int)rep.width.get() && height == (int)rep.height.get()) {
				tmp = true;
			}
		} else if(width == (int)adaptationSet.width.get() && height == (int)adaptationSet.height.get()) {
			tmp = true;
		}
		if(tmp) {
			dp2p_assert(rep.id.isSet() && !rep.id.get().empty());
			ret.push_back(rep.bandwidth.get());
		}
	}

	/* sort */
	bool done = ret.empty();
	while(!done)
	{
		done = true;
		for(unsigned i = 0; i < ret.size() - 1; ++i) {
			if(ret.at(i) > ret.at(i+1)) {
				done = false;
				int tmp = ret.at(i);
				ret.at(i) = ret.at(i+1);
				ret.at(i+1) = tmp;
			}
		}
	}

	return ret;
}

string MpdWrapper::getSegmentURL(const ContentIdSegment& segId)
{
	/* Get the representation. */
	const dashp2p::mpd::Representation& rep = getRepresentationByBitrate(segId.periodIndex(), segId.adaptationSetIndex(), segId.bitRate());

	/* Initialize the return value with the base URL. */
	string URL(mpd->baseURLs.get().at(0)->value.get());

	/* Get the segment URL. */
	if(segId.segmentIndex() == 0) {
		URL.append(getInitSegmentURL(rep));
	} else {
		URL.append(rep.segmentList.get().segmentURLs.get().at(segId.segmentIndex() - 1)->media.get());
	}

	return URL;
}

dashp2p::URL MpdWrapper::getSegmentUrl(const SegmentId& segmentId)
{
	/* Get the segment URL. */
	if(segmentId.isInitSegment()) {
		return getInitSegmentUrl(segmentId);
	} else {
		/* Get the base URL */
		string _url(mpd->baseURLs.get().at(0)->value.get());
		/* Get the representation */
		const dashp2p::mpd::Representation& rep = getRepresentation(segmentId);
		_url.append(rep.segmentList.get().segmentURLs.get().at(segmentId.segmentIndex - 1)->media.get());
		return dashp2p::Utilities::splitURL(_url);
	}
}

string MpdWrapper::getInitSegmentURL(const dashp2p::mpd::Representation& rep)
{
	if(rep.segmentList.isSet() && rep.segmentList.get().initialization.isSet())
		return rep.segmentList.get().initialization.get().sourceURL.get();
	else
		return rep.segmentBase.get().initialization.get().sourceURL.get();
}

dashp2p::URL MpdWrapper::getInitSegmentUrl(const RepresentationId& representationId)
{
	/* Get the base URL */
	string _url(mpd->baseURLs.get().at(0)->value.get());

	const dashp2p::mpd::Representation& rep = getRepresentation(representationId);

	if(rep.segmentList.isSet() && rep.segmentList.get().initialization.isSet())
		_url.append(rep.segmentList.get().initialization.get().sourceURL.get());
	else
		_url.append(rep.segmentBase.get().initialization.get().sourceURL.get());

	return dashp2p::Utilities::splitURL(_url);
}

ContentIdSegment MpdWrapper::getNextSegment(const ContentIdSegment& segId)
{
	const dashp2p::mpd::Representation& rep = getRepresentationByBitrate(segId.periodIndex(), segId.adaptationSetIndex(), segId.bitRate());
	dp2p_assert(segId.valid() && segId.segmentIndex() < getNumSegments(rep) - 1);
	return ContentIdSegment(segId.periodIndex(), segId.adaptationSetIndex(), segId.bitRate(), segId.segmentIndex() + 1);
}

pair<int, int> MpdWrapper::getSpatialResolution(const RepresentationId& representationId)
{
	const dashp2p::mpd::AdaptationSet& adaptationSet = getAdaptationSet(representationId);
	const dashp2p::mpd::Representation& rep = getRepresentation(representationId);
	if(rep.width.isSet()) {
		dp2p_assert(rep.height.isSet());
		return pair<int, int>(rep.width.get(), rep.height.get());
	} else {
		dp2p_assert(adaptationSet.width.isSet() && adaptationSet.height.isSet());
		return pair<int, int>(adaptationSet.width.get(), adaptationSet.height.get());
	}
}

int MpdWrapper::getBitrate(const RepresentationId& representationId)
{
	const dashp2p::mpd::Representation& rep = getRepresentation(representationId);
	return rep.bandwidth.get();
}

int MpdWrapper::getWidth(const RepresentationId& representationId)
{
	return getSpatialResolution(representationId).first;
}

int MpdWrapper::getHeight(const RepresentationId& representationId)
{
	return getSpatialResolution(representationId).second;
}

void MpdWrapper::outputVideoStatistics(const string& /*fileName*/)
{
	// TBD
}

const dashp2p::mpd::Period& MpdWrapper::getPeriod(int periodIndex)
{
	const dashp2p::mpd::Period& period = *(mpd->periods.get().at(periodIndex));
	return period;
}

const dashp2p::mpd::Period& MpdWrapper::getPeriod(const PeriodId& periodId)
{
	return getPeriod(periodId.periodIndex);
}

const dashp2p::mpd::AdaptationSet& MpdWrapper::getAdaptationSet(int periodIndex, int adaptationSetIndex)
{
	const dashp2p::mpd::Period& period = getPeriod(periodIndex);
	const dashp2p::mpd::AdaptationSet& adaptationSet = *(period.adaptationSets.get().at(adaptationSetIndex));
	return adaptationSet;
}

const dashp2p::mpd::AdaptationSet& MpdWrapper::getAdaptationSet(const AdaptationSetId& adaptationSetId)
{
	return getAdaptationSet(adaptationSetId.periodIndex, adaptationSetId.adaptationSetIndex);
}

const dashp2p::mpd::Representation& MpdWrapper::getRepresentation(const RepresentationId& representationId)
{
	return getRepresentation(representationId.periodIndex, representationId.adaptationSetIndex, representationId.representationIndex);
}

const dashp2p::mpd::Representation& MpdWrapper::getRepresentation(int periodIndex, int adaptationSetIndex, int representationIndex)
{
	const dashp2p::mpd::AdaptationSet& adaptationSet = getAdaptationSet(periodIndex, adaptationSetIndex);
	const dashp2p::mpd::Representation& representation = *(adaptationSet.representations.get().at(representationIndex));
	return representation;
}

const dashp2p::mpd::Representation& MpdWrapper::getRepresentationByBitrate(int periodIndex, int adaptationSetIndex, int bitRate)
{
	const dashp2p::mpd::AdaptationSet& adaptationSet = getAdaptationSet(periodIndex, adaptationSetIndex);

	for(size_t i = 0; i < adaptationSet.representations.get().size(); ++i)
	{
		if((int)adaptationSet.representations.get().at(i)->bandwidth.get() == bitRate) {
			return *(adaptationSet.representations.get().at(i));
		}
	}

	dp2p_assert_v(false, "Could not find representation by bitrate (%d bps).", bitRate);
	exit(1);
}

bool PeriodId::operator<(const PeriodId& other) const
{
	if(MpdId::operator<(other))
		return true;
	else if(other.MpdId::operator<(*this))
		return false;
	else
		return (periodIndex < other.periodIndex);
}

bool AdaptationSetId::operator<(const AdaptationSetId& other) const
{
	if(PeriodId::operator<(other))
		return true;
	else if(other.PeriodId::operator<(*this))
		return false;
	else
		return (adaptationSetIndex < other.adaptationSetIndex);
}

bool RepresentationId::operator<(const RepresentationId& other) const
{
	if(AdaptationSetId::operator<(other))
		return true;
	else if(other.AdaptationSetId::operator<(*this))
		return false;
	else
		return (representationIndex < other.representationIndex);
}

bool SegmentId::operator<(const SegmentId& other) const
{
	if(RepresentationId::operator<(other))
		return true;
	else if(other.RepresentationId::operator<(*this))
		return false;
	else
		return (segmentIndex < other.segmentIndex);
}

}
