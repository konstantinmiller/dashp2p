/****************************************************************************
 * ModelReader.cpp                                                          *
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "mpd/ModelReader.h"

#include <iostream>

namespace dashp2p {
    namespace mpd {
        ModelReader::ModelReader(const ModelFactory& factory):
            _handler(*(new ModelHandler(factory)))
        {}

        ModelReader::~ModelReader() {
            delete &_handler;
        }

        MediaPresentationDescription *ModelReader::read(dashp2p::xml::BasicDocument& xmlDocument) {
            xmlDocument.parse(_handler);
            MediaPresentationDescription* result= _handler.mpd;
            _handler.mpd= NULL;
            return result;
        }


        ModelReader::ModelHandler::ModelHandler(const ModelFactory& factory):
            mpd(NULL),
            _toSkip(),
            _context(*(new ParserContext(factory))),
            _parser(NULL)
        {}

        ModelReader::ModelHandler::~ModelHandler() {
            delete &_context;
        }

        void ModelReader::ModelHandler::onElementValue(const std::string & value) {
            if(_toSkip.size() <= 0) {
                if(_parser != NULL) {
                    _parser->attachContent(value);
                }
            }
        }

        void ModelReader::ModelHandler::onDocumentEnd() {
            DBGMSG("document end");
        }

        void ModelReader::ModelHandler::onElementStart(const std::string& name) {
            if(_parser == NULL) {
                if(name == "MPD") {
                    _parser= _context.getParser(ParserDescriptor::MEDIA_PRESENTATION_DESCRIPTION);

                    if(_parser == NULL) {
                    	ERRMSG("root parser not found");
                        return;
                    }

                    mpd= static_cast<MediaPresentationDescription*>(_parser->pre(NULL));
                } else {
                    ERRMSG("invalid root element");
                }
            } else {
                if(_toSkip.size() > 0) {
//                    _dbg->printf("%s(%d): skip %s", __FILE__, __LINE__, name.c_str());
                    _toSkip.push_back(name);
                } else {
                    ElementParserBase* nextParser= _parser->attachElement(name);

                    if(nextParser == NULL) {
                    	DBGMSG("skip %s", name.c_str());
                        _toSkip.push_back(name);
                    } else {
                        _parser= nextParser;
                    }
                }
            }
        }

        void ModelReader::ModelHandler::onElementEnd(const std::string& name) {
            if(_toSkip.size() > 0) {
                const std::string& expected= _toSkip.back();

                if(expected == name) {
                    _toSkip.pop_back();

                    if(_toSkip.size() <= 0) {
                    	DBGMSG("skipped %s", name.c_str());
                    }
                } else {
                    ERRMSG("expected %s but got %s", expected.c_str(), name.c_str());
                }
            } else {
                if(_parser != NULL) {
                    _parser= _parser->post();
                } else {
                    ERRMSG("invalid end tag %s", name.c_str());
                }
            }
        }

        void ModelReader::ModelHandler::onAttribute(const std::string & name, const std::string & value) {
            if(_toSkip.size() <= 0 && _parser != NULL) {
                _parser->attachAttribute(name, value);
            }
        }

        void ModelReader::ModelHandler::onDocumentStart() {
            DBGMSG("documentStart");
        }
    }
}
