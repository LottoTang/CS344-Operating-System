//CS344 Assignment 2 TREE - Long To Lotto Tang (934382419)

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include <cs344/libtree_testbench.h> /* You should read this header file!
                            * Location: /class/local/include/cs344/libtree_testbench.h
                            */
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>

/***************************************************************************************************
 * Let's declare some file-scope variables and macros. Everything here is not externally visible,
 * and pertains only to this file. Macro definitions only affect the files they appear in, and the
 * static specifier indicates file-scope (not externally linked). Since everything here is local to
 * this file, you can change it, remove it, etc. It's *your* implementation, do what *you* want!
 *
 * Only extern symbols enter the global namespace and are "imported" by the linker, which matters
 * when you're working with multiple source files/libraries/etc. as we are here. We want some things
 * like print_tree() to be external, so that the libtree library can access it, but pretty much
 * everything else is part of the print_tree() implementation and should be file-scoped.
 */

/* This is an example of conditional inclusion using preprocessor directives. Here we define a
 * function-like macro: pdebug(...), which calls fprintf if the DEBUG macro is set; otherwise it
 * does nothing. The `gcc` compiler accepts arguments of the form -D MACRO_NAME[=value] to set a
 * macro at compilation time, which can be used to turn debugging output on or off.
 *
 * Notice the space between "#" and "define". The '#' must appear in column 0, but there can be
 * whitespace between it and the directive. This is typically used to visually indicate nesting
 * levels inside #if blocks
 *
 * Example: gcc -o tree -DDEBUG tree.c -ltree
 *
 * Important note:
 *     Be careful about inline comments and the #define directive! They will become part of the
 *     macro! The preproccessor does not know or care what C is! It follows simple, but sometimes
 *     confusing, macro replacement rules.
 */
#ifdef DEBUG
# define pdebug(...) fprintf(stderr, __VA_ARGS__)
#else
# define pdebug(...) ((void) 0)
#endif

/* A little helper macro to convert a bool value to a string */
#define boolstr(b) (b ? "true" : "false")

/* Best practice: Avoid magic numbers in code! Define a macro or const value instead */
#define INDENT_WIDTH 2

/* Let's write a helper function to print out our files and their attributes */
static void print_path_info(char const *, struct stat);

/* We need another helper function to use with qsort(3) as its compar function filesort needs the
 * filename and the file stats (for sorting by time), so let's make a struct for that purpose. */
struct fileinfo {
  char *name;
  struct stat st;
};
static int filesort(void const *, void const *);

/* END of local declarations
**************************************************************************************************
     *
 * print_tree() - Recursively print subdirectory tree of given path
 * @path: The path of the starting directory
 *
 * print_tree() prints the contents of the given directory at @path, recursing through each
 * subdirectory to print its contents as well. Each level of depth is indented by two spaces.
 *
 * This function will be the entry point of your program. The libtree library defines its own main()
 * function which parses arguments on the command-line and then calls into your print_tree()
 * function with a given path. When you link against the libtree library (with gcc ... -ltree), the
 * symbols provided in the library become available, and you end up with an executable program.
 *
 * Notes:
 *
 * If there were no errors, print_tree() returns 0. If there is an internal error, print_tree()
 * should immediately return a non-zero value *without* printing anything about the error. It is ok
 * if print_tree() has already produced some output before the error occurred.
 *
 * print_tree() may change the current working directory; the caller is responsible for restoring it
 * if necessary.
 *
 * Writes to stdout and stderr may be assumed to always succeed and do not need to be checked for
 * errors.
 *
 * Failure to open the specified directory or subdirectory at @path should not be treated as an
 * internal error. Instead, print_tree will print an informative message as part of its regular
 * output (on stdout). See the example code below.
 *
 * Most of this program has been completed for you!
 */
