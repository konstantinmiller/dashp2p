/****************************************************************************
 * dashp2p.cpp: MPEG DASH plugin for VLC                                    *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 21, 2012                                                 *
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

#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <cassert>
#include <string>
#include <cstdarg>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_access.h>
#include <vlc_playlist.h>

#include "dashp2p.h"
//#include "Dashp2pTypes.h"
#include "Utilities.h"
#include "Statistics.h"
#include "OverlayAdapter.h"
#include "Control.h"
#include "XmlAdapter.h"
#include "TcpConnectionManager.h"
#include "SourceManager.h"

#define DP2P_dashp2p_cpp
#include "StatisticsVlc.h"
#undef DP2P_dashp2p_cpp

using namespace dashp2p;

// TODO: update access_t::info

/*****************************************************************************
* Local prototypes.
*****************************************************************************/

struct access_sys_t
{
    int64_t decoderBufferSize;
    bool withOverlay;
};

static int      Open    ( vlc_object_t * );
static void     Close   ( vlc_object_t * );
static int      seek    ( access_t *, uint64_t );
static int      control ( access_t *, int i_query, va_list args );
//static block_t* block   ( access_t *p_access );
static ssize_t  read    ( access_t* p_access, uint8_t* buffer, size_t size );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/

vlc_module_begin()
    set_shortname("dashp2p")
    set_description("MPEG-DASH (Dynamic Adaptive Streaming over HTTP)")
    set_capability( "access", 60 )
    set_callbacks( Open, Close )
    set_category( CAT_INPUT )
    set_subcategory( SUBCAT_INPUT_ACCESS )
    add_shortcut( "http" )

    /* Recording traces */
    add_string("dashp2p-record-traces", "", "Directory for recording of traces or empty if no traces shall be recorded.",
    		"Directory for recording of traces or empty if no traces shall be recorded.", false)
    add_string("dashp2p-logfile", "", "Directory for ADDITIONAL debugging to file.", "Directory for ADDITIONAL debugging to file.", true)
    add_string("dashp2p-shm", "", "Shared memory name for communication with VLC.", "Shared memory name for communication with VLC.", true)
    add_bool("dashp2p-log-tcp", false, "Enables extensive logging of TCP internal state.", "Enables extensive logging of TCP internal state.", true)

    /* VLC related */
    add_integer("dashp2p-decoder-buffer-size", 500, "Decoder buffer size in [ms]", "Decoder buffer size in [ms]", true)

    /* Device related */
    add_integer("dashp2p-width", 1, "Picture width. -1 (+1) for lowest (highest) available in the MPD.", "Picture width. -1 (+1) for lowest (highest) available in the MPD.", true)
    add_integer("dashp2p-height", 1, "Picture height. -1 (+1) for lowest (highest) available in the MPD.", "Picture height. -1 (+1) for lowest (highest) available in the MPD.", true)

    /* Adaptation related */
    add_integer_with_range("dashp2p-adaptation-strategy", 0, 0, 3, "Adaptation strategy", "Adaptation strategy", false)
    add_string("dashp2p-adaptation-config", "2:10:30:0.75:0.8:0.8:0.8:0.9:5:0", "Configuration of the selected adaptation strategy.",
    		"Configuration of the selected adaptation strategy.", false)

    /* Application layer handover related */
    //add_bool("dashp2p-handover", false, "Experimental: enable application-layer handover.", "Experimental: enable application-layer handover.", true)

    add_string("dashp2p-not-an-actual-option", "", "Compiled on " __DATE__ ", at " __TIME__ ".", "", false)

vlc_module_end ()

