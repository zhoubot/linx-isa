
#include "common.h"
#include "array_defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

void set_1d_array(real_t * arr, int length, real_t value, int stride);
void set_2d_array(real_t arr[LEN_2D][LEN_2D], real_t value, int stride);

enum {SET1D_RECIP_IDX = -1, SET1D_RECIP_IDX_SQ = -2};

real_t sum1d(real_t arr[LEN_1D]);
real_t sum2d(real_t arr[LEN_2D][LEN_2D]);

real_t sum_x();
real_t sum_a();
real_t sum_b();
real_t sum_c();
real_t sum_e();

real_t sum_half_xx();

real_t sum_a_aa();

real_t sum_aa();
real_t sum_bb();
real_t sum_cc();
real_t sum_xx();

real_t sum_aa_bb();

real_t sum_flat_2d_array();

real_t sum1d(real_t arr[LEN_1D]){
    real_t ret = 0.;
    for (int i = 0; i < LEN_1D; i++)
        ret += arr[i];
    return ret;
}

real_t sum2d(real_t arr[LEN_2D][LEN_2D]){
    real_t sum = 0.;
    for (int i = 0; i < LEN_2D; i++){
        for (int j = 0; j < LEN_2D; j++){
            sum += arr[i][j];
        }
    }

    return sum;
}

real_t sum_x()
{
    return sum1d(x);
}

real_t sum_xx()
{
    return sum1d(xx);
}

real_t sum_a()
{
    return sum1d(a);
}

real_t sum_b()
{
    return sum1d(b);
}

real_t sum_a_aa()
{
    return sum1d(a) + sum2d(aa);
}

real_t sum_c()
{
    return sum1d(c);
}

real_t sum_e()
{
    return sum1d(e);
}

real_t sum_aa()
{
    return sum2d(aa);
}

real_t sum_bb()
{
    return sum2d(bb);
}

real_t sum_aa_bb()
{
    return sum2d(aa) + sum2d(bb);
}

real_t sum_cc()
{
    return sum2d(cc);
}

real_t sum_half_xx()
{
    real_t temp = 00;

    for (int i = 0; i < LEN_1D/2; i++){
        temp += xx[i];
    }

    return temp;
}

real_t sum_flat_2d_array()
{
    real_t sum = 0.;

    for (int i = 0; i < LEN_2D*LEN_2D; i++){
        sum += flat_2d_array[i];
    }

    return sum;
}


void set_1d_array(real_t * arr, int length, real_t value, int stride)
{
    if (stride == SET1D_RECIP_IDX) {
        for (int i = 0; i < length; i++) {
            arr[i] = 1. / (real_t) (i+1);
        }
    } else if (stride == SET1D_RECIP_IDX_SQ) {
        for (int i = 0; i < length; i++) {
            arr[i] = 1. / (real_t) ((i+1) * (i+1));
        }
    } else {
        for (int i = 0; i < length; i += stride) {
            arr[i] = value;
        }
    }
}

void set_2d_array(real_t arr[LEN_2D][LEN_2D], real_t value, int stride)
{
    for (int i = 0; i < LEN_2D; i++) {
        set_1d_array(arr[i], LEN_2D, value, stride);
    }
}

void init(int** ip, real_t* s1, real_t* s2){
    xx = (real_t*) memalign(ARRAY_ALIGNMENT, LEN_1D*sizeof(real_t));
    *ip = (int *) memalign(ARRAY_ALIGNMENT, LEN_1D*sizeof(real_t));

    for (int i = 0; i < LEN_1D; i = i+5){
        (*ip)[i]   = (i+4);
        (*ip)[i+1] = (i+2);
        (*ip)[i+2] = (i);
        (*ip)[i+3] = (i+3);
        (*ip)[i+4] = (i+1);
    }

    set_1d_array(a, LEN_1D, 1.,1);
    set_1d_array(b, LEN_1D, 1.,1);
    set_1d_array(c, LEN_1D, 1.,1);
    set_1d_array(d, LEN_1D, 1.,1);
    set_1d_array(e, LEN_1D, 1.,1);
    set_1d_array(x, LEN_1D, 1.,1);
    set_2d_array(aa, 0.,SET1D_RECIP_IDX);
    set_2d_array(bb, 0.,SET1D_RECIP_IDX);
    set_2d_array(cc, 0.,SET1D_RECIP_IDX);

    for (int i = 0; i < LEN_1D; i++) {
        indx[i] = (i+1) % 4+1;
    }

    *s1 = 1.0;
    *s2 = 2.0;
}

