/****************************************************************************
 * ThroughputModel.cpp                                                      *
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

#include "ThroughputModel.hpp"
#include <stdlib.h>
#include <math.h>
#include <limits>
using std::numeric_limits;

/*******************************
 ***** Auxiliary functions *****
 *******************************/
template <class X>
static X sumVector(const vector<X>& x, unsigned from = 0, unsigned to = 0)
{
  dp2p_assert(from <= to);
  dp2p_assert(to < x.size());
  if (to == 0)
    to = x.size() - 1;
  X sum = 0;
  for (unsigned i = from; i <= to; ++i)
    sum += x.at(i);
  return sum;
}

ThroughputModel::ThroughputModel(unsigned long long dt)
: minActivity(0.001),
  getStatisticsHistory(128, false, true)
{
  this->dt = dt;
}

ThroughputModel::~ThroughputModel()
{
  // blank
}

double ThroughputModel::pr(const unsigned fromSample, const unsigned toSample,
    const unsigned long long bytes) const
{
  /* calculate probability that we can download AT LEAST
   * bytes [byte] during samples [fromSample, toSample] */
  return 0;
}

Result ThroughputModel::getThroughputLastGET(unsigned long long& throughput) const
{
  dp2p_assert(!getStatisticsHistory.empty());
  const GETStatistics& gs = getStatisticsHistory.getLast();
  unsigned long long completed = gs.chunks.getLast().received;
  throughput = (8000000 * gs.content_size) / (completed - gs.started); // [bit/s]
  return ok;
}

Result ThroughputModel::getThroughput(unsigned long long startTime, unsigned long long& throughput)
{
  /* initialize the throughput variable */
  throughput = 0;

  /* check if we already have something */
  if (!throughputProcess.size())
    return not_ok;

  /* assert that we can lookback that far */
  Usec beginOfHistory = throughputProcess.at(0).start;
  if (beginOfHistory > startTime)
    return not_ok;

  /* what is the first sample to consider */
  unsigned i_first = (startTime - beginOfHistory) / dt;

  /* iterate over the samples from the lookback period */
  unsigned cnt = 0;
  Usec minAct = numeric_limits<Usec>::max();
  Throughput minThrpt = numeric_limits<Throughput>::max();
  Throughput maxThrpt = numeric_limits<Throughput>::min();
  for (unsigned i = i_first; i < throughputProcess.size(); ++i)
  {
    const ThroughputSample& sample = throughputProcess.at(i);
    if (sample.active > (Usec)(minActivity * dt)) {
      const Throughput thrpt = ((Usec)8000000 * sample.num_bytes) / sample.active;
      throughput += thrpt;
      ++cnt;
      if (sample.active < minAct)
        minAct = sample.active;
      if (thrpt < minThrpt)
        minThrpt = thrpt;
      if (thrpt > maxThrpt)
        maxThrpt = thrpt;
    }
  }

  //printf("considered %u samples for throughput calculation.\n", cnt);
  //printf("minimum encountered activity: %llu, min throughput: %llu, max throughput: %llu\n",
  //    minAct, minThrpt, maxThrpt);

  /* return not_ok if we did not encounter a period with high enough activity */
  if (cnt) {
    throughput = throughput / cnt;
    return ok;
  } else {
    return not_ok;
  }
}

