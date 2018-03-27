
// Command-line client for the rotsit library

#include <stdio.h>
#include <stdlib.h>

#include "rotsit/rotsit.h"

#include "xerror/xerror.h"
#include "xstring/xstring.h"
#include "xcfg/xcfg.h"
#include "xfile/xfile.h"

#define EDITORVAR          "EDITOR"

#ifdef PLATFORM_WINDOWS
#define UNAMEVAR        "\%USERNAME\%"
#define EDITOR          "\%EDITOR\%"
#define ED_DEFAULT      "notepad.exe"
#else
#define UNAMEVAR        "$USER"
#define EDITOR          "$EDITOR"
#define ED_DEFAULT      "vi"
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
      XERROR ("Unable to open issue [%s]\n", msg);
      goto errorexit;
   }

   if (!rotsit_add_record (rs, rec)) {
      XERROR ("Unable to add issue to database [%s]\n", msg);
      goto errorexit;
   }

   XLOG ("Added new issue: %s\n", rotrec_get_field (rec, RF_GUID));

   ret = 0x00000100;

errorexit:
   if (ret & 0xff) {
      rotrec_del (rec);
   }
   return ret;
}

static rotrec_t *safe_rotrec_by_id (rotsit_t *rs, const char **args)
{
   const char *id = args[1];
   const char *f = args[0];
   rotrec_t *rr = NULL;

   if (!id) {
      XERROR ("Expected an id for function [%s], no id specified\n", f);
      return NULL;
   }

   rr = rotsit_find_by_id (rs, id);
   if (!rr) {
      XERROR ("No record found with id [%s] in function [%s]\n", id, f);
   }

   return rr;
}

static uint32_t cmd_show (rotsit_t *rs, char *msg, const char **args)
{
   msg = msg;

   if (!rotrec_dump (safe_rotrec_by_id (rs, args), stdout)) {
      return 0xff;
   }

   return 0;
}

static uint32_t cmd_comment (rotsit_t *rs, char *msg, const char **args)
{

   if (!rotrec_add_comment (safe_rotrec_by_id (rs, args), msg)) {
      return 0x00ff;
   }

   return 0x0100;
}

static uint32_t cmd_dup (rotsit_t *rs, char *msg, const char **args)
{
   msg = msg;

   if (!args[1] || !args[2]) {
      XERROR ("Marking duplicate requires two IDs\n");
      return 0x00ff;
   }

   if (!rotrec_dup (safe_rotrec_by_id (rs, args), args[2])) {
      return 0x00ff;
   }

   return 0x0100;
}

static uint32_t cmd_reopen (rotsit_t *rs, char *msg, const char **args)
{
   if (!rotrec_reopen (safe_rotrec_by_id (rs, args), msg)) {
      return 0x00ff;
   }

   return 0x0100;
}

static uint32_t cmd_close (rotsit_t *rs, char *msg, const char **args)
{
   if (!rotrec_close (safe_rotrec_by_id (rs, args), msg)) {
      return 0x00ff;
   }
   return 0x0100;
}

static uint32_t cmd_export (rotsit_t *rs, char *msg, const char **args)
{
   msg = msg;
   args = args;

   uint32_t numrecs = rotsit_count_records (rs);

   for (uint32_t i=0; i<numrecs; i++) {
      rotrec_dump (rotsit_get_record (rs, i), stdout);
   }

   return 0x0000;
}

