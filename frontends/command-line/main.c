#include <stdlib.h>
#include <stdio.h>
#include <gphoto2.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"
#include "test.h"
#include "interface.h"

void usage();
int  verify_options (int argc, char **argv);
int  option_count;
void debug_print (char *message, char *str);

CameraInit init;

/* Command-line option
   -----------------------------------------------------------------------
   this is funky and may look a little scary, but sounded cool to do. 
   it makes sense since this is just a wrapper for a functional/flow-based 
   library.

   When the program starts it calls (in order):
	1) verify_options() to make sure all options are valid,
	   and that any options needing an argument have one (or if they
	   need an argument, they don't have one).
	2) (optional) option_is_present() to see if a particular option was
	   typed in the command-line. This can be used to set up something
	   before the next step.
	3) execute_options() to actually parse the command-line and execute
	   the command-line options in order.

   This might sound a little complex, but makes adding options REALLY easy,
   and it is very flexible.

   How to add a command-line option:

/* 1) Add a forward-declaration here 					*/
/*    ----------------------------------------------------------------- */
/*    Use the OPTION_CALLBACK(function) macro.				*/

OPTION_CALLBACK(help);
OPTION_CALLBACK(test);
OPTION_CALLBACK(list);
OPTION_CALLBACK(filename);
OPTION_CALLBACK(port);
OPTION_CALLBACK(model);
OPTION_CALLBACK(quiet);
OPTION_CALLBACK(debug);
OPTION_CALLBACK(get_picture);
OPTION_CALLBACK(get_thumbnail);

/* 2) Add an entry in the option table 				*/
/*    ----------------------------------------------------------------- */
/*    Format for option is:						*/
/*     {"short", "long", "argument", "description", callback_function}, */
/*    if it is just a flag, set the argument to "".			*/
/*    Order is important! Options are exec'd in the order in the table! */

Option option[] = {

/* Settings 			*/
{"" , "port",		"<port>",	"Specify port device to use",	port},
{"" , "camera",		"<model>",	"Specify camera model",		model},
{"d", "debug",		"",		"Turn on debugging",		debug},
{"f", "filename",	"<filename>",	"Specify a filename",		filename},
{"q", "quiet",		"",		"Quiet output",			quiet},

/* Display and die functions 	*/
{"",  "test",		"",		"Verifies gPhoto installation",	test},
{"h", "help",		"",		"Displays this help screen",	help},
{"l", "list", 		"",		"List supported cameras",	list},

/* Actions (Depend on settings) */
{"p", "get-picture",	"#", 		"Get picture # from camera", 	get_picture},
{"t", "get-thumbnail",	"#", 		"Get thumbnail # from camera",	get_thumbnail},

/* End of list 			*/
{"" , "", 		"",		"",				NULL}
};

/* 3) Add any necessary global variables				*/
/*    ----------------------------------------------------------------- */
/*    Most flags will set options, like choosing a port, camera model,  */
/*    etc...								*/

int  glob_debug;
int  glob_quiet=0;
int  glob_filename_override=0;
char glob_filename[128];
char glob_port[128];
char glob_model[64];


/* 4) Finally, add your callback function.				*/
/*    ----------------------------------------------------------------- */
/*    The callback function is passed "char *arg" to the argument of    */
/*    command-line option. It must return GP_OK or GP_ERROR.		*/
/*    Again, use the OPTION_CALLBACK(function) macro.			*/
/*    glob_debug is set if the user types in the "-d/--debug" flag. Use */
/*    debug_print(char *message, char *arg) to display debug output.    */

OPTION_CALLBACK(help) {

	if (glob_debug)
		debug_print("Displaying usage", "");

	usage();
	exit(EXIT_SUCCESS);
}

OPTION_CALLBACK(test) {

	if (glob_debug)
		debug_print("Testing gPhoto Installation", "");

	test_gphoto();
	exit(EXIT_SUCCESS);
}

