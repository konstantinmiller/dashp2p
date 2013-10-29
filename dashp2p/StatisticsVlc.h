/****************************************************************************
 * StatisticsVlc.h                                                          *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Jun 15, 2012                                                 *
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

#ifndef STATISTICSVLC_H_
#define STATISTICSVLC_H_

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define DP2P_ASSERT(x) if(!(x)) {printf("Error in %s:%d.\n", __FILE__, __LINE__); fflush(NULL); abort(); exit(1);}

typedef struct Dp2pHeader {int size;} Dp2pHeader;
typedef struct Dp2pElement {
	int64_t ptsRel;
	int64_t ptsAbs;
	int64_t tsDemuxed;
	int64_t tsDecoded;
	int64_t tsPopped;
	int64_t tsDplFirst;
	int64_t tsDplLast;
} Dp2pElement;

static const int     dp2pMaxEl = 65536;
static int64_t       dp2pShmSize = 0;
static char shmName[1024];

static Dp2pHeader*  hdr  = NULL;
static Dp2pElement* data = NULL;

static void dp2p_init(vlc_object_t *p_this)
{
	int fd = -1;
	if(var_InheritString(p_this, "dashp2p-shm"))
	    strcpy(shmName, var_InheritString(p_this, "dashp2p-shm"));
	else
	    strcpy(shmName, "/DP2P_VOUT_SHM");

#ifdef DP2P_dashp2p_cpp
	fd = shm_open(shmName, O_CREAT | O_TRUNC | O_RDWR, 0666);
	DP2P_ASSERT(fd > 0);
#else
	fd = shm_open(shmName, O_RDWR, 0666);
	if(fd <= 0) {
	    printf("WARNING: Operating without DP2P shm.");
	    return;
	}
#endif

	const size_t region_size = sysconf(_SC_PAGE_SIZE);
	dp2pShmSize = region_size * ((sizeof(Dp2pHeader) + dp2pMaxEl * sizeof(Dp2pElement)) / region_size + 1);

#ifdef DP2P_dashp2p_cpp
	DP2P_ASSERT(0 == ftruncate(fd, dp2pShmSize));
#endif

	hdr = (Dp2pHeader*) mmap(0, dp2pShmSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	DP2P_ASSERT(hdr != MAP_FAILED);
	DP2P_ASSERT(0 == close(fd));

	data = (Dp2pElement*) (hdr + 1);

#ifdef DP2P_dashp2p_cpp
	memset(hdr, 0, dp2pShmSize);
#endif
}

#ifdef DP2P_dashp2p_cpp
static void dp2p_cleanup(const char* tracesDir)
#else
static void dp2p_cleanup()
#endif
{
    if(!hdr) return;

#ifdef DP2P_dashp2p_cpp
    DP2P_ASSERT(tracesDir);
    char buf[1024];
    sprintf(buf, "%s/dp2p_vlc_statistics.txt", tracesDir);
	FILE* f = fopen(buf, "w");
	DP2P_ASSERT(f);
	for(int i = 0; i < hdr->size; ++i) {
		fprintf(f, "%"PRId64" %"PRId64" %"PRId64" %"PRId64" %"PRId64" %"PRId64" %"PRId64"\n",
		        data[i].ptsRel, data[i].ptsAbs, data[i].tsDemuxed, data[i].tsDecoded,
		        data[i].tsPopped, data[i].tsDplFirst, data[i].tsDplLast);
	}
	DP2P_ASSERT(0 == fclose(f));
#endif

	DP2P_ASSERT(0 == munmap(hdr, dp2pShmSize));

#ifdef DP2P_dashp2p_cpp
	DP2P_ASSERT(0 == shm_unlink(shmName));
#endif
}

#ifdef DP2P_demux_c
static void dp2p_record_demuxed(int64_t ptsRel)
{
    if(!hdr) return;
	DP2P_ASSERT(hdr->size < dp2pMaxEl - 1);
	data[hdr->size].ptsRel = ptsRel;
	//printf("%x, %" PRId64 ", %" PRId64 ", %" PRId64 "\n", hdr, hdr->size, data[hdr->size].ptsRel, data[hdr->size - 1].ptsRel); fflush(NULL);
	//DP2P_ASSERT(hdr->size == 0 || data[hdr->size].ptsRel > data[hdr->size - 1].ptsRel);
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	data[hdr->size].tsDemuxed = (int64_t)1000000 * (int64_t)ts.tv_sec + (int64_t)ts.tv_nsec / (int64_t)1000;
	++ hdr->size;
	//printf("%20lld: demuxed frame %20lld.\n", data[hdr->size - 1].tsDemuxed, data[hdr->size - 1].ptsRel);
}
#endif

#ifdef DP2P_decoder_c
static void dp2p_record_decoded(int64_t ptsRel, int64_t ptsAbs)
{
    if(!hdr) return;
	DP2P_ASSERT(hdr->size > 0);
	int i = hdr->size - 1;
	for(; data[i].ptsRel != ptsRel; --i) {}
	//DP2P_ASSERT(data[i].ptsRel == ptsRel);
	data[i].ptsAbs = ptsAbs;
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	data[i].tsDecoded = (int64_t)1000000 * (int64_t)ts.tv_sec + (int64_t)ts.tv_nsec / (int64_t)1000;
	//printf("%20lld: decoded frame %20lld (%20lld).\n", data[i].tsDecoded, data[i].ptsRel, data[i].ptsAbs);
}
#endif

#ifdef DP2P_video_output_c
static void dp2p_record_vout_popped(int64_t ptsAbs)
{
    if(!hdr) return;
	DP2P_ASSERT(hdr->size > 0);
	int i = hdr->size - 1;
	for(; 0 == data[i].ptsAbs || data[i].ptsAbs != ptsAbs; --i) {}
	//DP2P_ASSERT(data[i].ptsAbs == ptsAbs);
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	data[i].tsPopped = (int64_t)1000000 * (int64_t)ts.tv_sec + (int64_t)ts.tv_nsec / (int64_t)1000;
	//printf("%20lld: popped frame %20lld (%20lld).\n", data[i].tsPopped, data[i].ptsRel, data[i].ptsAbs);
}

static void dp2p_record_vout_displayed(int64_t ptsAbs)
{
    if(!hdr) return;
	DP2P_ASSERT(hdr->size > 0);
	int i = hdr->size - 1;
	for(; 0 == data[i].ptsAbs || data[i].ptsAbs != ptsAbs; --i) {}
	//DP2P_ASSERT(data[i].ptsAbs == ptsAbs);
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	if(data[i].tsDplFirst == 0)
		data[i].tsDplFirst = (int64_t)1000000 * (int64_t)ts.tv_sec + (int64_t)ts.tv_nsec / (int64_t)1000;
	data[i].tsDplLast = (int64_t)1000000 * (int64_t)ts.tv_sec + (int64_t)ts.tv_nsec / (int64_t)1000;
	//printf("%20lld: displayed frame %20lld (%20lld).\n", data[i].tsDplLast, data[i].ptsRel, data[i].ptsAbs);
}
#endif

#endif /* STATISTICSVLC_H_ */