static uint32_t cmd_list (rotsit_t *rs, char *msg, const char **args)
{
   msg = msg;

   printf (" *****************************************************\n");
   printf (" ARGS: [%s]\n", args[1]);
   printf (" *****************************************************\n");

   if (!args[1]) {
      XERROR ("No search expression specified\n");
      return 0x00ff;
   }

   rotrec_t **results = rotsit_filter (rs, args[1]);
   if (!results) {
      XERROR ("Internal error in filter function\n");
      return 0x00ff;
   }

   for (size_t i=0; results[i]; i++) {
      rotrec_dump (results[i], stdout);
   }
   free (results);
   return 0x0000;
}

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
"  list <listexpr>   Short-form list of all the entries matching listexpr",
"",
"<listexpr>",
"  List expression is a single string that specifies which records must",
"  be in the results set. Fields that can be used in the expression are",
"  listed below along with a description of each field.",
"",
"  Each expression consists of exactly two terms and a comparison operator",
"  optionally enclosed by brackets. For example both of the following are",
"  equivalent expressions:",
"",
"     opened-by == jsmith@example.com",
"     (opened-by == jsmith@example.com)",
"",
"  The brackets are used to enforce precedence. Where brackets are missing",
"  the default precedence is to apply the operators from right to left,",
"  working back from the end of the expression. When there is more than",
"  two operands and more than one operator brackets are not optional and",
"  must be used to group the three or more operands into pairs. For example",
"",
"opened-by == jsmith@example.com & closed-by jsmith@example.com      [Wrong]",
"(opened-by == jsmith@example.com) & closed-by jsmith@example.com    [Right]",
"",
"",
"  OPERATOR LIST",
"     <        Less than",
"     >        Greater than",
"     <=       Less than or equal to",
"     >=       Greater than or equal to",
"     ==       Equal to (matches keywords or dates exactly",
"     !=       Not equal to",
"     |        Logical OR",
"     &        Logical AND",
"",
"",
"  FIELD LIST",
"  guid              The GUID field",
"  order             The order field",
"  opened_by         User who opened the issue",
"  opened_on         Date/time when issue was opened",
"  closed_by         User who closed the issue",
"  closed_on         Date/time when issue was closed",
"  assigned_by       User who set the assignment of the issue",
"  assigned_to       User who is responsible for closing the issue",
"  assigned_on       Date when the assignment was performed",
"  status            Match the status (OPEN/CLOSED)",
"  message           Case-insensitive match of keyword in OPEN message",
"  closed_message    Case-insensitive match of keyword in the CLOSE message",
"  comment           Case-insensitive match of keyword in comment",
"  allwords          Case-insensitive match of keyword anywhere in issue",
"  alltimes          Matches any date in any field",
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
   char *tmp_fname = NULL;
   char *edit_cmd = NULL;
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
      char *tmp = xstr_cat (ENV_USERNAME, "=", username, NULL);
      if (!tmp) {
         XERROR ("Out of memory\n");
         goto errorexit;
      }
      putenv (tmp);
      // free (tmp);
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
   }
   fclose (inf);
   inf = NULL;

   fcontents = xstr_readfile (dbfile);
   if (!fcontents) {
      XERROR ("Unable to read issues from [%s]\n", dbfile);
      goto errorexit;   // TODO: Double check this - might return NULL for
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
   // or start the EDITOR so that the user can write a message.
   if (needs_message (argv[cmdidx]) && !msg) {
      // Use a default editor. If environment editor exists, use that
      // instead. Either way there will be a legit string in editor.
      const char *editor = ED_DEFAULT;
      const char *env_editor = getenv (EDITORVAR);
      if (env_editor && env_editor[0]) {
         editor = env_editor;
      }

      tmp_fname = xfile_tmpname (NULL, NULL);
      if (!tmp_fname) {
         XERROR ("Unable to create a temporary file, aborting\n:%m\n");
         goto errorexit;
      }
      edit_cmd = xstr_cat (editor, " ", tmp_fname, NULL);
      if (!edit_cmd) {
         XERROR ("Out of memory\n");
         goto errorexit;
      }

      if (system (edit_cmd)!=0) {
         XERROR ("Failed to run editor [%s]\n", edit_cmd);
         goto errorexit;
      }

      msg = xstr_readfile (tmp_fname);
      if (msg==NULL) {
         XERROR ("Out of memory error\n");
         goto errorexit;
      }
      if (*msg==0) {
         XERROR ("No message specified, aborting\n");
         goto errorexit;
      }
   }

   // Execute the command - the return value is 4 bytes:
   // ret[0] = return status (0=success)
   // ret[1] = object now dirty, command mutated the object
   // ret[2], ret[3] = RFU
   uint32_t result = cmdfptr (issues, msg, (const char **)&argv[cmdidx]);
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
         fclose (outf);
      }
   }

   free (msg);
   if (tmp_fname) {
      xfile_rm (tmp_fname, "r");
      free (tmp_fname);
   }
   free (edit_cmd);
   free (fcontents);
   xerror_set_logfile (NULL);
   rotsit_del (issues);
   xcfg_shutdown ();
   return ret;
}

