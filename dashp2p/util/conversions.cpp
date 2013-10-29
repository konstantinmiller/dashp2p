/****************************************************************************
 * conversions.cpp                                                          *
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

#include "util/conversions.h"

#include <stdexcept>
#include <sstream>

namespace dashp2p {
    namespace util {
        double stringToDouble(const std::string& value, const std::locale& locale) throw() {
            std::istringstream stream;
            stream.imbue(locale);
            stream.str(value);
            double number= 0.0;
            stream >> number;

            if(stream.fail()) {
                throw std::invalid_argument("not a double");
            }

            return number;
        }

        long durationStringToMillis(const std::string& value, const std::locale& locale) throw() {
            if(value.length() <= 3) {
                throw std::invalid_argument("invalid duration");
            }

            bool isNegative= false;
            int offset= 0;
            if(value.at(0) == '-') {
                isNegative= true;
                offset= 1;
            }

            if(value.at(offset) != 'P') {
                throw std::invalid_argument("invalid duration");
            }

            std::istringstream stream;
            stream.imbue(locale);
            stream.str(value);
            stream.ignore(offset + 1);

            bool inTimeSection= false;
            long duration= 0;
            char ch;
            do {
                double number= 0.0;
                double factor= 0.0;
                ch= '\0';
                stream >> number;
                stream.clear();
                stream >> ch;

                switch(ch) {
                    case 'Y': case 'W': {
                        // ignore years and weeks
                        break;
                    }

                    case 'M': {
                        if(inTimeSection) {
                            factor= 60 * 1000;
                        }

                        // else ignore months
                        break;
                    }

                    case 'D': {
                        factor= 24 * 60 * 60 * 1000;
                        break;
                    }

                    case 'T': {
                        inTimeSection= true;
                        break;
                    }

                    case 'H': {
                        factor= 60 * 60 * 1000;
                        break;
                    }

                    case 'S': {
                        factor= 1000;
                        break;
                    }

                    default: {
                        break;
                    }
                }

                duration+= (long) (factor * number);
            } while(ch);

            if(isNegative) {
                duration= -duration;
            }

            return duration;
        }

        bool stringToBool(const std::string&) throw() {
            return false;
        }

        unsigned int stringToUnsignedInt(const std::string&) throw() {
            return 0;
        }


        static void interalBuildDurationPart(std::stringstream& buffer, long& remainingMillis, const long& factor, const char& symbol, const bool& force= false) {
            int value= remainingMillis / factor;

            if(value > 0 || force) {
                buffer << value << symbol;
                remainingMillis%= factor;
            }
        }

        std::string millisToDurationString(const long& millis) throw() {
            std::stringstream buffer;
            long restMillis= millis;
            if(restMillis < 0) {
                buffer << '-';
                restMillis= -restMillis;
            }

            buffer << 'P';
            interalBuildDurationPart(buffer, restMillis, (24 * 60 * 60 * 1000), 'D');

            buffer << 'T';
            interalBuildDurationPart(buffer, restMillis, (60 * 60 * 1000), 'H');
            interalBuildDurationPart(buffer, restMillis, (60 * 1000), 'M');
            interalBuildDurationPart(buffer, restMillis, 1000, '.', true);
            interalBuildDurationPart(buffer, restMillis, 1, 'S', true);

            return buffer.str();
        }
    }
}