int initialise_arrays(const char* name)
{
    real_t any=0.;
    real_t zero=0.;
    real_t half=.5;
    real_t one=1.;
    real_t two=2.;
    real_t small = .000001;
    int unit =1;
    int frac = SET1D_RECIP_IDX;
    int frac2 = SET1D_RECIP_IDX_SQ;

    printf("%5s\t", name);

    if    (!strcmp(name, "s000")) {
      for (int i = 0; i < LEN_1D; i++) {
            a[i] = 1+i;
            b[i] = 2+i;
            c[i] = 3+i;
            d[i] = 4+i;
            e[i] = 5+i;
          }
    } else if (!strcmp(name, "s111")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
        set_1d_array(c, LEN_1D, any,frac2);
        set_1d_array(d, LEN_1D, any,frac2);
        set_1d_array(e, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s112")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s113")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s114")) {
        set_2d_array(aa, any,frac);
        set_2d_array(bb, any,frac2);
    } else if (!strcmp(name, "s115")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_2d_array(aa,small,unit);
        set_2d_array(bb,small,unit);
        set_2d_array(cc,small,unit);
    } else if (!strcmp(name, "s116")) {
        set_1d_array(a, LEN_1D, one,unit);
    } else if (!strcmp(name, "s118")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_2d_array(bb,small,unit);
    } else if (!strcmp(name, "s119")) {
        set_2d_array(aa, one,unit);
        set_2d_array(bb, any,frac2);
    } else if (!strcmp(name, "s121")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s122")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s123")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, one,unit);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s124")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, one,unit);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s125")) {
        set_1d_array(flat_2d_array, LEN_2D*LEN_2D,zero,unit);
        set_2d_array(aa, one,unit);
        set_2d_array(bb,half,unit);
        set_2d_array(cc, two,unit);
    } else if (!strcmp(name, "s126")) {
        set_2d_array(bb, one,unit);
        set_1d_array( flat_2d_array, LEN_2D*LEN_2D,any,frac);
        set_2d_array(cc, any,frac);
    } else if (!strcmp(name, "s127")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s128")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, two,unit);
        set_1d_array(c, LEN_1D, one,unit);
        set_1d_array(d, LEN_1D, one,unit);
    } else if (!strcmp(name, "s131")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s132")) {
        set_2d_array(aa, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
    } else if (!strcmp(name, "s141")) {
        set_1d_array( flat_2d_array, LEN_2D*LEN_2D, one,unit);
        set_2d_array(bb, any,frac2);
    } else if (!strcmp(name, "s151")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s152")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D,zero,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s161")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array( &b[0], LEN_1D/2, one,2);
        set_1d_array( &b[1], LEN_1D/2,-one,2);
        set_1d_array(c, LEN_1D, one,unit);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s162")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
    } else if (!strcmp(name, "s171")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s172")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s173")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s174")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s175")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s176")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
    } else if (!strcmp(name, "s211")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s212")) {
        set_1d_array(a, LEN_1D, any,frac);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, one,unit);
        set_1d_array(d, LEN_1D, any,frac);
    } else if (!strcmp(name, "s221")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
    } else if (!strcmp(name, "s222")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, one,unit);
    } else if (!strcmp(name, "s231")) {
        set_2d_array(aa, one,unit);
        set_2d_array(bb, any,frac2);
    } else if (!strcmp(name, "s232")) {
        set_2d_array(aa, one,unit);
        set_2d_array(bb,zero,unit);
    } else if (!strcmp(name, "s233")) {
        set_2d_array(aa, any,frac);
        set_2d_array(bb, any,frac);
        set_2d_array(cc, any,frac);
    } else if (!strcmp(name, "s234")) {
        set_2d_array(aa, one,unit);
        set_2d_array(bb, any,frac);
        set_2d_array(cc, any,frac);
    } else if (!strcmp(name, "s235")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
        set_2d_array(aa, one,unit);
        set_2d_array(bb, any, frac2);
    } else if (!strcmp(name, "s241")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, one,unit);
        set_1d_array(d, LEN_1D, one,unit);
    } else if (!strcmp(name, "s242")) {
        set_1d_array(a, LEN_1D,small,unit);
        set_1d_array(b, LEN_1D,small,unit);
        set_1d_array(c, LEN_1D,small,unit);
        set_1d_array(d, LEN_1D,small,unit);
    } else if (!strcmp(name, "s243")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s244")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D,small,unit);
        set_1d_array(d, LEN_1D,small,unit);
    } else if (!strcmp(name, "s251")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s252")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, one,unit);
    } else if (!strcmp(name, "s253")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D,small,unit);
        set_1d_array(c, LEN_1D, one,unit);
        set_1d_array(d, LEN_1D, any,frac);
    } else if (!strcmp(name, "s254")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
    } else if (!strcmp(name, "s255")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
    } else if (!strcmp(name, "s256")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_2d_array(aa, two,unit);
        set_2d_array(bb, one,unit);
    } else if (!strcmp(name, "s257")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_2d_array(aa, two,unit);
        set_2d_array(bb, one,unit);
    } else if (!strcmp(name, "s258")) {
        set_1d_array(a, LEN_1D, any,frac);
        set_1d_array(b, LEN_1D,zero,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D,zero,unit);
        set_2d_array(aa, any,frac);
    } else if (!strcmp(name, "s261")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
        set_1d_array(c, LEN_1D, any,frac2);
        set_1d_array(d, LEN_1D, one,unit);
    } else if (!strcmp(name, "s271")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
    } else if (!strcmp(name, "s272")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, two,unit);
    } else if (!strcmp(name, "s273")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, one,unit);
        set_1d_array(d, LEN_1D,small,unit);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s274")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, one,unit);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s275")) {
        set_2d_array(aa, one,unit);
        set_2d_array(bb,small,unit);
        set_2d_array(cc,small,unit);
    } else if (!strcmp(name, "s276")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
    } else if (!strcmp(name, "s277")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array( b, LEN_1D/2, one,unit);
        set_1d_array( &b[LEN_1D/2], LEN_1D/2,-one,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s278")) {
        set_1d_array( a, LEN_1D/2,-one,unit);
        set_1d_array( &a[LEN_1D/2], LEN_1D/2,one,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s279")) {
        set_1d_array( a, LEN_1D/2,-one,unit);
        set_1d_array( &a[LEN_1D/2], LEN_1D/2,one,unit);
//        set_1d_array(a, LEN_1D, -one,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s2710")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s2711")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
    } else if (!strcmp(name, "s2712")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
    } else if (!strcmp(name, "s281")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, one,unit);
    } else if (!strcmp(name, "1s281")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, one,unit);
        set_1d_array(d, LEN_1D, one,unit);
        set_1d_array(e, LEN_1D, one,unit);
        set_1d_array(x, LEN_1D, one,unit);
    } else if (!strcmp(name, "s291")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
    } else if (!strcmp(name, "s292")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
    } else if (!strcmp(name, "s293")) {
        set_1d_array(a, LEN_1D, any,frac);
    } else if (!strcmp(name, "s2101")) {
        set_2d_array(aa, one,unit);
        set_2d_array(bb, any,frac);
        set_2d_array(cc, any,frac);
    } else if (!strcmp(name, "s2102")) {
        set_2d_array(aa,zero,unit);
    } else if (!strcmp(name, "s2111")) {
        set_2d_array(aa, small,unit);
    } else if (!strcmp(name, "s311")) {
        set_1d_array(a, LEN_1D, any,frac);
    } else if (!strcmp(name, "s312")) {
        set_1d_array(a, LEN_1D,1.000001,unit);
    } else if (!strcmp(name, "s313")) {
        set_1d_array(a, LEN_1D, any,frac);
        set_1d_array(b, LEN_1D, any,frac);
    } else if (!strcmp(name, "s314")) {
        set_1d_array(a, LEN_1D, any,frac);
    } else if (!strcmp(name, "s315")) {
        set_1d_array(a, LEN_1D, any,frac);
    } else if (!strcmp(name, "s316")) {
        set_1d_array(a, LEN_1D, any,frac);
    } else if (!strcmp(name, "s317")) {
    } else if (!strcmp(name, "s318")) {
        set_1d_array(a, LEN_1D, any,frac);
        a[LEN_1D-1] = -two;
    } else if (!strcmp(name, "s319")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D,zero,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s3110")) {
        set_2d_array(aa, any,frac);
        aa[LEN_2D-1][LEN_2D-1] = two;
    } else if (!strcmp(name, "s3111")) {
        set_1d_array(a, LEN_1D, any,frac);
    } else if (!strcmp(name, "s3112")) {
        set_1d_array(a, LEN_1D, any,frac2);
        set_1d_array(b, LEN_1D,zero,unit);
    } else if (!strcmp(name, "s3113")) {
        set_1d_array(a, LEN_1D, any,frac);
        a[LEN_1D-1] = -two;
    } else if (!strcmp(name, "s321")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D,zero,unit);
    } else if (!strcmp(name, "s322")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D,zero,unit);
        set_1d_array(c, LEN_1D,zero,unit);
    } else if (!strcmp(name, "s323")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s331")) {
        set_1d_array(a, LEN_1D, any,frac);
        a[LEN_1D-1] = -one;
    } else if (!strcmp(name, "s332")) {
        set_1d_array(a, LEN_1D, any,frac2);
        a[LEN_1D-1] = two;
    } else if (!strcmp(name, "s341")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, any,frac);
    } else if (!strcmp(name, "s342")) {
        set_1d_array(a, LEN_1D, any,frac);
        set_1d_array(b, LEN_1D, any,frac);
    } else if (!strcmp(name, "s343")) {
        set_2d_array(aa, any,frac);
        set_2d_array(bb, one,unit);
    } else if (!strcmp(name, "s351")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, one,unit);
        c[0] = 1.;
    } else if (!strcmp(name, "s352")) {
        set_1d_array(a, LEN_1D, any,frac);
        set_1d_array(b, LEN_1D, any,frac);
    } else if (!strcmp(name, "s353")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, one,unit);
        c[0] = 1.;
    } else if (!strcmp(name, "s411")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
    } else if (!strcmp(name, "s412")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
    } else if (!strcmp(name, "s413")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, one,unit);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s414")) {
        set_2d_array(aa, one,unit);
        set_2d_array(bb, any,frac);
        set_2d_array(cc, any,frac);
    } else if (!strcmp(name, "s415")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
        a[LEN_1D-1] = -one;
    } else if (!strcmp(name, "s421")) {
        set_1d_array(a, LEN_1D, any,frac2);
        set_1d_array(flat_2d_array, LEN_1D, one, unit);
    } else if (!strcmp(name, "s422")) {
        set_1d_array(flat_2d_array, LEN_1D,one,unit);
        set_1d_array(a, LEN_1D, any,frac2);
        set_1d_array(flat_2d_array, LEN_1D, zero, unit);
    } else if (!strcmp(name, "s1421")) {
        set_1d_array(b, LEN_1D, one, unit);
    } else if (!strcmp(name, "s423")) {
        set_1d_array(flat_2d_array, LEN_1D,zero,unit);
        set_1d_array(a, LEN_1D, any,frac2);
        set_1d_array(flat_2d_array, LEN_1D, one, unit);
    } else if (!strcmp(name, "s424")) {
        set_1d_array(flat_2d_array, LEN_1D,one,unit);
        set_1d_array(a, LEN_1D, any,frac2);
        set_1d_array(flat_2d_array, LEN_1D, zero, unit);
    } else if (!strcmp(name, "s431")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s432")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s441")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(&d[0],             LEN_1D/3  , -one,unit);
        set_1d_array(&d[LEN_1D/3],      LEN_1D/3  , zero,unit);
        set_1d_array(&d[(2*LEN_1D/3)],  LEN_1D/3+1, one,unit);
    } else if (!strcmp(name, "s442")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
    } else if (!strcmp(name, "s443")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
    } else if (!strcmp(name, "s451")) {
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
    } else if (!strcmp(name, "s452")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D,small,unit);
    } else if (!strcmp(name, "s453")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s471")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, one,unit);
        set_1d_array(d, LEN_1D, any,frac);
        set_1d_array(e, LEN_1D, any,frac);
        set_1d_array(x, LEN_1D, zero, unit);
    } else if (!strcmp(name, "s481")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
    } else if (!strcmp(name, "s482")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
    } else if (!strcmp(name, "s491")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
    } else if (!strcmp(name, "s4112")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
    } else if (!strcmp(name, "s4113")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, any,frac2);
    } else if (!strcmp(name, "s4114")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
    } else if (!strcmp(name, "s4115")) {
        set_1d_array(a, LEN_1D, any,frac);
        set_1d_array(b, LEN_1D, any,frac);
    } else if (!strcmp(name, "s4116")) {
        set_1d_array(a, LEN_1D, any,frac);
        set_2d_array(aa, any,frac);
    } else if (!strcmp(name, "s4117")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D, any,frac);
        set_1d_array(d, LEN_1D, any,frac);
    } else if (!strcmp(name, "s4121")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
    } else if (!strcmp(name, "va")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "vag")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "vas")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "vif")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "vpv")) {
        set_1d_array(a, LEN_1D,zero,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "vtv")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, one,unit);
    } else if (!strcmp(name, "vpvtv")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, any,frac);
    } else if (!strcmp(name, "vpvts")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, any,frac2);
    } else if (!strcmp(name, "vpvpv")) {
        set_1d_array(a, LEN_1D, any,frac2);
        set_1d_array(b, LEN_1D, one,unit);
        set_1d_array(c, LEN_1D,-one,unit);
    } else if (!strcmp(name, "vtvtv")) {
        set_1d_array(a, LEN_1D, one,unit);
        set_1d_array(b, LEN_1D, two,unit);
        set_1d_array(c, LEN_1D,half,unit);
    } else if (!strcmp(name, "vsumr")) {
        set_1d_array(a, LEN_1D, any,frac);
    } else if (!strcmp(name, "vdotr")) {
        set_1d_array(a, LEN_1D, any,frac);
        set_1d_array(b, LEN_1D, any,frac);
    } else if (!strcmp(name, "vbor")) {
        set_1d_array(a, LEN_1D, any,frac);
        set_1d_array(b, LEN_1D, any,frac);
        set_1d_array(c, LEN_1D, one,frac);
        set_1d_array(d, LEN_1D, two,frac);
        set_1d_array(e, LEN_1D,half,frac);
        set_2d_array(aa, any,frac);
    } else {
    }

    return 0;
}

