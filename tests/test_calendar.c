#include <assert.h>
#include <stdio.h>
typedef long LONG;
#include "nd_calendar.h"
static int weekday(int year,int month){long n=calendar_days_before(year,month)%7;return (int)(n<0 ? n+7 : n);}
int main(void){int y,m,d;
 assert(weekday(1977,11)==4);assert(weekday(1978,0)==0);
 assert(calendar_month_days(1900,1)==28);assert(calendar_month_days(2000,1)==29);
 calendar_date_from_days(calendar_days_before(2000,1)+28,&y,&m,&d);
 assert(y==2000 && m==1 && d==29);
 puts("Calendar epoch and leap-year regressions passed.");return 0;}
