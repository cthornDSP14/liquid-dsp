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

//
// fec_viterbi.c
//
// Portable soft-decision Viterbi decoder for rate 1/R, constraint length K
// convolutional codes. Used by the convolutional and punctured convolutional
// FEC schemes when libfec is not available.
//
// The public methods deliberately share the signatures of libfec's
// create/init/update/chainback/delete functions so they can be assigned
// directly to the function pointers in struct fec_s.
//
// Conventions (matching fec_conv_encode()):
//  - the encoder shift register 'sr' shifts each input bit into its LSB:
//        sr = (sr << 1) | bit
//    and emits R bits, output r being parity(sr & poly[r])
//  - the decoder state is the most recent K-1 input bits, i.e. the low
//    K-1 bits of 'sr'; there are 2^(K-1) states
//  - input soft bits are in [0,255]: 0 is a confident '0', 255 a confident
//    '1', and 127 (LIQUID_SOFTBIT_ERASURE) carries almost no information
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "liquid.internal.h"

// initial metric for states the encoder cannot be in; large enough that
// any path from the true start state beats it, small enough that adding
// branch metrics to it cannot overflow 32 bits
#define FEC_VITERBI_METRIC_INIT (1u << 24)

struct fec_viterbi_s {
    unsigned int K;             // constraint length
    unsigned int R;             // inverse rate (outputs per input bit)
    unsigned int num_states;    // number of trellis states, 2^(K-1)
    unsigned int max_steps;     // decisions buffer capacity (trellis steps)
    unsigned int num_steps;     // number of steps processed since init

    unsigned char * branch_out; // [2^K] R-bit output pattern for each register value
    uint32_t * bm;              // [2^R] branch metric for each output pattern
    uint32_t * metric;          // [num_states] path metric of each state
    uint32_t * metric_next;     // [num_states] path metrics being computed

    // survivor decisions: one bit per state per step, packed into 32-bit
    // words; bit 's' of step 'n' says which predecessor won into state 's'
    unsigned int words_per_step;
    uint32_t * decisions;       // [max_steps x words_per_step]
};

// create Viterbi decoder object
//  _K          : constraint length, 3 <= _K <= 16
//  _R          : inverse code rate, 1 <= _R <= 8
//  _poly       : generator polynomials, [size: _R x 1]
//  _max_bits   : maximum number of decoded (data) bits per frame
fec_viterbi fec_viterbi_create(unsigned int _K,
                               unsigned int _R,
                               const int *  _poly,
                               unsigned int _max_bits)
{
    if (_K < 3 || _K > 16)
        return liquid_error_config("fec_viterbi_create(), constraint length must be in [3,16]");
    if (_R < 1 || _R > 8)
        return liquid_error_config("fec_viterbi_create(), inverse rate must be in [1,8]");
    if (_poly == NULL)
        return liquid_error_config("fec_viterbi_create(), polynomials cannot be NULL");

    fec_viterbi q = (fec_viterbi) malloc(sizeof(struct fec_viterbi_s));
    q->K          = _K;
    q->R          = _R;
    q->num_states = 1u << (_K-1);
    q->max_steps  = _max_bits + _K - 1;   // data bits plus tail bits
    q->num_steps  = 0;

    // pre-compute the R output bits for every K-bit register value; the
    // first polynomial lands in the most significant bit of the pattern
    unsigned int i, r;
    q->branch_out = (unsigned char*) malloc((1u << _K)*sizeof(unsigned char));
    for (i=0; i<(1u << _K); i++) {
        unsigned char pattern = 0;
        for (r=0; r<_R; r++)
            pattern = (pattern << 1) | liquid_count_ones_mod2(i & (unsigned int)_poly[r]);
        q->branch_out[i] = pattern;
    }

    q->bm          = (uint32_t*) malloc((1u << _R)*sizeof(uint32_t));
    q->metric      = (uint32_t*) malloc(q->num_states*sizeof(uint32_t));
    q->metric_next = (uint32_t*) malloc(q->num_states*sizeof(uint32_t));

    q->words_per_step = (q->num_states + 31) / 32;
    q->decisions = (uint32_t*) malloc((size_t)q->max_steps*q->words_per_step*sizeof(uint32_t));

    fec_viterbi_init(q, 0);
    return q;
}

// convenience constructors matching libfec's create_viterbiXX(int len)
void * fec_viterbi_create_v27(int _len)
    { return fec_viterbi_create(7, 2, fec_conv27_poly,  _len); }
void * fec_viterbi_create_v29(int _len)
    { return fec_viterbi_create(9, 2, fec_conv29_poly,  _len); }
void * fec_viterbi_create_v39(int _len)
    { return fec_viterbi_create(9, 3, fec_conv39_poly,  _len); }
void * fec_viterbi_create_v615(int _len)
    { return fec_viterbi_create(15,6, fec_conv615_poly, _len); }

// destroy Viterbi decoder object
void fec_viterbi_destroy(void * _q)
{
    fec_viterbi q = (fec_viterbi) _q;
    if (q == NULL)
        return;
    free(q->branch_out);
    free(q->bm);
    free(q->metric);
    free(q->metric_next);
    free(q->decisions);
    free(q);
}

