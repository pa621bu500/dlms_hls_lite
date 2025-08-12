#ifndef POLL_RESULT_H
#define POLL_RESULT_H

typedef struct
{
   char meter_identity[16];
   // char timestamp[19];
   struct tm *t;
   double kwh;
   int relay_status;
   int parse_stage;
} t_poll_result;

#endif