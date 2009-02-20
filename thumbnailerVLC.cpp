/*
 * thumbnailerVLC.cpp
 *
 *  Created on: Feb 6, 2009
 *      Author: eff2
 */

#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vlc/vlc.h>
#include <unistd.h>
//#include <cunistd>

using namespace std;

#include <boost/format.hpp>
using namespace boost;

#include <boost/program_options.hpp>
namespace args = boost::program_options;

#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>

boost::condition_variable seekCondition;
boost::mutex mut;
bool seek_ready = false;

int seekCounter = 0;

// http://wiki.videolan.org/LibVLC_Tutorial

/* "position changed" callback */
static void callback( const libvlc_event_t *ev, void *param )
{
    if( ev->type == libvlc_MediaPlayerPositionChanged)
    {
		float new_pos;
		new_pos = ev->u.media_player_position_changed.new_position;

//        if( new_pos >= POS - ( POS / 10 ) )
		cout << format("Jumped to position %1%") % new_pos << endl;
		if (seek_ready == false)
		{
			cout << "Waiting for seek, notifying" << endl;
			{
				boost::lock_guard<boost::mutex> lock(mut);
				seek_ready = true;
			}
			cout << "Notification done" << endl;
			seekCondition.notify_one();
		} else {
			cout << "seek was already ready ??" << endl;
		}
    }
    else
    {
        fprintf( stderr, "Error: catched event %s\n",
                libvlc_event_type_name( ev->type ) );
        exit(1);
    }
}


static void raise(libvlc_exception_t * ex)
{
    if (libvlc_exception_raised (ex))
    {
         cerr << boost::format("error: %1%") % (libvlc_exception_get_message(ex)) << endl;
         exit (-1);
    }
}

