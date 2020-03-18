## Revenge of the Simple Issue Tracker

### What is this?
This project, **RotSIT** (AKA Revenge of the Simple Issue Tracker) is a
simple issue tracker for software projects. If you are looking for a proper
issue tracker, this isn't it. Actually, even if you really wanted an
improper issue tracker this also wouldn't be it (Sidenote: If you do find
an _improper_ issue tracker please let me know - I'd love to see what that
looks like).

This is a _simple_ issue tracker. It uses a plain-text database to store
issues in a manner that a source control system such as Git or SVN can track.
**RotSIT** was tested on Linux and Windows. It uses a rather idiosyncratic
build process that works on both OSes (and was briefly tested on one of the
BSDs, I forget which).

### What can RotSIT do?
Issues can be opened, closed, reopened and commented upon. A rudimentary
tool is built into the software that can filter issues according to
certain criteria (date, username, etc).

However since this is a plain-text database using newlines in the record
separators you can use any text processing tool to search through the
database.

Additionally your source control (git or svn) tool should track the
`issues.sitdb` file as part of the repo. Anytime it is changed the source
control should be able to show you the diff between versions as well as
who made the change to the database and what the change was.

### Why use RotSIT over other issue trackers?
I built this software because I wanted a source-controlled, diff-friendly and
text-only issue tracker. No other issue tracker (well, there _is_ one,
that I know of. Professional courtesy forces me to mention it. It's called
`SIT` and lives somewhere on github) allows the user to easily diff the
incremental changes in the source code **and get the changes in the issues
database too**.

- _Source-controlled_: The database must be managed by my source control
  software. Each issue opened, each closure, each comment added by a team
  member ... I wanted all those things tracked in a database that is
  stored with the actual code.
- _Diff-friendly_: I wanted to examine the database from version to
  version to see only what had changed between versions. Being able to run
  diff on incremental versions of the database is just as useful as
  being able to run diff on incremental versions of the source code.

### How do I track my issues?
Simply use the command-line app that is built in `cliapp/`. At this time
there is no other application to modify the **RotSIT** database. For
convenience you should copy the `cliapp/main-d.elf` or `cliapp/main-d.exe`
(depending on which platform you built it on) to your path.

At some point in the future there will be an installation script which
will rename the built `cliapp` program to `rotsit`.

The `cliapp` program by default uses the file `issues.sitdb` in the
current directory. Make sure that this file is being tracked by your
source control system.

The following command-line options are recognised:
```
rotsit [--option[=value]] command [command-arguments]
  Options:
  --help:     Print this message, then exit with success
  --message:  Provide a message for commands that take a message
  --file:     Read a message from file for commands that take a message
  --dbfile:   Use specified filename as the db (defaults to 'issues.sitdb')
  --user:     Set the username (defaults to $USER)
```

_Note that if a message is required for an action, but no message is
provided via the command-line, then the default editor is used_

The following commands manipulate the issues database:
```
  add               Adds a new issue, $EDITOR used.
  show <id>         Displays an issue.
  reopen <id>       Reopens a closed issue, $EDITOR used
  comment <id>      Adds a comment to an issue, $EDITOR used.
  dup <id1> <id2>   Marks id1 as a duplicate of id2, $EDITOR used.
  close <id>        Closes issue with id, $EDITOR used.
  export            Plain-text export of every issue
  list <listexpr>   Short-form list of all the entries matching listexpr
```

For more detailed information run the application with `--help`.

### Won't multiple developers all modifying the database at the same time result in merge conflicts?
No. Firstly, the IDs used are **very** unlikely to clash.
Secondly, the modifications performed on the issues database are in
well-defined chunks that are easy for `patch` to figure out. Finally, no
deletions are allowed (although it wouldn't conflict even if we did do
deletions).

### I have trouble building this.
That's not a question.

### When will you provide prebuilt binaries?
Soon. Very soon. Possibly not more than a week away from today. Check back
often. Tell your friends. Your family too. Maybe the dog as well. Everyone
should have a look at this :-)

