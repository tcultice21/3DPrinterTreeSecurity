/***********************************************************************************
* FourQ: 4-dimensional decomposition on a Q-curve with CM in twisted Edwards form
*
*    Copyright (c) Microsoft Corporation. All rights reserved.
*
*
* Abstract: testing code for FourQ's curve arithmetic 
*
* This code is based on the paper "FourQ: four-dimensional decompositions on a 
* Q-curve over the Mersenne prime" by Craig Costello and Patrick Longa, in Advances 
* in Cryptology - ASIACRYPT, 2015.
* Preprint available at http://eprint.iacr.org/2015/565.
************************************************************************************/   

#include "FourQ_internal.h"
#include "test_extras.h"
#include <malloc.h>
#include <stdio.h>


// Benchmark and test parameters 
#define BENCH_LOOPS           1000      // Number of iterations per bench
#define SHORT_BENCH_LOOPS     100       // Number of iterations per bench (for expensive operations)
#define TEST_LOOPS            1000      // Number of iterations per test


bool ecc_test()
{
    bool clear_cofactor, OK = true;
    unsigned int n;
    int passed;
    point_t A;
    vpoint_t VA;
    vpoint_extproj_t VP;
    vpoint_extproj_precomp_t VQ;
    v2elm_t t1;
    uint64_t scalar[4], res_x[4], res_y[4];
    
    printf("\n--------------------------------------------------------------------------------------------------------\n\n"); 
    printf("Testing FourQ's curve arithmetic: \n\n"); 

    // Point doubling
    passed = 1;
    eccset(A, &curve4Q); 
    point_setup(A, VP);

    for (n=0; n<TEST_LOOPS; n++)
    {
        eccdouble(VP);                     // 2*P
    }
    eccnorm(VP, VA);
    from_ext_to_std(VA->x, A->x);
    from_ext_to_std(VA->y, A->y);
    
    // Result
    res_x[0] = 0xC9099C54855859D6; res_x[1] = 0x2C3FD8822C82270F; res_x[2] = 0xA7B3F6E2043E8E68; res_x[3] = 0x4DA5B9E83AA7A1B2;
    res_y[0] = 0x3EE089F0EB49AA14; res_y[1] = 0x2001EB3A57688396; res_y[2] = 0x1FEE5617A7E954CD; res_y[3] = 0x0FFDB0D761421F50;

    if (fp2compare64((uint64_t*)A->x, res_x)!=0 || fp2compare64((uint64_t*)A->y, res_y)!=0) passed=0;
    if (passed==1) printf("  Point doubling tests .................................................................... PASSED");
    else { printf("  Point doubling tests ... FAILED"); printf("\n"); return false; }
    printf("\n");
  
    // Point addition
    eccset(A, &curve4Q); 
    point_setup(A, VP);
    
    for (n=0; n<TEST_LOOPS; n++)
    {
        from_std_to_ext((felm_t*)&curve4Q.d, t1);
        v2mul1271(t1, VP->ta, t1);           // d*ta
        v2add1271(t1, t1, t1);               // 2*d*ta
        v2mul1271(t1, VP->tb, VQ->t2);       // 2*d*t
        v2add1271(VP->x, VP->y, VQ->xy);     // x+y    
        v2sub1271(VP->y, VP->x, VQ->yx);     // y-x
        v2copy1271(VP->z, VQ->z2); 
        v2add1271(VQ->z2, VQ->z2, VQ->z2);   // 2*z
        eccadd(VQ, VP);                      // 2*P
    }
    eccnorm(VP, VA);
    from_ext_to_std(VA->x, A->x);
    from_ext_to_std(VA->y, A->y);

    // Result
    res_x[0] = 0xC9099C54855859D6; res_x[1] = 0x2C3FD8822C82270F; res_x[2] = 0xA7B3F6E2043E8E68; res_x[3] = 0x4DA5B9E83AA7A1B2;
    res_y[0] = 0x3EE089F0EB49AA14; res_y[1] = 0x2001EB3A57688396; res_y[2] = 0x1FEE5617A7E954CD; res_y[3] = 0x0FFDB0D761421F50;

    if (fp2compare64((uint64_t*)A->x, res_x)!=0 || fp2compare64((uint64_t*)A->y, res_y)!=0) passed=0;
    
    eccset(A, &curve4Q); 
    point_setup(A, VP);
    from_std_to_ext((felm_t*)&curve4Q.d, t1);
    v2mul1271(t1, VP->x, t1);                // d*x
    v2add1271(t1, t1, t1);                   // 2*d*x
    v2mul1271(t1, VP->y, VQ->t2);            // 2*d*t
    v2add1271(VP->x, VP->y, VQ->xy);         // x+y    
    v2sub1271(VP->y, VP->x, VQ->yx);         // y-x
    v2zero1271(VQ->z2); VQ->z2[0] = 2;       // 2*z
    eccdouble(VP);                           // P = 2P 

    for (n=0; n<TEST_LOOPS; n++)
    {
        eccadd(VQ, VP);                      // P = P+Q
    }    
    eccnorm(VP, VA);
    from_ext_to_std(VA->x, A->x);
    from_ext_to_std(VA->y, A->y); 

    // Result
    res_x[0] = 0x6480B1EF0A151DB0; res_x[1] = 0x3E243958590C4D90; res_x[2] = 0xAA270F644A65D473; res_x[3] = 0x5327AF7D84238CD0;
    res_y[0] = 0x5E06003D73C43EB1; res_y[1] = 0x3EF69A49CB7E0237; res_y[2] = 0x4E752648AC2EF0AB; res_y[3] = 0x293EB1E26DD23B4E;

    if (fp2compare64((uint64_t*)A->x, res_x)!=0 || fp2compare64((uint64_t*)A->y, res_y)!=0) passed=0;
    
    if (passed==1) printf("  Point addition tests .................................................................... PASSED");
    else { printf("  Point addition tests ... FAILED"); printf("\n"); return false; }
    printf("\n");
   
    // Psi endomorphism
    eccset(A, &curve4Q); 
    point_setup(A, VP);

    for (n=0; n<TEST_LOOPS; n++)
    {
        ecc_psi(VP);                        // P = Psi(P)
    }    
    eccnorm(VP, VA);
    from_ext_to_std(VA->x, A->x);
    from_ext_to_std(VA->y, A->y); 

    // Result
    res_x[0] = 0xD8F3C8C24A2BC7E2; res_x[1] = 0x75AF54EDB41A2B93; res_x[2] = 0x4DE2466701F009A9; res_x[3] = 0x065249F9EDE0C798;
    res_y[0] = 0x1C6E119ADD608104; res_y[1] = 0x06DBB85BFFB7C21E; res_y[2] = 0xFD234D6C4CFA3EC1; res_y[3] = 0x060A30903424BF13;

    if (fp2compare64((uint64_t*)A->x, res_x)!=0 || fp2compare64((uint64_t*)A->y, res_y)!=0) passed=0;

    if (passed==1) printf("  Psi endomorphism tests .................................................................. PASSED");
    else { printf("  Psi endomorphism tests ... FAILED"); printf("\n"); return false; }
    printf("\n");
   
    // Phi endomorphism
    eccset(A, &curve4Q); 
    point_setup(A, VP);

    for (n=0; n<TEST_LOOPS; n++)
    {
        ecc_phi(VP);                        // P = Phi(P)
        eccnorm(VP, VA);
        from_ext_to_std(VA->x, A->x);
        from_ext_to_std(VA->y, A->y); 
        point_setup(A, VP);
    } 

    // Result
    res_x[0] = 0xD5B5A3061287DB16; res_x[1] = 0x5550AAB9E7A620EE; res_x[2] = 0xEC321E6CF33610FC; res_x[3] = 0x3E61EBB9A1CB0210;
    res_y[0] = 0x7E2851D5A8E83FB9; res_y[1] = 0x5474BF8EC55603AE; res_y[2] = 0xA5077613491788D5; res_y[3] = 0x5476093DBF8BF6BF;

    if (fp2compare64((uint64_t*)A->x, res_x)!=0 || fp2compare64((uint64_t*)A->y, res_y)!=0) passed=0;
    if (passed==1) printf("  Phi endomorphism tests .................................................................. PASSED");
    else { printf("  Phi endomorphism tests ... FAILED"); printf("\n"); return false; }
    printf("\n");
    
    // Scalar decomposition and recoding
    {        
    uint64_t acc1, acc2, acc3, acc4, scalars[4];
    unsigned int digits[65], sign_masks[65];
    uint64_t k[4];
    int i;

    for (n=0; n<TEST_LOOPS*10; n++)
    {
        random_scalar_test(k);
        decompose(k, scalars);  
        fp2copy1271((felm_t*)scalars, (felm_t*)scalar);
        recode(scalars, digits, sign_masks); 

        acc1 = acc2 = acc3 = acc4 = 0; 

        for (i = 64; i >= 0; i--)
        {
            acc1 = 2*acc1; acc2 = 2*acc2; acc3 = 2*acc3; acc4 = 2*acc4; 
            if (sign_masks[i] == (unsigned int)-1) {
                acc1 += 1;
                acc2 += (digits[i] & 1);
                acc3 += ((digits[i] >> 1) & 1);
                acc4 += ((digits[i] >> 2) & 1);
            } else if (sign_masks[i] == 0) {
                acc1 -= 1;
                acc2 -= (digits[i] & 1);
                acc3 -= ((digits[i] >> 1) & 1);
                acc4 -= ((digits[i] >> 2) & 1);
            }
        }   
        if (scalar[0] != acc1 || scalar[1] != acc2  || scalar[2] != acc3 || scalar[3] != acc4) { passed=0; break; }
    }
    
    if (passed==1) printf("  Recoding and decomposition tests ........................................................ PASSED");
    else { printf("  Recoding and decomposition tests ... FAILED"); printf("\n"); return false; }
    printf("\n");
    }
    
    // Scalar multiplication
    eccset(A, &curve4Q); 
    clear_cofactor = false;
    scalar[0] = 0x3AD457AB55456230; scalar[1] = 0x3A8B3C2C6FD86E0C; scalar[2] = 0x7E38F7C9CFBB9166; scalar[3] = 0x0028FD6CBDA458F0;
    
    for (n=0; n<TEST_LOOPS; n++)
    {
        scalar[1] = scalar[2];
        scalar[2] += scalar[0];

        ecc_mul(A, (digit_t*)scalar, A, clear_cofactor, &curve4Q);
    }

    res_x[0] = 0xDFD2B477BD494BEF; res_x[1] = 0x257C122BBFC94A1B; res_x[2] = 0x769593547237C459; res_x[3] = 0x469BF80CB5B11F01;
    res_y[0] = 0x281C5067996F3344; res_y[1] = 0x0901B3817C0E936C; res_y[2] = 0x4FE8C429915F1245; res_y[3] = 0x570B948EACACE210;
    if (fp2compare64((uint64_t*)A->x, res_x)!=0 || fp2compare64((uint64_t*)A->y, res_y)!=0) passed=0;
 
    eccset(A, &curve4Q); 
    clear_cofactor = true;
    scalar[0] = 0x3AD457AB55456230; scalar[1] = 0x3A8B3C2C6FD86E0C; scalar[2] = 0x7E38F7C9CFBB9166; scalar[3] = 0x0028FD6CBDA458F0;
    
    for (n=0; n<TEST_LOOPS; n++)
    {
        scalar[1] = scalar[2];
        scalar[2] += scalar[0];

        ecc_mul(A, (digit_t*)scalar, A, clear_cofactor, &curve4Q);
    }

    res_x[0] = 0x85CF54A3BEE3FD23; res_x[1] = 0x7A7EC43976FAAD92; res_x[2] = 0x7697567B785E2327; res_x[3] = 0x4CBDAB448B1539F2;
    res_y[0] = 0xE9193B41CDDF94D0; res_y[1] = 0x5AA6C859ECC810D5; res_y[2] = 0xAA876E760AA8B331; res_y[3] = 0x320C53F02230094A;
    if (fp2compare64((uint64_t*)A->x, res_x)!=0 || fp2compare64((uint64_t*)A->y, res_y)!=0) passed=0;
        
    if (passed==1) printf("  Scalar multiplication tests ............................................................. PASSED");
    else { printf("  Scalar multiplication tests ... FAILED"); printf("\n"); return false; }
    printf("\n"); 
     
#if defined(USE_FIXED_BASE_SM)  
    {    
    point_t A, B, C;    
    digit_t *T_fixed = NULL;
    vpoint_extproj_t VS;
    unsigned int i, j, w, v, e, d, l;
    uint64_t k[4];
    unsigned int digits_fixed[NBITS_ORDER_PLUS_ONE+(W_FIXEDBASE*V_FIXEDBASE)-1] = {0};
    v2elm_t d_curve;
        
    // Scalar recoding using the mLSB-set representation    
    w = W_FIXEDBASE;
    v = V_FIXEDBASE;
    e = E_FIXEDBASE;     
    d = D_FIXEDBASE;                                 
    l = L_FIXEDBASE;      
          
    for (n=0; n<TEST_LOOPS; n++) 
    {
        random_scalar_test(scalar);
        modulo_order((digit_t*)scalar, (digit_t*)scalar, &curve4Q);    // k = k mod (order) 
        conversion_to_odd((digit_t*)scalar, (digit_t*)k, &curve4Q);                       
        for (j = 0; j < NWORDS64_ORDER; j++) scalar[j] = k[j];
        mLSB_set_recode(k, digits_fixed);
            
        if (verify_mLSB_recoding(scalar, (int*)digits_fixed)==false) { passed=0; break; }
    }
    
    if (passed==1) printf("  mLSB-set recoding tests ................................................................. PASSED");
    else { printf("  mLSB-set recoding tests ... FAILED"); printf("\n"); return false; }
    printf("\n");
    
    // Precomputation
    from_std_to_ext((felm_t*)&curve4Q.d, d_curve);

    eccset(A, &curve4Q); 
    point_setup(A, VP);
    ecccopy(VP, VS);
    for (j = 0; j < (w-1); j++) {    // Compute very last point in the table
        for (i = 0; i < d; i++) eccdouble(VS);
        v2add1271(VS->x, VS->y, VQ->xy); v2sub1271(VS->y, VS->x, VQ->yx); v2add1271(VS->z, VS->z, VQ->z2);
        v2mul1271(VS->ta, VS->tb, VQ->t2); v2mul1271(VQ->t2, d_curve, VQ->t2); v2add1271(VQ->t2, VQ->t2, VQ->t2);
        eccadd(VQ, VP);
    }
    for (i = 0; i < e*(v-1); i++) eccdouble(VP);
    eccnorm(VP, VA);
    v2add1271(VA->x, VA->y, VQ->xy); v2sub1271(VA->y, VA->x, VQ->yx);
    v2mod1271(VQ->xy, VQ->xy); 
    v2mod1271(VQ->yx, VQ->yx); 
    from_ext_to_std(VQ->xy, C->x);
    from_ext_to_std(VQ->yx, C->y);
    
    eccset(A, &curve4Q); 
    T_fixed = ecc_allocate_precomp();
    ecc_precomp_fixed(A, T_fixed, false, &curve4Q);
    from_ext_to_std(((vpoint_precomp_t*)T_fixed)[v*(1 << (w-1))-1]->xy, A->x);
    from_ext_to_std(((vpoint_precomp_t*)T_fixed)[v*(1 << (w-1))-1]->yx, A->y);
        
    if (fp2compare64((uint64_t*)A->x,(uint64_t*)C->x)!=0 || fp2compare64((uint64_t*)A->y,(uint64_t*)C->y)!=0) { passed=0; }
    
    if (T_fixed != NULL) {
        free(T_fixed);
    }
    
    eccset(A, &curve4Q); 
    point_setup(A, VP);
    cofactor_clearing(VP, d_curve);
    eccnorm(VP, VA);
    from_ext_to_std(VA->x, A->x);
    from_ext_to_std(VA->y, A->y); 
    point_setup(A, VP);
    ecccopy(VP, VS);
    for (j = 0; j < (w-1); j++) {    // Compute very last point in the table
        for (i = 0; i < d; i++) eccdouble(VS);
        v2add1271(VS->x, VS->y, VQ->xy); v2sub1271(VS->y, VS->x, VQ->yx); v2add1271(VS->z, VS->z, VQ->z2);
        v2mul1271(VS->ta, VS->tb, VQ->t2); v2mul1271(VQ->t2, d_curve, VQ->t2); v2add1271(VQ->t2, VQ->t2, VQ->t2);
        eccadd(VQ, VP);
    }
    for (i = 0; i < e*(v-1); i++) eccdouble(VP);
    eccnorm(VP, VA);
    v2add1271(VA->x, VA->y, VQ->xy); v2sub1271(VA->y, VA->x, VQ->yx);
    v2mod1271(VQ->xy, VQ->xy); 
    v2mod1271(VQ->yx, VQ->yx); 
    from_ext_to_std(VQ->xy, C->x);
    from_ext_to_std(VQ->yx, C->y);
    
    eccset(A, &curve4Q); 
    T_fixed = ecc_allocate_precomp();
    ecc_precomp_fixed(A, T_fixed, true, &curve4Q);
    from_ext_to_std(((vpoint_precomp_t*)T_fixed)[v*(1 << (w-1))-1]->xy, A->x);
    from_ext_to_std(((vpoint_precomp_t*)T_fixed)[v*(1 << (w-1))-1]->yx, A->y);
        
    if (fp2compare64((uint64_t*)A->x,(uint64_t*)C->x)!=0 || fp2compare64((uint64_t*)A->y,(uint64_t*)C->y)!=0) { passed=0; }
    
    if (T_fixed != NULL) {
        free(T_fixed);
    }  
    
    if (passed==1) printf("  Fixed-base precomputation tests ......................................................... PASSED");
    else { printf("  Fixed-base precomputation tests ... FAILED"); printf("\n"); return false; }
    printf("\n");
    
    // Fixed-base scalar multiplication
    
    for (n=0; n<TEST_LOOPS; n++)
    {
        eccset(A, &curve4Q);
        random_scalar_test(scalar); 
        T_fixed = ecc_allocate_precomp();
        ecc_precomp_fixed(A, T_fixed, false, &curve4Q);
        ecc_mul_fixed(T_fixed, (digit_t*)scalar, B, &curve4Q);
        eccset(A, &curve4Q);
        ecc_mul(A, (digit_t*)scalar, C, false, &curve4Q);
        
        if (fp2compare64((uint64_t*)B->x,(uint64_t*)C->x)!=0 || fp2compare64((uint64_t*)B->y,(uint64_t*)C->y)!=0) { passed=0; break; }
    } 
        
    for (n=0; n<TEST_LOOPS; n++)
    {
        eccset(A, &curve4Q);
        random_scalar_test(scalar); 
        T_fixed = ecc_allocate_precomp();
        ecc_precomp_fixed(A, T_fixed, true, &curve4Q);
        ecc_mul_fixed(T_fixed, (digit_t*)scalar, B, &curve4Q);
        eccset(A, &curve4Q);
        ecc_mul(A, (digit_t*)scalar, C, true, &curve4Q);
        
        if (fp2compare64((uint64_t*)B->x,(uint64_t*)C->x)!=0 || fp2compare64((uint64_t*)B->y,(uint64_t*)C->y)!=0) { passed=0; break; }
    } 
    
    if (passed==1) printf("  Fixed-base scalar multiplication tests .................................................. PASSED");
    else { printf("  Fixed-base scalar multiplication tests ... FAILED"); printf("\n"); return false; }
    printf("\n");

    if (T_fixed != NULL) {
        free(T_fixed);
    }  
    }
#endif
         
#if defined(USE_DOUBLE_SM)  
    {    
    point_t PP, QQ, RR, UU, TT;    
    digit_t *P_table = NULL;
    vpoint_extproj_t VS, VT;
    vpoint_extproj_precomp_t VR;
    uint64_t k[4], l[4], scalar[4];
    v2elm_t d_curve;

    // Double scalar multiplication
    from_std_to_ext((felm_t*)&curve4Q.d, d_curve);
    eccset(QQ, &curve4Q); 
    eccset(PP, &curve4Q);
    P_table = ecc_allocate_precomp_double();
    ecc_precomp_double(PP, P_table, &curve4Q);
    
    for (n=0; n<TEST_LOOPS; n++)
    {
        random_scalar_test(scalar); 
        ecc_mul(QQ, (digit_t*)scalar, QQ, false, &curve4Q);
        random_scalar_test(k); 
        random_scalar_test(l); 
        ecc_mul_double(P_table, (digit_t*)k, QQ, (digit_t*)l, RR, &curve4Q);
        ecc_mul(PP, (digit_t*)k, UU, false, &curve4Q);
        ecc_mul(QQ, (digit_t*)l, TT, false, &curve4Q);
    
        point_setup(UU, VS); 
        from_std_to_ext(TT->x, VT->x); 
        from_std_to_ext(TT->y, VT->y); 
        v2add1271(VT->x, VT->y, VR->xy); 
        v2sub1271(VT->y, VT->x, VR->yx); 
        v2zero1271(VR->z2); VR->z2[0] = 2;
        v2mul1271(VT->x, VT->y, VR->t2);    
        v2add1271(VR->t2, VR->t2, VR->t2); 
        v2mul1271(VR->t2, d_curve, VR->t2); 

        eccadd(VR, VS);
        eccnorm(VS, VA);
        from_ext_to_std(VA->x, A->x);
        from_ext_to_std(VA->y, A->y);
        
        if (fp2compare64((uint64_t*)A->x,(uint64_t*)RR->x)!=0 || fp2compare64((uint64_t*)A->y,(uint64_t*)RR->y)!=0) { passed=0; break; }
    }

    if (passed==1) printf("  Double scalar multiplication tests ...................................................... PASSED");
    else { printf("  Double scalar multiplication tests ... FAILED"); printf("\n"); return false; }
    printf("\n");

    if (P_table != NULL) {
        free(P_table);
    }  
    }
#endif
    
    return OK;
}