OPTION_CALLBACK(list) {

	int x, n;
	char buf[64];

	if (glob_debug)
		debug_print("Listing Cameras", "");

	if ((n = gp_camera_count())==GP_ERROR)
                printf("\nERROR: can't retrieve the number of supported cameras!\n\n");
	if (glob_quiet)
		printf("%i\n", n);
	   else
	        printf("Number of supported cameras: %i\nSupported cameras:\n", n);

        for (x=0; x<n; x++) {
                if (gp_camera_name(x, buf)==GP_ERROR)
                        printf("\nERROR: can't retrieve the name of camera #%i\n\n", x);
		if (glob_quiet)
			printf("%s\n", buf);
		   else
	                printf("\t\"%s\"\n", buf);
        }

	return (GP_OK);
}

OPTION_CALLBACK(filename) {

	if (glob_debug)
		debug_print("Setting filename", arg);

	strcpy(glob_filename, arg);

	return (GP_OK);
}

OPTION_CALLBACK(port) {

	if (glob_debug)
		debug_print("Setting port", arg);

	strcpy(init.port_settings.serial_port, "/dev/ttyS0");
        init.port_settings.serial_baud = 57600;

	strcpy(glob_port, arg);

	return (GP_OK);
}

OPTION_CALLBACK(model) {

	if (glob_debug)
		debug_print("Setting camera model", arg);

	strcpy(glob_model, arg);

	return (GP_OK);
}

OPTION_CALLBACK(debug) {

	glob_debug = 1;
	debug_print("Turning on debug mode", "");

	return (GP_OK);
}

OPTION_CALLBACK(quiet) {

	if (glob_debug)
		debug_print("Setting to quiet mode", "");
	glob_quiet=1;

	return (GP_OK);
}

OPTION_CALLBACK(get_picture) {

	CameraFile *f;

	if (glob_debug)
		debug_print("Getting picture", arg);

	/* gp_file_get(atoi(arg), f);	*/
	/* if glob_filename_override==1 */
	/* 	save as glob_filename   */
	/*   else			*/
	/*      save as file->filename  */

	return (GP_OK);
}

OPTION_CALLBACK(get_thumbnail) {

	if (glob_debug)
		debug_print("Getting picture thumbnail", arg);

	/* gp_file_get_preview(atoi(arg), f);	*/
	/* if glob_filename_override==1 	*/
	/* 	save as glob_filename   	*/
	/*   else				*/
	/*      save as file->filename  	*/

	return (GP_OK);
}


/* Command-line option functions					*/
/* ------------------------------------------------------------------	*/

int option_is_present (char *op, int argc, char **argv) {
	/* checks to see if op is in the command-line. it will */
	/* check for both short and long option-formats for op */

	int x;
	char s[5], l[20];

	sprintf(s, "%s%s", SHORT_OPTION, op);
	sprintf(l, "%s%s", LONG_OPTION, op);
	for (x=1; x<argc; x++)
		if ((strcmp(s, argv[x])==0)||(strcmp(l, argv[x])==0))
			return (GP_OK);

	return (GP_ERROR);
}

int verify_options (int argc, char **argv) {
	/* This function makes sure that all command-line options are
	   valid and have the correct number of arguments */

	int x, y, match, missing_arg, which;
	char s[5], l[24];

	which = 0;
	strcpy(init.port_settings.serial_port, "/dev/ttyS0");
        init.port_settings.serial_baud = 57600;

	for (x=1; x<argc; x++) {
		if (glob_debug) 
			debug_print("checking \"%s\": \n", argv[x]);
		match = 0;
		missing_arg = 0;
		for (y=0; y<option_count; y++) {
			/* Check to see if the option matches */
			sprintf(s, "%s%s", SHORT_OPTION, option[y].short_id);
			sprintf(l, "%s%s", LONG_OPTION, option[y].long_id);
			if ((strcmp(s, argv[x])==0)||(strcmp(l, argv[x])==0)) {
				/* Check to see if the option requires an argument */
				if (strlen(option[y].argument)>0) {
					if (x+1 < argc) {
					   if (
				(strncmp(argv[x+1], SHORT_OPTION, strlen(SHORT_OPTION))!=0) &&
				(strncmp(argv[x+1], LONG_OPTION, strlen(LONG_OPTION))!=0)
					      ) {
						match=1;
						x++;
					   } else {
						which=y;
						missing_arg=1;
					   }
					} else {
					   missing_arg=1;
					   which=y;
					}
				}  else
					match=1;
			}
		}
		if (!match) {
			printf("\n** Bad option \"%s\": ", argv[x]);
			if (missing_arg)
				printf("\n\tMissing argument. You must specify \"%s\"",
					option[which].argument);
			    else
				printf("unknown option");
			printf(". **\n\n");
			return (GP_ERROR);
		}
	}
	return (GP_OK);
}