Result ThroughputModel::getGETThroughput(Usec now, Usec lookback, unsigned long long& throughput)
{
  /* checks */
  dp2p_assert(!getStatisticsHistory.empty());
  dp2p_assert(now >= lookback);
  dp2p_assert(now - lookback >= getStatisticsHistory.getFirst().started);

  /* determine the first GET request to consider */
  unsigned i_first = getStatisticsHistory.size();
  for (unsigned i = 0; i < getStatisticsHistory.size(); ++i) {
    if (getStatisticsHistory.getLast(i).started <= now - lookback) {
      if (getStatisticsHistory.getLast(i).chunks.getLast().received > now - lookback) {
        i_first = getStatisticsHistory.size() - i - 1;
        break;
      } else if (i == 0) {
        throughput = 0;
        return not_ok;
      } else {
        i_first = getStatisticsHistory.size() - i;
        break;
      }
    }
  }

  /* calculate the throughput */
  const Usec getDuration = getStatisticsHistory[i_first].chunks.getLast().received - getStatisticsHistory[i_first].started;
  //printf("ende=%.3f, now=%.3f, lookback=%.3f\n", getStatisticsHistory[i_first].chunks.getLast().received / 1e6, now / 1e6, lookback / 1e6);
  const Usec getDurationRelevant = getStatisticsHistory[i_first].chunks.getLast().received - (now - lookback);
  const Bytes getBytes = getStatisticsHistory[i_first].content_size;
  Usec totalActivity = getDurationRelevant;
  Bytes totalBytes = getBytes * ((double)getDurationRelevant / getDuration);
  for (unsigned i = i_first + 1; i < getStatisticsHistory.size(); ++i) {
    totalActivity += getStatisticsHistory[i].chunks.getLast().received - getStatisticsHistory[i].started;
    totalBytes += getStatisticsHistory[i].content_size;
  }
  throughput = (8000000 * totalBytes) / totalActivity;
  //printf("getDuration = %llu, getDurationRelevant = %llu, getBytes = %llu\n", getDuration, getDurationRelevant, getBytes);
  //printf("total bytes = %llu, total activity = %llu\n", totalBytes, totalActivity);

  return ok;
}

