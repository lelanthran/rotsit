## Revenge of the Simple Issue Tracker

### What is this?
This project, **RotSIT** (AKA Revenge of the Simple Issue Tracker) is a
simple issue tracker for software projects. If you are looking for a proper
issue tracker, this isn't it. Actually, even if you really wanted an
improper issue tracker this also wouldn't be it (If you find an _improper_
issue tracker please let me know - I'd love to see what that looks like).

This is a simple issue tracker that uses a plain-text database to store
issues in a manner that a source control system such as Git or SVN can track.

### What can RotSIT do?
Issues can be opened, closed, reopened and commented upon. A rudimentary
tool is built into the software that can filter issues according to
certain criteria (date, username, etc).

However since this is a plain-text database using newlines in the field
separators you can use any text processing tool to search through the
database.

Additionally your source control (git or svn) tool should track the
`issues.sitdb` file as part of the repo. Anytime it is changed the source
control should be able to show you the diff between versions as well as
who made the change to the database and what the change was.

### How do I track my issues?
Simply use the command-line app that is built in `cliapp/`.
