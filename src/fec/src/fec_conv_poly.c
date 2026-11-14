/*
 * Copyright (c) 2007 - 2015 Joseph Gaeddert
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
// convolutional code polynomials
//

#include "liquid.internal.h"

// Generator polynomials, identical to libfec's V27POLYA etc. so that
// encoded streams are compatible whether or not libfec is installed.
// Bit i of each polynomial taps the input bit from i steps ago (bit 0 is
// the newest), which is the bit-reversed form of the textbook octal values
// (e.g. 0x6d = 1101101b is the reverse of 133 octal = 1011011b).

int fec_conv27_poly[2]  = {0x6d, 0x4f};

int fec_conv29_poly[2]  = {0x1af, 0x11d};

int fec_conv39_poly[3]  = {0x1ed, 0x19b, 0x127};

int fec_conv615_poly[6] = {042631, 047245, 056507,
                           073363, 077267, 064537};

