#include "clay_libgit2.h"
#include <fileops.h>

static void
check_dirname(const char *A, const char *B)
{
	char dir[64], *dir2;

	must_be_true(git_path_dirname_r(dir, sizeof(dir), A) >= 0);
	must_be_true(strcmp(dir, B) == 0);
	must_be_true((dir2 = git_path_dirname(A)) != NULL);
	must_be_true(strcmp(dir2, B) == 0);

	free(dir2);
}

static void
check_basename(const char *A, const char *B)
{
	char base[64], *base2;

	must_be_true(git_path_basename_r(base, sizeof(base), A) >= 0);
	must_be_true(strcmp(base, B) == 0);
	must_be_true((base2 = git_path_basename(A)) != NULL);
	must_be_true(strcmp(base2, B) == 0);

	free(base2);
}

static void
check_topdir(const char *A, const char *B)
{
	const char *dir;

	must_be_true((dir = git_path_topdir(A)) != NULL);
	must_be_true(strcmp(dir, B) == 0);
}

static void
check_joinpath(const char *path_a, const char *path_b, const char *expected_path)
{
	char joined_path[GIT_PATH_MAX];

	git_path_join(joined_path, path_a, path_b);
	must_be_true(strcmp(joined_path, expected_path) == 0);
}

static void
check_joinpath_n(
	const char *path_a,
	const char *path_b,
	const char *path_c,
	const char *path_d,
	const char *expected_path)
{
	char joined_path[GIT_PATH_MAX];

	git_path_join_n(joined_path, 4, path_a, path_b, path_c, path_d);
	must_be_true(strcmp(joined_path, expected_path) == 0);
}


/* get the dirname of a path */
void test_path__0(void)
{

	check_dirname(NULL, ".");
	check_dirname("", ".");
	check_dirname("a", ".");
	check_dirname("/", "/");
	check_dirname("/usr", "/");
	check_dirname("/usr/", "/");
	check_dirname("/usr/lib", "/usr");
	check_dirname("/usr/lib/", "/usr");
	check_dirname("/usr/lib//", "/usr");
	check_dirname("usr/lib", "usr");
	check_dirname("usr/lib/", "usr");
	check_dirname("usr/lib//", "usr");
	check_dirname(".git/", ".");
}

/* get the base name of a path */
void test_path__1(void)
{
	check_basename(NULL, ".");
	check_basename("", ".");
	check_basename("a", "a");
	check_basename("/", "/");
	check_basename("/usr", "usr");
	check_basename("/usr/", "usr");
	check_basename("/usr/lib", "lib");
	check_basename("/usr/lib//", "lib");
	check_basename("usr/lib", "lib");
}

/* get the latest component in a path */
void test_path__2(void)
{
	check_topdir(".git/", ".git/");
	check_topdir("/.git/", ".git/");
	check_topdir("usr/local/.git/", ".git/");
	check_topdir("./.git/", ".git/");
	check_topdir("/usr/.git/", ".git/");
	check_topdir("/", "/");
	check_topdir("a/", "a/");

	must_be_true(git_path_topdir("/usr/.git") == NULL);
	must_be_true(git_path_topdir(".") == NULL);
	must_be_true(git_path_topdir("") == NULL);
	must_be_true(git_path_topdir("a") == NULL);
}

/* properly join path components */
void test_path__5(void)
{
	check_joinpath("", "", "");
	check_joinpath("", "a", "a");
	check_joinpath("", "/a", "/a");
	check_joinpath("a", "", "a/");
	check_joinpath("a", "/", "a/");
	check_joinpath("a", "b", "a/b");
	check_joinpath("/", "a", "/a");
	check_joinpath("/", "", "/");
	check_joinpath("/a", "/b", "/a/b");
	check_joinpath("/a", "/b/", "/a/b/");
	check_joinpath("/a/", "b/", "/a/b/");
	check_joinpath("/a/", "/b/", "/a/b/");
}

/* properly join path components for more than one path */
void test_path__6(void)
{
	check_joinpath_n("", "", "", "", "");
	check_joinpath_n("", "a", "", "", "a/");
	check_joinpath_n("a", "", "", "", "a/");
	check_joinpath_n("", "", "", "a", "a");
	check_joinpath_n("a", "b", "", "/c/d/", "a/b/c/d/");
	check_joinpath_n("a", "b", "", "/c/d", "a/b/c/d");
}