real_t calc_checksum(const char * name)
{
    if (!strcmp(name, "s000")) {
        return sum_a();
    } else if (!strcmp(name, "s111")) {
        return sum_a();
    } else if (!strcmp(name, "s1111")) {
        return sum_a();
    } else if (!strcmp(name, "s112")) {
        return sum_a();
    } else if (!strcmp(name, "s1112")) {
        return sum_a();
    } else if (!strcmp(name, "s113")) {
        return sum_a();
    } else if (!strcmp(name, "s1113")) {
        return sum_a();
    } else if (!strcmp(name, "s114")) {
        return sum_aa();
    } else if (!strcmp(name, "s115")) {
        return sum_a();
    } else if (!strcmp(name, "s1115")) {
        return sum_aa();
    } else if (!strcmp(name, "s116")) {
        return sum_a();
    } else if (!strcmp(name, "s118")) {
        return sum_a();
    } else if (!strcmp(name, "s119")) {
        return sum_aa();
    } else if (!strcmp(name, "s1119")) {
        return sum_aa();
    } else if (!strcmp(name, "s121")) {
        return sum_a();
    } else if (!strcmp(name, "s122")) {
        return sum_a();
    } else if (!strcmp(name, "s123")) {
        return sum_a();
    } else if (!strcmp(name, "s124")) {
        return sum_a();
    } else if (!strcmp(name, "s125")) {
        return sum_flat_2d_array();
    } else if (!strcmp(name, "s126")) {
        return sum_bb();
    } else if (!strcmp(name, "s127")) {
        return sum_a();
    } else if (!strcmp(name, "s128")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s131")) {
        return sum_a();
    } else if (!strcmp(name, "s132")) {
        return sum_aa();
    } else if (!strcmp(name, "s141")) {
        return sum_flat_2d_array();
    } else if (!strcmp(name, "s151")) {
        return sum_a();
    } else if (!strcmp(name, "s152")) {
        return sum_a();
    } else if (!strcmp(name, "s161")) {
        return sum_a() + sum_c();
    } else if (!strcmp(name, "s1161")) {
        return sum_a() + sum_c();
    } else if (!strcmp(name, "s162")) {
        return sum_a();
    } else if (!strcmp(name, "s171")) {
        return sum_a();
    } else if (!strcmp(name, "s172")) {
        return sum_a();
    } else if (!strcmp(name, "s173")) {
        return sum_a();
    } else if (!strcmp(name, "s174")) {
        return sum_a();
    } else if (!strcmp(name, "s175")) {
        return sum_a();
    } else if (!strcmp(name, "s176")) {
        return sum_a();
    } else if (!strcmp(name, "s211")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s212")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s1213")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s221")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s1221")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s222")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s231")) {
        return sum_aa();
    } else if (!strcmp(name, "s232")) {
        return sum_aa();
    } else if (!strcmp(name, "s1232")) {
        return sum_aa();
    } else if (!strcmp(name, "s233")) {
        return sum_aa_bb();
    } else if (!strcmp(name, "s2233")) {
        return sum_aa_bb();
    } else if (!strcmp(name, "s235")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s241")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s242")) {
        return sum_a();
    } else if (!strcmp(name, "s243")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s244")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s1244")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s2244")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s251")) {
        return sum_a();
    } else if (!strcmp(name, "s1251")) {
        return sum_a();
    } else if (!strcmp(name, "s2251")) {
        return sum_a();
    } else if (!strcmp(name, "s3251")) {
        return sum_a();
    } else if (!strcmp(name, "s252")) {
        return sum_a();
    } else if (!strcmp(name, "s253")) {
        return sum_a() + sum_c();
    } else if (!strcmp(name, "s254")) {
        return sum_a();
    } else if (!strcmp(name, "s255")) {
        return sum_a();
    } else if (!strcmp(name, "s256")) {
        return sum_a_aa();
    } else if (!strcmp(name, "s257")) {
        return sum_a_aa();
    } else if (!strcmp(name, "s258")) {
        return sum_b() + sum_e();
    } else if (!strcmp(name, "s261")) {
        return sum_a() + sum_c();
    } else if (!strcmp(name, "s271")) {
        return sum_a();
    } else if (!strcmp(name, "s272")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s273")) {
        return sum_a() + sum_b() + sum_c();
    } else if (!strcmp(name, "s274")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s275")) {
        return sum_aa();
    } else if (!strcmp(name, "s2275")) {
        return sum_aa();
    } else if (!strcmp(name, "s276")) {
        return sum_a();
    } else if (!strcmp(name, "s277")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s278")) {
        return sum_a() + sum_b() + sum_c();
    } else if (!strcmp(name, "s279")) {
        return sum_a() + sum_b() + sum_c();
    } else if (!strcmp(name, "s1279")) {
        return sum_a() + sum_b() + sum_c();
    } else if (!strcmp(name, "s2710")) {
        return sum_a() + sum_b() + sum_c();
    } else if (!strcmp(name, "s2711")) {
        return sum_a();
    } else if (!strcmp(name, "s2712")) {
        return sum_a();
    } else if (!strcmp(name, "s281")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s1281")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s291")) {
        return sum_a();
    } else if (!strcmp(name, "s292")) {
        return sum_a();
    } else if (!strcmp(name, "s293")) {
        return sum_a();
    } else if (!strcmp(name, "s2101")) {
        return sum_aa();
    } else if (!strcmp(name, "s2102")) {
        return sum_aa();
    } else if (!strcmp(name, "s2111")) {
        return sum_aa();
    } else if (!strcmp(name, "s311")) {
        return sum_a();
    } else if (!strcmp(name, "s31111")) {
        return sum_a();
    } else if (!strcmp(name, "s321")) {
        return sum_a();
    } else if (!strcmp(name, "s322")) {
        return sum_a();
    } else if (!strcmp(name, "s323")) {
        return sum_a() + sum_b();
    } else if (!strcmp(name, "s341")) {
        return sum_a();
    } else if (!strcmp(name, "s342")) {
        return sum_a();
    } else if (!strcmp(name, "s343")) {
        return sum_flat_2d_array();
    } else if (!strcmp(name, "s351")) {
        return sum_a();
    } else if (!strcmp(name, "s1351")) {
        return sum_a();
    } else if (!strcmp(name, "s353")) {
        return sum_a();
    } else if (!strcmp(name, "s421")) {
        return sum_xx();
    } else if (!strcmp(name, "s1421")) {
        return sum_half_xx();
    } else if (!strcmp(name, "s422")) {
        return sum_xx();
    } else if (!strcmp(name, "s423")) {
        return sum_flat_2d_array();
    } else if (!strcmp(name, "s424")) {
        return sum_xx();
    } else if (!strcmp(name, "s431")) {
        return sum_a();
    } else if (!strcmp(name, "s441")) {
        return sum_a();
    } else if (!strcmp(name, "s442")) {
        return sum_a();
    } else if (!strcmp(name, "s443")) {
        return sum_a();
    } else if (!strcmp(name, "s451")) {
        return sum_a();
    } else if (!strcmp(name, "s452")) {
        return sum_a();
    } else if (!strcmp(name, "s453")) {
        return sum_a();
    } else if (!strcmp(name, "s471")) {
        return sum_x() + sum_b();
    } else if (!strcmp(name, "s481")) {
        return sum_a();
    } else if (!strcmp(name, "s482")) {
        return sum_a();
    } else if (!strcmp(name, "s491")) {
        return sum_a();
    } else if (!strcmp(name, "s4112")) {
        return sum_a();
    } else if (!strcmp(name, "s4113")) {
        return sum_a();
    } else if (!strcmp(name, "s4114")) {
        return sum_a();
    } else if (!strcmp(name, "s4117")) {
        return sum_a();
    } else if (!strcmp(name, "s4121")) {
        return sum_a();
    } else if (!strcmp(name, "va")) {
        return sum_a();
    } else if (!strcmp(name, "vag")) {
        return sum_a();
    } else if (!strcmp(name, "vas")) {
        return sum_a();
    } else if (!strcmp(name, "vif")) {
        return sum_a();
    } else if (!strcmp(name, "vpv")) {
        return sum_a();
    } else if (!strcmp(name, "vtv")) {
        return sum_a();
    } else if (!strcmp(name, "vpvtv")) {
        return sum_a();
    } else if (!strcmp(name, "vpvts")) {
        return sum_a();
    } else if (!strcmp(name, "vpvpv")) {
        return sum_a();
    } else if (!strcmp(name, "vtvtv")) {
        return sum_a();
    } else if (!strcmp(name, "vsumr")) {
        return sum_a();
    } else if (!strcmp(name, "vbor")) {
        return sum_x();
    } else {
        fprintf(stderr, "Unknown function name passed to calc_checksum: %s\n", name);
        exit(1);
    }
}

