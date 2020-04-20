#ifndef _PAW_PAULI_H_
#define _PAW_PAULI_H_

/*
   $Id: paw_pauli.h 25011 2013-12-19 17:45:25Z d3y133 $
*/


extern int   paw_R_Pauli(int n,
                                   int l,
                                   double  Z,
                                   double* v,
                                   double *Eig,
                                   double* u,
                                   double* uprime);

extern  int paw_R_Pauli_Fixed_E(
        int n,
        int l,
        double Z,
        double *v,
        int match,
        double E,
        double *u,
        double *uprime
    );



#endif



