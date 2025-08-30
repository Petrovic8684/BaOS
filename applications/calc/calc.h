#ifndef CALC_H
#define CALC_H

typedef enum
{
    CALC_OK = 0,
    CALC_ERR_DIV0,
    CALC_ERR_SYNTAX
} CalcStatus;

typedef struct
{
    double value;
    CalcStatus status;
} CalcResult;

#endif