int main(int argc, char* argv[])
{
	//TODO: boost::args

	args::positional_options_description posArgs;
	posArgs.add("in", 1);

	args::options_description argDesc("Allowed options");
	argDesc.add_options()("help,h", "produce help message")
			("in,i", args::value<string>(),"input file (no -i flag needed, arg autopickuped)")
			("seek,s", program_options::value<float>(), "seek to second N")
			("frames,f", program_options::value<int>(), "save X frames")
			("jump,j", program_options::value<float>(), "jump Y seconds between each thumbnail")
			("outdir,o", args::value<string>(),"output directory")
			("verbose,v", "Verbose");
//			("gaussfloats,g", args::value<string>(),"gaussian float projection data file")
//			("iterations,i", program_options::value<int>(), "N repetitions, eliminates startup time for accurate description speed figures")
//			("floats", program_options::value<float>(), "Generate and retrieve raw floating point descriptors");
//			("unifloats,f", args::value<string>(), "uniform float projection data file")
//			("uniuints,u", args::value<string>(), "uniform uint projection data file")
//			("testkeys,t", "enable testing of hash keys against CPU-generated reference")
//			("outdir,o", args::value<string>(), "output directory")
//			("filter", "extra (redundant) filtering of line- or gridlike junk descriptors (already done internally, this is an obsolete arg)")
//			("points,p", "Save interest point locations")
//			("device,d", program_options::value<int>(), "Select CUDA device 0-n");

	args::variables_map vars;
	args::store(args::command_line_parser(argc, argv).options(argDesc).positional(posArgs).allow_unregistered().run(), vars);
	//	args::store(po::parse_command_line(argc, argv, argDesc), vars);
	args::notify(vars);

	bool verbose = vars.count("verbose");
	if (verbose)
		cout << "Verbose ON" << endl;

	int seek = 0;
	int framesToSave = 0;
	float jump = 0;

	string outdirname = "./";

	string infilename;

	//	cout << "Checking vars ..." << endl;
	if (vars.count("help"))
	{
		cout << argDesc << endl;;
		return 1;
	}
	if (vars.count("seek"))
	{
		seek = vars["seek"].as<float>();
		if (verbose) cout << format("Seeking %1% seconds into file") % seek << endl;
	}
	if (vars.count("frames"))
	{
		framesToSave = vars["frames"].as<int>();
		if (verbose) cout << format("Saving %1% frames") % framesToSave << endl;
	}
	if (vars.count("jump"))
	{
		jump = vars["jump"].as<float>();
		if (verbose) cout << format("Jumping by %1% second increments") % jump << endl;
	}
	if (vars.count("outdir"))
	{
		outdirname = vars["outdir"].as<string>();
		if (verbose) cout << format("Output directory: '%1%'") % outdirname << endl;
	}
	if (vars.count("in"))
	{
		infilename = vars["in"].as<string>();
		if (verbose) cout << format("Reading from file '%1%'") % infilename << endl;
	} else {
		cerr << "Input file missing" << endl;
		cout << argDesc << endl;
		return 1;
	}
	// TODO: frames, jump

//	string moviefile(argv[1]);

    //TODO: take no interface / audio / etc arg settings from SampleCode Thumbnailer:
		/*
		const char* const args[] = {
				"-I", "dummy",                      // no interface
				"--vout", "dummy",                  // we don't want video (output)
				"--no-audio",                       // we don't want audio
				"--verbose=0",                      // show only errors
				"--no-media-library",               // don't want that
				"--services-discovery", "",         // nor that
				"--no-video-title-show",            // nor the filename displayed
				"--no-stats",                       // no stats
				"--ignore-config",            // don't use/overwrite the config
				"--no-sub-autodetect",              // don't want subtitles
				"--control", "",                    // don't want interface (again)
				"--no-inhibit",                     // i say no interface !
				"--no-disable-screensaver",         // wanna fight ?
				"--extraintf", ""                   // ok, it will be a piece of cake
			};
		*/
	// old args:
/*    const char * const vlc_args[] = {
              "-I", "dummy",  Don't use any interface
              "--ignore-config",  Don't use VLC's config
              "--plugin-path=/set/your/path/to/libvlc/module/if/you/are/on/windows/or/macosx" };*/
	const char* const vlc_args[] = {
										"-I", "dummy",                      // no interface
										"--vout", "dummy",                  // we don't want video (output)
//										"--novideo",                  // KD // vlc update? // we don't want video (output)
										"--no-audio",                       // we don't want audio
										"--verbose=1" /*=0*/,                      // show only errors
										"--no-media-library",               // don't want that
										"--services-discovery", "",         // nor that
										"--no-video-title-show",            // nor the filename displayed
										"--no-stats",                       // no stats
										"--ignore-config",            // don't use/overwrite the config
										"--no-sub-autodetect",              // don't want subtitles
										"--control", "",                    // don't want interface (again)
										"--no-inhibit",                     // i say no interface !
										"--no-disable-screensaver",         // wanna fight ?
										"--extraintf", ""                   // ok, it will be a piece of cake
									};
	cout << "Watch the VLC args - default vout / interface (?)" << endl;
    libvlc_exception_t ex;
    libvlc_instance_t * inst;
    libvlc_media_player_t *mp;
    libvlc_media_t *m;

    libvlc_exception_init (&ex);
    /* init vlc modules, should be done only once */



    inst = libvlc_new (sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args, &ex);
    raise (&ex);

    {
    /* Create a new item */
    	char infile_crap[10240];
    	strcpy(infile_crap, infilename.c_str());
    	cout << "opening " << infile_crap << endl;

    	m = libvlc_media_new (inst, infile_crap, &ex); //TODO: read video file here
    	raise (&ex);
    }
    cout << "ok, done new media: " << m << endl;
    /* XXX: demo art and meta information fetching */

    /* Create a media player playing environement */
    mp = libvlc_media_player_new_from_media (m, &ex);
    raise (&ex);

    /* No need to keep the media now */
    libvlc_media_release (m);

    /* play the media_player */
    libvlc_media_player_play (mp, &ex);
    raise (&ex);
//    libvlc_media_player_pause(mp, &ex);
//    raise (&ex);

    libvlc_event_manager_t *em;
    em = libvlc_media_player_event_manager( mp, &ex );
    raise(&ex);

    libvlc_event_attach( em, libvlc_MediaPlayerPositionChanged,
                         callback, NULL, &ex );
    raise(&ex);

    cout << "About to save " << endl;

	libvlc_state_t state = libvlc_media_player_get_state(mp, &ex);
	cout << "Media player state: " << state << endl;

//	int voutYes = libvlc_media_player_has_vout(mp, &ex);
	cout << "VOut query result: " << libvlc_media_player_has_vout(mp, &ex) << endl;
	raise(&ex);
	//(  	libvlc_media_player_t *   	,
//			libvlc_exception_t *
//		)


//    VLC_PUBLIC_API void libvlc_media_player_set_time( libvlc_media_player_t *, libvlc_time_t, libvlc_exception_t *);
    for (int i = 0; i < framesToSave; i++)
    {
    	cout << i << endl;

    	libvlc_state_t state = libvlc_media_player_get_state(mp, &ex);
    	cout << "Media player state: " << state << endl;
    	cout << "VOut query result: " << libvlc_media_player_has_vout(mp, &ex) << endl;

    	string nameStr = "/tmp/snapshot.png";

    	{
			boost::unique_lock<boost::mutex> lock(mut);

			seek_ready = false;
			libvlc_media_player_set_time(mp, 1000 * (seek + (i * jump)), &ex);

			if (seek_ready == false)
			{
				cout << "Waiting ... " << endl;
				seekCondition.wait(lock);
				cout << "... end wait." << endl;
				cout << "Extra wait!" << endl;
				usleep(10000); // 100 is too little here // TODO: boost sync mechanism!
			}
    	}

    	state = libvlc_media_player_get_state(mp, &ex);
    	cout << "Media player state: " << state << endl;
    	cout << "VOut query result: " << libvlc_media_player_has_vout(mp, &ex) << endl;

    	// we halt here until the VLC thread calls the callback fn defined in libvlc_event_attach
    	// ... but then we automatically continue
    	//TODO: dimeout

    	// save image
    	if (seek_ready)
    	{
    		nameStr = (format("%2%.png") % outdirname % i).str();
			int width, height = 0;
			char outname[10240];
			strcpy(outname, nameStr.c_str());
			cout << "save " << outname << endl;
	    	state = libvlc_media_player_get_state(mp, &ex);
	    	cout << "Media player state: " << state << endl;
			libvlc_video_take_snapshot( mp, outname, width, height, &ex );
			cout << "save ? exception" << endl;
			raise(&ex);
			cout << "save done exception" << endl;
			cout << "Extra snapshot sleep!" << endl;
			usleep(400000); //TODO: boost sync mechanism!
				// usleep 10K fails all but 6
				// usleep 100K - misses 0! - gets us 70% (at 1000 seek wait)
				// usleep 100K - misses 0! - gets us 70% (at 10K seek wait)
				// usleep 200K - misses 0! - gets us 9/10 (at 10K seek wait)
				// usleep 400K 10/10 (at 10K seek wait)
    	} else {
    		cout << "Error, done waiting for VLC thread but seek not ready" << endl;
    	}
//        libvlc_media_player_pause(mp, &ex);
    }

    /* Stop playing */
    libvlc_media_player_stop (mp, &ex);

    /* Free the media_player */
    libvlc_media_player_release (mp);

    libvlc_release (inst);
    raise (&ex);

    return 0;
}
