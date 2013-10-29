/****************************************************************************
 * model.h                                                                  *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Feb 13, 2012                                                 *
 * Authors: Marcel Patzlaff                                                 *
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

#ifndef MODEL_H_
#define MODEL_H_


//#include "Dashp2pTypes.h"
#include "ContentId.h"
#include "Utilities.h"


#include <vector>
#include <string>
#include <list>
#include <ctime>
#include <string.h>

#if 0
/* Type for storing information about a block of data in memory.
 * NOTE: It does not release memory upon destruction! */
class DataBlock {
public:
    DataBlock(char* buffer, unsigned size) : p(buffer), from(buffer), to(buffer + (size - 1)), size(size) {dp2p_assert(buffer && size);}
    DataBlock(const DataBlock& db) {p = new char[db.size]; memcpy(p, db.p, db.size); from = p + (db.from - db.p); to = p + (db.to - db.p); size = db.size;}
    virtual ~DataBlock(){} // default is not to delete p
    void reset() {delete [] p; p = NULL; from = NULL; to = NULL; size = 0;}
    char* p;
    char* from;
    char* to;
    unsigned size;
};
#endif


namespace dashp2p {
    namespace mpd {
        typedef std::time_t DateTime;
        typedef long Duration; /* in ms */

        typedef std::string StringNoWhitespace; /* [^\r\n\t \p{Z}]* */
        typedef unsigned char StreamAccessPoint; /* [0..6] */
        typedef std::string Ratio; /* [0-9]*:[0-9]* */
        typedef std::string FrameRate; /* [0-9]*[0-9](/[0-9]*[0-9])? */

        class BaseURL;
        class ProgramInformation;
        class MediaPresentationDescription;
        class URLRange;
        class SegmentBase;
        class Period;
        class ConditionalUint;
        class Descriptor;
        class RepresentationBase;
        class AdaptationSet;
        class ContentComponent;
        class Representation;
        class SubRepresentation;
        class Subset;
        class MultipleSegmentBase;
        class SegmentURL;
        class SegmentList;
        class SegmentTemplate;
        class S;
        class SegmentTimeline;
        class Range;
        class Metrics;

        class ModelFactory;
    }
}

namespace dashp2p {
    namespace mpd {
        template<typename T> class field_access {
        public:
            field_access():_ptr(NULL) {}

            field_access(const T& val):_ptr(NULL) {
                set(val);
            }

            ~field_access() {
                if(_ptr != NULL) {
                    delete _ptr;
                    _ptr= NULL;
                }
            }

            const T& get() const throw() {
                if(_ptr == NULL) {
                    throw "uninitialised";
                }

                return *_ptr;
            }

            T& get() throw() {
                if(_ptr == NULL) {
                    throw "uninitialised";
                }

                return *_ptr;
            }

            void set(const T& val) {
                if(_ptr != NULL) {
                    delete _ptr;
                }

                _ptr= new T(val);
            }

            bool isSet() const {
                return _ptr != NULL;
            }

            void handOver(T* ptr) {
                if(_ptr != NULL && _ptr != ptr) {
                    delete _ptr;
                }

                _ptr= ptr;
            }

            /**
             * Should only be used by parsers
             */
            T* getPtr() {
                return _ptr;
            }
        private:
            T* _ptr;
        };


        template<typename E> class sequence_access {
        public:
            sequence_access():_p_seq(NULL) {}

            ~sequence_access() {
                if(_p_seq != NULL) {
                    // delete all elements
                    typename std::vector<E*>::iterator iter;
                    for(iter= _p_seq->begin(); iter != _p_seq->end(); iter++) {
                        delete *iter;
                    }
                    delete _p_seq;
                    _p_seq= NULL;
                }
            }

            void add(const E& val) {
                if(_p_seq == NULL) {
                    _p_seq= new std::vector<E*>();
                }

                _p_seq->push_back(new E(val));
            }

            void addRef(E* ptr) {
                if(_p_seq == NULL) {
                    _p_seq= new std::vector<E*>();
                }

                _p_seq->push_back(ptr);
            }

            bool isSet() const {
                return _p_seq != NULL;
            }

            const std::vector<E*>& get() const throw() {
                if(_p_seq == NULL) {
                    throw "uninitialised";
                }

                return *_p_seq;
            }

            std::vector<E*>& get() throw() {
                if(_p_seq == NULL) {
                    throw "uninitialised";
                }

                return *_p_seq;
            }

        private:
            std::vector<E*>* _p_seq;
        };

