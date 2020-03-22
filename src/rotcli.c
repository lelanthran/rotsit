
// Command-line client for the rotsit library

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>

#include "cdb.h"

#define PROG_ERR(...)      do {\
   fprintf (stderr, "[%s:%i] ", __FILE__, __LINE__);\
   fprintf (stderr, __VA_ARGS__);\
} while (0)

#define EDITORVAR          "EDITOR"

#ifdef PLATFORM_WINDOWS
#define ENV_USERNAME    "USERNAME"
#define UNAMEVAR        "\%USERNAME\%"
#define EDITOR          "\%EDITOR\%"
#define ED_DEFAULT      "notepad.exe"
#else
#define ENV_USERNAME    "USER"
#define UNAMEVAR        "$USER"
#define EDITOR          "$EDITOR"
#define ED_DEFAULT      "vi"
#endif

// For testing only
static uint64_t my_seed = 1;

static void my_setseed (const char *msg)
{
   for (size_t i=0; msg[i]; i++) {
      my_seed = (my_seed << 8) ^ msg[i];
   }
}

static uint32_t my_rand (void)
{
   my_seed = ((my_seed * 1103515245) + 12345) & 0xffffffff;
   return (uint32_t) (my_seed & 0xffffffff);
}

static char *readfile (const char *filename)
{
   char *ret = NULL;
   size_t nbytes = 0;
   int c;

   FILE *inf = fopen (filename, "rt");
   if (!inf)
      return NULL;

   while (!ferror (inf) && !feof (inf) && ((c = fgetc (inf))!=EOF)) {
      char *tmp = realloc (ret, nbytes + 2);
      if (!tmp) {
         PROG_ERR ("Out of memory error reading [%s]\n", filename);
         free (ret);
         fclose (inf);
         return NULL;
      }

      ret = tmp;
      ret[nbytes++] = c;
      ret[nbytes] = 0;
   }
   return ret;
}


// All the user commands are handled by functions that follow this
// specification.
typedef uint32_t (*cmdfptr_t) (char **, char *, const char **);

// All the user commands
static uint32_t cmd_add (char **rs, char *msg, const char **args)
{
   uint32_t ret = 0x000001ff;
   args = args;
   char *rec = NULL;

   my_setseed (msg);

   PROG_ERR ("Creating new record (This may take a few minutes)\n");

   // ret = 0x00000100;

errorexit:
   if (ret & 0xff) {
      free (rec);
   }
   return ret;
}

static char *safe_rotrec_by_id (char **rs, const char **args)
{
   const char *id = args[1];
   const char *f = args[0];
   char *ret = NULL;

   if (!id) {
      PROG_ERR ("Expected an id for function [%s], no id specified\n", f);
      return NULL;
   }

   return ret;
}

static uint32_t cmd_show (char **rs, char *msg, const char **args)
{
   msg = msg;

   if (!cdb_record_print (safe_rotrec_by_id (rs, args), stdout)) {
      return 0xff;
   }

   return 0xff;
}

static uint32_t cmd_comment (char **rs, char *msg, const char **args)
{
   my_setseed (msg);

   /*
   if (!cdb_add_comment (safe_rotrec_by_id (rs, args), msg)) {
      return 0x00ff;
   }
   */

   return 0x01ff;
}

static uint32_t cmd_dup (char **rs, char *msg, const char **args)
{
   msg = msg;

   if (!args[1] || !args[2]) {
      PROG_ERR ("Marking duplicate requires two IDs\n");
      return 0x00ff;
   }

   /*
   if (!rotrec_dup (safe_rotrec_by_id (rs, args), args[2])) {
      return 0x00ff;
   }
   */

   return 0x01ff;
}

static uint32_t cmd_reopen (char **rs, char *msg, const char **args)
{
   /*
   if (!rotrec_reopen (safe_rotrec_by_id (rs, args), msg)) {
      return 0x00ff;
   }
   */

   return 0x01ff;
}

static uint32_t cmd_close (char **rs, char *msg, const char **args)
{
   /*
   if (!rotrec_close (safe_rotrec_by_id (rs, args), msg)) {
      return 0x00ff;
   }
   */
   return 0x01ff;
}

static uint32_t cmd_export (char **rs, char *msg, const char **args)
{
   msg = msg;
   args = args;

   uint32_t numrecs = 0;

   for (size_t i=0; rs && rs[i]; i++) {
      numrecs++;
   }

   /*
   for (uint32_t i=0; i<numrecs; i++) {
      rotrec_dump (rotsit_get_record (rs, i), stdout);
   }
   */

   return 0x00ff;
}

/*
static uint32_t cmd_list (char **rs, char *msg, const char **args)
{
   msg = msg;

   printf (" *****************************************************\n");
   printf (" ARGS: [%s]\n", args[1]);
   printf (" *****************************************************\n");

   if (!args[1]) {
      PROG_ERR ("No search expression specified\n");
      return 0x00ff;
   }

   rotrec_t **results = rotsit_filter (rs, args[1]);
   if (!results) {
      PROG_ERR ("Internal error in filter function\n");
      return 0x00ff;
   }

   for (size_t i=0; results[i]; i++) {
      rotrec_dump (results[i], stdout);
   }
   free (results);
   return 0x0000;
}
*/

