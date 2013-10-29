/****************************************************************************
 * AdaptationST.h                                                           *
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

#ifndef ADAPTATIONST_H_
#define ADAPTATIONST_H_


#include "mpd/model.h"
#include "DebugAdapter.h"
#include <vector>
#include <stdarg.h>


/**
 * Performs the adaptation as in the ST paper.
 *
 * Configuration parameters:
 *   o Delta_t - time window for averaging throughput
 *   o alfa_1, ..., alfa_5
 *   o B_min, B_low, B_high
 *   o minimum number of completed requests before which we enforce lowest representation
 *
 * Input information (static):
 *   o r                   - Bit-rates of available representations in [bit/sec], sorted in ascending order
 *   o tau                 - Segment duration (assumed to be constant) in [sec]
 *
 * Input information (dynamic):
 *   o beta_min_decreasing - First point in time when beta_min was decreasing in [sec]
 *   o beta                - Current buffer level in [sec]
 *   o rho                 - Average throughput over [t-Delta_t, t] in [bit/sec]
 *   o rhoLast             - Throughput of the last GET request in [bit/sec]
 *   o number of completed requests
 *   o r_last              - Last selected representation
 *
 * Output information:
 *   o B_delay             - Buffer level until which to delay next GET request in [sec]
 *   o r_next              - Next represenation
 */
class AdaptationST
{
private:
    AdaptationST(){}
    virtual ~AdaptationST(){}

/* Public types */
public:

    /* Data tupe to hold the decision. */
    class Decision {
    public:
        Decision(const Representation& rep, double Bdelay, int reason) : rep(rep), Bdelay(Bdelay), reason(reason) {}
        Representation rep;
        double         Bdelay;
        int            reason; // for debugging/logging purposes
    };

/* Public methods */
public:

    /* Initialization. */
    static void init(std::vector<Representation> representations, std::string adaptationConfiguration);
        //double Bmin, double Blow, double Bhigh, double alfa1, double alfa2, double alfa3, double alfa4, double alfa5, double Delta_t);

    /* Selects the representation for the next segment and the buffer level when the download should be started (Inf, if immediately). */
    static Decision selectRepresentation(bool ifBetaMinIncreasing, double beta,
            double rho, double rhoLast, unsigned completedRequests, unsigned minRequiredCompletedRequests,
            const Representation& r_last);

    static double get_Delta_t() { return Delta_t;}

#if 0
    /* Always selects the lowest representation. */
    static std::pair<Representation, double> selectRepresentationAlwaysLowest();

    /* Always selects the given representation. */
    static std::pair<Representation, double> selectRepresentation(const std::string& repID);
#endif

/* Private fields */
private:

    /* Stores the vector of available representations. Assumes it to be sorted in ascending order. */
    static std::vector<Representation> representations;

    /* Parameters */
    static double Bmin;
    static double Blow;
    static double Bhigh;
    static double alfa1;
    static double alfa2;
    static double alfa3;
    static double alfa4;
    static double alfa5;
    static double Delta_t;

    /* Indicates if we are in the initial increase phase. */
    static bool initialIncrease;

    /* Time when the initial increase phase was terminated. */
    static double initialIncreaseTerminationTime;

/* Private methods */
private:
    /* Returns the index of a given representation. */
    static unsigned getIndex(const Representation& r);
};

#endif /* ADAPTATIONST_H_ */
