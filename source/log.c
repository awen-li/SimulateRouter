/***********************************************************
 * Author: Wen Li
 * Date  : 04/03/2020
 * Describe: debug logging
 * History:
   <1> 04/03/2020 , create
************************************************************/
#include "macro.h"
#include <stdarg.h>

void Log(const char* Format, ...)
{
    char Log[1024] = {0};

    va_list ap;
    va_start(ap, Format);
    (void)vsnprintf (Log, sizeof(Log)-1, Format, ap);
    va_end(ap);
 
    FILE *fd = fopen("route.log", "a");
    if (fd != NULL)
    {
        time_t Now;
        time(&Now);

        char TimeBuf[64];
        sprintf(TimeBuf, "%s", ctime(&Now));
        TimeBuf[strlen(TimeBuf)-1] = 0;
 
        fprintf(fd, "[%s]%s\r\n", TimeBuf, Log);
        fflush(fd);
        fclose(fd);

        printf("[%s]%s\r\n", TimeBuf, Log);
    }
}