Result ThroughputModel::updateThroughputProcess(unsigned long long endTime)
{
  /* if we do not already have a valid dt value -> return */
  dp2p_assert(dt);

  /* if we have no GET statistics -> return */
  if(getStatisticsHistory.empty())
    return ok;

  /* what is our startSample? if something precalculated,
   * recalculate last stored sample. */
  const unsigned startSample = throughputProcess.empty() ? 0 : throughputProcess.size() - 1;
  const unsigned long long startTime = throughputProcess.empty() ? 0 : throughputProcess.back().start;

  /* if endTime == 0, set it to the completion time of last GET request */
  if (endTime == 0)
    endTime = getStatisticsHistory.getLast().chunks.getLast().received;

  /* round up endTime */
  if (endTime % dt != 0)
    endTime = endTime - endTime % dt + dt;

  /* calculate endSample */
  const unsigned endSample = startSample + (endTime - startTime) / dt - 1;

  /* consistency check */
  if (startSample > endSample)
    return not_ok;

  /* resize throughputProcess */
  throughputProcess.resize(endSample + 1);

  /* get the range of GET requests that we need to consider */
  unsigned i_first = getStatisticsHistory.size();
  unsigned i_last = getStatisticsHistory.size();
  for (unsigned i = 0; i < getStatisticsHistory.size(); ++i)
  {
    const GETStatistics& gs = getStatisticsHistory.getLast(i);
    const unsigned long long& beginActivity = gs.chunks.getFirst().received;
    const unsigned long long& endActivity   = gs.chunks.getLast().received;
    if (i_last == getStatisticsHistory.size() && beginActivity < endTime) {
      i_last = getStatisticsHistory.size() - i - 1;
    }
    if (endActivity <= startTime) {
      dp2p_assert(i != 0);
      i_first = getStatisticsHistory.size() - i;
      break;
    }
    if (i == getStatisticsHistory.size() - 1) {
      i_first = 0;
      break;
    }
  }

  //printf("Will update throughputProcess from sample %u until sample %u\n", startSample, endSample);
  //printf("Will consider GETs (%u .. %u)\n", i_first, i_last);


  /* iterate over the samples that are to be calculated */
  unsigned cur_request = i_first;
  for (cur_request = i_first; cur_request <= i_last; ++cur_request) {
    if (getStatisticsHistory[cur_request].chunks.size() > 1)
      break;
  }
  unsigned cur_chunk   = 1;
  for (unsigned i = startSample; i <= endSample; ++i)
  {
    /* just for debugging */
    dp2p_assert(cur_request > i_last || getStatisticsHistory[cur_request].chunks.size() > 1);
    /* start and end of the current sample */
    unsigned long long beginOfSample = startTime + (i - startSample) * dt;
    unsigned long long endOfSample   = beginOfSample + dt;
    //printf("looking at sample %u (%.6f .. %.6f)\n", i, beginOfSample/1e6, endOfSample/1e6);
    /* update sample parameters */
    throughputProcess.at(i).start     = beginOfSample;
    throughputProcess.at(i).stop      = endOfSample;
    throughputProcess.at(i).num_bytes = 0;
    throughputProcess.at(i).active    = 0;
    /* if we are beyond the last GET request that we want consider,
     * just continue (in order to have subsequent samples also initialized). */
    if (cur_request > i_last) {
      //printf("skipping sample. cur_request=%u, i_last=%u\n", cur_request, i_last);
      continue;
    }
    /* go through all chunks that intersect with the current sample */
    while (true)
    {
      /* parameters of the current chunk */
      unsigned long long beginOfChunk = getStatisticsHistory[cur_request].chunks[cur_chunk - 1].received;
      unsigned long long endOfChunk   = getStatisticsHistory[cur_request].chunks[cur_chunk].received;
      unsigned long long sizeOfChunk  = getStatisticsHistory[cur_request].chunks[cur_chunk].size;
      unsigned numCombinedChunks = 1;
      for (unsigned j = 1; true; ++j) {
        if (cur_chunk + j == getStatisticsHistory[cur_request].chunks.size())
          break;
        if (!getStatisticsHistory[cur_request].chunks[cur_chunk + j - 1].ifFull)
          break;
        ++numCombinedChunks;
        endOfChunk = getStatisticsHistory[cur_request].chunks[cur_chunk + j].received;
        sizeOfChunk += getStatisticsHistory[cur_request].chunks[cur_chunk + j].size;
      }

      /* calculate the intersection length of chunk and sample.
       * for num_bytes: round down on the left edge, round up on the right edge. */
      if (endOfChunk <= beginOfSample || beginOfChunk >= endOfSample)
      {
        // blank
      }
      else if (beginOfChunk <= beginOfSample && endOfChunk <= endOfSample)
      {
        /* we intersect only at the left edge */
        double frac = (double)(endOfChunk - beginOfSample) / (double)(endOfChunk - beginOfChunk);
        throughputProcess.at(i).num_bytes += (unsigned long long) floor(sizeOfChunk * frac);
        throughputProcess.at(i).active += endOfChunk - beginOfSample;
      }
      else if (beginOfChunk >= beginOfSample && endOfChunk <= endOfSample)
      {
        /* chunk is completely within the sample */
        throughputProcess.at(i).num_bytes += sizeOfChunk;
        throughputProcess.at(i).active += endOfChunk - beginOfChunk;
      }
      else if (beginOfChunk <= beginOfSample && endOfChunk >= endOfSample)
      {
        /* sample is completely within the chunk */
        double frac_left = (double)(beginOfSample - beginOfChunk) / (double)(endOfChunk - beginOfChunk);
        double frac_right = (double)(endOfChunk - endOfSample) / (double)(endOfChunk - beginOfChunk);
        throughputProcess.at(i).num_bytes += sizeOfChunk -
            (unsigned long long) (ceil(frac_left * sizeOfChunk) + floor(frac_right * sizeOfChunk));
        throughputProcess.at(i).active += dt;
      }
      else if (beginOfChunk >= beginOfSample && endOfChunk >= endOfSample)
      {
        /* we intersect only at the right edge */
        double frac = (double)(endOfSample - beginOfChunk) / (double)(endOfChunk - beginOfChunk);
        throughputProcess.at(i).num_bytes += (unsigned long long) ceil(sizeOfChunk * frac);
        throughputProcess.at(i).active += endOfSample - beginOfChunk;
      }
      else
      {
        // blank
      }

      /* if the end of the chunk is beyond the end of the sample, we are done with the sample */
      if (endOfChunk >= endOfSample)
        break;
      /* if we are not done with the sample, go to the next chunk */
      cur_chunk += numCombinedChunks;
      /* if it is not the last chunk of the request, continue */
      if (cur_chunk < getStatisticsHistory[cur_request].chunks.size())
        continue;
      /* if it was the last chunk of a request, go to next request */
      cur_chunk = 1;
      /* skip requests that have only one chunk */
      while (true) {
        ++cur_request;
        if (cur_request > i_last)
          break;
        else if (getStatisticsHistory[cur_request].chunks.size() > 1)
          break;
      }
      /* if no more requests, break */
      if (cur_request > i_last)
        break;
    }
    //printf("sample has %llu bytes\n", throughputProcess.at(i).num_bytes);
  }

  return ok;
}

