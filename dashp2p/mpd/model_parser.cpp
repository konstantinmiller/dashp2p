/****************************************************************************
 * model_parser.cpp                                                         *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Feb 18, 2012                                                 *
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

#include "mpd/model_parser.h"
#include <iostream>

#include <cassert>
#include <cstdlib>
#include <exception>

#include "util/conversions.h"

namespace dashp2p {
    namespace mpd {
        /********************************************
         * Internal Class Definitions               *
         ********************************************/
        template<class P> struct TypedParserDescriptor : public ParserDescriptor {
        private:
            inline ElementParserBase* create() const {
                return new P();
            }

            friend class ParserContext;
        };


        class SimpleStringParser : public ElementParserBase {
        public:
            SimpleStringParser() : _fieldPtr(NULL) {}

            void setFieldPtr(field_access<std::string>* fieldPtr) {
                _fieldPtr= fieldPtr;
            }

            void attachAttribute(const std::string&, const std::string&) {}

            void attachContent(const std::string& content) {
                _fieldPtr->set(content);
            }

            ElementParserBase* attachElement(const std::string&) {
                return NULL;
            }

        protected:
            void* initialiseObject() {
                return NULL;
            }

            void validateObject() {
                _fieldPtr= NULL;
            }

        private:
            field_access<std::string>* _fieldPtr;
        };

        class SimpleStringSequenceParser : public ElementParserBase {
        public:
            SimpleStringSequenceParser() : _sequencePtr(NULL) {}

            void setSequencePtr(sequence_access<std::string>* sequencePtr) {
                _sequencePtr= sequencePtr;
            }

            void attachAttribute(const std::string&, const std::string&) {}

            void attachContent(const std::string& content) {
                _sequencePtr->add(content);
            }

            ElementParserBase* attachElement(const std::string&) {
                return NULL;
            }

        protected:
            void* initialiseObject() {
                return NULL;
            }

            void validateObject() {
                _sequencePtr= NULL;
            }

        private:
            sequence_access<std::string>* _sequencePtr;
        };


        class BaseURLParser : public ElementParserBase {
        public:
            BaseURLParser();
            void attachAttribute(const std::string& name, const std::string& value);
            void attachContent(const std::string& content);

        protected:
            void* initialiseObject();
            void validateObject();

        private:
            BaseURL* _element;
        };


        class MediaPresentationDescriptionParser : public ElementParserBase {
        public:
            MediaPresentationDescriptionParser();
            void attachAttribute(const std::string& name, const std::string& value);
            ElementParserBase* attachElement(const std::string& name);

        protected:
            void* initialiseObject();
            void validateObject();

        private:
            MediaPresentationDescription* _element;
        };


        class PeriodParser : public ElementParserBase {
        public:
            PeriodParser();
            void attachAttribute(const std::string& name, const std::string& value);
            ElementParserBase* attachElement(const std::string& name);

        protected:
            void* initialiseObject();
            void validateObject();

        private:
            Period* _element;
        };


        class ProgramInformationParser : public ElementParserBase {
        public:
            ProgramInformationParser();
            void attachAttribute(const std::string& name, const std::string& value);
            ElementParserBase* attachElement(const std::string& name);

        protected:
            void* initialiseObject();
            void validateObject();

        private:
            ProgramInformation* _element;
        };


        class RepresentationBaseParser : public ElementParserBase {
        public:
            virtual void attachAttribute(const std::string& name, const std::string& value);
            virtual ElementParserBase* attachElement(const std::string& name);

        protected:
            virtual void* initialiseObject()= 0;
            virtual void validateObject()= 0;
            virtual RepresentationBase* getModelObject()= 0;
        };


        class AdaptationSetParser : public RepresentationBaseParser {
        public:
            AdaptationSetParser();
            void attachAttribute(const std::string& name, const std::string& value);
            ElementParserBase* attachElement(const std::string& name);

        protected:
            void* initialiseObject();
            void validateObject();
            RepresentationBase* getModelObject();

        private:
            AdaptationSet* _element;
        };


        class RepresentationParser : public RepresentationBaseParser {
        public:
            RepresentationParser();
            void attachAttribute(const std::string& name, const std::string& value);
            ElementParserBase* attachElement(const std::string& name);

        protected:
            void* initialiseObject();
            void validateObject();
            RepresentationBase* getModelObject();

        private:
            Representation* _element;
        };


        class AbstractSegmentBaseParser : public ElementParserBase {
        public:
            virtual void attachAttribute(const std::string& name, const std::string& value);
            virtual ElementParserBase* attachElement(const std::string& name);

        protected:
            virtual void* initialiseObject()= 0;
            virtual void validateObject()= 0;
            virtual SegmentBase* getSegmentBase()= 0;
        };


        class AbstractMultipleSegmentBaseParser : public AbstractSegmentBaseParser {
        public:
            virtual void attachAttribute(const std::string& name, const std::string& value);
            virtual ElementParserBase* attachElement(const std::string& name);

        protected:
            virtual void* initialiseObject()= 0;
            virtual void validateObject()= 0;
            virtual MultipleSegmentBase* getMultipleSegmentBase()= 0;
            SegmentBase* getSegmentBase();
        };


        class SegmentBaseParser : public AbstractSegmentBaseParser {
        public:
            SegmentBaseParser();

        protected:
            void* initialiseObject();
            void validateObject();
            SegmentBase* getSegmentBase();

        private:
            SegmentBase* _element;
        };


        class SegmentListParser : public AbstractMultipleSegmentBaseParser {
        public:
            SegmentListParser();
            void attachAttribute(const std::string& name, const std::string& value);
            ElementParserBase* attachElement(const std::string& name);

        protected:
            void* initialiseObject();
            void validateObject();
            MultipleSegmentBase* getMultipleSegmentBase();

        private:
            SegmentList* _element;
        };


        class SegmentURLParser : public ElementParserBase {
        public:
            SegmentURLParser();
            void attachAttribute(const std::string& name, const std::string& value);

        protected:
            void* initialiseObject();
            void validateObject();

        private:
            SegmentURL* _element;
        };


        class URLRangeParser : public ElementParserBase {
        public:
            URLRangeParser();
            void attachAttribute(const std::string& name, const std::string& value);

        protected:
            void* initialiseObject();
            void validateObject();

        private:
            URLRange* _element;
        };


        /********************************************
         * Parser Descriptors                       *
         ********************************************/
        static const ParserDescriptor& NULL_DESCRIPTOR= ParserDescriptor();
        const ParserDescriptor& ParserDescriptor::ADAPTATION_SET= TypedParserDescriptor<AdaptationSetParser>();
        const ParserDescriptor& ParserDescriptor::BASE_URL= TypedParserDescriptor<BaseURLParser>();
        const ParserDescriptor& ParserDescriptor::CONTENT_COMPONENT= NULL_DESCRIPTOR;
        const ParserDescriptor& ParserDescriptor::DESCRIPTOR= NULL_DESCRIPTOR;
        const ParserDescriptor& ParserDescriptor::MEDIA_PRESENTATION_DESCRIPTION= TypedParserDescriptor<MediaPresentationDescriptionParser>();
        const ParserDescriptor& ParserDescriptor::METRICS= NULL_DESCRIPTOR;
        const ParserDescriptor& ParserDescriptor::PERIOD= TypedParserDescriptor<PeriodParser>();
        const ParserDescriptor& ParserDescriptor::PROGRAM_INFORMATION= TypedParserDescriptor<ProgramInformationParser>();
        const ParserDescriptor& ParserDescriptor::RANGE= NULL_DESCRIPTOR;
        const ParserDescriptor& ParserDescriptor::REPRESENTATION= TypedParserDescriptor<RepresentationParser>();
        const ParserDescriptor& ParserDescriptor::SEGMENT_BASE= TypedParserDescriptor<SegmentBaseParser>();
        const ParserDescriptor& ParserDescriptor::SEGMENT_LIST= TypedParserDescriptor<SegmentListParser>();
        const ParserDescriptor& ParserDescriptor::SEGMENT_TEMPLATE= NULL_DESCRIPTOR;
        const ParserDescriptor& ParserDescriptor::SEGMENT_TIMELINE= NULL_DESCRIPTOR;
        const ParserDescriptor& ParserDescriptor::SEGMENT_URL= TypedParserDescriptor<SegmentURLParser>();
        const ParserDescriptor& ParserDescriptor::SUB_REPRESENTATION= NULL_DESCRIPTOR;
        const ParserDescriptor& ParserDescriptor::SUBSET= NULL_DESCRIPTOR;
        const ParserDescriptor& ParserDescriptor::URL_RANGE= TypedParserDescriptor<URLRangeParser>();

        static const ParserDescriptor& SIMPLE_STRING= TypedParserDescriptor<SimpleStringParser>();
        static const ParserDescriptor& SIMPLE_STRING_SEQUENCE= TypedParserDescriptor<SimpleStringSequenceParser>();

        static Duration* convertDuration(const std::string& value) {
            try {
                long duration= dashp2p::util::durationStringToMillis(value);
// TODO: remove debug message
//std::cout << __FILE__ << '(' << __LINE__ << ") CONVERTED DURATION '" << value << "' to '" << duration << "'" << std::endl;
                return new Duration(duration);
            } catch (std::exception& e) {
// TODO: log this?!
std::cout << __FILE__ << '(' << __LINE__ << ") ERROR DURING CONVERSION " << e.what() << std::endl;
                return NULL;
            }
        }

        static double* convertDouble(const std::string& value) {
            try {
                double d= dashp2p::util::stringToDouble(value);
// TODO: remove debug message
std::cout << __FILE__ << '(' << __LINE__ << ") CONVERTED DOUBLE '" << value << "' to '" << d << "'" << std::endl;
                return new double(d);
            } catch (std::exception& e) {
// TODO: log this?
std::cout << __FILE__ << '(' << __LINE__ << ") ERROR DURING CONVERSION " << e.what() << std::endl;
                return NULL;
            }
        }


        static DateTime* convertDateTime(const std::string& /*value*/) {
            // TODO
            return NULL;
        }

        inline static unsigned int convertUnsignedInt(const std::string& value) {
            // TODO: is this a problem?
            return (unsigned) std::atoi(value.c_str());
        }

        static bool convertBool(const std::string& value) {
            // FIXME: better conversion!
            bool val= true;
            if(value.at(0) == 'F' || value.at(0) == 'f') {
                val= false;
            }

            return val;
        }

        ParserContext::ParserContext(const ModelFactory& modelFactory):
            factory(modelFactory),
            _parsers()
        {}

        ParserContext::~ParserContext() {
            std::map<const ParserDescriptor*, ElementParserBase*>::iterator iter;
            for(iter= _parsers.begin(); iter != _parsers.end(); iter++) {
                // delete the parser instance
                delete (iter->second);
            }
        }

        ElementParserBase* ParserContext::getParser(const ParserDescriptor& descriptor) {
            const ParserDescriptor* descrPtr= &descriptor;
            ElementParserBase* parser;

            std::map<const ParserDescriptor*, ElementParserBase*>::const_iterator lookup= _parsers.find(descrPtr);

            if(lookup == _parsers.end()) {
                parser= descriptor.create();
                if(parser != NULL) {
                    parser->ctx= this;
                    _parsers[descrPtr]= parser;
                }
            } else {
                parser= lookup->second;
            }

            return parser;
        }

        ElementParserBase::~ElementParserBase(){}

        void* ElementParserBase::pre(ElementParserBase* parent) {
            _parent= parent;
            return initialiseObject();
        }

        void ElementParserBase::attachAttribute(const std::string&, const std::string&) {
            // TODO: report unknown attribute
        }

        void ElementParserBase::attachContent(const std::string&) {
            // TODO: report content is invalid for complex type
        }

        ElementParserBase* ElementParserBase::attachElement(const std::string&) {
            // TODO report unknown element
            return NULL;
        }

        ElementParserBase* ElementParserBase::post() {
            validateObject();

            ElementParserBase* p= _parent;
            _parent= NULL;
            return p;
        }

        /********************************************
         * Parser Implementations                   *
         ********************************************/
        AdaptationSetParser::AdaptationSetParser() : _element(NULL) {}

        void* AdaptationSetParser::initialiseObject() {
            _element= ctx->factory.createAdaptationSet();
            return _element;
        }

        void AdaptationSetParser::validateObject() {
            // TODO
            _element= NULL;
        }

        RepresentationBase* AdaptationSetParser::getModelObject() {
            return _element;
        }

        void AdaptationSetParser::attachAttribute(const std::string& name, const std::string& value) {
            // TODO: parse all attributes!

            if(name == "bitstreamSwitching") {
                _element->bitstreamSwitching.set(convertBool(value));
            } else {
                RepresentationBaseParser::attachAttribute(name, value);
            }
        }

        ElementParserBase* AdaptationSetParser::attachElement(const std::string& name) {
            // TODO: parse all inner elements!
            ElementParserBase* parser= NULL;
            if(name == "Representation") {
                parser= ctx->getParser(ParserDescriptor::REPRESENTATION);
                if(parser != NULL) {
                    Representation* r= static_cast<Representation*>(parser->pre(this));
                    _element->representations.addRef(r);
                }
            }


            if(parser == NULL) {
                parser= RepresentationBaseParser::attachElement(name);
            }

            return parser;
        }


        BaseURLParser::BaseURLParser() : _element(NULL) {}

        void* BaseURLParser::initialiseObject() {
            _element= ctx->factory.createBaseURL();
            return _element;
        }

        void BaseURLParser::validateObject() {
            // TODO
            _element= NULL;
        }

        void BaseURLParser::attachAttribute(const std::string& name, const std::string& value) {
            if(name == "serviceLocation") {
                _element->serviceLocation.set(value);
            } else if(name == "byteRange") {
                _element->byteRange.set(value);
            } else {
                // TODO: report unknown attribute
            }
        }

        void BaseURLParser::attachContent(const std::string& content) {
            _element->value.set(content);
        }


        MediaPresentationDescriptionParser::MediaPresentationDescriptionParser():
            _element(NULL)
        {}

        void* MediaPresentationDescriptionParser::initialiseObject() {
            _element= ctx->factory.createMediaPresentationDescription();
            return _element;
        }

        void MediaPresentationDescriptionParser::validateObject() {
            dp2p_assert(_element->profiles.isSet() && _element->minBufferTime.isSet());
            _element= NULL;
        }

        void MediaPresentationDescriptionParser::attachAttribute(const std::string &name, const std::string &value) {
            if(name == "id") {
                _element->id.set(value);
            } else if(name == "profiles") {
                _element->profiles.set(value);
            } else if(name == "type") {
                if(value == "static") {
                    _element->type.set(EPresentation::STATIC);
                } else if(value == "dynamic") {
                    _element->type.set(EPresentation::DYNAMIC);
                } else {
                    // TODO: report invalid value
                }
            } else if(name == "availabilityStartTime") {
                _element->availabilityStartTime.handOver(convertDateTime(value));
            } else if(name == "availabilityEndTime") {
                _element->availabilityEndTime.handOver(convertDateTime(value));
            } else if(name == "mediaPresentationDuration") {
                _element->mediaPresentationDuration.handOver(convertDuration(value));
            } else if(name == "minimumUpdatePeriod") {
                _element->minimumUpdatePeriod.handOver(convertDuration(value));
            } else if(name == "minBufferTime") {
                _element->minBufferTime.handOver(convertDuration(value));
            } else if(name == "timeShiftBufferDepth") {
                _element->timeShiftBufferDepth.handOver(convertDuration(value));
            } else if(name == "suggestedPresentationDelay") {
                _element->suggestedPresentationDelay.handOver(convertDuration(value));
            } else if(name == "maxSegmentDuration") {
                _element->maxSegmentDuration.handOver(convertDuration(value));
            } else if(name == "maxSubsegmentDuration") {
                _element->maxSubsegmentDuration.handOver(convertDuration(value));
            } else {
                // TODO report warning about unknown attribute
            }
        }

        ElementParserBase* MediaPresentationDescriptionParser::attachElement(const std::string & name) {
            ElementParserBase* parser= NULL;
            if(name == "ProgramInformation") {
                parser= ctx->getParser(ParserDescriptor::PROGRAM_INFORMATION);
                if(parser != NULL) {
                    ProgramInformation* pi= static_cast<ProgramInformation*>(parser->pre(this));
                    _element->programInformations.addRef(pi);
                }
            } else if(name == "BaseURL") {
                parser= ctx->getParser(ParserDescriptor::BASE_URL);
                if(parser != NULL) {
                    BaseURL* b= static_cast<BaseURL*>(parser->pre(this));
                    _element->baseURLs.addRef(b);
                }
            } else if(name == "Location") {
                parser= ctx->getParser(SIMPLE_STRING_SEQUENCE);
                if(parser != NULL) {
                    parser->pre(this);
                    (static_cast<SimpleStringSequenceParser*>(parser))->setSequencePtr(&(_element->locations));
                }
            } else if(name == "Period") {
                parser= ctx->getParser(ParserDescriptor::PERIOD);
                if(parser != NULL) {
                    Period* p= static_cast<Period*>(parser->pre(this));
                    _element->periods.addRef(p);
                }
            } else if(name == "Metrics") {
                parser= ctx->getParser(ParserDescriptor::METRICS);
                if(parser != NULL) {
                    Metrics* m= static_cast<Metrics*>(parser->pre(this));
                    _element->metrics.addRef(m);
                }
            }

            return parser;
        }


        PeriodParser::PeriodParser() : _element(NULL) {}

        void* PeriodParser::initialiseObject() {
            _element= ctx->factory.createPeriod();
            return _element;
        }

        void PeriodParser::validateObject() {
            // TODO:
            _element= NULL;
        }

        void PeriodParser::attachAttribute(const std::string& name, const std::string& value) {
            if(name == "href") {
                _element->href.set(value);
            } else if(name == "actuate") {
                if(value == "onLoad") {
                    _element->actuate.set(EActuate::ON_LOAD);
                } else if (value == "onRequest") {
                    _element->actuate.set(EActuate::ON_REQUEST);
                } else {
                    // TODO: report invalid attribute value!
                }
            } else if(name == "id") {
                _element->id.set(value);
            } else if(name == "start") {
                _element->start.handOver(convertDuration(value));
            } else if(name == "duration") {
                _element->duration.handOver(convertDuration(value));
            } else if(name == "bitstreamSwitching") {
                _element->bitstreamSwitching.set(convertBool(value));
            }
        }

        ElementParserBase* PeriodParser::attachElement(const std::string& name) {
            ElementParserBase* parser= NULL;
            if(name == "BaseURL") {
                parser= ctx->getParser(ParserDescriptor::BASE_URL);
                if(parser != NULL) {
                    BaseURL* b= static_cast<BaseURL*>(parser->pre(this));
                    _element->baseURLs.addRef(b);
                }
            } else if(name == "SegmentBase") {
                parser= ctx->getParser(ParserDescriptor::SEGMENT_BASE);
                if(parser != NULL) {
                    SegmentBase* sb= static_cast<SegmentBase*>(parser->pre(this));
                    _element->segmentBase.handOver(sb);
                }
            } else if(name == "SegmentList") {
                parser= ctx->getParser(ParserDescriptor::SEGMENT_LIST);
                if(parser != NULL) {
                    SegmentList* sl= static_cast<SegmentList*>(parser->pre(this));
                    _element->segmentList.handOver(sl);
                }
            } else if(name == "SegmentTemplate") {
                parser= ctx->getParser(ParserDescriptor::SEGMENT_TEMPLATE);
                if(parser != NULL) {
                    SegmentTemplate* st= static_cast<SegmentTemplate*>(parser->pre(this));
                    _element->segmentTemplate.handOver(st);
                }
            } else if(name == "AdaptationSet") {
                parser= ctx->getParser(ParserDescriptor::ADAPTATION_SET);
                if(parser != NULL) {
                    AdaptationSet* as= static_cast<AdaptationSet*>(parser->pre(this));
                    _element->adaptationSets.addRef(as);
                }
            } else if(name == "Subset") {
                parser= ctx->getParser(ParserDescriptor::SUBSET);
                if(parser != NULL) {
                    Subset* ss= static_cast<Subset*>(parser->pre(this));
                    _element->subSets.addRef(ss);
                }
            }

            return parser;
        }


        ProgramInformationParser::ProgramInformationParser() : _element(NULL) {}

        void* ProgramInformationParser::initialiseObject() {
            _element= ctx->factory.createProgramInformation();
            return _element;
        }

        void ProgramInformationParser::validateObject() {
            // TODO:
            _element= NULL;
        }

        void ProgramInformationParser::attachAttribute(const std::string& name, const std::string& value) {
            if(name == "lang") {
                _element->lang.set(value);
            } else if(name == "moreInformationURL") {
                _element->moreInformationURL.set(value);
            } else {
                // TODO: report unknown attribute
            }
        }

        ElementParserBase* ProgramInformationParser::attachElement(const std::string& name) {
            ElementParserBase* parser= NULL;
            if(name == "Title") {
                parser= ctx->getParser(SIMPLE_STRING);
                if(parser != NULL) {
                    parser->pre(this);
                    (static_cast<SimpleStringParser*>(parser))->setFieldPtr(&(_element->title));
                }
            } else if(name == "Source") {
                parser= ctx->getParser(SIMPLE_STRING);
                if(parser != NULL) {
                    parser->pre(this);
                    (static_cast<SimpleStringParser*>(parser))->setFieldPtr(&(_element->source));
                }
            } else if(name == "Copyright") {
                parser= ctx->getParser(SIMPLE_STRING);
                if(parser != NULL) {
                    parser->pre(this);
                    (static_cast<SimpleStringParser*>(parser))->setFieldPtr(&(_element->copyright));
                }
            }

            return parser;
        }


        RepresentationParser::RepresentationParser() : _element(NULL) {}

        void* RepresentationParser::initialiseObject() {
            _element= ctx->factory.createRepresentation();
            return _element;
        }

        void RepresentationParser::validateObject() {
            // TODO
            _element= NULL;
        }

        RepresentationBase* RepresentationParser::getModelObject() {
            return _element;
        }

        void RepresentationParser::attachAttribute(const std::string& name, const std::string& value) {
            if(name == "id") {
                _element->id.set(value);
            } else if(name == "bandwidth") {
                _element->bandwidth.set(convertUnsignedInt(value));
            } else if(name == "qualityRanking") {
                _element->qualityRanking.set(convertUnsignedInt(value));
            } else if(name == "dependencyId") {
                // TODO: split string and put parts into sequence
            } else if(name == "mediaStreamStructureId") {
                // TODO: split string and put parts into sequence
            } else {
                RepresentationBaseParser::attachAttribute(name, value);
            }
        }

        ElementParserBase* RepresentationParser::attachElement(const std::string& name) {
            ElementParserBase* parser= NULL;
            if(name == "BaseURL") {
                parser= ctx->getParser(ParserDescriptor::BASE_URL);
                if(parser != NULL) {
                    BaseURL* b= static_cast<BaseURL*>(parser->pre(this));
                    _element->baseURLs.addRef(b);
                }
            } else if(name == "SubRepresentation") {
                parser= ctx->getParser(ParserDescriptor::SUB_REPRESENTATION);
                if(parser != NULL) {
                    SubRepresentation* sr= static_cast<SubRepresentation*>(parser->pre(this));
                    _element->subRepresentations.addRef(sr);
                }
            } else if(name == "SegmentBase") {
                parser= ctx->getParser(ParserDescriptor::SEGMENT_BASE);
                if(parser != NULL) {
                    SegmentBase* sb= static_cast<SegmentBase*>(parser->pre(this));
                    _element->segmentBase.handOver(sb);
                }
            } else if(name == "SegmentList") {
                parser= ctx->getParser(ParserDescriptor::SEGMENT_LIST);
                if(parser != NULL) {
                    SegmentList* sl= static_cast<SegmentList*>(parser->pre(this));
                    _element->segmentList.handOver(sl);
                }
            } else if(name == "SegmentTemplate") {
                parser= ctx->getParser(ParserDescriptor::SEGMENT_TEMPLATE);
                if(parser != NULL) {
                    SegmentTemplate* st= static_cast<SegmentTemplate*>(parser->pre(this));
                    _element->segmentTemplate.handOver(st);
                }
            } else {
                parser= RepresentationBaseParser::attachElement(name);
            }

            return parser;
        }


        void RepresentationBaseParser::attachAttribute(const std::string& name, const std::string& value) {
            RepresentationBase* element= getModelObject();

            if(name == "profiles") {
                element->profiles.set(value);
            } else if(name == "width") {
                element->width.set(convertUnsignedInt(value));
            } else if(name == "height") {
                element->height.set(convertUnsignedInt(value));
            } else if(name == "sar") {
                element->sar.set(value);
            } else if(name == "frameRate") {
                element->frameRate.set(value);
            } else if(name == "audioSamplingRate") {
                element->audioSamplingRate.set(value);
            } else if(name == "mimeType") {
                element->mimeType.set(value);
            } else if(name == "segmentProfiles") {
                element->segmentProfiles.set(value);
            } else if(name == "codecs") {
                element->codecs.set(value);
            } else if(name == "maximumSAPPeriod") {
                element->maximumSAPPeriod.handOver(convertDouble(value));
            } else if(name == "startWithSAP") {
                // FIXME: better conversion ?!
                element->startWithSAP.set((unsigned char) std::atoi(value.c_str()));
            } else if(name == "maxPlayoutRate") {
                element->maxPlayoutRate.handOver(convertDouble(value));
            } else if(name == "codingDependency") {
                element->codingDependency.set(convertBool(value));
            } else if(name == "scanType") {
                if(value == "progressive") {
                    element->scanType.set(EVideoScan::PROGRESSIVE);
                } else if(value == "interlaced") {
                    element->scanType.set(EVideoScan::INTERLACED);
                } else if(value == "unknown") {
                    element->scanType.set(EVideoScan::UNKNOWN);
                } else {
                    // TODO: report invalid value
                }
            } else {
                // TODO: report unknown attribute
            }
        }

        ElementParserBase* RepresentationBaseParser::attachElement(const std::string& name) {
            RepresentationBase* element= getModelObject();
            ElementParserBase* parser= NULL;
            if(name == "FramePacking") {
                parser= ctx->getParser(ParserDescriptor::DESCRIPTOR);
                if(parser != NULL) {
                    Descriptor* fp= static_cast<Descriptor*>(parser->pre(this));
                    element->framePackagings.addRef(fp);
                }
            } else if(name == "AudioChannelConfiguration") {
                parser= ctx->getParser(ParserDescriptor::DESCRIPTOR);
                if(parser != NULL) {
                    Descriptor* acc= static_cast<Descriptor*>(parser->pre(this));
                    element->audioChannelConfigurations.addRef(acc);
                }
            } else if(name == "ContentProtection") {
                parser= ctx->getParser(ParserDescriptor::DESCRIPTOR);
                if(parser != NULL) {
                    Descriptor* cp= static_cast<Descriptor*>(parser->pre(this));
                    element->contentProtections.addRef(cp);
                }
            }

            return parser;
        }


        void AbstractSegmentBaseParser::attachAttribute(const std::string& name, const std::string& value) {
            SegmentBase* element= getSegmentBase();

            if(name == "timescale") {
                element->timescale.set(convertUnsignedInt(value));
            } else if(name == "presentationTimeOffset") {
                element->presentationTimeOffset.set(convertUnsignedInt(value));
            } else if(name == "indexRange") {
                element->indexRange.set(value);
            } else if(name == "indexRangeExact") {
                element->indexRangeExact.set(convertBool(value));
            } else {
                // TODO report unknown attribute
            }
        }

        ElementParserBase* AbstractSegmentBaseParser::attachElement(const std::string& name) {
            SegmentBase* element= getSegmentBase();
            ElementParserBase* parser= NULL;

            if(name == "Initialization" || name == "Initialisation") {
                parser= ctx->getParser(ParserDescriptor::URL_RANGE);
                if(parser != NULL) {
                    URLRange* u= static_cast<URLRange*>(parser->pre(this));
                    element->initialization.handOver(u);
                }
            } else if(name == "RepresentationIndex") {
                parser= ctx->getParser(ParserDescriptor::URL_RANGE);
                if(parser != NULL) {
                    URLRange* u= static_cast<URLRange*>(parser->pre(this));
                    element->representationIndex.handOver(u);
                }
            } else {
                // TODO: report unknown element
            }

            return parser;
        }


        void AbstractMultipleSegmentBaseParser::attachAttribute(const std::string& name, const std::string& value) {
            MultipleSegmentBase* element= getMultipleSegmentBase();

            if(name == "duration") {
                element->duration.set(convertUnsignedInt(value));
            } else if(name == "startNumber") {
                element->startNumber.set(convertUnsignedInt(value));
            } else {
                AbstractSegmentBaseParser::attachAttribute(name, value);
            }
        }

        ElementParserBase* AbstractMultipleSegmentBaseParser::attachElement(const std::string& name) {
            MultipleSegmentBase* element= getMultipleSegmentBase();
            ElementParserBase* parser= NULL;

            if(name == "SegmentTimeline") {
                parser= ctx->getParser(ParserDescriptor::SEGMENT_TIMELINE);
                if(parser != NULL) {
                    SegmentTimeline* st= static_cast<SegmentTimeline*>(parser->pre(this));
                    element->segmentTimeline.handOver(st);
                }
            } else if(name == "BitstreamSwitching") {
                parser= ctx->getParser(ParserDescriptor::URL_RANGE);
                if(parser != NULL) {
                    URLRange* u= static_cast<URLRange*>(parser->pre(this));
                    element->bitstreamSwitching.handOver(u);
                }
            } else {
                parser= AbstractSegmentBaseParser::attachElement(name);
            }

            return parser;
        }

        SegmentBase* AbstractMultipleSegmentBaseParser::getSegmentBase() {
            return getMultipleSegmentBase();
        }


        SegmentBaseParser::SegmentBaseParser() : _element(NULL) {}

        void* SegmentBaseParser::initialiseObject() {
            _element= ctx->factory.createSegmentBase();
            return _element;
        }

        void SegmentBaseParser::validateObject() {
            // TODO
            _element= NULL;
        }

        SegmentBase* SegmentBaseParser::getSegmentBase() {
            return _element;
        }


        SegmentListParser::SegmentListParser() : _element(NULL) {}

        void* SegmentListParser::initialiseObject() {
            _element= ctx->factory.createSegmentList();
            return _element;
        }

        void SegmentListParser::validateObject() {
            // TODO
            _element= NULL;
        }

        MultipleSegmentBase* SegmentListParser::getMultipleSegmentBase() {
            return _element;
        }

        void SegmentListParser::attachAttribute(const std::string& name, const std::string& value) {
            if(name == "href") {
                _element->href.set(value);
            } else if(name == "actuate") {
                if(value == "onLoad") {
                    _element->actuate.set(EActuate::ON_LOAD);
                } else if (value == "onRequest") {
                    _element->actuate.set(EActuate::ON_REQUEST);
                } else {
                    // TODO: report invalid attribute value!
                }
            } else {
                AbstractMultipleSegmentBaseParser::attachAttribute(name, value);
            }
        }

        ElementParserBase* SegmentListParser::attachElement(const std::string& name) {
            ElementParserBase* parser= NULL;

            if(name == "SegmentURL") {
                parser= ctx->getParser(ParserDescriptor::SEGMENT_URL);
                if(parser != NULL) {
                    SegmentURL* su= static_cast<SegmentURL*>(parser->pre(this));
                    _element->segmentURLs.addRef(su);
                }
            } else {
                parser= AbstractMultipleSegmentBaseParser::attachElement(name);
            }

            return parser;
        }


        SegmentURLParser::SegmentURLParser() : _element(NULL) {}

        void* SegmentURLParser::initialiseObject() {
            _element= ctx->factory.createSegmentURL();
            return _element;
        }

        void SegmentURLParser::validateObject() {
            // TODO
            _element= NULL;
        }

        void SegmentURLParser::attachAttribute(const std::string& name, const std::string& value) {
            if(name == "media") {
                _element->media.set(value);
            } else if(name == "mediaRange") {
                _element->mediaRange.set(value);
            } else if(name == "index") {
                _element->index.set(value);
            } else if(name == "indexRange") {
                _element->indexRange.set(value);
            }
        }


        URLRangeParser::URLRangeParser() : _element(NULL) {}

        void* URLRangeParser::initialiseObject() {
            _element= ctx->factory.createURLRange();
            return _element;
        }

        void URLRangeParser::validateObject() {
            // TODO
            _element= NULL;
        }

        void URLRangeParser::attachAttribute(const std::string& name, const std::string& value) {
            if(name == "sourceURL") {
                _element->sourceURL.set(value);
            } else if(name == "range") {
                _element->range.set(value);
            }
        }
    }
}