/*****************************************************************************
 * Open: initialize interface
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
    // TODO: check if restart of the plugin 100% correct

//#if !defined DP2P_VLC || DP2P_VLC == 0
//	msg_Err(p_this, "DP2P_VLC is not set!");
//	abort();
//#endif

    access_t *p_access = (access_t*)p_this;

    /* Check if we can proceed. */
    if(strncasecmp(p_access->psz_location + strlen(p_access->psz_location) - 3, "mpd", 3) != 0) {
        msg_Dbg( p_this, "No MPEG DASH MPD file extension detected. Aborting." );
        return VLC_EGENERIC;
    } else {
        msg_Info( p_this, "MPEG-DASH MPD file extension detected. dashp2p plugin takes over. (Compiled on " __DATE__ ", at " __TIME__ ".)" );
    }

    /* Input parameters */
    const string mpdUrl                 = p_access->psz_location;
    const string tracesDir              = var_InheritString  (p_this, "dashp2p-record-traces") ? var_InheritString  (p_this, "dashp2p-record-traces") : "";
    const string logFile                = var_InheritString  (p_this, "dashp2p-logfile") ? var_InheritString  (p_this, "dashp2p-logfile") : "";
    const int64_t decoderBufferSize     = var_InheritInteger (p_this, "dashp2p-decoder-buffer-size");
    const int windowWidth               = var_InheritInteger (p_this, "dashp2p-width"              );
    const int windowHeight              = var_InheritInteger (p_this, "dashp2p-height"             );
    const int controlType               = var_InheritInteger (p_this, "dashp2p-adaptation-strategy");
    const string adaptationConfig       = var_InheritString  (p_this, "dashp2p-adaptation-config"  );
    //const bool dashp2pHandover          = var_InheritBool    (p_this, "dashp2p-handover"           );

    /* Initializing the module */
    p_access->p_sys = (access_sys_t*) malloc( sizeof(access_sys_t) );
    p_access->p_sys->decoderBufferSize = 1000 * decoderBufferSize; // [ms] -> [us]
    p_access->p_sys->withOverlay = false;
    p_access->pf_seek = seek;
    p_access->pf_seek = NULL;
    p_access->pf_control = control;
    p_access->pf_read = read;
    p_access->pf_block = NULL; // = block;

    /* Setting reference time to current system time. */
    dashp2p::Utilities::setReferenceTime();

    /* Initialize the debugging system */
    msg_Dbg(p_this, "verbose=%" PRId64 ", file-logging=%s, log-verbose=%" PRId64, var_InheritInteger(p_this, "verbose"),
            var_InheritBool(p_this, "file-logging") ? "yes" : "no", var_InheritInteger(p_this, "log-verbose"));

    DebuggingLevel dl = DebuggingLevel_Info;
    if(var_InheritInteger(p_this, "verbose") >= 2
            || (var_InheritBool(p_this, "file-logging") && var_InheritInteger(p_this, "log-verbose") >= 2)
            || (var_InheritBool(p_this, "file-logging") && var_InheritInteger(p_this, "log-verbose") == -1 && var_InheritInteger(p_this, "verbose") >= 2)) {
    		dl = DebuggingLevel_Dbg;
    }
    if(logFile.empty())
    	DebugAdapter::init(dl, p_this, NULL);
    else
    	DebugAdapter::init(dl, p_this, logFile.c_str());

    DBGMSG("Starting dashp2p at %s with URL: %s.", dashp2p::Utilities::getTimeString().c_str(), p_access->psz_location);

    /* Check if we are using the dynamicoverlay plugin */
    string overlayInput;
    string overlayOutput;
    if(NULL != var_InheritString(p_this, "sub-source")
            && 0 == strcmp(var_InheritString(p_this, "sub-source"), "overlay")
            && NULL != var_InheritString(p_this, "overlay-input")
            && NULL != var_InheritString(p_this, "overlay-output"))
    {
        DBGMSG("We are going to use the dynamicoverlay plugin for overlay output.");
        DBGMSG("overlay-input: %s, overlay-output: %s", var_InheritString(p_this, "overlay-input"), var_InheritString(p_this, "overlay-output"));
        overlayInput = var_InheritString(p_this, "overlay-input");
        overlayOutput = var_InheritString(p_this, "overlay-output");
        p_access->p_sys->withOverlay = true;
    } else {
    	INFOMSG("If you want overlay debugging information, add: --sub-source=overlay --overlay-input=dashp2p_overlay_input --overlay-output=dashp2p_overlay_output");
    }

    /* Initialize XML adapter */
    //XmlAdapter::init(p_this);

    /* Initialize the statistics module */
    {
    	const bool logTcpState = var_InheritBool(p_this, "dashp2p-log-tcp");
    	const bool logScalarValues = true;
    	const bool logAdaptationDecision = true;
    	const bool logGiveDataToVlc = false;
    	const bool logBytesStored = true;
    	const bool logSecStored = true;
    	const bool logUnderruns = true;
    	const bool logReconnects = false;
    	const bool logSegmentSizes = false;
    	const bool logRequestStatistics = true;
    	const bool logRequestDownloadProgress = true;
    	Statistics::init(tracesDir, logTcpState, logScalarValues, logAdaptationDecision, logGiveDataToVlc, logBytesStored,
    			logSecStored, logUnderruns, logReconnects, logSegmentSizes, logRequestStatistics, logRequestDownloadProgress);
    	if(!tracesDir.empty())
    	    dp2p_init(p_this);
    }

    /* Initialize overlay module */
    if(p_access->p_sys->withOverlay) {
        DBGMSG("Initializing overlay module.");
        OverlayAdapter::init(overlayInput, overlayOutput);
    }

    /* Initializing the Control module, which starts to retrieve data. */
    const ControlType _controlType = (ControlType)controlType;
    Control::init(mpdUrl, windowWidth, windowHeight, _controlType, adaptationConfig);

    return VLC_SUCCESS;
}

