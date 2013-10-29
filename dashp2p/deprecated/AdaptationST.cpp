/****************************************************************************
 * AdaptationST.cpp                                                         *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Jun 10, 2012                                                 *
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


#include "AdaptationST.h"
#include "Utilities.h"
#include "Statistics.h"
#include <limits>
#include <stdio.h>


#define COLORED_OUTPUT
#ifdef COLORED_OUTPUT
#define rst          "\e[0m"
#define lightred     "\033[01;31m"
#define lightgreen   "\033[01;32m"
#define lightblue    "\033[01;34m"
#define lightmagenta "\033[01;35m"
#define lightcyan    "\033[01;36m"
#else
#define rst
#define lightred
#define lightgreen
#define lightblue
#define lightmagenta
#define lightcyan
#endif


std::vector<Representation> AdaptationST::representations;
double AdaptationST::Bmin = 10;
double AdaptationST::Blow = 20;
double AdaptationST::Bhigh = 50;
double AdaptationST::alfa1 = 0.75;
double AdaptationST::alfa2 = 0.33;
double AdaptationST::alfa3 = 0.5;
double AdaptationST::alfa4 = 0.75;
double AdaptationST::alfa5 = 0.9;
double AdaptationST::Delta_t = 10;
bool AdaptationST::initialIncrease = true;
double AdaptationST::initialIncreaseTerminationTime = std::numeric_limits<double>::infinity();


void AdaptationST::init(std::vector<Representation> representations, std::string adaptationConfiguration)
//double Bmin, double Blow, double Bhigh, double alfa1, double alfa2, double alfa3, double alfa4, double alfa5, double Delta_t)
{
    AdaptationST::representations = representations;
    dp2p_assert(9 == sscanf(adaptationConfiguration.c_str(), "%lf:%lf:%lf:%lf:%lf:%lf:%lf:%lf:%lf", &Bmin, &Blow, &Bhigh, &alfa1, &alfa2, &alfa3, &alfa4, &alfa5, &Delta_t));

    Statistics::recordScalarDouble("Bmin", Bmin);
    Statistics::recordScalarDouble("Blow", Blow);
    Statistics::recordScalarDouble("Bhigh", Bhigh);
    Statistics::recordScalarDouble("alfa1", alfa1);
    Statistics::recordScalarDouble("alfa2", alfa2);
    Statistics::recordScalarDouble("alfa3", alfa3);
    Statistics::recordScalarDouble("alfa4", alfa4);
    Statistics::recordScalarDouble("alfa5", alfa5);
    Statistics::recordScalarDouble("Delta_t", Delta_t);

    //AdaptationST::Bmin = Bmin;
    //AdaptationST::Blow = Blow;
    //AdaptationST::Bhigh = Bhigh;
    //AdaptationST::alfa1 = alfa1;
    //AdaptationST::alfa2 = alfa2;
    //AdaptationST::alfa3 = alfa3;
    //AdaptationST::alfa4 = alfa4;
    //AdaptationST::alfa5 = alfa5;
    //AdaptationST::Delta_t = Delta_t;
}

enum DecisionReason {
    NOT_ENOUGH_COMPLETED_REQUESTS = 10,
    II1_UP = 20,
    II1_KEEP = 30,
    II2_UP = 40,
    II2_KEEP = 50,
    II3_UP = 60,
    II3_KEEP = 70,
    NO0_LOWEST = 80,
    NO1_LOWEST = 90,
    NO1_KEEP = 100,
    NO1_DOWN = 110,
    NO2_DELAY = 120,
    NO2_NODELAY = 130,
    NO3_DELAY = 140,
    NO3_UP = 150
};

AdaptationST::Decision AdaptationST::selectRepresentation(bool ifBetaMinIncreasing, double beta,
        double rho, double rhoLast, unsigned completedRequests, unsigned minRequiredCompletedRequests,
        const Representation& r_last)
{
    /* debug output */
    DBGMSG("\nselectRepresentation(%s, beta: %.3f sec, rho: %.3f Mbit/sec, rhoLast: %.3f Mbit/sec, numComplReq: %u, minNumComplReq: %u, r_last: %s)",
            ifBetaMinIncreasing ? "beta_min incr." : "beta_min decr.",
            beta, rho / 1e6, rhoLast / 1e6, completedRequests, minRequiredCompletedRequests, r_last.ID.c_str());

    /* current time since beginning of download */
    const dash::Usec now = dash::Utilities::getTime();

    /* verify that we already obtained a list of representations */
    dp2p_assert(representations.size());

    /* If we completed strictly less than a minimum number of requests, stay with the lowest representation */
    if (completedRequests == 0 || completedRequests < minRequiredCompletedRequests)
    {
        DBGMSG("Number of completed GET requests is %u < %u. Selecting lowest representation.",
                completedRequests, minRequiredCompletedRequests);
        return Decision(representations.at(0), std::numeric_limits<double>::infinity(), NOT_ENOUGH_COMPLETED_REQUESTS);
    }

    /* Get indexes of some relevant representations for faster access. */
    const unsigned iLowest = 0;
    const unsigned iLast = getIndex(r_last);
    const unsigned iHighest = representations.size() - 1;

    DBGMSG("Number of completed GET requests is %u >= %u.", completedRequests, minRequiredCompletedRequests);

    /* Are we in the initial increase phase? */
    if (initialIncrease)
    {
        DBGMSG("We are in the initial increase phase.");

        /* Check if we shall terminate the initial increase phase. */
        {
            /* Check if we already selected highest representation. */
            if (initialIncrease && iLast == iHighest) {
                initialIncrease = false;
                DBGMSG("Terminating initial increase phase. Reached highest representation.");
            }

            /* Check if the discretized minimum buffer level had been decreasing. */
            if (initialIncrease && !ifBetaMinIncreasing) {
                initialIncrease = false;
                DBGMSG("Terminating initial increase phase. Discretized min buffer level had been decreasing.");
            }

            /* Check if current representation is above alfa1 * rho. */
            if (initialIncrease && representations.at(iLast).bandwidth > alfa1 * rho) {
                DBGMSG(lightred"Terminating initial increase phase. Bit-rate is above alfa1*rho (%u>%f*%f."rst,
                        representations.at(iLast).bandwidth, alfa1, rho);
                initialIncrease = false;
            }
        }

        /* if we didn't terminate the initial increase phase,
         * act depending on buffer level and throughput. */
        if (initialIncrease && beta < Bmin)
        {
            DBGMSG("We are in [0,Bmin).");
            if (representations.at(iLast + 1).bandwidth <= alfa2 * rho) {
                DBGMSG("Have enough throughput. Increase bit-rate.");
                return Decision(representations.at(iLast + 1), std::numeric_limits<double>::infinity(), II1_UP);
            } else {
                DBGMSG("DO NOT have enough throughput. DO NOT increase encoding rate.");
                return Decision(representations.at(iLast), std::numeric_limits<double>::infinity(), II1_KEEP);
            }
        }
        else if (initialIncrease && beta < Blow)
        {
            DBGMSG("We are in [Bmin, Blow).");
            if (representations.at(iLast + 1).bandwidth <= alfa3 * rho) {
                DBGMSG("Have enough throughput. Increase encoding rate.");
                return Decision(representations.at(iLast + 1), std::numeric_limits<double>::infinity(), II2_UP);
            } else {
                DBGMSG("DO NOT have enough throughput. DO NOT increase encoding rate.");
                return Decision(representations.at(iLast), std::numeric_limits<double>::infinity(), II2_KEEP);
            }
        }
        else if (initialIncrease)
        {
            DBGMSG("We are in [Blow, Inf).");
            if (representations.at(iLast + 1).bandwidth <= alfa4 * rho) {
                DBGMSG("Have enough throughput. Increase encoding rate.");
                if(Bhigh - representations.at(iLast + 1).segmentDuration < beta) {
                    DBGMSG("Delay until beta == %f.", Bhigh - representations.at(iLast + 1).segmentDuration);
                }
                return Decision(representations.at(iLast + 1), Bhigh - representations.at(iLast + 1).segmentDuration, II3_UP);
            } else {
                DBGMSG("DO NOT have enough throughput. DO NOT increase encoding rate.");
                if(Bhigh - representations.at(iLast).segmentDuration < beta) {
                    DBGMSG("Delay until beta == %f.", Bhigh - representations.at(iLast + 1).segmentDuration);
                }
                return Decision(representations.at(iLast), Bhigh - representations.at(iLast).segmentDuration, II3_KEEP);
            }
        }
        else
        {
            /* we just left the initial increase phase */
            initialIncreaseTerminationTime = now;
        }
    }

    /* if we are not in the initial increase phase,
     * act depending on buffer level and throughput. */

    /* we are in [0, Bmin) */
    if (beta < Bmin)
    {
        DBGMSG("We are in [0, Bmin): %.3f sec. Selecting lowest representation.", beta);
        return Decision(representations.at(iLowest), std::numeric_limits<double>::infinity(), NO0_LOWEST);
    }
    /* We are in [Bmin, Blow). */
    else if (beta < Blow)
    {
        DBGMSG("We are in [Bmin, Blow): %.3f in [%.3f, %.3f].", beta, Bmin, Blow);

        if (iLast == iLowest) {
            DBGMSG("We are at lowest representation. Keep it.");
            return Decision(representations.at(iLast), std::numeric_limits<double>::infinity(), NO1_LOWEST);
        } else {
            DBGMSG("We are NOT at lowest representation. Checking if shall switch down.");
            if (rhoLast > (double)representations.at(iLast).bandwidth) {
                DBGMSG("Buffer is rising %f > %u. Not switching down.", rhoLast, representations.at(iLast).bandwidth);
                return Decision(representations.at(iLast), std::numeric_limits<double>::infinity(), NO1_KEEP);
            } else {
                DBGMSG("Buffer is falling %f <= %u. Switching down.", rhoLast, representations.at(iLast).bandwidth);
                return Decision(representations.at(iLast - 1), std::numeric_limits<double>::infinity(), NO1_DOWN);
            }
        }
    }
    /* we are in [Blow, Bhigh) */
    else if  (beta < Bhigh)
    {
        DBGMSG("We are in the target interval [%.3f, %.3f] sec.", Blow, Bhigh);
        DBGMSG("We are NOT in the initial increase phase. Keep current representation.");

        /* check if throughput is enough for next higher encoding rate. */
        if(iLast == iHighest || representations.at(iLast + 1).bandwidth >= alfa5 * rho) {
            /* delay */
            DBGMSG("Delay until beta == %f.", std::max<double>(beta - (double)representations.at(iLast).segmentDuration, 0.5 * (Blow + Bhigh)));
            return Decision(representations.at(iLast),
                    std::max<double>(beta - (double)representations.at(iLast).segmentDuration, 0.5 * (Blow + Bhigh)), NO2_DELAY);
        } else {
            /* don't delay */
            DBGMSG("Don't delay.");
            return Decision(representations.at(iLast), std::numeric_limits<double>::infinity(), NO2_NODELAY);
        }
    }
    /* we are in [Bhigh, Inf) */
    else
    {
        DBGMSG("We are above the target interval: %.3f sec > %.3f sec.", beta, Bhigh);
        DBGMSG("We are NOT in the initial increase phase.");

        /* check if throughput is enough for next higher encoding rate. */
        if(iLast == iHighest || representations.at(iLast + 1).bandwidth >= alfa5 * rho) {
            /* delay */
            DBGMSG("Keep current representation. Delay until beta == %f.", std::max<double>(beta - (double)representations.at(iLast).segmentDuration, 0.5 * (Blow + Bhigh)));
            return Decision(representations.at(iLast),
                    std::max<double>(beta - (double)representations.at(iLast).segmentDuration, 0.5 * (Blow + Bhigh)), NO3_DELAY);
        } else {
            /* don't delay */
            DBGMSG("Don't delay. Switch to next higher representation.");
            return Decision(representations.at(iLast + 1), std::numeric_limits<double>::infinity(), NO3_UP);
        }
    }
}

#if 0
std::pair<Representation, double> AdaptationST::selectRepresentationAlwaysLowest()
{
    return std::pair<Representation, double>(representations.at(0), 0.5 * (Blow + Bhigh));
}

std::pair<Representation, double> AdaptationST::selectRepresentation(const std::string& repID)
{
    for(unsigned i = 0; i < representations.size(); ++i) {
        if(representations.at(i).ID == repID) {
            return std::pair<Representation, double>(representations.at(i), 0.5 * (Blow + Bhigh));
        }
    }
    dp2p_assert(false);
    return std::pair<Representation, double>(representations.at(0), 0.5 * (Blow + Bhigh));
}
#endif

unsigned AdaptationST::getIndex(const Representation& r)
{
    for(unsigned i = 0; i < representations.size(); ++i) {
        if(representations.at(i).ID == r.ID) {
            return i;
        }
    }
    dp2p_assert(false);
    return 0;
}
