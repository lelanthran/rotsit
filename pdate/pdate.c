#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "xstring/xstring.h"

#include "pdate.h"

#define ISENGLISH(x)    (x & (1 << 0))
#define ISYEAR(x)       (x & (1 << 1))
#define ISMONTH(x)      (x & (1 << 2))
#define ISDAY(x)        (x & (1 << 3))
#define ISTIME(x)       (x & (1 << 4))

#define SETENGLISH(x)   (x |= (1 << 0))
#define SETYEAR(x)      (x |= (1 << 1))
#define SETMONTH(x)     (x |= (1 << 2))
#define SETDAY(x)       (x |= (1 << 3))
#define SETTIME(x)      (x |= (1 << 4))

int32_t strmonth (const char *string)
{
   const char *months[] = {
      "jan", "feb", "mar", "apr",
      "may", "jun", "jul", "aug", 
      "sep", "oct", "nov", "dec",
   };
   for (size_t i=0; i<sizeof months/sizeof months[0]; i++) {
      if ((memcmp (months[i], string, 3))==0) {
         return i+1;
      }
   }
   return -1;
}

int32_t clamp_day (int32_t month)
{
   int32_t nr_days[] = {
      31, 29, 31,
      30, 31, 30,
      31, 31, 30,
      31, 30, 31,
   };

   return (month < 1 || month > 12) ? -1 : nr_days[month -1];
}

enum pdate_errcode_t pdate_parse (const char *string, time_t *ret,
                                  bool fromdate)
{
   time_t tv;
   struct tm *tm;
   enum pdate_errcode_t errcode = pdate_error;
   int32_t year = -1;
   int32_t month = -1;
   int32_t day = -1;
   int32_t hour = -1;
   int32_t min = -1;
   int32_t sec = -1;
   char *copy = xstr_dup (string);
   char **tokens = NULL;
   uint32_t typed[6]; // More than 6 tokens and we return errorcode.

   if (!copy) goto errorexit;

   memset (typed, 0, sizeof typed);

   char *tmp = copy;
   while (*tmp) {
      *tmp = tolower (*tmp);
      tmp++;
   }

   time (&tv);
   tm = localtime (&tv);
   tokens = xstr_split (copy, ",\n\t \\/-");
   free (copy); copy = NULL;
   if (!tokens) goto errorexit;

   // Definitive checks
   for (size_t i=0; tokens[i]; i++) {
      int32_t t_year = -1;
      int32_t t_month = -1;
      int32_t t_day = -1;
      int32_t t_hour = -1;
      int32_t t_min = -1;
      int32_t t_sec = -1;
      size_t toklen = strlen (tokens[i]);
      // Month Check
      if ((t_month = strmonth (tokens[i]))!=-1) {
         if (month!=-1) {
            errcode = pdate_ambig_month;
            goto errorexit;
         }
         month = t_month;
         tokens[i][0] = 0;
         continue;
      }
      // Year check
      if (toklen==4) {
         if ((sscanf (tokens[i], "%4i", &t_year))==1) {
            if (year!=-1) {
               errcode = pdate_ambig_year;
               goto errorexit;
            }
            year = t_year;
            tokens[i][0] = 0;
            continue;
         }
      }
      // Day check
      if (toklen==2) {
         if ((sscanf (tokens[i], "%2i", &t_day))==1) {
            if (t_day>12) {
               if (day!=-1) {
                  errcode = pdate_ambig_day;
                  goto errorexit;
               }
               day = t_day;
               tokens[i][0] = 0;
               continue;
            }
         }
      }
      // Time check
      if (strchr (tokens[i], ':') ||strchr (tokens[i], 'h')) {
         char *s_min;
         char *s_sec;
         if ((sscanf (tokens[i], "%2i", &t_hour))==1) {
            if (hour!=-1) {
               errcode = pdate_ambig_hour;
               goto errorexit;
            }
            hour = t_hour;
         }

         s_min = strchr (tokens[i], 'h');
         if (!s_min) s_min = strchr (tokens[i], ':');

         if ((sscanf (s_min+1, "%2i", &t_min))==1) {
            if (min!=-1) {
               errcode = pdate_ambig_min;
               goto errorexit;
            }
            min = t_min;
         }
         tokens[i][0] = 0;
         s_sec = strchr (s_min+1, ':');
         if (!s_sec) 
            continue;
         if ((sscanf (s_sec+1, "%2i", &t_sec))==1) {
            if (sec!=-1) {
               errcode = pdate_ambig_sec;
               goto errorexit;
            }
            sec = t_sec;
         }
         continue;
      }
   }

   // Try to determine what the ambiguous fields meant
   for (size_t i=0; tokens[i]; i++) {
      int32_t tmpval;
      size_t toklen = strlen (tokens[i]);
      if (toklen==0)
         continue;
      if ((sscanf (tokens[i], "%i", &tmpval))!=1) {
         errcode = pdate_unknown_field;
         goto errorexit;
      }
      if (day==-1) {
         day = tmpval;
         tokens[i][0] = 0;
         continue;
      }
      if (month==-1) {
         month = tmpval;
         tokens[i][0] = 0;
         continue;
      }
      if (year==-1) {
         year = tmpval;
         tokens[i][0] = 0;
         continue;
      }
      errcode = pdate_unknown_field;
   }

   if (day==-1 && month==-1) {
      errcode = pdate_ambig_month;
      goto errorexit;
   }
   // Sanity check for all the values we read
   if (month==-1) {
      month = fromdate ? 1 : 12;
   }
   if (day==-1) {
      day = fromdate ? 1 : clamp_day (month);
   }
   if (year==-1) {
      year = tm->tm_year + 1900;
   }
   if (hour==-1) {
      hour = fromdate ? 0 : 59;
   }
   if (min==-1) {
      min = fromdate ? 0 : 59;
   }
   if (sec==-1) {
      sec = fromdate ? 0 : 59;
   }

   month--;
   memset (&tm, 0, sizeof tm);
   year = year - 1900;
   tm->tm_sec = sec;
   tm->tm_min = min;
   tm->tm_hour = hour;
   tm->tm_mday = day;
   tm->tm_mon = month;
   tm->tm_year = year;

   tv = mktime (tm);
   if (tv==(time_t)-1) {
      errcode = pdate_invalid;
      goto errorexit;
   }

   errcode = pdate_valid;
   memcpy (ret, &tv, sizeof tv);

   xstr_delarray (tokens);
   return errcode;


errorexit:
   free (copy);
   if (tokens)
      xstr_delarray (tokens);
   return errcode;
}