        /* enumerations */

        /**
         * See http://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Type_Safe_Enum
         */
        template<typename def, typename inner = typename def::type>
        class safe_enum : public def {
            typedef typename def::type type;
            inner val;

        public:

            safe_enum(type v) : val(v) {}
            inner underlying() const { return val; }

            bool operator == (const safe_enum & s) const { return this->val == s.val; }
            bool operator != (const safe_enum & s) const { return this->val != s.val; }
            bool operator <  (const safe_enum & s) const { return this->val <  s.val; }
            bool operator <= (const safe_enum & s) const { return this->val <= s.val; }
            bool operator >  (const safe_enum & s) const { return this->val >  s.val; }
            bool operator >= (const safe_enum & s) const { return this->val >= s.val; }
        };

        struct e_actuate_def {
            enum type {ON_LOAD, ON_REQUEST};
        };
        typedef safe_enum<e_actuate_def> EActuate;

        struct e_presentation_def {
            enum type {STATIC, DYNAMIC};
        };
        typedef safe_enum<e_presentation_def> EPresentation;

        struct e_video_scan_def {
            enum type {PROGRESSIVE, INTERLACED, UNKNOWN};
        };
        typedef safe_enum<e_video_scan_def> EVideoScan;


        /* model classes */

        class BaseURL {
        public:
            BaseURL();
            virtual ~BaseURL(){}

            field_access<std::string> value; /*required*/
            field_access<std::string> serviceLocation;
            field_access<std::string> byteRange;
        };

        class ConditionalUint {
        public:
            ConditionalUint(bool value);
            ConditionalUint(unsigned int value);

            bool isBool() {
                return _isBool;
            }

            unsigned int getUint() {
                return _value;
            }

            bool getBool() {
                return _value != 0;
            }

            void setValue(bool value);
            void setValue(unsigned int value);

        private:
            unsigned int _value;
            bool _isBool;
        };


        class ContentComponent {
        public:
            ContentComponent();
            virtual ~ContentComponent(){}

            field_access<unsigned int> id;
            field_access<std::string> lang;
            field_access<std::string> contentType;
            field_access<Ratio> par;

            sequence_access<Descriptor> accessibilities;
            sequence_access<Descriptor> roles;
            sequence_access<Descriptor> ratings;
            sequence_access<Descriptor> viewPoints;
        };


        class Descriptor {
        public:
            Descriptor();
            virtual ~Descriptor(){}

            field_access<std::string> schemIdUri; /*required*/
            field_access<std::string> value;
        };


        class Metrics {
        public:
            Metrics();
            virtual ~Metrics(){}

            field_access<std::string> metrics; /*required*/
            sequence_access<Descriptor> reporting;
            sequence_access<Range> range;
        };


        class Range {
        public:
            Range();
            virtual ~Range(){}

            field_access<Duration> starttime;
            field_access<Duration> duration;
        };


        class RepresentationBase {
        public:
            virtual ~RepresentationBase(){}

        protected:
            RepresentationBase();

        public:
            field_access<std::string> profiles;
            field_access<unsigned int> width;
            field_access<unsigned int> height;
            field_access<Ratio> sar;
            field_access<FrameRate> frameRate;
            field_access<std::string> audioSamplingRate;
            field_access<std::string> mimeType;
            field_access<std::string> segmentProfiles;
            field_access<std::string> codecs;
            field_access<double> maximumSAPPeriod;
            field_access<StreamAccessPoint> startWithSAP;
            field_access<double> maxPlayoutRate;
            field_access<bool> codingDependency;
            field_access<EVideoScan> scanType;

            sequence_access<Descriptor> framePackagings;
            sequence_access<Descriptor> audioChannelConfigurations;
            sequence_access<Descriptor> contentProtections;
        };


        class SubRepresentation : public RepresentationBase {
        public:
            SubRepresentation();
            virtual ~SubRepresentation(){}