// todo: shall we require that the given list of requests reaches at least until the prediction horizon???
Result ThroughputModel::calculateUnderrunProbability(unsigned long long bytesNextGET,
    unsigned long long usecNextGET, unsigned long long usecQueued,
    unsigned long long now, double& p, unsigned long long& lastFeasibleTime)
{
  // todo: calculate RTT properly
  const unsigned long long RTT = 0;

  /* initialize the return variable */
  p = numeric_limits<double>::max();

  /* if we don't have history information -> quit */
  if(throughputProcess.empty())
    return not_ok;

  /* get the beginning of the throughput process */
  unsigned long long t0 = throughputProcess.at(0).start;
  dp2p_assert(t0 % dt == 0); //paranoia

  /* what is the sample "now" belongs to? */
  dp2p_assert(now >= t0);
  const unsigned nowSample = (now - t0) / dt;

  /* what is the last valid sample we have */
  unsigned lastValidSample = throughputProcess.size();
  for (unsigned i = throughputProcess.size() - 1; i >= 0; --i)
  {
    const ThroughputSample& ts = throughputProcess.at(i);
    if (ts.active >= minActivity * (ts.stop - ts.start)) {
      lastValidSample = i;
      break;
    }
  }
  if (lastValidSample == throughputProcess.size())
    return not_ok; // no valid samples
  /*if (lastValidSample < nowSample - 1) {
    printf ("now = %.6f, nowSample = %u, lastValidSample = %u\n", now/1e6, nowSample, lastValidSample);
    fflush(stdout);
    FILE* f = fopen("debug_throughputProcess.txt", "w");
    dp2p_assert(f);
    dumpThroughputProcess(f);
    fclose(f);
    dp2p_assert(0);
  }*/

  /* how far will we look into the past and future? */
  const unsigned long long lookbackStart = t0; // todo: do we want a maxLookbackHorizon here?
  const unsigned lookbackStartSample = (lookbackStart - t0) / dt;
  const unsigned long long maxPredictionHorizon = now - lookbackStart;

  /* what is the last feasible time until which the total number
   * of playback seconds of given requests might avoid underruns.
   * round to sample boundaries. */
  for (unsigned sample = nowSample; true; ++sample)
  {
    const unsigned long long endOfSample = t0 + (sample + 1) * dt;
    const unsigned long long remainingTime = endOfSample - now - RTT;
    if (usecQueued + usecNextGET < remainingTime)
      break;
    lastFeasibleTime = endOfSample;
  }

  /* what is the last sample to be predicted.
   * note that we don't want to predict longer than the lookback period. */
  const unsigned lastSampleToBePredicted = std::min<unsigned>(
      (now + maxPredictionHorizon - t0) / dt,
      std::min<unsigned>(
          nowSample + (lastValidSample - lookbackStart + 1) - 1,
          (lastFeasibleTime - t0) / dt - 1)); // -1 since lastFeasibleTime is on sample boundary!

  printf("Now = %.6f, nowSample = %u.\n", now/1e6, nowSample);
  printf("bytes = %llu, usec = %.6f, usecQueued = %.6f.\n", bytesNextGET, usecNextGET/1e6, usecQueued/1e6);
  printf("lastValidSample = %u.\n", lastValidSample);
  printf("lookbackStart = %.6f, lookbackStartSample = %u\n", lookbackStart/1e6, lookbackStartSample);
  printf("underrun at latest at %.6f\n", lastFeasibleTime/1e6);
  printf("will predict until sample %u\n", lastSampleToBePredicted);

  /* vector to hold conditional underrun probabilities for sample i,
   * under the condition that (i) there was no underrun until sample i-1,
   * the download of the request does not finish before sample i. */
  vector<double> condProb(lastSampleToBePredicted - nowSample + 1, numeric_limits<double>::max());

  /* iterate over samples for which a prediction is requested */
  for (unsigned sample = nowSample; sample <= lastSampleToBePredicted; ++sample)
  {
    /* remaining time from now until the end of sample - RTT */
    const unsigned long long endOfSample = t0 + (sample + 1) * dt;
    const unsigned long long remainingTime = endOfSample - now - RTT;

    /* if we have more playback seconds queued than remains
     * until the end of a sample, underrun probability
     * for this sample is 0, independent of the history. */
    if (usecQueued > remainingTime) {
      printf("sample %u within queued time\n", sample);
      condProb.at(sample - nowSample) = 0;
      continue;
    }

    /* what is the minimum required throughput in order to avoid an underrun */
    const unsigned long long minPbusecToDownload = remainingTime - usecQueued;
    const double minBytesToDownload = ((double)minPbusecToDownload / (double)usecNextGET) * (double)bytesNextGET;
    const double minRequiredThroughput = (8000000.0 * minBytesToDownload) / remainingTime; // [bit/s]

    /* */
    const unsigned predictionDistance = sample - nowSample + 1;

    /* number of combined samples we have from the past. always >= 1. */
    const unsigned numCombinedSamples =
        (lastValidSample == nowSample) ?
            ( nowSample      - lookbackStartSample + 1) / predictionDistance :
            ((nowSample - 1) - lookbackStartSample + 1) / predictionDistance;

    //printf("considering sample %u. prediction distance: %u samples\n", sample, predictionDistance);
    //printf("can use %u combined samples for prediction\n", numCombinedSamples);

    /* calculate the histogram */
    double minThroughput = -1;
    double maxThroughput = -1;
    const unsigned numBins = std::min<unsigned>(50, numCombinedSamples);
    vector<double> histogram;
    histogram.reserve(numBins);
    histogram.resize(0);
    unsigned validCombinedSamples = 0;
    const unsigned fromSample = lookbackStartSample;
    const unsigned toSample = (lastValidSample == nowSample) ? nowSample : nowSample - 1;
    Result result = calculateThroughputHistogram(fromSample, toSample, predictionDistance,
        minActivity, minThroughput, maxThroughput, numBins, histogram, validCombinedSamples);
    if (result == not_ok || validCombinedSamples == 0) {
      printf("failed to calculate histogram! validCombinedSamples: %u\n", validCombinedSamples);
      return not_ok;
    }

    dp2p_assert(numCombinedSamples > 0); //todo: remove after debugging
    if (numCombinedSamples > 1)
    {
      /* transfer histogramm into a probability density function.
       * normalize its sum to 1 / binSize so that the integral equals 1. */
      const double binSize = (maxThroughput - minThroughput) / numBins;
      vector<double> pdf(histogram.size());
      double sum = sumVector(histogram);
      for (unsigned i = 0; i < histogram.size(); ++i)
        pdf.at(i) = histogram.at(i) / (sum * binSize);

      /* in which bin is minRequiredThroughput */
      unsigned bin = 0;
      if (minRequiredThroughput >= maxThroughput)
        bin = histogram.size() - 1;
      else if (minRequiredThroughput <= minThroughput)
        bin = 0;
      else
        bin = (unsigned) floor((minRequiredThroughput - minThroughput) / binSize);

      /* calculate probability that throughput < minRequiredThroughput.
       * integrate from minThroughput to minRequiredThroughput. */
      condProb.at(sample - nowSample) = 0;
      for (unsigned i = 0; i < bin; ++i)
        condProb.at(sample - nowSample) += pdf.at(i) * binSize;
      const double minRequiredThroughputProjection =
          (minRequiredThroughput < minThroughput) ? minThroughput :
              ((minRequiredThroughput > maxThroughput) ? maxThroughput : minRequiredThroughput);
      condProb.at(sample - nowSample) +=
          pdf.at(bin) * (minRequiredThroughputProjection - (minThroughput + bin * binSize));

      //printf("histogram: using samples (%u .. %u), range (%f .. %f), %u bins, valid combined samples: %u\n",
      //    fromSample, toSample, minThroughput, maxThroughput, numBins, validCombinedSamples);
      //for(unsigned i = 0; i < histogram.size(); ++i)
      //  printf("%f, ", histogram.at(i));
      //printf("\nremainingTime=%llu, usecQueued=%llu, minPbusecToDownload = %llu, minBytesToDownload=%.9f, "
      //    "minRequiredThroughput=%.9f, minRequiredThroughputProjection=%.9f, bin=%u\n",
      //    remainingTime, usecQueued, minPbusecToDownload, minBytesToDownload, minRequiredThroughput,
      //    minRequiredThroughputProjection, bin);
      //printf("corresponding PDF:");
      //for(unsigned i = 0; i < pdf.size(); ++i)
      //  printf("%.9f, ", pdf.at(i));
      //printf("\n");
    }
    else
    {
      dp2p_assert(minThroughput == maxThroughput); //todo: just for debugging
      if (minRequiredThroughput <= minThroughput)
        condProb.at(sample - nowSample) = 0;
      else
        condProb.at(sample - nowSample) = 1;
    }

    printf("sample %u: predicted conditional underrun probability is %f\n",
        sample, condProb.at(sample - nowSample));
    if(condProb.at(sample - nowSample) < 0) { //todo: just for debugging
      fflush(stdout);
      dp2p_assert(0);
    }
  }

  /* calculate the underrun probability */
  p = condProb.at(0);
  for (unsigned sample = nowSample + 1; sample <= lastSampleToBePredicted; ++sample)
  {
    p = (1 - p) * condProb.at(sample - nowSample);
  }

  printf("predicted underrun probability: %f\n", p);

  return ok;
}

