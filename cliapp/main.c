
// Command-line client for the rotsit library

#include <stdio.h>
#include <stdlib.h>

#include "rotsit/rotsit.h"

#include "xerror/xerror.h"
#include "xstring/xstring.h"
#include "xcfg/xcfg.h"

#ifdef PLATFORM_WINDOWS
#define UNAMEVAR        "\%USERNAME\%"
#else
#define UNAMEVAR        "$USER"
#endif

// All the user commands are handled by functions that follow this
// specification.
typedef uint32_t (*cmdfptr_t) (rotsit_t *, char *, const char **);

// All the user commands
static uint32_t cmd_add (rotsit_t *rs, char *msg, const char **args)
{
   uint32_t ret = 0x000001ff;
   args = args;

   XLOG ("Creating new record (This may take a few minutes)\n");

   rotrec_t *rec = rotrec_new (msg);
   if (!rec) {
      XERROR ("Unable to open issue [%s]\n");
      goto errorexit;
   }

   if (!rotsit_add_record (rs, rec)) {
      XERROR ("Unable to add issue to database [%s]\n", msg);
      goto errorexit;
   }

   ret = 0x00000100;

errorexit:
   if (ret & 0xff) {
      rotrec_del (rec);
   }
   return ret;
}

static uint32_t cmd_show (rotsit_t *rs, char *msg, const char **args)
{
   msg = msg;

   uint32_t ret = 0x000000ff;
   const char *id = args[1];
   rotrec_t *rr = NULL;

   if (!id) {
      XERROR ("Expected an id for function [show], no id specified\n");
      goto errorexit;
   }

   rr = rotsit_find_by_id (rs, id);
   if (!rr) {
      XERROR ("No record found with id [%s]\n", id);
      goto errorexit;
   }

   rotrec_dump (rr, stdout);

   ret = 0x00;

errorexit:
   return ret;
}

static uint32_t cmd_comment (rotsit_t *rs, char *msg, const char **args)
{
   uint32_t ret = 0x000001ff;
   const char *id = args[1];
   rotrec_t *rr = NULL;

   if (!id) {
      XERROR ("Expected an id for function [show], no id specified\n");
      goto errorexit;
   }

   rr = rotsit_find_by_id (rs, id);
   if (!rr) {
      XERROR ("No record found with id [%s]\n", id);
      goto errorexit;
   }

   if (!rotrec_add_comment (rr, msg)) {
      XERROR ("Unable to add comment to record [%s]\n", id);
      goto errorexit;
   }

   ret = 0x00000100;

errorexit:
   return ret;
}

static uint32_t cmd_dup (rotsit_t *rs, char *msg, const char **args)
{
   rs = rs; msg = msg; args = args;
   return 0;
}

static uint32_t cmd_reopen (rotsit_t *rs, char *msg, const char **args)
{
   rs = rs; msg = msg; args = args;
   return 0;
}

static uint32_t cmd_export (rotsit_t *rs, char *msg, const char **args)
{
   rs = rs; msg = msg; args = args;
   return 0;
}

static uint32_t cmd_list (rotsit_t *rs, char *msg, const char **args)
{
   rs = rs; msg = msg; args = args;
   return 0;
}

static bool needs_message (const char *command)
{
   static const char *cmds[] = {
      "add", "comment", "dup", "close", "reopen",
   };

   for (size_t i=0; i<sizeof cmds/sizeof cmds[0]; i++) {
      if (strcmp (cmds[i], command) == 0) {
         return true;
      }
   }
   return false;
}

