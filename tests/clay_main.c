
/*
 * Clay v0.3.0
 *
 * This is an autogenerated file. Do not modify.
 * To add new unit tests or suites, regenerate the whole
 * file with `./clay`
 */

#define clay_print(...) printf(__VA_ARGS__)

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* required for sandboxing */
#include <sys/stat.h>
#include <unistd.h>

#include "clay.h"

struct clay_error {
	const char *test;
	int test_number;
	const char *suite;
	const char *file;
	int line_number;
	const char *error_msg;
	char *description;

	struct clay_error *next;
};

static struct {
	const char *active_test;
	const char *active_suite;

	int suite_errors;
	int total_errors;

	int test_count;

	struct clay_error *errors;
	struct clay_error *last_error;

	void (*local_cleanup)(void *);
	void *local_cleanup_payload;

	jmp_buf trampoline;
	int trampoline_enabled;
} _clay;

struct clay_func {
	const char *name;
	void (*ptr)(void);
	size_t suite_n;
};

struct clay_suite {
	const char *name;
	struct clay_func initialize;
	struct clay_func cleanup;
	const struct clay_func *tests;
	size_t test_count;
};

/* From clay_sandbox.c */
static void clay_unsandbox(void);
static int clay_sandbox(void);

static void
clay_run_test(
	const struct clay_func *test,
	const struct clay_func *initialize,
	const struct clay_func *cleanup)
{
	int error_st = _clay.suite_errors;

	_clay.trampoline_enabled = 1;

	if (setjmp(_clay.trampoline) == 0) {
		if (initialize->ptr != NULL)
			initialize->ptr();

		test->ptr();
	}

	_clay.trampoline_enabled = 0;

	if (_clay.local_cleanup != NULL)
		_clay.local_cleanup(_clay.local_cleanup_payload);

	if (cleanup->ptr != NULL)
		cleanup->ptr();

	_clay.test_count++;

	/* remove any local-set cleanup methods */
	_clay.local_cleanup = NULL;
	_clay.local_cleanup_payload = NULL;

	clay_print("%c", (_clay.suite_errors > error_st) ? 'F' : '.');
}

static void
clay_print_error(int num, const struct clay_error *error)
{
	clay_print("  %d) Failure:\n", num);

	clay_print("%s::%s (%s) [%s:%d] [-t%d]\n",
		error->suite,
		error->test,
		"no description",
		error->file,
		error->line_number,
		error->test_number);

	clay_print("  %s\n", error->error_msg);

	if (error->description != NULL)
		clay_print("  %s\n", error->description);

	clay_print("\n");
}

static void
clay_report_errors(void)
{
	int i = 1;
	struct clay_error *error, *next;

	error = _clay.errors;
	while (error != NULL) {
		next = error->next;
		clay_print_error(i++, error);
		free(error->description);
		free(error);
		error = next;
	}
}

static void
clay_run_suite(const struct clay_suite *suite)
{
	const struct clay_func *test = suite->tests;
	size_t i;

	_clay.active_suite = suite->name;
	_clay.suite_errors = 0;

	for (i = 0; i < suite->test_count; ++i) {
		_clay.active_test = test[i].name;
		clay_run_test(&test[i], &suite->initialize, &suite->cleanup);
	}
}

static void
clay_run_single(const struct clay_func *test,
	const struct clay_suite *suite)
{
	_clay.suite_errors = 0;
	_clay.active_suite = suite->name;
	_clay.active_test = test->name;

	clay_run_test(test, &suite->initialize, &suite->cleanup);
}

static int
clay_usage(void)
{
	exit(-1);
}

