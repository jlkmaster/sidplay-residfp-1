/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2004, 2010 Dag Lem <resid@nimrod.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include <stdint.h>

#include "siddefs-fp.h"

namespace reSIDfp
{

/**
 * Java port of the reSID 1.0 filter VCR+opamp+capacitor element (integrator) by Dag Lem.
 *
 * Java port and subthreshold current simulation added by Antti S. Lankila.
 *
 * @author Antti S. Lankila
 * @author Dag Lem
 * @author Leandro Nini
 */
class Integrator
{
private:
    unsigned int Vddt_Vw_2;
    int kVddt, n_snake, vx;
    int vc;
    const unsigned short* vcr_kVg;
    const unsigned short* vcr_n_Ids_term;
    const int* opamp_rev;

public:
    Integrator(const unsigned short* vcr_kVg, const unsigned short* vcr_n_Ids_term,
               const int* opamp_rev, int kVddt, int n_snake) :
        Vddt_Vw_2(0),
        kVddt(kVddt),
        n_snake(n_snake),
        vx(0),
        vc(0),
        vcr_kVg(vcr_kVg),
        vcr_n_Ids_term(vcr_n_Ids_term),
        opamp_rev(opamp_rev) {}

    void setVw(unsigned int Vw) { Vddt_Vw_2 = (kVddt - Vw) * (kVddt - Vw) >> 1; }

    int solve(int vi);
};

} // namespace reSIDfp

#if RESID_INLINING || defined(INTEGRATOR_CPP)

namespace reSIDfp
{

RESID_INLINE
int Integrator::solve(int vi)
{
    // "Snake" voltages for triode mode calculation.
    const int Vgst = kVddt - vx;
    const int Vgdt = kVddt - vi;

    const uint64_t Vgst_2 = (int64_t)Vgst * (int64_t)Vgst;
    const uint64_t Vgdt_2 = (int64_t)Vgdt * (int64_t)Vgdt;

    // "Snake" current, scaled by (1/m)*2^13*m*2^16*m*2^16*2^-15 = m*2^30
    const int n_I_snake = n_snake * ((Vgst_2 - Vgdt_2) >> 15);

    // VCR gate voltage.       // Scaled by m*2^16
    // Vg = Vddt - sqrt(((Vddt - Vw)^2 + Vgdt^2)/2)
    const int kVg = (int)vcr_kVg[(Vddt_Vw_2 + (Vgdt_2 >> 1)) >> 16];

    // VCR voltages for EKV model table lookup.
    const int Vgs = kVg > vx ? kVg - vx : 0;
    const int Vgd = kVg > vi ? kVg - vi : 0;

    // VCR current, scaled by m*2^15*2^15 = m*2^30
    const int n_I_vcr = (int)(vcr_n_Ids_term[Vgs & 0xffff] - vcr_n_Ids_term[Vgd & 0xffff]) << 15;

    // Change in capacitor charge.
    vc += n_I_snake + n_I_vcr;

    // vx = g(vc)
    vx = opamp_rev[((vc >> 15) + (1 << 15)) & 0xffff];

    // Return vo.
    return vx - (vc >> 14);
}

} // namespace reSIDfp

#endif

#endif
