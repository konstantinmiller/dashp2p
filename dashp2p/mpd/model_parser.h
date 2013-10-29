/****************************************************************************
 * model_parser.h                                                           *
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

#ifndef MODEL_PARSER_H_
#define MODEL_PARSER_H_

#include "mpd/model.h"
#include <string>
#include <map>

namespace dashp2p {
    namespace mpd {
        struct ParserDescriptor;
        class ParserContext;
        class ElementParserBase;
    }
}

namespace dashp2p {
    namespace mpd {
        class ParserDescriptor {
        public:
        	virtual ~ParserDescriptor() {}
            static const ParserDescriptor& ADAPTATION_SET;
            static const ParserDescriptor& BASE_URL;
            static const ParserDescriptor& CONTENT_COMPONENT;
            static const ParserDescriptor& DESCRIPTOR;
            static const ParserDescriptor& MEDIA_PRESENTATION_DESCRIPTION;
            static const ParserDescriptor& METRICS;
            static const ParserDescriptor& PERIOD;
            static const ParserDescriptor& PROGRAM_INFORMATION;
            static const ParserDescriptor& RANGE;
            static const ParserDescriptor& REPRESENTATION;
            static const ParserDescriptor& SEGMENT_BASE;
            static const ParserDescriptor& SEGMENT_LIST;
            static const ParserDescriptor& SEGMENT_TEMPLATE;
            static const ParserDescriptor& SEGMENT_TIMELINE;
            static const ParserDescriptor& SEGMENT_URL;
            static const ParserDescriptor& SUB_REPRESENTATION;
            static const ParserDescriptor& SUBSET;
            static const ParserDescriptor& URL_RANGE;

        private:
            inline virtual ElementParserBase* create() const {return NULL;};
            friend class ParserContext;
        };


        class ParserContext {
        public:
            ParserContext(const ModelFactory& modelFactory);
            ~ParserContext();

            ElementParserBase* getParser(const ParserDescriptor& );

            const ModelFactory& factory;

        private:
            std::map<const ParserDescriptor*, ElementParserBase*> _parsers;
        };


        class ElementParserBase {
        public:
            virtual ~ElementParserBase();

            virtual void* pre(ElementParserBase* parent);
            virtual ElementParserBase* post();

            virtual void attachAttribute(const std::string& name, const std::string& value);
            virtual ElementParserBase* attachElement(const std::string& name);
            virtual void attachContent(const std::string& content);

        protected:
            ParserContext* ctx;

            virtual void* initialiseObject()= 0;
            virtual void validateObject()= 0;

        private:
            ElementParserBase* _parent;

            friend class ParserContext;
        };
    }
}

#endif /* MODEL_PARSER_H_ */