static void
clay_parse_args(
	int argc, char **argv,
	const struct clay_func *callbacks,
	size_t cb_count,
	const struct clay_suite *suites,
	size_t suite_count)
{
	int i;

	for (i = 0; i < argc; ++i) {
		char *argument = argv[i];
		char action;
		int num;

		if (argument[0] != '-')
			clay_usage();

		action = argument[1];
		num = strtol(argument + 2, &argument, 10);

		if (*argument != '\0' || num < 0)
			clay_usage();

		switch (action) {
		case 't':
			if ((size_t)num >= cb_count) {
				fprintf(stderr, "Test number %d does not exist.\n", num);
				exit(-1);
			}

			clay_print("Started (%s::%s)\n",
				suites[callbacks[num].suite_n].name,
				callbacks[num].name);

			clay_run_single(&callbacks[num], &suites[callbacks[num].suite_n]);
			break;

		case 's':
			if ((size_t)num >= suite_count) {
				fprintf(stderr, "Suite number %d does not exist.\n", num);
				exit(-1);
			}

			clay_print("Started (%s::)\n", suites[num].name);
			clay_run_suite(&suites[num]);
			break;

		default:
			clay_usage();
		}
	}
}

static int
clay_test(
	int argc, char **argv,
	const char *suites_str,
	const struct clay_func *callbacks,
	size_t cb_count,
	const struct clay_suite *suites,
	size_t suite_count)
{
	clay_print("Loaded %d suites: %s\n", (int)suite_count, suites_str);

	if (!clay_sandbox())
		return -1;

	if (argc > 1) {
		clay_parse_args(argc - 1, argv + 1,
			callbacks, cb_count, suites, suite_count);

	} else {
		size_t i;
		clay_print("Started\n");

		for (i = 0; i < suite_count; ++i) {
			const struct clay_suite *s = &suites[i];
			clay_run_suite(s);
		}
	}

	clay_print("\n\n");
	clay_report_errors();

	clay_unsandbox();
	return _clay.total_errors;
}

void
clay__assert(
	int condition,
	const char *file,
	int line,
	const char *error_msg,
	const char *description,
	int should_abort)
{
	struct clay_error *error;

	if (condition)
		return;

	error = calloc(1, sizeof(struct clay_error));

	if (_clay.errors == NULL)
		_clay.errors = error;

	if (_clay.last_error != NULL)
		_clay.last_error->next = error;

	_clay.last_error = error;

	error->test = _clay.active_test;
	error->test_number = _clay.test_count;
	error->suite = _clay.active_suite;
	error->file = file;
	error->line_number = line;
	error->error_msg = error_msg;

	if (description != NULL)
		error->description = strdup(description);

	_clay.suite_errors++;
	_clay.total_errors++;

	if (should_abort) {
		if (!_clay.trampoline_enabled) {
			fprintf(stderr,
				"Unhandled exception: a cleanup method raised an exception.");
			exit(-1);
		}

		longjmp(_clay.trampoline, -1);
	}
}

void clay_set_cleanup(void (*cleanup)(void *), void *opaque)
{
	_clay.local_cleanup = cleanup;
	_clay.local_cleanup_payload = opaque;
}

#ifdef _WIN32
#	define PLATFORM_SEP '\\'
#else
#	define PLATFORM_SEP '/'
#endif

static char _clay_path[4096];

static int
is_valid_tmp_path(const char *path)
{
	struct stat st;
	return (lstat(path, &st) == 0 &&
		(S_ISDIR(st.st_mode) ||
		S_ISLNK(st.st_mode)) &&
		access(path, W_OK) == 0);
}

static int
find_tmp_path(char *buffer, size_t length)
{
	static const size_t var_count = 4;
	static const char *env_vars[] = {
		"TMPDIR", "TMP", "TEMP", "USERPROFILE"
 	};

 	size_t i;

#ifdef _WIN32
	if (GetTempPath((DWORD)length, buffer))
		return 1;
#endif

	for (i = 0; i < var_count; ++i) {
		const char *env = getenv(env_vars[i]);
		if (!env)
			continue;

		if (is_valid_tmp_path(env)) {
			strncpy(buffer, env, length);
			return 1;
		}
	}

	return 0;
}

static int clean_folder(const char *path)
{
	const char os_cmd[] =
#ifdef _WIN32
		"rd /s /q \"%s\"";
#else
		"rm -rf \"%s\"";
#endif

	char command[4096];
	snprintf(command, sizeof(command), os_cmd, path);
	return system(command);
}