Result ThroughputModel::calculateThroughputHistogram(unsigned fromSample, unsigned toSample,
    unsigned predictionDistance,
    double minActivity, double& minThroughput, double& maxThroughput, unsigned numBins,
    vector<double>& phi, unsigned& validCombinedSamples)
{
  /* consistency checks */
  dp2p_assert(phi.size() == 0);
  dp2p_assert(minActivity > 0 && minActivity <= 1);
  dp2p_assert(predictionDistance > 0);
  if (predictionDistance > toSample - fromSample + 1) { //todo: remove after debugging
    FILE* file = fopen("debug.txt", "w");
    dumpThroughputProcess(file);
    fclose(file);
  }
  dp2p_assert(predictionDistance <= (toSample - fromSample + 1));

  /* prepare the return variables */
  phi.resize(numBins);
  validCombinedSamples = 0;

  /* how many combined samples will we use? */
  const unsigned numCombinedSamples = (toSample - fromSample + 1) / predictionDistance;

  /* if one of the boundaries is not given -> calculate */
  if (minThroughput < 0 || maxThroughput < 0)
  {
    if (minThroughput == -1)
      minThroughput = numeric_limits<double>::max();
    /* iterate over the samples in question */
    for (unsigned i = toSample - predictionDistance * numCombinedSamples + 1; i <= toSample; i += predictionDistance)
    {
      /* calculate parameters of the combined sample */
      unsigned long long combinedActivity = 0;
      unsigned long long combinedNumBytes = 0;
      for (unsigned j = 0; j < predictionDistance; ++j) {
        /* get a reference to the current sample */
        const ThroughputSample& ts = throughputProcess.at(i + j);
        combinedActivity += ts.active;
        combinedNumBytes += ts.num_bytes;
      }

      /* skip combined samples with too low activity */
      if (combinedActivity < minActivity * (2.0 * dt))
        continue;
      ++validCombinedSamples;
      /* calculate throughput of the combined sample in [bit/s] */
      double thrpt = (8000000.0 * combinedNumBytes) / combinedActivity;
      if(thrpt < minThroughput)
        minThroughput = thrpt;
      if(thrpt > maxThroughput)
        maxThroughput = thrpt;
    }
  }

  /* size of the bins */
  double binSize = (maxThroughput - minThroughput) / numBins;

  /* iterate over the samples in question */
  for (unsigned i = toSample - predictionDistance * numCombinedSamples + 1; i <= toSample; i += predictionDistance)
  {
    /* calculate parameters of the combined sample */
    unsigned long long combinedActivity = 0;
    unsigned long long combinedNumBytes = 0;
    for (unsigned j = 0; j < predictionDistance; ++j) {
      /* get a reference to the current sample */
      const ThroughputSample& ts = throughputProcess.at(i + j);
      combinedActivity += ts.active;
      combinedNumBytes += ts.num_bytes;
    }

    /* skip combined samples with too low activity */
    if (combinedActivity < minActivity * (2.0 * dt))
      continue;
    /* calculate throughput of the combined sample in [bit/s] */
    double thrpt = (8000000.0 * combinedNumBytes) / combinedActivity;

    /* put the combined sample into the appropriate bin */
    if (thrpt <= minThroughput) {
      phi.at(0) += 1;
    } else if (thrpt >= maxThroughput) {
      phi.back() += 1;
    } else {
      unsigned bin = (unsigned) floor((thrpt - minThroughput) / binSize);
      phi.at(bin) += 1;
    }
  }

  return ok;
}