const char *pdate_errmsg (enum pdate_errcode_t pd)
{
   const char *msgs[] = {
      "Date valid",
      "Date ambig_year",
      "Date ambig_month",
      "Date ambig_day",
      "Date ambig_hour",
      "Date ambig_min",
      "Date ambig_sec",
      "Date unknown_field",
      "Date error",
      "Date invalid",
   };

   return msgs[pd];
}

void pdate_test (void)
{
   const char *dates[] = {
     "12 June 2016",
     "12 June ",
     "June 12",
     "Jun-12",
     "12 Jun 2016",
     "12 Jun, 2016",
     "12 June 2016",
     "12-June-2016",
     "12/6/2016",
     "12/06/2016",
     " 12:59:5 12 June 2016",
     " 12:69:5 12 June ",
     " 12h59:5 June 12",
     " 12:59:5 Jun-12",
     "12 Jun 23:01:19 2016",
     "12 Jun, 23:01:19 2016",
     "12 23:01:19 June 2016",
     "12-June 23:01:19 2016",
     "12/6/2016",
     "12/06/2016",
     /*
     "Yesterday",
     "Today",
     "Tomorrow",
     "Now",
     "Last week",
     "Next week",
     "Last five months",
     "last 5 months",
     */
   };

   for (size_t i=0; i<sizeof dates/sizeof dates[0]; i++) {
      time_t result;
      time (&result);
      enum pdate_errcode_t errcode = pdate_parse (dates[i], &result, true);
      printf ("%25s: %10s : %s", dates[i], pdate_errmsg (errcode),
                                 ctime (&result));
   }
}