// reset decoder, setting the known starting state of the encoder
int fec_viterbi_init(void * _q,
                     int    _starting_state)
{
    fec_viterbi q = (fec_viterbi) _q;
    unsigned int s;
    for (s=0; s<q->num_states; s++)
        q->metric[s] = FEC_VITERBI_METRIC_INIT;
    q->metric[(unsigned int)_starting_state & (q->num_states-1)] = 0;
    q->num_steps = 0;
    return LIQUID_OK;
}

// run the add-compare-select recursion over a block of received symbols
//  _q      : decoder object
//  _syms   : soft input symbols, [size: _R*_nbits x 1]
//  _nbits  : number of trellis steps (decoded bits, including tail)
int fec_viterbi_update_blk(void *          _q,
                           unsigned char * _syms,
                           int             _nbits)
{
    fec_viterbi q = (fec_viterbi) _q;
    if (_nbits < 0 || q->num_steps + (unsigned int)_nbits > q->max_steps)
        return liquid_error(LIQUID_EIRANGE,"fec_viterbi_update_blk(), too many steps for decoder size");

    unsigned int half = q->num_states >> 1;
    unsigned int n, p, r;
    for (n=0; n<(unsigned int)_nbits; n++) {
        const unsigned char * sym = _syms + n*q->R;

        // branch metrics: distance between the received soft bits and
        // each of the 2^R possible output patterns (smaller is better)
        for (p=0; p<(1u << q->R); p++) {
            uint32_t d = 0;
            for (r=0; r<q->R; r++) {
                unsigned int bit = (p >> (q->R-1-r)) & 1;
                d += bit ? 255 - sym[r] : sym[r];
            }
            q->bm[p] = d;
        }

        // Add-compare-select for each next state s. Its two predecessors
        // are j=s>>1 and j+half (a 'butterfly': states 2j and 2j+1 share
        // them). The full K-bit register for each transition is s or
        // s|num_states: the bit leaving the state memory is the
        // predecessor's top bit. Object fields are copied to locals so
        // the compiler need not reload them after every store (a store
        // through a uint32_t pointer could otherwise alias them).
        const unsigned int    ns   = q->num_states;
        const uint32_t *      bm   = q->bm;
        const unsigned char * out  = q->branch_out;
        const uint32_t *      m    = q->metric;
        uint32_t *            next = q->metric_next;
        uint32_t *            dec  = q->decisions + (size_t)q->num_steps*q->words_per_step;
        unsigned int w, s;
        for (w=0; w<q->words_per_step; w++) {
            // accumulate 32 decision bits in a register, store once
            uint32_t word = 0;
            unsigned int s_end = (w+1)*32 < ns ? (w+1)*32 : ns;
            for (s=w*32; s<s_end; s++) {
                unsigned int j = s >> 1;
                uint32_t c0 = m[j     ] + bm[out[s     ]];
                uint32_t c1 = m[j+half] + bm[out[s | ns]];
                // branchless compare-select; d=1 if predecessor j+half won
                uint32_t d = c1 < c0;
                next[s] = d ? c1 : c0;
                word |= d << (s & 31);
            }
            dec[w] = word;
        }

        // renormalize: only metric differences matter, so subtract the
        // minimum to keep values bounded for arbitrarily long frames
        uint32_t min_metric = next[0];
        for (s=1; s<ns; s++)
            min_metric = next[s] < min_metric ? next[s] : min_metric;
        for (s=0; s<ns; s++)
            next[s] -= min_metric;

        // swap metric buffers
        uint32_t * tmp = q->metric;
        q->metric      = q->metric_next;
        q->metric_next = tmp;

        q->num_steps++;
    }
    return LIQUID_OK;
}

// trace back through the stored decisions from a known end state and
// write the first _nbits decoded bits (MSB first) to _data
//  _q          : decoder object
//  _data       : decoded output bytes, [size: ceil(_nbits/8) x 1]
//  _nbits      : number of data bits to output (excluding tail)
//  _endstate   : known final encoder state (0 for zero-tail termination)
int fec_viterbi_chainback(void *          _q,
                          unsigned char * _data,
                          unsigned int    _nbits,
                          unsigned int    _endstate)
{
    fec_viterbi q = (fec_viterbi) _q;
    if (_nbits > q->num_steps)
        return liquid_error(LIQUID_EIRANGE,"fec_viterbi_chainback(), more bits requested than decoded");

    memset(_data, 0, (_nbits + 7) / 8);

    unsigned int state = _endstate & (q->num_states - 1);
    unsigned int n = q->num_steps;
    while (n > 0) {
        n--;
        const uint32_t * dec = q->decisions + (size_t)n*q->words_per_step;
        unsigned int d = (dec[state >> 5] >> (state & 31)) & 1;

        // the input bit that led into this state is its LSB
        if (n < _nbits && (state & 1))
            _data[n >> 3] |= 0x80 >> (n & 7);

        // step back to the surviving predecessor
        state = (state >> 1) | (d << (q->K - 2));
    }
    return LIQUID_OK;
}
