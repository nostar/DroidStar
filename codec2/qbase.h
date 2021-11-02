#ifndef QBASE_H
#define QBASE_H

#include "defines.h"

#define WO_BITS     7
#define WO_LEVELS   (1<<WO_BITS)
#define WO_DT_BITS  3

#define E_BITS      5
#define E_LEVELS    (1<<E_BITS)
#define E_MIN_DB   -10.0
#define E_MAX_DB    40.0

#define LSP_SCALAR_INDEXES    10
#define LSPD_SCALAR_INDEXES    10
#define LSP_PRED_VQ_INDEXES    3

#define WO_E_BITS   8

#define LPCPF_GAMMA 0.5
#define LPCPF_BETA  0.2

class CQbase {
public:
	int encode_WoE(MODEL *model, float e, float xq[]);
	void decode_WoE(C2CONST *c2const, MODEL *model, float *e, float xq[], int n1);
	int encode_log_Wo(C2CONST *c2const, float Wo, int bits);
	float decode_log_Wo(C2CONST *c2const, int index, int bits);
protected:
	long quantise(const float * cb, float vec[], float w[], int k, int m, float *se);
	void compute_weights2(const float *x, const float *xp, float *w);
	int find_nearest_weighted(const float *codebook, int nb_entries, float *x, const float *w, int ndim);

	const float ge_coeff[2] = { 0.8, 0.9 };

};

#endif
