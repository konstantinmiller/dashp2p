/****************************************************************************
 * basic_xml.h                                                              *
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

#ifndef BASIC_XML_H_
#define BASIC_XML_H_

#include <string>

namespace dashp2p {
    namespace xml {
        class BasicDocumentHandler {
        public:
            virtual ~BasicDocumentHandler() {}
            virtual void onDocumentStart()= 0;
            virtual void onAttribute(const std::string& name, const std::string& value)= 0;
            virtual void onElementStart(const std::string& name)= 0;
            virtual void onElementValue(const std::string& value)= 0;
            virtual void onElementEnd(const std::string& name)= 0;
            virtual void onDocumentEnd()= 0;
        };

        class BasicDocument {
        public:
            virtual ~BasicDocument(){}
            virtual void parse(BasicDocumentHandler& handler)= 0;
        };

        class BasicDocumentFactory {
        public:
            virtual ~BasicDocumentFactory(){}
            virtual BasicDocument *createDocument(char *xml, int size)= 0;
        };
    }
}


#endif /* BASIC_XML_H_ */
