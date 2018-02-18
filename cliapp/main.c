
// Command-line client for the rotsit library

#include <stdio.h>
#include <stdlib.h>

#include "rotsit/rotsit.h"

#include "xerror/xerror.h"
#include "xcfg/xcfg.h"

#ifdef PLATFORM_WINDOWS
#define UNAMEVAR        "\%USERNAME\%"
#else
#define UNAMEVAR        "$USER"
#endif

void print_help_msg (void)
{
   static const char *msg[] = {
"Revenge of the Simple Issue Tracker (0.0.1), c/line client",
"Usage:",
"rotsit [--option[=value]] command [command-arguments]",
"  Options:",
"  --help:     Print this message, then exit with success",
"  --msg:      Provide a message for commands that take a message",
"  --file:     Read a message from file for commands that take a message",
"  --user:     Set the username (defaults to " UNAMEVAR ")",
"",
"All commands which require a message will check --msg and --file",
"options for the message. If no message is specified, $EDITOR is used",
"to open an editor in which the user may type in the message.",
"",
"commands:",
"  add               Adds a new issue, $EDITOR used.",
"  show <id>         Displays an issue.",
"  comment <id>      Adds a comment to an issue, $EDITOR used.",
"  dup <id1> <id2>   Marks id1 as a duplicate of id2, $EDITOR used.",
"  close <id>        Closes issue with id, $EDITOR used.",
"  export            Plain-text export of every issue",
"  list <listexpr>   Short-form list of all the entries matching listexpr",
"",
"<listexpr>",
"  List expression consists of various constraints as follows:",
"  --from-time=<time>      Only entries newer than <time>",
"  --to-time=<time>        Only entries older than <time>",
"  --status=<status,>      All entries matching cvs list in <status,>",
"  --from-id=<id>          Only entries with id newer than <id>",
"  --to-id=<id>            Only entries with id older than <id>",
"  --keyword=<string>      Include entries which match <string>",
"  --by-owner=<uname>      Only entries by <uname> (excludes comments)",
"  --by-user=<uname>       Only entries by <uname> (comments only)",
"",
"",
"Copyright Lelanthran Manickum, 2018 (lelanthran@gmail.com)",
"",
"",
   };

   for (size_t i=0; i<sizeof msg/sizeof msg[0]; i++) {
      printf ("%s\n", msg[i]);
   }
}

int main (int argc, char **argv)
{
   argc = argc;
   int ret = EXIT_FAILURE;

   // Set the options we want to read to default values
   static const struct {
      const char *name;
      const char *def;
   } options[] = {
      { "help", NULL },
      { "msg",  NULL },
      { "file", NULL },
      { "user", NULL },
   };

   for (size_t i=0; i<sizeof options/sizeof options[0]; i++) {
      xcfg_configure ("none", options[i].name, "none", options[i].def);
   }

   if (xcfg_from_array ("none", (const char **)argv, "c/line")==(size_t)-1) {
      XERROR ("Error parsing command-line\n");
      goto errorexit;
   }

   atexit (xcfg_shutdown);

   // Check which options are set
   const char *help_requested = xcfg_get ("none", "help");
   if (help_requested) {
      ret = EXIT_SUCCESS;
      print_help_msg ();
      goto errorexit;
   }

   ret = EXIT_SUCCESS;

errorexit:
   return ret;
}

