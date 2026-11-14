/*
 * Copyright (c) 2007 - 2026 Joseph Gaeddert
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>

#include "liquid.autotest.h"
#include "liquid.internal.h"

// Encode a random message, flip one encoded bit every _spacing bits, and
// check that both hard- and soft-decision decoding recover the message.
// Spacings below are ~3x the smallest spacing each code was observed to
// correct for every error offset.
void testbench_fec_conv_errors(liquid_autotest __q__,
                               fec_scheme      _fs,
                               unsigned int    _n,
                               unsigned int    _spacing)
{
    fec q = fec_create(_fs, NULL);
    unsigned int n_enc = fec_get_enc_msg_length(_fs, _n);
    unsigned char msg[_n], msg_dec[_n];
    unsigned char msg_enc[n_enc], msg_soft[8*n_enc];

    unsigned int i;
    for (i=0; i<_n; i++)
        msg[i] = rand() & 0xff;
    fec_encode(q, _n, msg, msg_enc);

    // soft bits from the error-free codeword
    for (i=0; i<8*n_enc; i++)
        msg_soft[i] = (msg_enc[i/8] >> (7-(i%8))) & 1 ? LIQUID_SOFTBIT_1 : LIQUID_SOFTBIT_0;

    // channel: flip bits, starting mid-way into the first interval
    unsigned int num_errors = 0;
    for (i=_spacing/2; i<8*n_enc; i+=_spacing) {
        msg_enc[i/8] ^= 0x80 >> (i%8);
        msg_soft[i]   = 255 - msg_soft[i];
        num_errors++;
    }
    LIQUID_REQUIRE(num_errors > 0);

    // hard-decision decoding
    memset(msg_dec, 0, _n);
    fec_decode(q, _n, msg_enc, msg_dec);
    LIQUID_CHECK_ARRAY(msg, msg_dec, _n);

    // soft-decision decoding
    memset(msg_dec, 0, _n);
    fec_decode_soft(q, _n, msg_soft, msg_dec);
    LIQUID_CHECK_ARRAY(msg, msg_dec, _n);

    fec_destroy(q);
}

LIQUID_AUTOTEST(fec_conv_errors_v27,  "conv. code r1/2 K=7 corrects errors",  "fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V27,    64,  24); }
LIQUID_AUTOTEST(fec_conv_errors_v29,  "conv. code r1/2 K=9 corrects errors",  "fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V29,    64,  24); }
LIQUID_AUTOTEST(fec_conv_errors_v39,  "conv. code r1/3 K=9 corrects errors",  "fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V39,    64,  16); }
LIQUID_AUTOTEST(fec_conv_errors_v615, "conv. code r1/6 K=15 corrects errors", "fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V615,   16,  16); }

LIQUID_AUTOTEST(fec_conv_errors_v27p23,"punctured conv. code r2/3 K=7 corrects errors","fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V27P23, 64,  48); }
LIQUID_AUTOTEST(fec_conv_errors_v27p34,"punctured conv. code r3/4 K=7 corrects errors","fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V27P34, 64,  64); }
LIQUID_AUTOTEST(fec_conv_errors_v27p45,"punctured conv. code r4/5 K=7 corrects errors","fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V27P45, 64,  96); }
LIQUID_AUTOTEST(fec_conv_errors_v27p56,"punctured conv. code r5/6 K=7 corrects errors","fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V27P56, 64, 112); }
LIQUID_AUTOTEST(fec_conv_errors_v27p67,"punctured conv. code r6/7 K=7 corrects errors","fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V27P67, 64, 128); }
LIQUID_AUTOTEST(fec_conv_errors_v27p78,"punctured conv. code r7/8 K=7 corrects errors","fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V27P78, 64, 160); }

LIQUID_AUTOTEST(fec_conv_errors_v29p23,"punctured conv. code r2/3 K=9 corrects errors","fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V29P23, 64,  48); }
LIQUID_AUTOTEST(fec_conv_errors_v29p34,"punctured conv. code r3/4 K=9 corrects errors","fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V29P34, 64,  64); }
LIQUID_AUTOTEST(fec_conv_errors_v29p45,"punctured conv. code r4/5 K=9 corrects errors","fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V29P45, 64,  96); }
LIQUID_AUTOTEST(fec_conv_errors_v29p56,"punctured conv. code r5/6 K=9 corrects errors","fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V29P56, 64, 112); }
LIQUID_AUTOTEST(fec_conv_errors_v29p67,"punctured conv. code r6/7 K=9 corrects errors","fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V29P67, 64, 128); }
LIQUID_AUTOTEST(fec_conv_errors_v29p78,"punctured conv. code r7/8 K=9 corrects errors","fec,conv",0.1)
{ testbench_fec_conv_errors(__q__, LIQUID_FEC_CONV_V29P78, 64, 160); }

// Soft decisions carry confidence: wrong bits received with low confidence
// are corrected even when far denser than hard decoding could handle
LIQUID_AUTOTEST(fec_conv_soft_weak_errors,"soft decoding corrects dense low-confidence errors","fec,conv",0.1)
{
    fec_scheme fs = LIQUID_FEC_CONV_V27;
    unsigned int n = 64;
    fec q = fec_create(fs, NULL);
    unsigned int n_enc = fec_get_enc_msg_length(fs, n);
    unsigned char msg[n], msg_dec[n], msg_enc[n_enc], msg_soft[8*n_enc];

    unsigned int i;
    for (i=0; i<n; i++)
        msg[i] = rand() & 0xff;
    fec_encode(q, n, msg, msg_enc);

    // every 3rd bit lands on the wrong side of the threshold, but only just
    // (hard decoding needs errors at least ~7 bits apart for this code)
    for (i=0; i<8*n_enc; i++) {
        unsigned int bit = (msg_enc[i/8] >> (7-(i%8))) & 1;
        if ((i % 3) == 1)
            msg_soft[i] = bit ? 100 : 155;      // weakly wrong
        else
            msg_soft[i] = bit ? 255 :   0;      // strongly right
    }
    fec_decode_soft(q, n, msg_soft, msg_dec);
    LIQUID_CHECK_ARRAY(msg, msg_dec, n);
    fec_destroy(q);
}

// Erasures (soft value 127) carry no information; erasing one in four coded
// bits of the r1/2 code leaves an effective rate-2/3 code, still decodable
LIQUID_AUTOTEST(fec_conv_erasures,"conv. decoder handles erasures","fec,conv",0.1)
{
    fec_scheme fs = LIQUID_FEC_CONV_V27;
    unsigned int n = 64;
    fec q = fec_create(fs, NULL);
    unsigned int n_enc = fec_get_enc_msg_length(fs, n);
    unsigned char msg[n], msg_dec[n], msg_enc[n_enc], msg_soft[8*n_enc];

    unsigned int i;
    for (i=0; i<n; i++)
        msg[i] = rand() & 0xff;
    fec_encode(q, n, msg, msg_enc);
    for (i=0; i<8*n_enc; i++) {
        unsigned int bit = (msg_enc[i/8] >> (7-(i%8))) & 1;
        msg_soft[i] = (i % 4) == 3 ? LIQUID_SOFTBIT_ERASURE :
                      bit ? LIQUID_SOFTBIT_1 : LIQUID_SOFTBIT_0;
    }
    fec_decode_soft(q, n, msg_soft, msg_dec);
    LIQUID_CHECK_ARRAY(msg, msg_dec, n);
    fec_destroy(q);
}

// Textbook K=3, rate 1/2 code with generators (7,5) octal: input 1011
// followed by two zero tail bits encodes to 11 10 00 01 01 11. Checks the
// decoder against an external reference rather than liquid's encoder.
LIQUID_AUTOTEST(fec_viterbi_k3_vector,"Viterbi decoder, K=3 (7,5) test vector","fec,conv",0.1)
{
    int poly[2] = {07, 05};
    unsigned char coded[12] = {1,1, 1,0, 0,0, 0,1, 0,1, 1,1};
    unsigned char syms[12];
    unsigned int i;
    for (i=0; i<12; i++)
        syms[i] = coded[i] ? LIQUID_SOFTBIT_1 : LIQUID_SOFTBIT_0;

    // channel: flip one coded bit (dfree=5 corrects up to two)
    syms[4] = 255 - syms[4];

    fec_viterbi q = fec_viterbi_create(3, 2, poly, 4);
    LIQUID_REQUIRE(q != NULL);
    LIQUID_CHECK(fec_viterbi_init(q, 0) == LIQUID_OK);
    LIQUID_CHECK(fec_viterbi_update_blk(q, syms, 6) == LIQUID_OK);
    unsigned char data = 0xff;  // bits beyond _nbits must be cleared
    LIQUID_CHECK(fec_viterbi_chainback(q, &data, 4, 0) == LIQUID_OK);
    LIQUID_CHECK(data == 0xb0);     // 1011 0000
    fec_viterbi_destroy(q);
}

// Feeding symbols in several calls must match a single call
LIQUID_AUTOTEST(fec_viterbi_blocks,"Viterbi decoder, block-wise updates","fec,conv",0.1)
{
    unsigned int n = 32;                    // decoded bytes
    unsigned int K = 7, R = 2;
    unsigned int num_steps = 8*n + K - 1;   // trellis steps incl. tail
    unsigned char syms[R*num_steps];
    unsigned int i;
    for (i=0; i<R*num_steps; i++)
        syms[i] = rand() & 0xff;            // arbitrary (noise) input

    unsigned char out_a[n], out_b[n];
    fec_viterbi q = fec_viterbi_create(K, R, fec_conv27_poly, 8*n);

    fec_viterbi_init(q, 0);
    fec_viterbi_update_blk(q, syms, num_steps);
    fec_viterbi_chainback(q, out_a, 8*n, 0);

    fec_viterbi_init(q, 0);
    fec_viterbi_update_blk(q, syms,          1);
    fec_viterbi_update_blk(q, syms + R*1,   37);
    fec_viterbi_update_blk(q, syms + R*38,  num_steps - 38);
    fec_viterbi_chainback(q, out_b, 8*n, 0);

    LIQUID_CHECK_ARRAY(out_a, out_b, n);
    fec_viterbi_destroy(q);
}

LIQUID_AUTOTEST(fec_viterbi_config,"Viterbi decoder, invalid configurations","fec,conv,config",0.1)
{
    _liquid_error_downgrade_enable();
    int poly[2] = {0x6d, 0x4f};
    LIQUID_CHECK(fec_viterbi_create( 2, 2, poly, 8) == NULL);   // K too small
    LIQUID_CHECK(fec_viterbi_create(17, 2, poly, 8) == NULL);   // K too large
    LIQUID_CHECK(fec_viterbi_create( 7, 0, poly, 8) == NULL);   // R too small
    LIQUID_CHECK(fec_viterbi_create( 7, 9, poly, 8) == NULL);   // R too large
    LIQUID_CHECK(fec_viterbi_create( 7, 2, NULL, 8) == NULL);   // no polynomials

    // capacity: 8 data bits + 6 tail bits
    fec_viterbi q = fec_viterbi_create(7, 2, poly, 8);
    unsigned char syms[2*15], data[2];
    memset(syms, 0, sizeof(syms));
    LIQUID_CHECK(fec_viterbi_chainback(q, data, 1, 0) != LIQUID_OK);  // nothing decoded yet
    LIQUID_CHECK(fec_viterbi_update_blk(q, syms, 14) == LIQUID_OK);
    LIQUID_CHECK(fec_viterbi_update_blk(q, syms,  1) != LIQUID_OK);  // exceeds capacity
    LIQUID_CHECK(fec_viterbi_update_blk(q, syms, -1) != LIQUID_OK);  // negative length
    LIQUID_CHECK(fec_viterbi_chainback(q, data, 15, 0) != LIQUID_OK); // more bits than steps
    LIQUID_CHECK(fec_viterbi_chainback(q, data,  8, 0) == LIQUID_OK);
    LIQUID_CHECK(data[0] == 0);
    fec_viterbi_destroy(q);
    _liquid_error_downgrade_disable();
}
