#ifndef _STATS_H
#define _STATS_H
struct statistics
{
    long n_requests;
    double currentAverage;

    long opGet;
    long opSize;
    long opPut;
    long opGetKeys;
    long opTablePrint;
    long opDel;
    long opStats;
};
#endif