            field_access<unsigned int> level;
            sequence_access<unsigned int> dependencyLevel;
            field_access<unsigned int> bandwidth;
            sequence_access<std::string> contentComponent;
        };


        class URLRange {
        public:
            URLRange();
            virtual ~URLRange(){}

            field_access<std::string> sourceURL;
            field_access<std::string> range;
        };


        class SegmentURL {
        public:
            SegmentURL();
            virtual ~SegmentURL(){}

            //int localSegmentID;
            //std::string localSegmentStringID;

            field_access<std::string> media;
            field_access<std::string> mediaRange;
            field_access<std::string> index;
            field_access<std::string> indexRange;
        };


        class SegmentBase {
        public:
            SegmentBase();
            virtual ~SegmentBase(){}

            //const std::pair<const std::string&, const std::string&> getInitializationSegmentURL() const;

            //int localInitSegmentID;
            //std::string localInitSegmentStringID;

            field_access<unsigned int> timescale;
            field_access<unsigned int> presentationTimeOffset;
            field_access<std::string> indexRange;
            field_access<bool> indexRangeExact;

            field_access<URLRange> initialization;
            field_access<URLRange> representationIndex;
        };


        class MultipleSegmentBase : public SegmentBase {
        protected:
            MultipleSegmentBase();
            virtual ~MultipleSegmentBase(){}

        public:
            field_access<unsigned int> duration;
            field_access<unsigned int> startNumber;

            field_access<SegmentTimeline> segmentTimeline;
            field_access<URLRange> bitstreamSwitching;
        };


        class SegmentList : public MultipleSegmentBase {
        public:
            SegmentList();
            virtual ~SegmentList(){}

            field_access<std::string> href;
            field_access<EActuate> actuate;

            sequence_access<SegmentURL> segmentURLs;
        };


        class ProgramInformation {
        public:
            ProgramInformation();
            virtual ~ProgramInformation(){}

            field_access<std::string> lang;
            field_access<std::string> moreInformationURL;

            field_access<std::string> title;
            field_access<std::string> source;
            field_access<std::string> copyright;
        };


        class SegmentTemplate : public MultipleSegmentBase {
        public:
            SegmentTemplate();
            virtual ~SegmentTemplate(){}

            field_access<std::string> media;
            field_access<std::string> index;
            field_access<std::string> initialization;
            field_access<std::string> bitstreamSwitching;
        };


        class SegmentTimeline {
        public:
            class S {
            public:
                S();
                ~S(){}

                field_access<unsigned int> t;
                field_access<unsigned int> d; /*required*/
                field_access<unsigned int> r;
            };

            SegmentTimeline();
            virtual ~SegmentTimeline(){}

            sequence_access<S> s;
        };


        class Subset {
        public:
            Subset();
            virtual ~Subset(){}

            sequence_access<unsigned int> contains; /*required*/
        };


        class Representation : public RepresentationBase {
        public:
            Representation();
            virtual ~Representation(){}

            //unsigned getBandwidth() const;
            //int getNumSegments() const;
            //dash::Usec _getSegmentDurationUsec() const;
            //const std::pair<const std::string&, const std::string&> getSegmentURL(const data::LocalSegmentID& localSegmentID) const;

            //std::string ID;

            field_access<StringNoWhitespace> id; /*required*/
            field_access<unsigned int> bandwidth; /*required*/
            field_access<unsigned int> qualityRanking;
            sequence_access<std::string> dependencyId;
            sequence_access<std::string> mediaStreamStructureId;

            sequence_access<BaseURL> baseURLs;
            sequence_access<SubRepresentation> subRepresentations;
            field_access<SegmentBase> segmentBase;
            field_access<SegmentList> segmentList;
            field_access<SegmentTemplate> segmentTemplate;
        };


        class AdaptationSet : public RepresentationBase {
        public:
            AdaptationSet();
            virtual ~AdaptationSet(){}

            //int getNumRepresentations() const;
            //Representation& getRepresentation(int i);
            //Representation& getRepresentation(const std::string& repID);
            //int getLowestRepresentation() const;

#if 0
int getLowestRepresentation() const;
//data::LocalAdaptationSetID getID() const;
//const std::pair<const std::string&, const std::string&> getSegmentURL(const data::LocalRepresentationID& localRepresentationID, const data::LocalSegmentID& localSegmentID) const;
#endif