static void clay_unsandbox(void)
{
	clean_folder(_clay_path);
}

static int clay_sandbox(void)
{
	const char path_tail[] = "clay_tmp_XXXXXX";
	size_t len;

	if (!find_tmp_path(_clay_path, sizeof(_clay_path)))
		return 0;

	len = strlen(_clay_path);

	if (_clay_path[len - 1] != PLATFORM_SEP) {
		_clay_path[len++] = PLATFORM_SEP;
	}

	strcpy(_clay_path + len, path_tail);

	if (mktemp(_clay_path) == NULL)
		return 0;

	if (mkdir(_clay_path, 0700) != 0)
		return 0;

	if (chdir(_clay_path) != 0)
		return 0;

	return 1;
}



extern void test_dirent__dont_traverse_dot(void);
extern void test_dirent__traverse_subfolder(void);
extern void test_dirent__traverse_slash_terminated_folder(void);
extern void test_dirent__dont_traverse_empty_folders(void);
extern void test_dirent__traverse_weird_filenames(void);
extern void test_filebuf__0(void);
extern void test_filebuf__1(void);
extern void test_filebuf__2(void);
extern void test_path__0(void);
extern void test_path__1(void);
extern void test_path__2(void);
extern void test_path__5(void);
extern void test_path__6(void);
extern void test_rmdir__initialize();
extern void test_rmdir__delete_recursive(void);
extern void test_rmdir__fail_to_delete_non_empty_dir(void);
extern void test_string__0(void);
extern void test_string__1(void);
extern void test_vector__0(void);
extern void test_vector__1(void);
extern void test_vector__2(void);

static const struct clay_func _all_callbacks[] = {
    {"dont_traverse_dot", &test_dirent__dont_traverse_dot, 0},
	{"traverse_subfolder", &test_dirent__traverse_subfolder, 0},
	{"traverse_slash_terminated_folder", &test_dirent__traverse_slash_terminated_folder, 0},
	{"dont_traverse_empty_folders", &test_dirent__dont_traverse_empty_folders, 0},
	{"traverse_weird_filenames", &test_dirent__traverse_weird_filenames, 0},
	{"0", &test_filebuf__0, 1},
	{"1", &test_filebuf__1, 1},
	{"2", &test_filebuf__2, 1},
	{"0", &test_path__0, 2},
	{"1", &test_path__1, 2},
	{"2", &test_path__2, 2},
	{"5", &test_path__5, 2},
	{"6", &test_path__6, 2},
	{"delete_recursive", &test_rmdir__delete_recursive, 3},
	{"fail_to_delete_non_empty_dir", &test_rmdir__fail_to_delete_non_empty_dir, 3},
	{"0", &test_string__0, 4},
	{"1", &test_string__1, 4},
	{"0", &test_vector__0, 5},
	{"1", &test_vector__1, 5},
	{"2", &test_vector__2, 5}
};

static const struct clay_suite _all_suites[] = {
    {
        "dirent",
        {NULL, NULL, 0},
        {NULL, NULL, 0},
        &_all_callbacks[0], 5
    },
	{
        "filebuf",
        {NULL, NULL, 0},
        {NULL, NULL, 0},
        &_all_callbacks[5], 3
    },
	{
        "path",
        {NULL, NULL, 0},
        {NULL, NULL, 0},
        &_all_callbacks[8], 5
    },
	{
        "rmdir",
        {"initialize", &test_rmdir__initialize, 3},
        {NULL, NULL, 0},
        &_all_callbacks[13], 2
    },
	{
        "string",
        {NULL, NULL, 0},
        {NULL, NULL, 0},
        &_all_callbacks[15], 2
    },
	{
        "vector",
        {NULL, NULL, 0},
        {NULL, NULL, 0},
        &_all_callbacks[17], 3
    }
};

static const char _suites_str[] = "dirent, filebuf, path, rmdir, string, vector";

int main(int argc, char *argv[])
{
    return clay_test(
        argc, argv, _suites_str,
        _all_callbacks, 20,
        _all_suites, 6
    );
}
