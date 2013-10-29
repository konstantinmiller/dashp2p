/****************************************************************************
 * VLCDocumentAdapter.h                                                     *
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
#ifndef VLCDOCUMENTADAPTER_H_
#define VLCDOCUMENTADAPTER_H_

#include "xml/basic_xml.h"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if 0
# include <vlc_common.h>
# include <vlc_stream.h>
# include <vlc_xml.h>
#endif
#include <libxml/xmlreader.h>

namespace dashp2p {

    namespace xml {

        class VLCDocumentAdapter: public dashp2p::xml::BasicDocument {
        public:
#if 0
        	VLCDocumentAdapter(stream_t* p_stream): p_stream(p_stream), p_xml(NULL), p_xmlReader(NULL) {}
        	virtual ~VLCDocumentAdapter() {xml_ReaderDelete(p_xmlReader); xml_Delete(p_xml); stream_Delete(p_stream);}
#endif
        	VLCDocumentAdapter(char* buffer, int size): buffer(buffer), size(size), p_xmlReader(NULL) {}
        	virtual ~VLCDocumentAdapter() {xmlFreeTextReader(p_xmlReader); delete[] buffer;}

            virtual void parse(BasicDocumentHandler& handler);

        private:
            void setupReader();
            void readDocument(BasicDocumentHandler& handler);
            void processElement(BasicDocumentHandler& handler, const std::string& name);
            void readAttributes(BasicDocumentHandler& handler);

#if 0
            stream_t* p_stream;
            xml_t*  p_xml;
            xml_reader_t* p_xmlReader;
#endif
            char* buffer;
            int size;
            xmlTextReader* p_xmlReader;
        };




        class VLCDocumentAdapterFactory: public dashp2p::xml::BasicDocumentFactory
        {

        public:
#if 0
            VLCDocumentAdapterFactory(vlc_object_t* p_vlc): p_vlc_object(p_vlc) {}
            virtual ~VLCDocumentAdapterFactory() {p_vlc_object= NULL;}
#endif
            VLCDocumentAdapterFactory() {}
            virtual ~VLCDocumentAdapterFactory() {}

            /**
             * Takes the ownership of the memory buffer.
             * @param xml Will be deleted when the document is deleted.
             */
            virtual BasicDocument* createDocument(char *xml, int size) {
#if 0
            	// FIXME: is there an encoding problem?
            	return new VLCDocumentAdapter(stream_MemoryNew(p_vlc_object, (uint8_t*)xml, size, false));
#endif
            	return new VLCDocumentAdapter(xml, size);
            }
        private:
#if 0
            vlc_object_t* p_vlc_object;
#endif
        };

    }

}

#endif /* VLCDOCUMENTADAPTER_H_ */
