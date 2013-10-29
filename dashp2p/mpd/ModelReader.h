/****************************************************************************
 * ModelReader.h                                                            *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Feb 15, 2012                                                 *
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

#ifndef MODELREADER_H_
#define MODELREADER_H_

#include "mpd/model.h"
#include "mpd/model_parser.h"
#include "xml/basic_xml.h"

#include <vector>

namespace dashp2p {
    namespace mpd {
        class ModelReader {
        public:
            ModelReader(const ModelFactory& factory);
            ~ModelReader();

            MediaPresentationDescription* read(dashp2p::xml::BasicDocument& xmlDocument);

        private:
            class ModelHandler: public dashp2p::xml::BasicDocumentHandler {
            public:
                ModelHandler(const ModelFactory& factory);
                ~ModelHandler();

                void onDocumentStart();
                void onAttribute(const std::string& name, const std::string& value);
                void onElementStart(const std::string& name);
                void onElementValue(const std::string& value);
                void onElementEnd(const std::string& name);
                void onDocumentEnd();

            protected:
                MediaPresentationDescription* mpd;

            private:
                std::vector<std::string> _toSkip;
                ParserContext& _context;
                ElementParserBase* _parser;

                friend class ModelReader;
            };

            ModelHandler& _handler;
        };
    }
}

#endif /* MODELREADER_H_ */
