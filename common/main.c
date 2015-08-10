#define _GNU_SOURCE

#include <getopt.h>
#include <limits.h>
#include <sched.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "loader.h"
#include "parse.h"
#include "rwmsr.h"


#define PATH_ENV  "MSR_PATH"


static const char     *options_string = "hVvs:p:c:";
static struct option   options[] = {
	{"help",    no_argument,       0, 'h'},
	{"version", no_argument,       0, 'V'},
	{"verbose", no_argument,       0, 'v'},
	{"system",  required_argument, 0, 's'},
	{"path",    required_argument, 0, 'p'},
	{"cores",   required_argument, 0, 'c'},
	{ NULL,     0,                 0,  0 }
};

const char     *program = NULL;
const char     *module  = NULL;
uint8_t         verbose = 0;

static const char     *sysname = NULL;

static const char    **paths;
static const char     *paths_default[] = { ".", "/usr/lib/rwmsr" };
static size_t          paths_size;

static uint8_t        *cores;
static size_t          cores_size;

static struct command *commands;
static size_t          commands_size;
static size_t          commands_count;

static uint8_t        *engine_cores;
static size_t          engine_cores_size;


static void usage(void)
{
	printf("Usage: rwmsr [-h | --help] [-V | --version]\n"
	       "       rwmsr [-v] [-s <system>] [-p <paths>] [-c <cores>] "
	       "<commands...>\n"
	       "Read and write Machine Specific Registers.\n"
	       "Allow the user to read and write MSRs instantly or "
	       "perdiodically throught a set\n"
	       "of commands. Each command is in the following form:\n"
	       "\n"
	       "  commands ::= [':'[':']] <address> ['=' <value] ['@' <delay> "
	       "['-' <repeat>]]\n"
	       "\n");
	printf("The <address> is the MSR address and the optional <value> is "
	       "what to write in\n"
	       "the MSR if specified. For more informations about MSR "
	       "addresses and values they\n"
	       "can contain, please refer to the constructor documentation.\n"
	       "Both <address> and <value> can be specified either in decimal "
	       "form, or in\n");
	printf("hexadecimal form. In the last case, they should start with "
	       "'0x' or 'x'.\n"
	       "\n"
	       "The leading ':' or '::' optional characters indicate if the "
	       "value of the\n"
	       "register should be printed. ':' indicates to print it in "
	       "decimal form whereas\n"
	       "'::' indicates hexadecimal form is required.\n"
	       "\n");
	printf("The optional <delay> value is an amount of millisecond to "
	       "wait before to\n"
	       "actually execute the command. This can be usefull for MSR "
	       "multiplexing.\n"
	       "Finally the <repeat> value is another amout of millisecond to "
	       "wait between to\n"
	       "execute the command again. When this value is specified, the "
	       "command is\n"
	       "executed periodically until the user kills the program.\n"
	       "\n"
	       "\n");
	printf("By default, the MSR of the current core are used. This "
	       "behavior can be changed\n"
	       "with the '-c' (or '--cores') option. It indicates the set of "
	       "core on which to\n"
	       "execute the commands. The option value takes the following "
	       "form:\n"
	       "\n"
	       "  cores ::= <id> [',' <cores>]\n"
	       "            <id> '-' <id> [',' <cores>]\n"
	       "\n");
	printf("An <id> is simply the core number as specified by the "
	       "underlying system.\n"
	       "Multiple ids can be specified and separated by commas, and "
	       "ranges of ids can\n"
	       "be specified with hyphens. Additionally, the '--cores' "
	       "options can be specified\n"
	       "several times.\n"
	       "\n"
	       "\n");
	printf("This program can run on multiple systems. Currently, it can "
	       "work under\n"
	       "bare-metal GNU/Linux (codename 'linux'), or under Xen "
	       "vritualized GNU/Linux\n"
	       "(codename 'xen'). The program should be able to detect "
	       "automatically which is\n"
	       "the current system, but user can override this with the '-s' "
	       "(or '--system')\n"
	       "option.\n"
	       "\n");
	printf("Once the system has been detected (or provided), the program "
	       "searches an\n"
	       "appropriate module to communicate with the system. For this, "
	       "it tries all files\n"
	       "in an internal path list. By default, this list only contains "
	       "the '.' path.\n"
	       "More paths can be specified either with the '-p' "
	       "(or '--path') option, or with\n"
	       "the RWMSR_PATH environment variable.\n");
}

static void version(void)
{
	printf("rwmsr 1.0\nGauthier Voron\ngauthier.voron@lip6.fr\n");
}