bool ecc_run()
{
    bool OK = true;
    unsigned int n, i, sign_mask=-1, digit=1;
    unsigned long long nsec, nsec1, nsec2;
    point_t A, B;
    vpoint_extproj_t VP;
    vpoint_extproj_precomp_t VQ, Table[8];
    v2elm_t t1, d;    
    uint64_t scalar[4];
        
    printf("\n--------------------------------------------------------------------------------------------------------\n\n"); 
    printf("Benchmarking FourQ's curve arithmetic \n\n"); 

    // Point doubling
    eccset(A, &curve4Q);
    point_setup(A, VP);

    nsec = 0;
    for (n=0; n<BENCH_LOOPS; n++)
    {
        nsec1 = cpu_nseconds();
        for (i = 0; i < 1000; i++) {
            eccdouble(VP);
        }
        nsec2 = cpu_nseconds();
        nsec = nsec+(nsec2-nsec1);
    }
    printf("  Point doubling runs in ...                                       %8lld nsec", nsec/(BENCH_LOOPS*1000));
    printf("\n");
    
    // Point addition
    eccset(A, &curve4Q); 
    point_setup(A, VP);
    from_std_to_ext((felm_t*)&curve4Q.d, t1);
    v2mul1271(t1, VP->x, t1);                // d*x
    v2add1271(t1, t1, t1);                   // 2*d*x
    v2mul1271(t1, VP->y, VQ->t2);            // 2*d*t
    v2add1271(VP->x, VP->y, VQ->xy);         // x+y    
    v2sub1271(VP->y, VP->x, VQ->yx);         // y-x
    v2zero1271(VQ->z2); VQ->z2[0] = 2;       // 2*z
    eccdouble(VP);                           // P = 2P 

    nsec = 0;
    for (n=0; n<BENCH_LOOPS; n++)
    {
        nsec1 = cpu_nseconds();
        for (i = 0; i < 1000; i++) {
            eccadd(VQ, VP);
        }
        nsec2 = cpu_nseconds();
        nsec = nsec+(nsec2-nsec1);
    }
    printf("  Point addition runs in ...                                       %8lld nsec", nsec/(BENCH_LOOPS*1000));
    printf("\n");
   
    // Psi endomorphism
    eccset(A, &curve4Q);
    point_setup(A, VP);

    nsec = 0;
    for (n=0; n<BENCH_LOOPS; n++)
    {
        nsec1 = cpu_nseconds();
        nsec1 = cpu_nseconds();
        for (i = 0; i < 1000; i++) {
            ecc_psi(VP);
        }
        nsec2 = cpu_nseconds();
        nsec = nsec+(nsec2-nsec1);
    }
    printf("  Psi mapping runs in ...                                          %8lld nsec", nsec/(BENCH_LOOPS*1000));
    printf("\n");
   
    // Phi endomorphism
    eccset(A, &curve4Q);
    point_setup(A, VP);

    nsec = 0;
    for (n=0; n<BENCH_LOOPS; n++)
    {
        nsec1 = cpu_nseconds();
        nsec1 = cpu_nseconds();
        for (i = 0; i < 1000; i++) {
            ecc_phi(VP);
        }
        nsec2 = cpu_nseconds();
        nsec = nsec+(nsec2-nsec1);
    }
    printf("  Phi mapping runs in ...                                          %8lld nsec", nsec/(BENCH_LOOPS*1000));
    printf("\n");
   
    // Scalar decomposition
    {
    uint64_t scalars[4];
    random_scalar_test(scalar); 
    
    nsec = 0;
    for (n=0; n<BENCH_LOOPS; n++)
    {
        nsec1 = cpu_nseconds();
        nsec1 = cpu_nseconds();
        for (i = 0; i < 1000; i++) {
            decompose(scalar, scalars);
        }
        nsec2 = cpu_nseconds();
        nsec = nsec+(nsec2-nsec1);
    }
    
    printf("  Scalar decomposition runs in ...                                 %8lld nsec", nsec/(BENCH_LOOPS*1000));
    printf("\n");
    }

    // Scalar recoding
    {
    unsigned int digits[65], sign_masks[65];
    random_scalar_test(scalar); 

    nsec = 0;
    for (n=0; n<BENCH_LOOPS; n++)
    {
        nsec1 = cpu_nseconds();
        nsec1 = cpu_nseconds();
        for (i = 0; i < 1000; i++) {
            recode(scalar, digits, sign_masks);
        }
        nsec2 = cpu_nseconds();
        nsec = nsec+(nsec2-nsec1);
    }
    
    printf("  Scalar recoding runs in ...                                      %8lld nsec", nsec/(BENCH_LOOPS*1000));
    printf("\n");  
    }

    // Precomputation
    eccset(A, &curve4Q);
    point_setup(A, VP);
    from_std_to_ext((felm_t*)&curve4Q.d, d);

    nsec = 0;
    for (n=0; n<BENCH_LOOPS; n++)
    {
        nsec1 = cpu_nseconds();
        for (i = 0; i < 100; i++) {
            ecc_precomp(VP, Table, d);
        }
        nsec2 = cpu_nseconds();
        nsec = nsec+(nsec2-nsec1);
    }
    
    printf("  Precomputation runs in ...                                       %8lld nsec", nsec/(BENCH_LOOPS*100));
    printf("\n");  

    // Table lookup
    eccset(A, &curve4Q);
    point_setup(A, VP);
    from_std_to_ext((felm_t*)&curve4Q.d, d);
    ecc_precomp(VP, Table, d);

    nsec = 0;
    for (n=0; n<BENCH_LOOPS; n++)
    {
        nsec1 = cpu_nseconds();
        for (i = 0; i < 1000; i++) {
            table_lookup_1x8(Table, VQ, digit, sign_mask);
        }
        nsec2 = cpu_nseconds();
        nsec = nsec+(nsec2-nsec1);
    }    

    printf("  Table lookup runs in ...                                         %8lld nsec", nsec/(BENCH_LOOPS*1000));
    printf("\n");  
    
    // Scalar multiplication
    random_scalar_test(scalar); 
        
    nsec = 0;
    for (n=0; n<SHORT_BENCH_LOOPS; n++)
    {
        eccset(A, &curve4Q);
        nsec1 = cpu_nseconds();
        for (i = 0; i < 100; i++) {
            ecc_mul(A, (digit_t*)scalar, B, false, &curve4Q);
        }
        nsec2 = cpu_nseconds();
        nsec = nsec+(nsec2-nsec1);
    }
    
    printf("  Scalar multiplication (without clearing cofactor) runs in ...... %8lld nsec", nsec/(SHORT_BENCH_LOOPS*100));
    printf("\n"); 
    
    random_scalar_test(scalar); 

    nsec = 0;
    for (n=0; n<SHORT_BENCH_LOOPS; n++)
    {
        eccset(A, &curve4Q);
        nsec1 = cpu_nseconds();
        ecc_mul(A, (digit_t*)scalar, B, true, &curve4Q);
        nsec2 = cpu_nseconds();
        nsec = nsec+(nsec2-nsec1);
    }
    
    printf("  Scalar multiplication (including clearing cofactor) runs in .... %8lld nsec", nsec/SHORT_BENCH_LOOPS);
    printf("\n"); 
     
#if defined(USE_FIXED_BASE_SM)  
    {      
    vpoint_precomp_t T; 
    digit_t *T_fixed = NULL;
    unsigned int digits_fixed[256+(W_FIXEDBASE*V_FIXEDBASE)-1] = {0};
    unsigned int w, v, l, d, e; 

    // Scalar recoding using the mLSB-set representation    
    w = W_FIXEDBASE;
    v = V_FIXEDBASE;
    e = (curve4Q.nbits+w*v-1)/(w*v);            // e = ceil(t/w.v)
    d = e*v;                                 
    l = d*w;                                    // Fixed length of the mLSB-set representation 
    random_scalar_test(scalar); 

    nsec = 0;
    for (n=0; n<SHORT_BENCH_LOOPS; n++)
    {
        nsec1 = cpu_nseconds();
        mLSB_set_recode(scalar, digits_fixed);
        mLSB_set_recode(scalar, digits_fixed);
        mLSB_set_recode(scalar, digits_fixed);
        mLSB_set_recode(scalar, digits_fixed);
        mLSB_set_recode(scalar, digits_fixed);
        mLSB_set_recode(scalar, digits_fixed);
        mLSB_set_recode(scalar, digits_fixed);
        mLSB_set_recode(scalar, digits_fixed);
        mLSB_set_recode(scalar, digits_fixed);
        mLSB_set_recode(scalar, digits_fixed);
        nsec2 = cpu_nseconds();
        nsec = nsec+(nsec2-nsec1);
    }
    
    printf("  Fixed-base recoding runs in ...                                  %8lld nsec", nsec/(SHORT_BENCH_LOOPS*10));
    printf("\n"); 

    // Table lookup for fixed-base scalar multiplication
    eccset(A, &curve4Q);
    T_fixed = ecc_allocate_precomp();
    ecc_precomp_fixed(A, T_fixed, false, &curve4Q);

    nsec = 0;
    for (n=0; n<BENCH_LOOPS; n++)
    {
        nsec1 = cpu_nseconds();
        table_lookup_fixed_base((vpoint_precomp_t*)T_fixed, T, 1, 0);
        table_lookup_fixed_base((vpoint_precomp_t*)T_fixed, T, 2, (unsigned int)-1);
        table_lookup_fixed_base((vpoint_precomp_t*)T_fixed, T, 1, 0);
        table_lookup_fixed_base((vpoint_precomp_t*)T_fixed, T, 2, (unsigned int)-1);
        table_lookup_fixed_base((vpoint_precomp_t*)T_fixed, T, 1, 0);
        table_lookup_fixed_base((vpoint_precomp_t*)T_fixed, T, 2, (unsigned int)-1);
        table_lookup_fixed_base((vpoint_precomp_t*)T_fixed, T, 1, 0);
        table_lookup_fixed_base((vpoint_precomp_t*)T_fixed, T, 2, (unsigned int)-1);
        table_lookup_fixed_base((vpoint_precomp_t*)T_fixed, T, 1, 0);
        table_lookup_fixed_base((vpoint_precomp_t*)T_fixed, T, 2, (unsigned int)-1);
        nsec2 = cpu_nseconds();
        nsec = nsec+(nsec2-nsec1);
    }
    if (T_fixed != NULL) {
        free(T_fixed);
    }
    
    printf("  Fixed-base table lookup runs in ...                              %8lld nsec", nsec/(BENCH_LOOPS*10));
    printf("\n");   

    // Fixed-base scalar multiplication  
    eccset(A, &curve4Q);
    T_fixed = ecc_allocate_precomp();
    ecc_precomp_fixed(A, T_fixed, false, &curve4Q);

    nsec = 0;
    for (n=0; n<SHORT_BENCH_LOOPS; n++)
    {        
        random_scalar_test(scalar); 
        nsec1 = cpu_nseconds();
        ecc_mul_fixed(T_fixed, (digit_t*)scalar, B, &curve4Q);
        nsec2 = cpu_nseconds();
        nsec = nsec+(nsec2-nsec1);
    } 
    if (T_fixed != NULL) {
        free(T_fixed);
    }
    
    printf("  Fixed-base scalar mul runs in ...                                %8lld nsec with w=%d and v=%d", nsec/SHORT_BENCH_LOOPS, W_FIXEDBASE, V_FIXEDBASE);
    printf("\n"); 
    }
#endif   
             
#if defined(USE_DOUBLE_SM)  
    {    
    point_t PP, QQ, RR;    
    digit_t *P_table = NULL;
    uint64_t k[4], l[4], scalar[4];

    // Double scalar multiplication
    eccset(QQ, &curve4Q); 
    eccset(PP, &curve4Q);
    P_table = ecc_allocate_precomp_double();
    ecc_precomp_double(PP, P_table, &curve4Q);
    random_scalar_test(scalar); 
    ecc_mul(QQ, (digit_t*)scalar, QQ, false, &curve4Q);
    
    nsec = 0;
    for (n=0; n<SHORT_BENCH_LOOPS; n++)
    {        
        random_scalar_test(k); 
        random_scalar_test(l);  
        nsec1 = cpu_nseconds();
        ecc_mul_double(P_table, (digit_t*)k, QQ, (digit_t*)l, RR, &curve4Q);
        nsec2 = cpu_nseconds();
        nsec = nsec+(nsec2-nsec1);
    }

    if (P_table != NULL) {
        free(P_table);
    }  
    
    printf("  Double scalar mul runs in ...                                    %8lld nsec with wP=%d and wQ=%d", nsec/SHORT_BENCH_LOOPS, WP_DOUBLEBASE, WQ_DOUBLEBASE);
    printf("\n"); 
    }
#endif 
    
    return OK;
} 


int main()
{
    bool OK = true;

    OK = OK && ecc_test();         // Test FourQ's curve functions
    OK = OK && ecc_run();          // Benchmark FourQ's curve functions
    
    return OK;
}
