/****************************************************************************
 * XmlAdapter.cpp                                                           *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 24, 2012                                                 *
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


#include "XmlAdapter.h"
#include "xml/VLCDocumentAdapter.h"
#include "mpd/ModelReader.h"

namespace dashp2p {

//vlc_object_t* XmlAdapter::dashp2pPluginObject = NULL;

mpd::MediaPresentationDescription* XmlAdapter::parseMpd(char* buffer, int bufferSize)
{

	//dp2p_assert(dashp2pPluginObject);
    //xml::VLCDocumentAdapterFactory* documentFactory = new xml::VLCDocumentAdapterFactory(dashp2pPluginObject);
	xml::VLCDocumentAdapterFactory* documentFactory = new xml::VLCDocumentAdapterFactory();

    /* the parser will work on a copy of the string and delete it afterwards */
    xml::BasicDocument* document= documentFactory->createDocument(buffer, bufferSize);
    dp2p_assert(document);
    mpd::ModelReader modelReader(mpd::ModelFactory::DEFAULT_FACTORY);
    mpd::MediaPresentationDescription* mpd = modelReader.read(*document);

    /* Clean-up */
    delete document;
    delete documentFactory;

    return mpd;
}

}