int execute_options (int argc, char **argv) {

	int x, y, ret;
	char s[5], l[24];

	/* Execute the command-line options */
	for (x=0; x<option_count; x++) {
		sprintf(s, "%s%s", SHORT_OPTION, option[x].short_id);
		sprintf(l, "%s%s", LONG_OPTION, option[x].long_id);
		for (y=1; y<argc; y++) {
			if ((strcmp(argv[y],s)==0)||(strcmp(argv[y],l)==0)) {
				if (option[x].execute) {
				   if (strlen(option[x].argument) > 0) {
					ret=(*option[x].execute)(argv[++y]);
				   }  else
					ret=(*option[x].execute)(NULL);
				   if (ret != GP_OK) {
					printf("\nERROR: option \"%s\" did not execute properly.\n\n",
						argv[y]);
					return (GP_ERROR);
				   }
				}
			}
		}
	}

	return (GP_OK);
}

void usage () {

	int x=0, y=0;
	char buf[128], s[5], l[24], a[16];

	/* Standard licensing stuff */
	printf(
	"gPhoto2 (v%s)- Cross-platform digital camera library.\n"
	"Copyright (C) 2000 Scott Fritzinger\n"
	"gPhoto is licensed under the Lesser GNU Public License (LGPL).\n"
	"Usage:\n", VERSION
	);

	/* Run through option and print them out */
	while (x < option_count) {
		/* maybe sort these by short option? can't be an in-place sort.
		   would need to memcpy() to a new struct array. */
		if (strlen(option[x].short_id) > 0)
			sprintf(s, "%s%s,", SHORT_OPTION, option[x].short_id);
		   else
			sprintf(s, " ");

		if (strlen(option[x].long_id) > 0)
			sprintf(l, "%s%s", LONG_OPTION, option[x].long_id);
		   else
			sprintf(l, " ");

		if (strlen(option[x].argument) > 0)
			sprintf(a, "%s", option[x].argument);
		   else
			sprintf(a, " ");
		sprintf(buf, " %-04s %s %s", s, l, a);
		printf("%-38s %s\n", buf, option[x].description);
		x++;
	}
}

/* Misc functions							*/
/* ------------------------------------------------------------------	*/

void debug_print(char *message, char *str) {

	printf("cli: %s: %s\n", message, str);
}

/* ------------------------------------------------------------------	*/

int main (int argc, char **argv) {

	/* Count the number of command-line options we have */
	option_count = 0;
	while ((strlen(option[option_count].short_id)>0) ||
	       (strlen(option[option_count].long_id)>0)) {
		option_count++;
	}

	/* Make sure we were called correctly */
	if ((argc == 1)||(verify_options(argc, argv)==GP_ERROR)) {
		usage();
		exit(EXIT_FAILURE);
	}

	/* Check to see if we need to turn on debugging output */
	if (option_is_present("d", argc, argv)==GP_OK) {
		gp_debug_set(1);
		glob_debug=1;
	}

	/* Initialize gPhoto core */
	gp_init();

	if (execute_options(argc, argv) == GP_ERROR) {
		usage();
		exit(EXIT_FAILURE);
	}

	/* Exit gPhoto core */
	gp_exit();

	return (EXIT_SUCCESS);
}