extern int
print_tree(char const *path)
{
  /* Let's print some informative debugging info */
  pdebug("%s(\"%s\") called!\n", __func__, path);

  /* opts is declared in cs344/libtree_testbench.h and pre-populated by libtree's main() function */
  pdebug("*opts = {\n");
  pdebug("  .all      = %5s, /* print hidden '.' files */\n", boolstr(opts->all));
  pdebug("  .dirsonly = %5s, /* list directories only */\n", boolstr(opts->dirsonly));
  pdebug("  .perms    = %5s, /* print file type and permissions */ \n", boolstr(opts->perms));
  pdebug("  .user     = %5s, /* print the username of the file */\n", boolstr(opts->user));
  pdebug("  .group    = %5s, /* print the group name of file */\n", boolstr(opts->group));
  pdebug("  .size     = %5s, /* print file size in bytes */\n", boolstr(opts->size));
  pdebug("  .sort     = %5s, /* sorting method to use */\n",
      (char *[]){"NONE", "ALPHA", "RALPHA", "TIME"}[opts->sort]);
  pdebug("}\n\n");

  static int indent_level = 0; /*  Note, this is evaluated at program initialization, not each time
                                   this function is run; indent_level keeps its value between
                                   invocations of this function. This can be nice for recursion :)
                                   */

  struct fileinfo *file_list = NULL;
  struct stat st;
  size_t nfiles = 0;
  DIR *d = NULL;
  int dfd = -1;
  int ret = 0;

  /* Get stats for the given path, then print it */
  if (lstat(path, &st) == -1) {
    pdebug("lstat(\"%s\") failed\n", path);
    ret = -1;
    goto exit;
  }
  /* FIXME: Print indent spaces before printing the path info */
  // for each level, print ("  ") at the beginning for identation
  for (int i = 0; i < indent_level; i++) {
    printf("  ");
  }

  print_path_info(path, st);

  /*  FIXME: If path is not a directory, we should not try to open it... right?  */
  //only directory will be opened, not for files
  if (!S_ISDIR(st.st_mode)) {
      printf("\n");
      goto exit;
  }

  /*   Here we use a technique called short-circuit evaluation to call opendir() if and only if the
   *   chdir() succeeds; the contents of the if block are executed if either operation fails. */

  if (chdir(path) == -1 || (d = opendir(".")) == NULL) {
    printf(" [could not open directory %s]\n", path); /*  NOT an *internal* error! */
    goto exit;
  }

  /*  We need to get the fd of the directory we just opened so we can use fchdir() later to return
   *  here (after returning from recursion). */
  dfd = dirfd(d);
  if (dfd == -1) {
    pdebug("dirfd(d) failed\n");
    ret = -1;
    goto exit;
  }

  printf ("\n");

  for (;;) {
    errno = 0;
    struct dirent *de = readdir(d);
    if (de == NULL) {
      if (errno) {
        pdebug("de is NULL\n");
        ret = -1;
        goto exit;
      }
      else break; /* end of directory */
    } else if (strcoll(de->d_name, ".") == 0 || strcoll(de->d_name, "..") == 0) {
      continue; /* Skip the "." and ".." subdirectories */
    }

    /* FIXME: might be a good spot to think about hidden files */

   // if no -a argument passed, hidden files will not be displayed 
    if (!opts->all) {
        if(de->d_name[0] == '.')
            continue;
    }

    // if -d argument passed, check the current file to see if it is a valid directory
    if (opts->dirsonly) {
        lstat(de->d_name, &st);
        if(!S_ISDIR(st.st_mode))
            continue;
    }

    ++nfiles;
    file_list = realloc(file_list, sizeof *file_list * nfiles);
    file_list[nfiles - 1].name = strdup(de->d_name);
    if (lstat(de->d_name, &file_list[nfiles - 1].st) == -1) {
      pdebug("lstat(\"%s\") failed\n", de->d_name);
      ret = -1;
      goto exit;
    }
  }

  /* See QSORT(3) for info about this function. It's not super important. It just sorts the list of
   * files using the filesort() function, which is the part you need to finish. */
  qsort(file_list, nfiles, sizeof *file_list, filesort);

  ++indent_level;
  for (size_t i = 0; i < nfiles; ++i) {
    char const * const name = file_list[i].name; /*  Get a shorter name to use in this block  */
    struct stat st;
    if (lstat(name, &st) == -1) {
      pdebug("lstat(\"%s\") failed\n", name);
      ret = -1;
      goto exit;
    }
    print_tree(name); /*  Recurse */
    if (fchdir(dfd) == -1) { /*  Return to correct directory */
      pdebug("fchdir(dfd) failed\n");
      ret = -1;
      goto exit;
    }
  }
  --indent_level;

  /* You may hear some admonish the use of goto statements. Do not listen to people who say such
   * nonsense. They are a part of the language, and you should use them (carefully). A very common
   * use case is to support a single exit point for a function that needs to perform cleanup
   * operations. goto labels have function scope. */
 exit:;
  if (file_list) {
    for (size_t i = 0; i < nfiles; ++i) free(file_list[i].name);
    free(file_list);
    file_list = NULL;
  }
  if (d) closedir(d);
  return ret;
}