static cmdfptr_t find_cmd (const char *name)
{
   static const struct {
      const char *name;
      cmdfptr_t fptr;
   } cmds[] = {
      { "add",       cmd_add     },
      { "show",      cmd_show    },
      { "comment",   cmd_comment },
      { "dup",       cmd_dup     },
      { "reopen",    cmd_reopen  },
      { "export",    cmd_export  },
      { "list",      cmd_list    },
   };

   for (size_t i=0; i<sizeof cmds/sizeof cmds[0]; i++) {
      if (strcmp (cmds[i].name, name) == 0) {
         return cmds[i].fptr;
      }
   }
   return NULL;
}

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
"  --dbfile:   Use specified filename as the db (defaults to 'issues.sitdb')",
"  --user:     Set the username (defaults to " UNAMEVAR ")",
"",
"All commands which require a message will check --msg and --file",
"options for the message. If no message is specified, $EDITOR is used",
"to open an editor in which the user may type in the message.",
"",
"commands:",
"  add               Adds a new issue, $EDITOR used.",
"  show <id>         Displays an issue.",
"  reopen <id>       Reopens a closed issue, $EDITOR used",
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

   char *msg = NULL;
   FILE *inf = NULL;
   rotsit_t *issues = NULL;
   bool issues_dirty = false;
   char *fcontents = NULL;

   // Set the options we want to read to default values
   static const struct {
      const char *name;
      const char *def;
   } options[] = {
      { "help",      NULL },
      { "msg",       NULL },
      { "file",      NULL },
      { "user",      NULL },
      { "dbfile",    "issues.sitdb" },
   };

   for (size_t i=0; i<sizeof options/sizeof options[0]; i++) {
      xcfg_configure ("none", options[i].name, "none", options[i].def);
   }

   if (xcfg_from_array ("none", (const char **)argv, "c/line")==(size_t)-1) {
      XERROR ("Error parsing command-line\n");
      goto errorexit;
   }

   // Check which options are set
   const char *help_requested = xcfg_get ("none", "help");
   if (help_requested) {
      ret = EXIT_SUCCESS;
      print_help_msg ();
      goto errorexit;
   }

   const char *filename = xcfg_get ("none", "file");
   msg = xstr_dup (xcfg_get ("none", "msg"));
   if (!msg && filename) {
      msg = xstr_readfile (filename);
   }

   const char *username = xcfg_get ("none", "user");
   if (username) {
      setenv (ENV_USERNAME, username, 1);
   }

   const char *dbfile = xcfg_get ("none", "dbfile");
   if (!dbfile || !*dbfile) {
      XERROR ("Missing option dbfile. Did you override the default "
              "using --dbfile?\n");
      goto errorexit;
   }

   inf = fopen (dbfile, "rb");
   if (!inf) {
      inf = fopen (dbfile, "wb");
      if (!inf) {
         XERROR ("Unable to create database file [%s]\n", dbfile);
         goto errorexit;
      }
      fclose (inf);
      inf = NULL;
   }

   fcontents = xstr_readfile (dbfile);
   if (!fcontents) {
      XERROR ("Unable to read issues from [%s]\n", filename);
      goto errorexit;   // TODO: Double hceck this - might return NULL for
                        // empty file and we must be able to work with an
                        // empty file.
   }

   issues = rotsit_parse (fcontents);

   // Check which command was requested - the first non-option argument
   // is a command
   size_t cmdidx = (size_t)-1;
   for (size_t i=1; argv[i]; i++) {

      if (argv[i][0]=='-' && argv[i][1]=='-')
         continue;

      cmdidx = i;
      break;
   }

   if (cmdidx==(size_t)-1) {
      XERROR ("No command specified. Try using --help\n");
      goto errorexit;
   }

   cmdfptr_t cmdfptr = find_cmd (argv[cmdidx]);

   if (!cmdfptr) {
      XERROR ("Command [%s] not recognised as a command. Try --help\n",
               argv[cmdidx]);
      goto errorexit;
   }

   // If command specified needs a message, check that we have a message
   // or use start the EDITOR so that the user can write a message.
   if (needs_message (argv[cmdidx]) && !msg) {
      // TODO: Start the editor
   }

   // Execute the command - the reutrn value is 4 bytes:
   // ret[0] = return status (0=success)
   // ret[1] = object now dirty, command mutated the object
   // ret[2], ret[3] = RFU
   uint32_t result = cmdfptr (issues, msg, &argv[cmdidx]);
   if (result & 0xff) {
      XERROR ("Command [%s] returned error 0x%02x\n", argv[cmdidx],
                                                      result & 0xff);
      goto errorexit;
   }

   if ((result >> 8) & 0xff) {
      issues_dirty = true;
   }

   ret = EXIT_SUCCESS;

errorexit:
   if (inf)
      fclose (inf);

   if (issues_dirty) {
      FILE *outf = fopen (dbfile, "wb");
      if (!outf) {
         XERROR ("Unable to write file [%s]: %m\n", dbfile);
      } else {
         rotsit_write (issues, outf);
         XERROR ("WROTE FILE!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
         fclose (outf);
      }
   }

   free (msg);
   free (fcontents);
   xerror_set_logfile (NULL);
   rotsit_del (issues);
   xcfg_shutdown ();
   return ret;
}