/*****************************************************************************
 * Close: destroy interface
 *****************************************************************************/
static void Close( vlc_object_t *p_this )
{
    access_t *p_access = (access_t*)p_this;
    access_sys_t* p_sys = p_access->p_sys;

    DBGMSG("Terminating dashp2p.");

    /* Some file manipulation. */
    FILE* f = fopen("log_VLC.txt", "r");
    if(f) {
        uint64_t timestamp = 0;
        dp2p_assert(1 == fscanf(f, "%" SCNu64, &timestamp));
        Statistics::recordScalarU64("VLC_launched", timestamp);
        fclose(f);
        dp2p_assert(0 == remove("log_VLC.txt"));
    }

    /* More file manipulation. */
    f = fopen("log_video_visibility.txt", "r");
    if(f) {
        uint64_t timestamp = 0;
        int ifVisible = -1;
        dp2p_assert(2 == fscanf(f, "%" SCNu64 " %d", &timestamp, &ifVisible));
        dp2p_assert(ifVisible == 1);
        Statistics::recordScalarU64("videoVisible", timestamp);
        fclose(f);
        dp2p_assert(0 == remove("log_video_visibility.txt"));
    }

    /* Output traces */
    Statistics::recordScalarU64("endTime", Utilities::getAbsTime());
    Statistics::outputStatistics();
    if(!Statistics::getLogDir().empty())
        dp2p_cleanup(Statistics::getLogDir().c_str());

    /* Clean-up. */
    Control::cleanUp();
    //XmlAdapter::cleanup();
    Statistics::cleanUp();
    if(p_sys->withOverlay)
        OverlayAdapter::cleanup();
    HttpClientManager::cleanup();
    TcpConnectionManager::cleanup();
    SourceManager::cleanup();
    MpdWrapper::cleanup();

    DBGMSG("The End.");

#ifdef DEBUG_BLOCK_OUTPUT
    fclose(p_access->p_sys->f_debug_block_output);
#endif

    free( p_sys );

    DebugAdapter::cleanUp();
}

/* TODO: implement seeking */
static int seek( access_t * /*p_access*/, uint64_t )
{
    /*
     * The seeking function will be called whenever a seek is requested.
     * The arguments are a pointer to the module structure, and the requested position.
     * NB: Seeking function can be NULL, if it is not possible to seek in the protocol or device.
     * You shall set p_access->info.b_eof to false if seek worked.
     * Return:
     * If the seek has succeeded, it should return VLC_SUCCESS, else it should return VLC_EGENERIC.
     */
    //access_sys_t* p_sys = p_access->p_sys;

    DBGMSG("seek entered" );
    dp2p_assert(0);
    return VLC_EGENERIC;
}

static int control( access_t* p_access, int i_query, va_list args)
{
    //access_sys_t* p_sys = p_access->p_sys;

    /* You can find the list of arguments corresponding to query types
     * in the comments of access_query_e definition in vlc_access.h. */
    switch( i_query )
    {
    case ACCESS_CAN_SEEK:
    {
        DBGMSG("VLC asking if ACCESS_CAN_SEEK");
        bool* pb_bool = (bool*)va_arg( args, bool* );
        *pb_bool = false;
        return VLC_SUCCESS;
    }

    case ACCESS_CAN_FASTSEEK:
    {
        DBGMSG("VLC asking if ACCESS_CAN_FASTSEEK");
        bool* pb_bool = (bool*)va_arg( args, bool* );
        *pb_bool = false;
        return VLC_SUCCESS;
    }

    case ACCESS_CAN_PAUSE:
    {
        DBGMSG("VLC asking if ACCESS_CAN_PAUSE");
        bool* pb_bool = (bool*)va_arg( args, bool* );
        *pb_bool = true;
        return VLC_SUCCESS;
    }

    case ACCESS_CAN_CONTROL_PACE:
    {
        DBGMSG("VLC asking if ACCESS_CAN_CONTROL_PACE");
        bool* pb_bool = (bool*)va_arg( args, bool* );
        *pb_bool = true;
        return VLC_SUCCESS;
    }

    case ACCESS_GET_PTS_DELAY:
    {
        DBGMSG("VLC asking for ACCESS_GET_PTS_DELAY");
        int64_t* pi_64 = (int64_t*)va_arg( args, int64_t * );
        *pi_64 = (int64_t)p_access->p_sys->decoderBufferSize;
        return VLC_SUCCESS;
    }

    case ACCESS_GET_TITLE_INFO:
        DBGMSG("VLC asking for ACCESS_GET_TITLE_INFO");
        return VLC_EGENERIC;
    case ACCESS_GET_META:
        DBGMSG("VLC asking for ACCESS_GET_META");
        return VLC_EGENERIC;
    case ACCESS_GET_CONTENT_TYPE:
        DBGMSG("VLC asking for ACCESS_GET_CONTENT_TYPE");
        return VLC_EGENERIC;
    case ACCESS_GET_SIGNAL:
        DBGMSG("VLC asking for ACCESS_GET_SIGNAL");
        return VLC_EGENERIC;

    case ACCESS_SET_PAUSE_STATE:
        DBGMSG("asking for ACCESS_SET_PAUSE_STATE(%s)", va_arg(args, int) ? "true" : "false");
        return VLC_SUCCESS;

#if 0
    case ACCESS_SET_TITLE:
        dbgPrintf("dashp2p", "asking for ACCESS_SET_TITLE");
        return VLC_EGENERIC;
    case ACCESS_SET_SEEKPOINT:
        dbgPrintf("dashp2p", "asking for ACCESS_SET_SEEKPOINT");
        return VLC_EGENERIC;
#endif
    default:
        ERRMSG("control(): i_query = %d not recognized", i_query);
        dp2p_assert(false);
        return VLC_EGENERIC;
    }

    /* Error condition: this code should never be reached. Let's still make the compiler happy. */
    dp2p_assert(false);
    return VLC_EGENERIC;
}


