#ifndef ND_CALENDAR_H
#define ND_CALENDAR_H
static int calendar_leap(int year)
{
    return (year%4==0 && (year%100!=0 || year%400==0));
}
static int calendar_month_days(int year,int month)
{
    static const int days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    if (month==1 && calendar_leap(year)) return 29;
    return days[month];
}
static LONG calendar_days_before(int year,int month)
{
    LONG days=0;
    int y,m;
    for (y=1978;y<year;++y) days+=calendar_leap(y) ? 366 : 365;
    for (y=year;y<1978;++y) days-=calendar_leap(y) ? 366 : 365;
    for (m=0;m<month;++m) days+=calendar_month_days(year,m);
    return days;
}
static void calendar_date_from_days(LONG days,int *year,int *month,int *day)
{
    int y=1978,m=0,n;
    if (days<0) days=0;
    for (;;) {
        n=calendar_leap(y) ? 366 : 365;
        if (days<n) break;
        days-=n;++y;
    }
    while (days>=calendar_month_days(y,m)) {
        days-=calendar_month_days(y,m);++m;
    }
    *year=y;*month=m;*day=(int)days+1;
}
#endif
