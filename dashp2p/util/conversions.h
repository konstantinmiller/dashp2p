/****************************************************************************
 * conversions.h                                                            *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Mar 17, 2012                                                 *
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

#ifndef CONVERSIONS_H_
#define CONVERSIONS_H_

#include <string>
#include <locale>

namespace dash {
    namespace util {
        bool stringToBool(const std::string& value) throw();
        unsigned int stringToUnsignedInt(const std::string& value) throw();
        double stringToDouble(const std::string& value, const std::locale& locale=std::locale("C")) throw();

        /**
         * Only duration parts days, hours, minutes, seconds and mseconds are recognised.
         */
        long durationStringToMillis(const std::string& value, const std::locale& locale=std::locale("C")) throw();

        /**
         * Only durations with parts days, hours, minutes, seconds and mseconds are created.
         */
        std::string millisToDurationString(const long& millis) throw();
    }
}

#endif /* CONVERSIONS_H_ */