#if 0
static block_t* block( access_t* p_access )
{
    /* Get new stream data. */
    char* buffer = NULL;
    int bufferSize = 0;
    int bytesReturned = 0;
    double secReturned = 0;
    int ifExpectingMoreData = Control::getDataForPlayback(&buffer, &bufferSize, &bytesReturned, &secReturned);

    block_t* p_block = NULL;
    if(buffer)
    {
        dbgPrintf(moduleName.c_str(), "Got %u bytes from Control.", bufferSize);
        dp2p_assert(bufferSize);
        p_block = block_New( p_access, bufferSize );
        /* TODO: avoid copying here */
        memcpy(p_block->p_buffer, buffer, bufferSize);
        p_block->i_buffer = bufferSize;
    }
    else
    {
        dp2p_assert(!bufferSize);
        if(ifExpectingMoreData) {
            dbgPrintf(moduleName.c_str(), "Got NO bytes from Control. Expecting more data.");
            dp2p_assert( 0 == usleep(100000) );
        } else {
            dbgPrintf(moduleName.c_str(), "Got NO bytes from Control. EOF reached.");
            p_access->info.b_eof = true;
        }
    }

    return p_block;
}
#endif


ssize_t read(access_t* /*p_access*/, uint8_t* buffer, size_t size)
{
    DBGMSG("Asking for up to %d bytes.", size);

    enum _State {READ_INIT, READ_OK, READ_UNDERRUN};
    static _State _state = READ_INIT;

    /* Get new stream data. */
    char* _buffer = (char*)buffer;
    int _size = size;
    int bytesReturned = 0;
    int64_t usecReturned = 0;
    int ifExpectingMoreData = Control::vlcCb(&_buffer, &_size, &bytesReturned, &usecReturned);

    if(bytesReturned == 0 && !ifExpectingMoreData) {
    	DBGMSG("Returning 0 bytes and EOF.");
        return 0;
    } else if(bytesReturned == 0 && ifExpectingMoreData) {
        /* Logging */
        if(_state == READ_OK) {
            _state = READ_UNDERRUN;
            Statistics::recordGiveDataToVlc(dashp2p::Utilities::getTime(), 0.0, 0.0);
        }
        DBGMSG("Returning 0 bytes but NO EOF.");
        return -1;
    }

    if(_state == READ_INIT) {
        Statistics::recordScalarD64("startGiveDataToVLC", dashp2p::Utilities::getTime());
    }

    _state = READ_OK;

    /* Dump statistics */
    static int64_t usecConsumed = 0;
    static uint64_t byteConsumed = 0;
    usecConsumed += usecReturned;
    byteConsumed += bytesReturned;
    Statistics::recordGiveDataToVlc(dashp2p::Utilities::getTime(), usecConsumed / 1e6, (double)byteConsumed);

    /* Dump segment data */
#if 0
    static FILE* fileDumpSegmentData = NULL;
    if(!fileDumpSegmentData) {
    	fileDumpSegmentData = fopen("dbg_segment_data.mp4", "w");
    	dp2p_assert(fileDumpSegmentData);
    }
    dp2p_assert(bytesReturned == (int)fwrite(buffer, 1, bytesReturned, fileDumpSegmentData));
#endif

    DBGMSG("Returning %d bytes (NO EOF).", bytesReturned);
    return bytesReturned;
}
