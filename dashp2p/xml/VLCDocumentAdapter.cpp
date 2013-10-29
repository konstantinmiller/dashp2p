/****************************************************************************
 * VLCDocumentAdapter.cpp                                                   *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Feb 15, 2012                                                 *
 * Authors: Marcel Patzlaff,                                                *
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

#include "Dashp2pTypes.h"
#include "xml/VLCDocumentAdapter.h"

#if DP2P_VLC == 1
# include <vlc_messages.h>
#endif

namespace dash {
    namespace xml {

        void VLCDocumentAdapter::parse(BasicDocumentHandler& handler)
        {
        	setupReader();
            readDocument(handler);
        }

        void VLCDocumentAdapter::readAttributes(BasicDocumentHandler& handler)
        {
#if DP2P_VLC == 1
            const char* attributeName;
            const char* attributeValue;
            while((attributeName = xml_ReaderNextAttr(p_xmlReader, &attributeValue)) != NULL)
            {
                std::string name= attributeName;
                std::string value= attributeValue;
                handler.onAttribute(name, value);
            }
#else
            while( xmlTextReaderMoveToNextAttribute(p_xmlReader) == 1)
            {
            	string name((char*)xmlTextReaderConstName(p_xmlReader));
            	string value((char*)xmlTextReaderConstValue(p_xmlReader));
            	handler.onAttribute(name, value);
            }
#endif
        }

        void VLCDocumentAdapter::readDocument(BasicDocumentHandler& handler)
        {
#if DP2P_VLC == 1
            const char* rawStr;
            dp2p_assert(XML_READER_STARTELEM == xml_ReaderNextNode(p_xmlReader, &rawStr));
            string name = rawStr;
#else
            dp2p_assert(1 == xmlTextReaderRead(p_xmlReader));
            dp2p_assert(XML_READER_TYPE_ELEMENT == xmlTextReaderNodeType(p_xmlReader));
            string name((char*)xmlTextReaderConstName(p_xmlReader));
#endif

            handler.onDocumentStart();
            processElement(handler, name);
            handler.onDocumentEnd();
        }

        void VLCDocumentAdapter::processElement(BasicDocumentHandler& handler, const std::string& name)
        {
            handler.onElementStart(name);

#if DP2P_VLC == 1
            bool isEmpty = xml_ReaderIsEmptyElement(p_xmlReader);
#else
            bool isEmpty = xmlTextReaderIsEmptyElement(p_xmlReader);
#endif

            readAttributes(handler);

            while(!isEmpty)
            {
#if DP2P_VLC == 1
            	const char *rawStr;
                int elementType= xml_ReaderNextNode(p_xmlReader, &rawStr);
                if(elementType != -1 && elementType != XML_READER_NONE && elementType != XML_READER_ENDELEM)
                {
                	if(elementType == XML_READER_STARTELEM) {
                		std::string nextName= rawStr;
                		processElement(handler, nextName);
                	} else if(elementType == XML_READER_TEXT) {
                		std::string value= rawStr;
                		handler.onElementValue(value);
                	}
                } else {
                	break;
                }
#else
                int ret = xmlTextReaderRead(p_xmlReader);
                dp2p_assert(ret != -1);
                if(ret == 0)
                	break;

                int nodeType = xmlTextReaderNodeType(p_xmlReader);
                dp2p_assert(nodeType > -1);

                if(nodeType == XML_READER_TYPE_ELEMENT) {
                	processElement(handler, string((char*)xmlTextReaderConstName(p_xmlReader)));
                } else if(nodeType == XML_READER_TYPE_CDATA || nodeType == XML_READER_TYPE_TEXT) {
                	handler.onElementValue(string((char*)xmlTextReaderConstValue(p_xmlReader)));
                } else if(nodeType == XML_READER_TYPE_END_ELEMENT) {
                	break;
                } else {
                	continue;
                }
#endif
            }

            handler.onElementEnd(name);
        }

        void VLCDocumentAdapter::setupReader()
        {

#if DP2P_VLC == 1
        	p_xml= xml_Create(p_stream);
        	dp2p_assert(p_xml);

            p_xmlReader = xml_ReaderCreate(p_xml, p_stream);
#else
            p_xmlReader = xmlReaderForMemory(buffer, size, NULL, NULL, 0);
#endif

            dp2p_assert(p_xmlReader);
        }
    }
}
