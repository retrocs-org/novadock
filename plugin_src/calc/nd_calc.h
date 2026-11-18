#ifndef ND_CALC_H
#define ND_CALC_H
#define ND_CALC_HISTORY 32
struct NDCalcResult {double value;char decimal[48],hex[40],binary[40],error[64];};
int nd_calc_eval(const char *expression,struct NDCalcResult *out);
#endif
