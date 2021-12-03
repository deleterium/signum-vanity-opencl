typedef uchar uint8_t;
typedef uint uint32_t;
typedef ulong uint64_t;

#define ALIGN(x) __attribute__((aligned(x)))

#define H0 0x6a09e667
#define H1 0xbb67ae85
#define H2 0x3c6ef372
#define H3 0xa54ff53a
#define H4 0x510e527f
#define H5 0x9b05688c
#define H6 0x1f83d9ab
#define H7 0x5be0cd19

uint rotr(uint x, int n) { if (n < 32) return (x >> n) | (x << (32 - n)); return x; }
uint ch(uint x, uint y, uint z) { return (x & y) ^ (~x & z); }
uint maj(uint x, uint y, uint z) { return (x & y) ^ (x & z) ^ (y & z); }
uint sigma0(uint x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
uint sigma1(uint x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
uint gamma0(uint x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
uint gamma1(uint x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

__constant uint K[64]={
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

void sha256_crypt(const uint ulen, const char *plain_key, uint *digest){
    int t, gid, msg_pad;
    int stop, mmod;
    uint i, /* ulen, */ item, total;
    uint W[80], temp, A,B,C,D,E,F,G,H,T1,T2;
    int current_pad;

    msg_pad=0;
    // ulen = data_info[2];
    total = ulen%64>=56 ? 2 : 1 + ulen/64;
    digest[0] = H0; digest[1] = H1; digest[2] = H2; digest[3] = H3; digest[4] = H4; digest[5] = H5; digest[6] = H6; digest[7] = H7;
    for(item=0; item<total; item++) {
        A = digest[0]; B = digest[1]; C = digest[2]; D = digest[3]; E = digest[4]; F = digest[5]; G = digest[6]; H = digest[7];
        #pragma unroll
        for (t = 0; t < 80; t++){ W[t] = 0x00000000; }
        msg_pad=item*64;
        if(ulen > msg_pad) { current_pad = (ulen-msg_pad)>64?64:(ulen-msg_pad); }
        else { current_pad =-1; }
        if(current_pad>0) {
            i=current_pad;
            stop =  i/4;
            for (t = 0 ; t < stop ; t++){
                W[t] = ((uchar)  plain_key[msg_pad + t * 4]) << 24;
                W[t] |= ((uchar) plain_key[msg_pad + t * 4 + 1]) << 16;
                W[t] |= ((uchar) plain_key[msg_pad + t * 4 + 2]) << 8;
                W[t] |= (uchar)  plain_key[msg_pad + t * 4 + 3];
            }
            mmod = i % 4;
            if ( mmod == 3){
                W[t] = ((uchar)  plain_key[msg_pad + t * 4]) << 24;
                W[t] |= ((uchar) plain_key[msg_pad + t * 4 + 1]) << 16;
                W[t] |= ((uchar) plain_key[msg_pad + t * 4 + 2]) << 8;
                W[t] |=  ((uchar) 0x80) ;
            } else if (mmod == 2) {
                W[t] = ((uchar)  plain_key[msg_pad + t * 4]) << 24;
                W[t] |= ((uchar) plain_key[msg_pad + t * 4 + 1]) << 16;
                W[t] |=  0x8000 ;
            } else if (mmod == 1) {
                W[t] = ((uchar)  plain_key[msg_pad + t * 4]) << 24;
                W[t] |=  0x800000 ;
            } else /*if (mmod == 0)*/ {
                W[t] =  0x80000000 ;
            }
            if (current_pad<56) { W[15] =  ulen*8 ; }
        } else if(current_pad <0) {
            if( ulen%64==0) W[0]=0x80000000;
            W[15]=ulen*8;
        }
        for (t = 0; t < 64; t++) {
            if (t >= 16) W[t] = gamma1(W[t - 2]) + W[t - 7] + gamma0(W[t - 15]) + W[t - 16];
            T1 = H + sigma1(E) + ch(E, F, G) + K[t] + W[t];
            T2 = sigma0(A) + maj(A, B, C);
            H = G; G = F; F = E; E = D + T1; D = C; C = B; B = A; A = T1 + T2;
        }
        digest[0] += A; digest[1] += B; digest[2] += C; digest[3] += D; digest[4] += E; digest[5] += F; digest[6] += G; digest[7] += H;
    }
}


typedef long LL;
typedef long * LL10;
typedef uchar BYTE;

#define P25 33554431
#define P26 67108863

/* Check if reduced-form input >= 2^255-19 */
int is_overflow(LL10 x) {
    return (((x[0] > P26-19)) && ((x[1] & x[3] & x[5] & x[7] & x[9]) == P25) && ((x[2] & x[4] & x[6] & x[8]) == P26)) || (x[9] > P25);
}

/* checks if x is "negative", requires reduced input */
int is_negative(LL10 x) {
    return (int)(((is_overflow(x) || (x[9] < 0))?1:0) ^ (x[0] & 1));
}

/* Convert from internal format to little-endian byte format.  The
 * number must be in a reduced form which is output by the following ops:
 *     unpack, mul, sqr
 *     set --  if input in range 0 .. P25
 * If you're unsure if the number is reduced, first multiply it by 1.
 */
void pack(LL10 x, BYTE *m) {
    int ld = 0, ud = 0;
    LL t;
    ld = (is_overflow(x)?1:0) - ((x[9] < 0)?1:0);
    ud = ld * -(P25+1);
    ld *= 19;
    t = ld + x[0] + (x[1] << 26);
    m[ 0] = t & 0xFF;
    m[ 1] = (t >> 8) & 0xFF;
    m[ 2] = (t >> 16) & 0xFF;
    m[ 3] = (t >> 24) & 0xFF;
    t = (t >> 32) + (x[2] << 19);
    m[ 4] = t & 0xFF;
    m[ 5] = (t >> 8) & 0xFF;
    m[ 6] = (t >> 16) & 0xFF;
    m[ 7] = (t >> 24) & 0xFF;
    t = (t >> 32) + (x[3] << 13);
    m[ 8] = t & 0xFF;
    m[ 9] = (t >> 8) & 0xFF;
    m[10] = (t >> 16) & 0xFF;
    m[11] = (t >> 24) & 0xFF;
    t = (t >> 32) + (x[4] <<  6);
    m[12] = t & 0xFF;
    m[13] = (t >> 8) & 0xFF;
    m[14] = (t >> 16) & 0xFF;
    m[15] = (t >> 24) & 0xFF;
    t = (t >> 32) + x[5] + (x[6] << 25);
    m[16] = t & 0xFF;
    m[17] = (t >> 8) & 0xFF;
    m[18] = (t >> 16) & 0xFF;
    m[19] = (t >> 24) & 0xFF;
    t = (t >> 32) + (x[7] << 19);
    m[20] = t & 0xFF;
    m[21] = (t >> 8) & 0xFF;
    m[22] = (t >> 16) & 0xFF;
    m[23] = (t >> 24) & 0xFF;
    t = (t >> 32) + (x[8] << 12);
    m[24] = t & 0xFF;
    m[25] = (t >> 8) & 0xFF;
    m[26] = (t >> 16) & 0xFF;
    m[27] = (t >> 24) & 0xFF;
    t = (t >> 32) + ((x[9] + ud) << 6);
    m[28] = t & 0xFF;
    m[29] = (t >> 8) & 0xFF;
    m[30] = (t >> 16) & 0xFF;
    m[31] = (t >> 24) & 0xFF;
}

/* Copy a number */
inline void cpy(LL10 out, LL10 in) {
    out[0] = in[0]; out[1] = in[1];
    out[2] = in[2]; out[3] = in[3];
    out[4] = in[4]; out[5] = in[5];
    out[6] = in[6]; out[7] = in[7];
    out[8] = in[8]; out[9] = in[9];
    //memcpy(out, in, sizeof(LL)*10);
}

/* Set a number to value, which must be in range -185861411 .. 185861411 */
inline void set(LL10 out, int in) {
    out[0] = in; out[1] = 0;
    out[2] = 0; out[3] = 0;
    out[4] = 0; out[5] = 0;
    out[6] = 0; out[7] = 0;
    out[8] = 0; out[9] = 0;
}

/* Add/subtract two numbers.  The inputs must be in reduced form, and the
 * output isn't, so to do another addition or subtraction on the output,
 * first multiply it by one to reduce it.
 */
inline void add(LL10 xy, LL10 x, LL10 y) {
    #pragma unroll
    for (int i=0; i<10; i++) { xy[i] = x[i] + y[i]; }
}
inline void sub(LL10 xy, LL10 x, LL10 y) {
    #pragma unroll
    for (int i=0; i<10; i++) { xy[i] = x[i] - y[i]; }
}

void mul_small(LL10 xy, LL10 x, LL y) {
    LL t;
    t = (x[8]*y);
    xy[8] = (t & P26);
    t >>= 26; t += (x[9]*y);
    xy[9] = (t & P25);
    t = 19 * (t >> 25) + (x[0]*y);
    xy[0] = (t & P26);
    t >>= 26; t += (x[1]*y);
    xy[1] = (t & P25);
    t >>= 25; t += (x[2]*y);
    xy[2] = (t & P26);
    t >>= 26; t += (x[3]*y);
    xy[3] = (t & P25);
    t >>= 25; t += (x[4]*y);
    xy[4] = (t & P26);
    t >>= 26; t += (x[5]*y);
    xy[5] = (t & P25);
    t >>= 25; t += (x[6]*y);
    xy[6] = (t & P26);
    t >>= 26; t += (x[7]*y);
    xy[7] = (t & P25);
    t >>= 25; t += xy[8];
    xy[8] = (t & P26);
    xy[9] += (t >> 26);
}

void mul(LL10 xy, LL10 x, LL10 y) {
    LL t;
    LL x_0=x[0], x_1=x[1], x_2=x[2], x_3=x[3], x_4=x[4],  x_5=x[5], x_6=x[6], x_7=x[7], x_8=x[8], x_9=x[9];
    LL y_0=y[0], y_1=y[1], y_2=y[2], y_3=y[3], y_4=y[4],  y_5=y[5], y_6=y[6], y_7=y[7], y_8=y[8], y_9=y[9];
    t = (x_0*y_8) + (x_2*y_6) + (x_4*y_4) + (x_6*y_2) + (x_8*y_0) + 2 * ((x_1*y_7) + (x_3*y_5) + (x_5*y_3) + (x_7*y_1)) + 38 * (x_9*y_9);
    xy[8] = (t & P26);
    t >>= 26;
    t += (x_0*y_9) + (x_1*y_8) + (x_2*y_7) + (x_3*y_6) + (x_4*y_5) + (x_5*y_4) + (x_6*y_3) + (x_7*y_2) + (x_8*y_1) + (x_9*y_0);
    xy[9] = (t & P25);
    t = (x_0*y_0) + 19 * ((t >> 25) + (x_2*y_8) + (x_4*y_6) + (x_6*y_4) + (x_8*y_2)) + 38 * ((x_1*y_9) + (x_3*y_7) + (x_5*y_5) + (x_7*y_3) + (x_9*y_1));
    xy[0] = (t & P26);
    t >>= 26;
    t += (x_0*y_1) + (x_1*y_0) + 19 * ((x_2*y_9) + (x_3*y_8) + (x_4*y_7) + (x_5*y_6) + (x_6*y_5) + (x_7*y_4) + (x_8*y_3) + (x_9*y_2));
    xy[1] = (t & P25);
    t >>= 25;
    t += (x_0*y_2) + (x_2*y_0) + 19 * ((x_4*y_8) + (x_6*y_6) + (x_8*y_4)) + 2 * (x_1*y_1) + 38 * ((x_3*y_9) + (x_5*y_7) + (x_7*y_5) + (x_9*y_3));
    xy[2] = (t & P26);
    t >>= 26;
    t += (x_0*y_3) + (x_1*y_2) + (x_2*y_1) + (x_3*y_0) + 19 * ((x_4*y_9) + (x_5*y_8) + (x_6*y_7) + (x_7*y_6) + (x_8*y_5) + (x_9*y_4));
    xy[3] = (t & P25);
    t >>= 25;
    t += (x_0*y_4) + (x_2*y_2) + (x_4*y_0) + 19 * ((x_6*y_8) + (x_8*y_6)) + 2 * ((x_1*y_3) + (x_3*y_1)) + 38 * ((x_5*y_9) + (x_7*y_7) + (x_9*y_5));
    xy[4] = (t & P26);
    t >>= 26;
    t += (x_0*y_5) + (x_1*y_4) + (x_2*y_3) + (x_3*y_2) + (x_4*y_1) + (x_5*y_0) + 19 * ((x_6*y_9) + (x_7*y_8) + (x_8*y_7) + (x_9*y_6));
    xy[5] = (t & P25);
    t >>= 25;
    t += (x_0*y_6) + (x_2*y_4) + (x_4*y_2) + (x_6*y_0) + 19 * (x_8*y_8) + 2 * ((x_1*y_5) + (x_3*y_3) + (x_5*y_1)) + 38 * ((x_7*y_9) + (x_9*y_7));
    xy[6] = (t & P26);
    t >>= 26;
    t += (x_0*y_7) + (x_1*y_6) + (x_2*y_5) + (x_3*y_4) + (x_4*y_3) + (x_5*y_2) + (x_6*y_1) + (x_7*y_0) + 19 * ((x_8*y_9) + (x_9*y_8));
    xy[7] = (t & P25);
    t >>= 25;
    t += xy[8];
    xy[8] = (t & P26);
    xy[9] += (t >> 26);
}

void sqr(LL10 x2, LL10 x) {
    LL x_0=x[0], x_1=x[1], x_2=x[2], x_3=x[3], x_4=x[4],  x_5=x[5], x_6=x[6], x_7=x[7], x_8=x[8], x_9=x[9];
    LL t;
    t = (x_4*x_4) + 2 * ((x_0*x_8) + (x_2*x_6)) + 38 * (x_9*x_9) + 4 * ((x_1*x_7) + (x_3*x_5));
    x2[8] = (t & P26);
    t >>= 26; t += 2 * ((x_0*x_9) + (x_1*x_8) + (x_2*x_7) + (x_3*x_6) + (x_4*x_5));
    x2[9] = (t & P25);
    t = 19 * (t >> 25) + (x_0*x_0) + 38 * ((x_2*x_8) + (x_4*x_6) + (x_5*x_5)) + 76 * ((x_1*x_9) + (x_3*x_7));
    x2[0] = (t & P26);
    t >>= 26; t += 2 * (x_0*x_1) + 38 * ((x_2*x_9) + (x_3*x_8) + (x_4*x_7) + (x_5*x_6));
    x2[1] = (t & P25);
    t >>= 25; t += 19 * (x_6*x_6) + 2 * ((x_0*x_2) + (x_1*x_1)) + 38 * (x_4*x_8) + 76 * ((x_3*x_9) + (x_5*x_7));
    x2[2] = (t & P26);
    t >>= 26; t += 2 * ((x_0*x_3) + (x_1*x_2)) + 38 * ((x_4*x_9) + (x_5*x_8) + (x_6*x_7));
    x2[3] = (t & P25);
    t >>= 25; t += (x_2*x_2) + 2 * (x_0*x_4) + 38 * ((x_6*x_8) + (x_7*x_7)) + 4 * (x_1*x_3) + 76 * (x_5*x_9);
    x2[4] = (t & P26);
    t >>= 26; t += 2 * ((x_0*x_5) + (x_1*x_4) + (x_2*x_3)) + 38 * ((x_6*x_9) + (x_7*x_8));
    x2[5] = (t & P25);
    t >>= 25; t += 19 * (x_8*x_8) + 2 * ((x_0*x_6) + (x_2*x_4) + (x_3*x_3)) + 4 * (x_1*x_5) + 76 * (x_7*x_9);
    x2[6] = (t & P26);
    t >>= 26; t += 2 * ((x_0*x_7) + (x_1*x_6) + (x_2*x_5) + (x_3*x_4)) + 38 * (x_8*x_9);
    x2[7] = (t & P25);
    t >>= 25; t += x2[8];
    x2[8] = (t & P26);
    x2[9] += (t >> 26);
}

void recip(LL10 y, LL10 x, int sqrtassist) {
    LL t0[10], t1[10], t2[10], t3[10], t4[10];
    int i;
    sqr(t1, x);
    sqr(t2, t1);
    sqr(t0, t2);
    mul(t2, t0, x);
    mul(t0, t2, t1);
    sqr(t1, t0);
    mul(t3, t1, t2);
    sqr(t1, t3);
    sqr(t2, t1);
    sqr(t1, t2);
    sqr(t2, t1);
    sqr(t1, t2);
    mul(t2, t1, t3);
    sqr(t1, t2);
    sqr(t3, t1);
    for (i = 1; i < 5; i++) {
        sqr(t1, t3);
        sqr(t3, t1);
    }
    mul(t1, t3, t2);
    sqr(t3, t1);
    sqr(t4, t3);
    for (i = 1; i < 10; i++) {
        sqr(t3, t4);
        sqr(t4, t3);
    }
    mul(t3, t4, t1);
    for (i = 0; i < 5; i++) {
        sqr(t1, t3);
        sqr(t3, t1);
    }
    mul(t1, t3, t2);
    sqr(t2, t1);
    sqr(t3, t2);
    for (i = 1; i < 25; i++) {
        sqr(t2, t3);
        sqr(t3, t2);
    }
    mul(t2, t3, t1);
    sqr(t3, t2);
    sqr(t4, t3);
    for (i = 1; i < 50; i++) {
        sqr(t3, t4);
        sqr(t4, t3);
    }
    mul(t3, t4, t2);
    for (i = 0; i < 25; i++) {
        sqr(t4, t3);
        sqr(t3, t4);
    }
    mul(t2, t3, t1);
    sqr(t1, t2);
    sqr(t2, t1);
    // if (sqrtassist!=0) {
    //     mul(y, x, t2);
    // } else {
        sqr(t1, t2);
        sqr(t2, t1);
        sqr(t1, t2);
        mul(y, t1, t0);
    // }
}

inline void mont_prep(LL10 t1, LL10 t2, LL10 ax, LL10 az) {
    add(t1, ax, az);
    sub(t2, ax, az);
}

inline void mont_add(LL10 t1, LL10 t2, LL10 t3, LL10 t4, LL10 ax, LL10 az, LL10 dx) {
    mul(ax, t2, t3);
    mul(az, t1, t4);
    add(t1, ax, az);
    sub(t2, ax, az);
    sqr(ax, t1);
    sqr(t1, t2);
    mul(az, t1, dx);
}

inline void mont_dbl(LL10 t1, LL10 t2, LL10 t3, LL10 t4, LL10 bx, LL10 bz) {
    sqr(t1, t3);
    sqr(t2, t4);
    mul(bx, t1, t2);
    sub(t2, t1, t2);
    mul_small(bz, t2, 121665);
    add(t1, t1, bz);
    mul(bz, t1, t2);
}

void x_to_y2(LL10 t, LL10 y2, LL10 x) {
    sqr(t, x);
    mul_small(y2, x, 486662);
    add(t, t, y2);
    t[0]++;
    mul(y2, t, x);
}

void curve25519_c_keygen(BYTE *Px, BYTE *k) {
    LL dx[10], t1[10], t2[10], t3[10], t4[10];
    LL x[2][10], z[2][10];
    int i, j;

    /* unpack the base */
    set(dx, 9);

    /* 0G = point-at-infinity */
    set(x[0], 1);
    set(z[0], 0);

    /* 1G = G */
    cpy(x[1], dx);
    set(z[1], 1);

    for (i = 32; i--!=0; ) {
        for (j = 8; j--!=0; ) {
            /* swap arguments depending on bit */
            int bit1 = (k[i] /* & 0xFF */) >> j & 1;
            int bit0 = ~(k[i] /* & 0xFF */) >> j & 1;
            LL10 ax = x[bit0];
            LL10 az = z[bit0];
            LL10 bx = x[bit1];
            LL10 bz = z[bit1];

            /* a' = a + b */
            /* b' = 2 b    */
            mont_prep(t1, t2, ax, az);
            mont_prep(t3, t4, bx, bz);
            mont_add(t1, t2, t3, t4, ax, az, dx);
            mont_dbl(t1, t2, t3, t4, bx, bz);
        }
    }

    recip(t1, z[0], 1);
    mul(dx, x[0], t1);
    pack(dx, Px);
}

__kernel void process (__global const uint *passLength, __global char *plain_key2,  __global uint *digest2) {
    size_t const thread = get_global_id (0);
    uchar privateKey[32];
    uchar publicKey[32];
    uint passwordHash[8];
    uint fullID[8];
    char internalPassword[120];
    uint passphraseLength = passLength[0];

    if (passphraseLength > 120) {
        return;
    }

    for (size_t i = 0; i < passphraseLength; i++) {
        internalPassword[i] = *(plain_key2 + passphraseLength * thread + i);
    }
    // NOTE This function for sha256 returns wrong values if lenght is greater of equal of 56.
    sha256_crypt(passphraseLength, internalPassword, passwordHash);
    #pragma unroll
    for (size_t i = 0; i < 32; i++) {
        // toggle endianess and split bytes
        privateKey[i] = (passwordHash[i/4] >> ((3 - (i%4))*8)) & (0xFF);
    }
    // clamp hash to get private key
    privateKey[0] &= 0xF8;
    privateKey[31] &= 0x7F;
    privateKey[31] |= 0x40;

    // for (size_t i = 0; i < 32; i+=4) {
    //             digest2[i/4]=privateKey[i] | privateKey[i+1] << 8 | privateKey[i+2] << 16 | privateKey[i+3] << 24;
    // }
    // return;
    curve25519_c_keygen(publicKey, privateKey);
    // for (size_t i = 0; i < 32; i+=4) {
    //             digest2[i/4]=publicKey[i] | publicKey[i+1] << 8 | publicKey[i+2] << 16 | publicKey[i+3] << 24;
    //     }
    // return;
    // for (size_t i = 0; i < 32; i++) {
    //    digest2[i] = (passwordHash[i/4] >> ((i%4)*8)) & (0xFF);
    // }

    sha256_crypt(32, (char *)publicKey, fullID);

    digest2[ 8* thread ] = (fullID[0] & 0xff) << 24 |
        (fullID[0] & 0xff00) << 8 |
        (fullID[0] & 0xff0000) >> 8 |
        (fullID[0] & 0xff000000) >> 24;
    digest2[ 8* thread + 1] = (fullID[1] & 0xff) << 24 |
        (fullID[1] & 0xff00) << 8 |
        (fullID[1] & 0xff0000) >> 8 |
        (fullID[1] & 0xff000000) >> 24;
}