void ThroughputModel::add(GETStatistics& x)
{
  getStatisticsHistory.append(x, false);
  updateThroughputProcess();

  /*static unsigned cnt = 0;
  char buf[1024];
  sprintf(buf, "debug_ThroughputProcess_%02u.txt", cnt++);
  FILE* f = fopen(buf, "w");
  dp2p_assert(f);
  dumpThroughputProcess(f);
  fclose(f);*/
}

const GETStatistics& ThroughputModel::getLastGETStatistics(unsigned i) const
{
  return getStatisticsHistory.getLast(i);
}

const ThroughputSample& ThroughputModel::getLastThroughputSample(unsigned i) const
{
  return throughputProcess.at(throughputProcess.size() - i - 1);
}

unsigned ThroughputModel::getNumberOfGETMeasurements() const
{
  return getStatisticsHistory.size();
}

void ThroughputModel::dumpThroughputProcess(FILE* file)
{
  unsigned long long cnt = 0;

  /* output an initialization line */
  fprintf (file, "%llu %d %d %d\n", dt, 0, 0, 0);
  /* output the data */
  for (unsigned i = 0; i < throughputProcess.size(); ++i) {
    const ThroughputSample& ts = throughputProcess.at(i);
    cnt += ts.num_bytes;
    fprintf (file, "%llu %llu %llu %llu\n", ts.start, ts.stop, ts.num_bytes, ts.active);
  }
  fflush(file);

  //printf("Total num_bytes = %llu\n", cnt);
}