static bool needs_message (const char *command)
{
   static const char *cmds[] = {
      "add", "comment", "close", "reopen",
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
      { "close",     cmd_close   },
      { "export",    cmd_export  },
      // { "list",      cmd_list    },
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
"Revenge of the Simple Issue Tracker (0.1.1), c/line client",
"Usage:",
"rotsit [--option[=value]] command [command-arguments]",
"  Options:",
"  --help:     Print this message, then exit with success",
"  --message:  Provide a message for commands that take a message",
"  --file:     Read a message from file for commands that take a message",
"  --dbfile:   Use specified filename as the db (defaults to 'issues.sitdb')",
"  --user:     Set the username (defaults to " UNAMEVAR ")",
"  --fastrand: (Used for testing - do not use)",
"",
"All commands which require a message will check --message and --file",
"options for the message. If no message is specified, "EDITOR" is used",
"to open an editor in which the user may type in the message.",
"",
"commands:",
"  add               Adds a new issue, "EDITOR" used.",
"  show <id>         Displays an issue.",
"  reopen <id>       Reopens a closed issue, "EDITOR" used",
"  comment <id>      Adds a comment to an issue, "EDITOR" used.",
"  dup <id1> <id2>   Marks id1 as a duplicate of id2, "EDITOR" used.",
"  close <id>        Closes issue with id, "EDITOR" used.",
"  export            Plain-text export of every issue",
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

   uint32_t version = 0;
   char *msg = NULL;
   char *edit_cmd = NULL;
   FILE *inf = NULL;
   char **issues = NULL;
   bool issues_dirty = false;

   my_seed = time (NULL);

   for (size_t i=1; argv[i]; i++) {
      if ((memcmp (argv[i], "--", 2))==0) {
         char *name = &argv[i][2];
         char *value = strchr (name, '=');
         if (value) {
            *value++ = 0;
         } else {
            value = "";
         }
         setenv (name, value, 1);
         PROG_ERR ("Set [%s:%s]\n", name, value);
      }
   }

   // Check which options are set
   if (getenv ("fastrand")) {
      // rotsit_user_rand = my_rand;
   }

   if (getenv ("help")) {
      ret = EXIT_SUCCESS;
      print_help_msg ();
      goto errorexit;
   }

   const char *filename = getenv ("file");
   msg = getenv ("message");
   if (!msg && filename) {
      msg = readfile (filename); // TODO: Write this function
   }

   const char *username = getenv ("user");
   if (username) {
      setenv (ENV_USERNAME, username, 1);
   }

   const char *dbfile = getenv ("dbfile") ? getenv ("dbfile") : "issues.sitdb";

   inf = fopen (dbfile, "rb");
   if (!inf) {
      inf = fopen (dbfile, "wb");
      if (!inf) {
         PROG_ERR ("Unable to create database file [%s]\n", dbfile);
         goto errorexit;
      }
      if (!(cdb_records_save (NULL, 1, inf))) {
         PROG_ERR ("Failed to create database file [%s]\n", dbfile);
         goto errorexit;
      }
      fclose (inf);
      if (!(inf = fopen (dbfile, "rb"))) {
         PROG_ERR ("Failed to reopen [%s]: %m\n", dbfile);
         goto errorexit;
      }
   }

   issues = cdb_records_load (inf, &version);
   fclose (inf);
   inf = NULL;

   cmdfptr_t cmdfptr = find_cmd (argv[1]);

   if (!cmdfptr) {
      PROG_ERR ("Command [%s] not recognised as a command. Try --help\n",
               argv[1]);
      goto errorexit;
   }

   // If command specified needs a message, check that we have a message
   // or start the EDITOR so that the user can write a message.
   if (needs_message (argv[1]) && !msg) {
      // Use a default editor. If environment editor exists, use that
      // instead. Either way there will be a legit string in editor.
      const char *editor = ED_DEFAULT;
      const char *env_editor = getenv (EDITORVAR);
      if (env_editor && env_editor[0]) {
         editor = env_editor;
      }

      char template[] = "tempfile.XXXXXX";
      int fd = mkstemp (template);
      if (fd < 0) {
         PROG_ERR ("Failed to create temporary file [%s]: %m\n", template);
         goto errorexit;
      }
      close (fd);

      static char edit_cmd[4096];
      snprintf (edit_cmd, sizeof edit_cmd, "%s %s", editor, template);

      if (system (edit_cmd)!=0) {
         PROG_ERR ("Failed to run editor [%s]\n", edit_cmd);
         goto errorexit;
      }

      msg = readfile (template); // TODO: Write this function
      remove (template);

      if (msg==NULL) {
         PROG_ERR ("Out of memory error\n");
         goto errorexit;
      }
      if (*msg==0) {
         PROG_ERR ("No message specified, aborting\n");
         goto errorexit;
      }
   }

   // Execute the command - the return value is 4 bytes:
   // ret[0] = return status (0=success)
   // ret[1] = object now dirty, command mutated the object
   // ret[2], ret[3] = RFU
   uint32_t result = cmdfptr (issues, msg, (const char **)&argv[2]);
   if (result & 0xff) {
      PROG_ERR ("Command [%s] returned error 0x%02x\n", argv[1],
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
         PROG_ERR ("Unable to write file [%s]: %m\n", dbfile);
      } else {
         cdb_records_save (issues, version, outf);
         fclose (outf);
      }
   }

   free (msg);
   free (edit_cmd);
   cdb_records_free (issues);
   return ret;
}