            //std::string ID;

            field_access<std::string> href;
            field_access<EActuate> actuate;
            field_access<unsigned int> id;
            field_access<unsigned int> group;
            field_access<std::string> lang;
            field_access<std::string> contentType;
            field_access<Ratio> par;
            field_access<unsigned int> minBandwith;
            field_access<unsigned int> maxBandwith;
            field_access<unsigned int> minWidth;
            field_access<unsigned int> maxWidth;
            field_access<unsigned int> minHeight;
            field_access<unsigned int> maxHeight;
            field_access<FrameRate> minFrameRate;
            field_access<FrameRate> maxFrameRate;
            field_access<ConditionalUint> segmentAlignment;
            field_access<ConditionalUint> subsegmentAlignment;
            field_access<StreamAccessPoint> subsegmentStartsWithSAP;
            field_access<bool> bitstreamSwitching;

            sequence_access<Descriptor> accessibilities;
            sequence_access<Descriptor> roles;
            sequence_access<Descriptor> ratings;
            sequence_access<Descriptor> viewPoints;
            sequence_access<ContentComponent> contentComponents;
            sequence_access<BaseURL> baseURLs;
            field_access<SegmentBase> segmentBase;
            field_access<SegmentList> segmentList;
            field_access<SegmentTemplate> segmentTemplate;
            sequence_access<Representation> representations;
        };


        class Period {
        public:
            Period();
            virtual ~Period(){}

            //data::PeriodID getID() const;
            //int getNumAdaptationSets() const {return adaptationSets.get().size();}
            //AdaptationSet& getAdaptationSet(int i) {return *adaptationSets.get().at(i);}
            //const AdaptationSet& getAdaptationSet(int i) const;
            //const std::pair<const std::string&, const std::string&> getSegmentURL(const data::LocalAdaptationSetID& localAdaptationSetID, const data::LocalRepresentationID& localRepresentationID, const data::LocalSegmentID& localSegmentID) const;

            //std::string ID;

            field_access<std::string> href;
            field_access<EActuate> actuate;
            field_access<std::string> id;
            field_access<Duration> start;
            field_access<Duration> duration;
            field_access<bool> bitstreamSwitching;

            sequence_access<BaseURL> baseURLs;
            field_access<SegmentBase> segmentBase;
            field_access<SegmentList> segmentList;
            field_access<SegmentTemplate> segmentTemplate;
            sequence_access<AdaptationSet> adaptationSets;
            sequence_access<Subset> subSets;
        };


        class MediaPresentationDescription {
        public:
            MediaPresentationDescription();
            virtual ~MediaPresentationDescription(){}

            /*
             * Simplified interface.
             */

            //dash::Usec getVideoDuration() {dp2p_assert(mediaPresentationDuration.isSet()); return (dash::Usec)1000 * (dash::Usec)mediaPresentationDuration.get();}

            /* Returns the list of bit-rates of the first period, first adaptation set, accompanied by their id's. */
            //std::vector< ::Representation> getRepresentations();
            //std::vector< ::Representation> getRepresentations(unsigned width, unsigned height);

            /* Returns the number of segments (including the init segment) of the first period, first adaptation set. */
            //unsigned getNumSegments();

            /*
             * Old stuff.
             */

            //const std::string& getStringID();
            //const std::string& getStringID(int periodIndex);
            //const std::string& getStringID(const std::string& periodID, int adaptationSetIndex);
            //const std::string& getStringID(const std::string& periodID, const std::string& adaptationSetID, int representationIndex);
            //const std::string& getStringID(const std::string& periodID, const std::string& adaptationSetID, const std::string& rpresentationID, int segmentIndex);

            //int getNumPeriods() const {return periods.get().size();}
            //int getNumAdaptationSets(const std::string& periodID);
            //int getNumRepresentations(const std::string& periodID, const std::string& adaptationSetID);
            //int getNumSegments(const std::string& periodID, const std::string& adaptationSetID, const std::string& repID);

