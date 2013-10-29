/****************************************************************************
 * ThroughputModel.hpp                                                      *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Jun 6, 2012                                                  *
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

#ifndef _THROUGHPUTMODEL_HPP_
#define _THROUGHPUTMODEL_HPP_

#include "RingBuffer.hpp"
#include <string>
using std::string;
#include <vector>
using std::vector;









/***************************************************************
 ***** Forward declarations and some general purpose types *****
 ***************************************************************/
class GETStatistics;
class ThroughputSample;
enum Result{not_ok=false, ok=true};
typedef unsigned long long Usec;
typedef unsigned long long Bytes;
typedef unsigned long long Throughput;












/*********************************
 ***** class ThroughputModel *****
 *********************************/
class ThroughputModel
{
public:
  ThroughputModel(unsigned long long dt = 100000);
  ~ThroughputModel();

/***********************************************
 ***** ThroughputModel:: global parameters *****
 ***********************************************/
public:
  /* can be set only once */
  void set_dt(unsigned long long dt) {dp2p_assert(this->dt == 0); this->dt = dt;}
  unsigned long long get_dt() const {return dt;}
private:
  Usec dt; // [usec]
  const double minActivity;

/*************************************
 ***** ThroughputModel:: new API *****
 *************************************/
private:
  double pr(const unsigned fromSample, const unsigned toSample, const unsigned long long bytes) const;


/*****************************************************
 ***** ThroughputModel:: throughput calculations *****
 *****************************************************/
public:
  /* [bit/s] */
  Result getThroughputLastGET(unsigned long long& throughput) const;
  Result getThroughput(unsigned long long startTime, unsigned long long& throughput);
  Result getGETThroughput(Usec now, Usec lookback, unsigned long long& throughput);
  Result calculateUnderrunProbability(unsigned long long bytes, unsigned long long usec,
      unsigned long long usecQueued, unsigned long long now, double& p, unsigned long long& lastFeasibleTime);
private:
  Result updateThroughputProcess(unsigned long long endTime = 0);
  Result calculateThroughputHistogram(unsigned fromSample, unsigned toSample, unsigned combineSamples,
      double minActivity, double& minThroughput, double& maxThroughput, unsigned numBins,
      vector<double>& phi, unsigned& validCombinedSamples);

/******************************************************
 ***** ThroughputModel:: storage of measurements. *****
 ******************************************************/
public:
  /* x must have been allocated by new. we take exclusive ownership. */
  void add(GETStatistics& x);
  const GETStatistics& getLastGETStatistics(unsigned i = 0) const;
  const ThroughputSample& getLastThroughputSample(unsigned i = 0) const;
  unsigned getNumberOfGETMeasurements() const;
private:
  RingBuffer<GETStatistics>    getStatisticsHistory;
  vector<ThroughputSample>     throughputProcess;

/****************************************************
 ***** ThroughputModel:: Output of measurements *****
 ****************************************************/
public:
  void dumpThroughputProcess(FILE* file);
  void dumpGETStatisticsHistory(FILE* file) const;
};





















/*************************
 ***** Further types *****
 *************************/

/* type for storage of measurements related to reception of a chunk of a GET request */
class ChunkStatistics {
public:
  unsigned long long received;
  unsigned long long size;
  bool ifFull;
};

/* type for storage of measurements related to a GET request. */
class GETStatistics {
public:
  const unsigned long long          started;
  const unsigned long long          header_received;
  const RingBuffer<ChunkStatistics> chunks;
  const unsigned long long          content_size;
  const unsigned long long          encodingRate; // [bit/s]
  const Usec                        duration;
  const long long                   usecInPipeAtStart;
  GETStatistics(unsigned long long started,
      Usec header_received,
      const RingBuffer<ChunkStatistics>& chunks,
      unsigned long long content_size,
      unsigned long long encodingRate,
      Usec duration,
      Usec usecInPipeAtStart);
};

/* type for itnernal storage of a throughput sample */
class ThroughputSample {
public:
  unsigned long long start;
  unsigned long long stop;
  unsigned long long num_bytes;
  unsigned long long active;
};

#endif