void ThroughputModel::dumpGETStatisticsHistory(FILE* file) const
{
  unsigned long long cnt = 0;
  const unsigned numValues = getStatisticsHistory.size();
  for (unsigned i = 0; i < numValues; ++i) {
    const GETStatistics& gs = getStatisticsHistory.getFirst(i);
    fprintf (file,
        "started=%llu[us], hdr=%llu[us], size=%llu[byte], encodingRate=%llu[bit/s], usecInPipeAtStart=%llu[us], "
        "num_chunks=%u, chunk_times={",
        gs.started, gs.header_received, gs.content_size, gs.encodingRate,
        gs.usecInPipeAtStart, gs.chunks.size());
    cnt += gs.content_size - gs.chunks[0].size;

    for (unsigned chunk = 0; chunk < gs.chunks.size() - 1; ++chunk) {
      const ChunkStatistics& cs = gs.chunks.getFirst(chunk);
      fprintf (file, "(%llu,%llu),", cs.received, cs.size);
    }
    const ChunkStatistics& cs = gs.chunks.getLast();
    fprintf (file, "(%llu,%llu)}, ", cs.received, cs.size);

    fprintf (file, "completed=%llu[us]\n", cs.received);
  }
  fflush(file);

  //printf("Total content_size = %llu\n", cnt);
}

GETStatistics::GETStatistics(unsigned long long started,
      Usec header_received,
      const RingBuffer<ChunkStatistics>& chunks,
      unsigned long long content_size,
      unsigned long long encodingRate,
      Usec duration,
      Usec usecInPipeAtStart)
: started(started),
  header_received(header_received),
  chunks(chunks),
  content_size(content_size),
  encodingRate(encodingRate),
  duration(duration),
  usecInPipeAtStart(usecInPipeAtStart)
{

}