            //unsigned getBitRate(const std::string& periodID, const std::string& adaptationSetID, const std::string& repID);
            //const std::string& getLowestRepresentation(const std::string& periodID, const std::string& adaptationSetID);

            //dash::Usec getSegmentDuration();
            //std::pair<std::string, std::string> getSegmentUrlWithoutBase(const std::string& periodID, const std::string& adaptationSetID, const std::string& representationID, const std::string& segmentID);
            //std::pair<std::string, std::string> getSegmentUrl(const std::string& periodID, const std::string& adaptationSetID, const std::string& representationID, const std::string& segmentID, const std::string& hostName);
            //long getSegmentSize(const std::string& periodID, const std::string& adSetID, const std::string& repID, const std::string& segID);
            //boost::tuple<bool, const std::string&, const std::string&, const std::string&, const std::string&> getNextSegment(const std::string& periodID, const std::string& adaptationSetID, const std::string& representationID, const std::string& segmentID);

            //const Period& getPeriod(Duration pos_msec) const;
            //const std::pair<const std::string&, const std::string&> getSegmentURL(const data::GlobalSegmentID& globalSegmentID) const;

        private:
            //Period& getPeriod(int i);
            //AdaptationSet& getAdaptationSet(int i, int j);
            //Representation& getRepresentation(int i, int j, int k);

            //Period& getPeriod(const std::string& periodID);
            //AdaptationSet& getAdaptationSet(const std::string& periodID, const std::string& adaptationSetID);
            //Representation& getRepresentation(const std::string& periodID, const std::string& adaptationSetID, const std::string& representationID);
            //SegmentURL& getSegment(const std::string& periodID, const std::string& adaptationSetID, const std::string& representationID, const std::string& segmentID);

            //double getSegmentDuration(int i, int j, int k, int l) const;

        private:
            //std::string ID;

        public:
            field_access<std::string> id;
            field_access<std::string> profiles; /*required*/
            field_access<EPresentation> type;
            field_access<DateTime> availabilityStartTime;
            field_access<DateTime> availabilityEndTime;
            field_access<Duration> mediaPresentationDuration;
            field_access<Duration> minimumUpdatePeriod;
            field_access<Duration> minBufferTime; /*required*/
            field_access<Duration> timeShiftBufferDepth;
            field_access<Duration> suggestedPresentationDelay;
            field_access<Duration> maxSegmentDuration;
            field_access<Duration> maxSubsegmentDuration;

            sequence_access<ProgramInformation> programInformations;
            sequence_access<BaseURL> baseURLs;
            sequence_access<std::string> locations;
            sequence_access<Period> periods;
            sequence_access<Metrics> metrics;
        };


        class ModelFactory {
        public:
            const static ModelFactory& DEFAULT_FACTORY;

            virtual BaseURL* createBaseURL() const {return new BaseURL();}
            virtual ProgramInformation* createProgramInformation() const {return new ProgramInformation();}
            virtual MediaPresentationDescription* createMediaPresentationDescription() const {return new MediaPresentationDescription();}
            virtual URLRange* createURLRange() const {return new URLRange();}
            virtual SegmentBase* createSegmentBase() const {return new SegmentBase();}
            virtual Period* createPeriod() const {return new Period();}
            virtual Descriptor* createDescriptor() const {return new Descriptor();}
            virtual AdaptationSet* createAdaptationSet() const {return new AdaptationSet();}
            virtual ContentComponent* createContentComponent() const {return new ContentComponent();}
            virtual Representation* createRepresentation() const {return new Representation();}
            virtual SubRepresentation* createSubRepresentation() const {return new SubRepresentation();}
            virtual Subset* createSubSet() const {return new Subset();}
            virtual SegmentURL* createSegmentURL() const {return new SegmentURL();}
            virtual SegmentList* createSegmentList() const {return new SegmentList();}
            virtual SegmentTemplate* createSegmentTemplate() const {return new SegmentTemplate();}
            virtual SegmentTimeline* createSegmentTimeline() const {return new SegmentTimeline();}
            virtual Range* createRange() const {return new Range();}
            virtual Metrics* createMetrics() const {return new Metrics();}

        protected:
            ModelFactory(){}

        public:
            virtual ~ModelFactory(){}
        };
    }
}


#endif /* MODEL_H_ */