/**
 * print_path_info() - Helper function
 *
 * Your final output should match the formatting used in this function, even if you don't use this
 * function!
 */
static void
print_path_info(char const *path, struct stat st)
{
  char sep = '[';
  if (opts->perms) {
    /* There is a very useful function in the libtree library called modestr().
     * It's documented in the cs344/libtree_testbench.h header.
     * You might want to look at it... just a suggestion :)  */
    printf("%c%s", sep, modestr(st.st_mode)); /* FIXME! */
    sep = ' ';
  }

  if (opts->user) {
    /*  Hint: getpwuid(3) */
    char *uname = {0};
    // print either username or UID if former is not available
    struct passwd *pwd = getpwuid(st.st_uid);
    if (pwd->pw_name) {
      uname = pwd->pw_name;
      printf("%c%s", sep, uname); /* FIXME! */
    }
    else
      printf("%c%ld", sep, (long)pwd->pw_uid);
    sep = ' ';
  }

  if (opts->group) {
    /*  Hint: getgrgid(3) */
    char *gname = {0};
    //print groupd name or GID if former is not available
    struct group* group = getgrgid(st.st_gid);
    if (group->gr_name) {
      gname = group->gr_name;
      printf("%c%s", sep, gname); /* FIXME! */
    }
    else
      printf("%c%i", sep, group->gr_gid);
    sep = ' ';
  }

  if (opts->size) {
    /*  Hint: stat.h(0p) */
    //print the file size
    printf("%c%zd", sep, st.st_size); /* FIXME! */
    sep = ' ';
  }

  if (sep != '[') printf("] ");

  printf("%s", path);

  if (S_ISLNK(st.st_mode)) {
    char rp[PATH_MAX + 1] = {0};
    readlink(path, rp, PATH_MAX);
    printf(" -> %s", rp);
  }
}

static int
filesort(void const *_lhs, void const *_rhs)
{
  struct fileinfo const *lhs = _lhs,
                        *rhs = _rhs;
  struct timespec const lt = lhs->st.st_mtim,
                        rt = rhs->st.st_mtim;
  switch (opts->sort)
  {
    case NONE:
      return 0; /*  Well that was easy */
    case ALPHA:
      return strcoll(lhs->name, rhs->name);
    case RALPHA:
      return strcoll(rhs->name, lhs->name); /* FIXME: implement reverse alphabetic sort */
    case TIME:
      /*  I did this one for you :) */
      if (rt.tv_sec != lt.tv_sec) {
        return rt.tv_sec - lt.tv_sec;
      } else {
        return rt.tv_nsec - lt.tv_nsec;
      }
  }
  return 0; /* oops */
}