void error(const char *format, ...)
{
	va_list ap;

	if (format != NULL) {
		if (module)
			fprintf(stderr, "%s: ", module);
		else
			fprintf(stderr, "%s: ", program);
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
		fprintf(stderr, "\n");
	}

	fprintf(stderr, "please type '%s --help' for more informations\n",
		program);
	exit(EXIT_FAILURE);
}

void vlog(const char *format, ...)
{
	va_list ap;

	if (module)
		fprintf(stderr, "%s: ", module);
	else
		fprintf(stderr, "%s: ", program);
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}


static int parse_early_options(int argc, char *const *argv)
{
	int c;
	size_t i;
	int defined_sysname = 0;
	size_t paths_default_size = sizeof (paths_default) / sizeof (char *);
	char *env;
	
	paths = malloc((paths_default_size + argc + 1) * sizeof (char *));
	paths_size = 0;

	for (i=0; i<paths_default_size; i++)
		paths[paths_size++] = paths_default[i];
	
	env = getenv(PATH_ENV);
	if (env)
		paths[paths_size++] = env;

	while (1) {
		c = getopt_long(argc, argv, options_string, options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'V':
			version();
			exit(EXIT_SUCCESS);
		case 'v':
			verbose = 1;
			break;
		case 's':
			if (defined_sysname++)
				error("already defined system: '%s'", sysname);
			sysname = optarg;
			break;
		case 'p':
			paths[paths_size++] = optarg;
			break;
			
		case 'c':
			break;

		default:
			error(NULL);
		}
	}

	optind = 0;

	return 0;
}

static int parse_late_options(int argc, char *const *argv)
{
	int c;
	size_t i;
	const char *err;

	while (1) {
		c = getopt_long(argc, argv, options_string, options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'c':
			if (!strcmp(optarg, "all")) {
				for (i=0; i<cores_size; i++)
					cores[i] = 1;
				break;
			}
			err = parse_cores(cores, cores_size, optarg);
			if (err)
				error("invalid core parameter: '%s'", optarg);
			break;

		case 'h':
		case 'V':
		case 'v':
		case 's':
		case 'p':
			break;

		default:
			error(NULL);
		}
	}

	return optind;
}


static void setup_early_config(void)
{
	int8_t ret;
	size_t maxid;
	char modpath[PATH_MAX];
	
	if (!sysname) {
		sysname = probe_system();
		if (!sysname)
			error("cannot find system type");
		if (verbose)
			vlog("found system type: '%s'", sysname);
	} else if (verbose) {
		vlog("provided system type: '%s'", sysname);
	}

	ret = load_module(sysname, paths, paths_size, modpath,
			  sizeof (modpath));
	if (ret)
		error("cannot find module for system type: '%s'", sysname);
	if (verbose)
		vlog("found module: '%s'", modpath);

	ret = coreinfo(&cores_size, &maxid);
	if (ret)
		error("cannot find core infos with '%s'", modpath);
	if (verbose)
		vlog("found %lu cores with max id %lu", cores_size, maxid);

	if (maxid != cores_size - 1)
		error("disjoint core ids not yet implemented");
	
	cores = calloc(cores_size, sizeof (sizeof(uint8_t)));
}

static void setup_late_config(void)
{
	size_t i;
	
	engine_cores_size = 0;
	for (i=0; i<cores_size; i++)
		if (cores[i])
			engine_cores_size++;

	if (engine_cores_size == 0) {
		cores[sched_getcpu()] = 1;
		engine_cores_size = 1;
	}

	engine_cores = malloc(engine_cores_size * sizeof (uint8_t));
	engine_cores_size = 0;
	for (i=0; i<cores_size; i++)
		if (cores[i])
			engine_cores[engine_cores_size++] = i;
}

int main(int argc, char *const *argv)
{
	int i, tmp;
	const char *err;

	program = argv[0];
	
	tmp = parse_early_options(argc, argv);
	argc -= tmp;
	argv += tmp;

	setup_early_config();

	tmp = parse_late_options(argc, argv);
	argc -= tmp;
	argv += tmp;

	commands_count = 0;
	commands_size = (size_t) argc;
	commands = alloca(commands_size * sizeof (struct command));
	for (i=0; i<argc; i++) {
		err = parse_command(&commands[i], argv[i]);
		if (err)
			error("command sytax error: '%s'", err);
		commands_count++;
	}

	setup_late_config();

	execute(commands, commands_count, engine_cores, engine_cores_size);

	free(paths);
	free(cores);
	free(engine_cores);

	destroy();
	unload_module();
	
	return EXIT_SUCCESS;
}
