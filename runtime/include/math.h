#ifndef MATH_H
#define MATH_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif

int isnan(double x);
int isinf(double x);
int isfinite(double x);

double fabs(double x);
double ceil(double x);
double floor(double x);

double ldexp(double x, int exp);
double frexp(double x, int *exp);
double modf(double x, double *intpart);

double fmod(double x, double y);

double sqrt(double x);
double pow(double x, double y);

double exp(double x);
double log(double x);
double log10(double x);

double sin(double x);
double cos(double x);
double tan(double x);
double asin(double x);
double acos(double x);
double atan(double x);
double atan2(double y, double x);

double sinh(double x);
double cosh(double x);
double tanh(double x);

double hypot(double x, double y);

double acosh(double x);
double asinh(double x);
double atanh(double x);

double log1p(double x);
double expm1(double x);

double cbrt(double x);
double round(double x);
double trunc(double x);
double fma(double x, double y, double z);
double nan(const char *tagp);
double inf(int sign);
double fmax(double x, double y);
double fmin(double x, double y);

#endif