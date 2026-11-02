// DK64's bone-matrix builder, func_global_asm_806195D0, as a host function.
//
// Once a frame, for each animated actor on screen, func_global_asm_80614A64
// points actor->unk8 at one of its animation state's two bone arrays and
// calls this routine (actor in a0). It decodes the actor's animation (one
// or two bit-packed keyframe streams, an additive layer over each, a list
// of fixed offsets), blends the two poses, turns each bone's Euler angles
// into a rotation matrix (through quaternions when it blends), scales and
// offsets bones, concatenates the hierarchy under the actor's own matrix
// and converts the result into the N64's fixed-point Mtx format, in place
// in actor->unk8. It is hand-written MIPS (lib/dk64_decomp/src/global_asm/
// code_1E2D0.s), and the recompiled version, which does every operation
// through the context in memory and every 64-bit shift as two words on the
// ARM11, was the game core's biggest single cost on an Old 3DS (16-22 %).
//
// The 3DS recompiler config lists func_global_asm_806195D0 in its
// `ignored` (the Makefile adds it to its copy of us.toml beside the gu
// helpers): it is not recompiled, and the one call to it, from the
// recompiled func_global_asm_80614A64, resolves to this function of the
// same name. Its internal subroutines (80619B44, 80619C2C, 80619F50,
// 8061A1A0) are only reached from inside it and are folded in here.
//
// The port is the asm's own arithmetic in its own order: every float
// operation single precision, operand for operand (build with
// -ffp-contract=off: no multiply-add may be fused or contracted), every
// integer operation at the width the MIPS one has (mult keeps the low word
// of the product, addu/subu wrap at 32 bits, the bit streams are 64-bit
// registers shifted by dsllv/dsrlv), so RDRAM ends up as the recompiled
// routine leaves it, bit for bit, the dead stack slots it saves registers
// to included. (GCC still makes vmla/vmls of a multiply and an add for the
// ARM11: VFPv2's are not fused, they round the product first, so they are
// the two operations' bits.) A host harness, bones_test, checks it against
// the recompiled bodies on console RDRAM captures (kept out of the repo:
// it builds from the ROM-derived RecompiledFuncs).
//
// Being a host function it has no TRACE_ENTRY: no preemption check at its
// entry (nor at its four subroutines') and, in the sampling profile, its
// time goes to its caller.
//
// What the asm does that C does not say by itself:
// - It sets the FPU's FR bit (Status bit 26) for its body, so the odd
//   float registers are separate singles (it uses all 32), and clears it
//   before it returns. The registers it saves on entry and restores on exit
//   are the FR=0 view (f20-f31 as six doubles), so in the caller's view the
//   float registers come back as they were; Status comes back with FR clear.
// - The keyframe streams are read 64 bits at a time with ldl/ldr from any
//   byte address, and each field is cut from the top of the register with
//   dsrlv and consumed with dsllv: a big-endian bit reader, refilled once
//   more than 64 bits would have been used.
// - A few loops are do-while on a count or an end pointer: a model with
//   no bones, or an animation with one channel, would run them past the
//   end. The port keeps them so (the game never gets there).
//
// Nothing is returned (v0/v1 are not written); s0-s7, ra and sp come back
// as the asm restores them (the low words of the saved values, sign-
// extended, as lw does).
//
// RDRAM words are host-endian at the N64 address less 0x80000000, and
// halfwords and bytes are at the address XOR 2 and XOR 3 (recomp.h's
// MEM_W, MEM_H, MEM_B).
#include <cmath>
#include <cstdint>
#include <cstring>

#include "recomp.h"

// bones_test builds this file with BONES_NATIVE_COVERAGE to count which of
// the asm's paths the captures take.
#ifdef BONES_NATIVE_COVERAGE
extern "C" unsigned long long bones_native_cov[64];
#define COV(n) (bones_native_cov[n]++)
#else
#define COV(n) ((void)0)
#endif

namespace {

// The globals the routine works in (DK64 US). D_80746E20 holds the actor
// being built; the four arrays hold three halfwords per animation channel:
// the rotations and scales of the first pose (6E24, 725C) and of the second
// one blended with it (7040, 7478). The sine table is the asm's own data
// (516 words after its code): a quarter turn of sin in 512 steps.
constexpr uint32_t kCurActor = 0x80746E20u;
constexpr uint32_t kRotA = 0x80746E24u;
constexpr uint32_t kScaleA = 0x8074725Cu;
constexpr uint32_t kRotB = 0x80747040u;
constexpr uint32_t kScaleB = 0x80747478u;
constexpr uint32_t kSinTable = 0x8061ACA0u;
constexpr uint32_t kRootScale = 0x80757490u;   // the root translation's units to the actor matrix's
constexpr uint32_t kStatusFR = 0x04000000u;

// RDRAM accesses. may_alias: the same words are written as floats, read
// as words and written as halfwords by the game.
typedef uint32_t __attribute__((may_alias)) u32a;
typedef uint16_t __attribute__((may_alias)) u16a;

struct Rd {
    uint8_t* base;
    uint32_t lw(uint32_t a) const { return *(const u32a*)(base + (a - 0x80000000u)); }
    void sw(uint32_t a, uint32_t v) const { *(u32a*)(base + (a - 0x80000000u)) = v; }
    uint32_t lhu(uint32_t a) const { return *(const u16a*)(base + ((a ^ 2u) - 0x80000000u)); }
    int32_t lh(uint32_t a) const { return (int16_t)lhu(a); }
    void sh(uint32_t a, uint32_t v) const { *(u16a*)(base + ((a ^ 2u) - 0x80000000u)) = (uint16_t)v; }
    uint32_t lbu(uint32_t a) const { return base[(a ^ 3u) - 0x80000000u]; }
    int32_t lb(uint32_t a) const { return (int8_t)lbu(a); }
    float lwc1(uint32_t a) const { const uint32_t v = lw(a); float f; memcpy(&f, &v, 4); return f; }
    void swc1(uint32_t a, float f) const { uint32_t v; memcpy(&v, &f, 4); sw(a, v); }
    // ldl + ldr at a and a+7: the eight bytes from a, big-endian, whatever
    // a's alignment (the register's old value is wholly replaced).
    uint64_t ld_unaligned(uint32_t a) const {
        const uint32_t al = a & ~3u, sh8 = (a & 3u) * 8;
        const uint32_t w0 = lw(al), w1 = lw(al + 4);
        if (sh8 == 0) return ((uint64_t)w0 << 32) | w1;
        const uint32_t w2 = lw(al + 8);
        return ((uint64_t)((w0 << sh8) | (w1 >> (32 - sh8))) << 32) | ((w1 << sh8) | (w2 >> (32 - sh8)));
    }
    float sin_tab(uint32_t byte_off) const { return lwc1(kSinTable + byte_off); }
};

inline float f32_of(uint32_t v) { float f; memcpy(&f, &v, 4); return f; }

// cvt.w.s: to the nearest integer in the current rounding mode. recomp.h's
// CVT_W_S is lrintf; on the ARM11 that is newlib's, which is vcvtr.s32.f32
// in effect (the same rounding, saturating where the exponent is too
// large, 0 for NaN) and costs a call and a dozen instructions more.
inline int32_t cvt_w_s(float x) {
#if defined(__arm__) && defined(__VFP_FP__) && !defined(__SOFTFP__)
    float r;
    __asm__("vcvtr.s32.f32 %0, %1" : "=t"(r) : "t"(x));
    int32_t i;
    memcpy(&i, &r, 4);
    return i;
#else
    return (int32_t)lrintf(x);
#endif
}

// The bit reader of two keyframes' streams side by side (the asm's s3/s7
// registers, t4/s4 their byte pointers, s2 the bits of the current 64
// used). Both streams have the same layout, so they advance together.
struct Streams {
    uint64_t a, b;
    uint32_t pa, pb;
    int32_t used;

    void init(const Rd& m, uint32_t frame_a, uint32_t frame_b) {
        pa = frame_a;
        pb = frame_b;
        a = m.ld_unaligned(pa);
        b = m.ld_unaligned(pb);
        used = 0;
    }
    // Account for a field of w bits; past 64 bits, reload both registers
    // from the first unread byte (the bits of it already read shifted out).
    void need(const Rd& m, uint32_t w) {
        used += (int32_t)w;
        if (used >= 65) {
            COV(0);
            used -= (int32_t)w;
            const uint32_t adv = (uint32_t)used >> 3;
            pb += adv;
            pa += adv;
            a = m.ld_unaligned(pa);
            b = m.ld_unaligned(pb);
            used &= 7;
            a <<= (used & 63);
            b <<= (used & 63);
            used += (int32_t)w;
        }
    }
    // The top w bits of each (dsrlv by 64 - w), then consume them (dsllv).
    void take(uint32_t w, uint64_t& va, uint64_t& vb) {
        const uint32_t sh = (64u - w) & 63u;
        va = a >> sh;
        vb = b >> sh;
        a <<= (w & 63u);
        b <<= (w & 63u);
    }
};

// One rotation field: the first keyframe's value plus the second's
// difference (an 11-bit signed delta) times the fraction t9 (1024ths),
// in units of 32 (the low 4 bits of the channel word are the field's width,
// bit 4 says a scale follows).
inline uint32_t rot_field(uint64_t va, uint64_t vb, int32_t t9) {
    int32_t d = (int32_t)((uint32_t)vb - (uint32_t)va);
    d = (int32_t)((uint32_t)d << 21) >> 21;
    const int32_t m = (int32_t)((uint32_t)d * (uint32_t)t9) >> 10;
    return ((uint32_t)va + (uint32_t)m) << 5;
}

// One scale field: units of 16, the interpolation's fraction in the low
// 4 bits (srl: the product taken unsigned).
inline uint32_t scale_field(uint64_t va, uint64_t vb, int32_t t9) {
    const uint32_t d = (uint32_t)vb - (uint32_t)va;
    const uint32_t m = d * (uint32_t)t9;
    return ((uint32_t)va << 4) + (m >> 6);
}

// 80619C2C: decode an animation slot's pose (its keyframes slot->unkC and
// slot->unkE, slot->unk8 the fraction between them) into a rotation and a
// scale array (three halfwords per channel) and the root translation. With
// use_b both streams read the second keyframe. An empty slot zeroes the
// arrays (the model's bone count of channels) and the translation.
void decode_pose(const Rd& m, uint32_t actor, uint32_t slot, uint32_t hdr, bool use_b,
                 uint32_t rot, uint32_t scale, float root[3]) {
    if (hdr == 0) {
        COV(1);
        const uint32_t model = m.lw(actor);
        const uint32_t end = rot + m.lbu(model + 0x20) * 6;
        do {
            m.sh(rot, 0);
            rot += 2;
            m.sh(scale, 0);
            scale += 2;
        } while (rot != end);
        root[0] = root[1] = root[2] = 0.0f;
        return;
    }
    const uint32_t stride = m.lbu(hdr + 0x13);
    const uint32_t data = (uint32_t)m.lh(hdr + 6) + hdr;
    uint32_t chan = hdr + 0x14;
    const uint32_t nchan = m.lbu(hdr + 0x11);
    uint32_t frame_a = (uint32_t)m.lh(slot + 0xC) * stride + data;
    const uint32_t frame_b = (uint32_t)m.lh(slot + 0xE) * stride + data;
    if (use_b) { COV(2); frame_a = frame_b; }
    // Header flag 0x20: hold the first keyframe (no interpolation).
    int32_t t9 = 0;
    if ((m.lh(hdr + 4) & 0x20) == 0) t9 = cvt_w_s(m.lwc1(slot + 8) * 1024.0f);
    else COV(3);

    Streams s;
    s.init(m, frame_a, frame_b);
    // The root translation: a base (header halfwords 8, A, C, in 1024ths)
    // plus an interpolated field of the header's width (bytes E, F, 10).
    for (int k = 0; k < 3; k++) {
        uint32_t t7 = (uint32_t)m.lh(hdr + 8 + 2 * k) << 10;
        const uint32_t w = m.lbu(hdr + 0xE + k);
        if (w != 0) {
            const uint32_t sh = (64u - w) & 63u;
            const uint64_t va = s.a >> sh, vb = s.b >> sh;
            const uint32_t d = (uint32_t)vb - (uint32_t)va;
            const uint32_t lo = d * (uint32_t)t9;
            t7 += ((uint32_t)va << 10) + lo;
            s.used += (int32_t)w;
            s.b <<= (w & 63u);
            s.a <<= (w & 63u);
        } else {
            COV(4);
        }
        root[k] = (float)(int32_t)t7;
    }

    // Three channels per bone after the root; each a halfword: the value's
    // top 12 bits, its field width, and bit 4 when a scale halfword follows.
    uint32_t count = (nchan - 1) * 3;
    for (;;) {
        uint32_t v = m.lhu(chan);
        const uint32_t w = v & 0xF;
        v &= 0xFFF0;
        if (w != 0) {
            s.need(m, w);
            uint64_t va, vb;
            s.take(w, va, vb);
            v += rot_field(va, vb, t9);
        } else {
            COV(5);
        }
        m.sh(rot, v);
        if ((v & 0x10) == 0) {
            count--;
            m.sh(scale, 0);
            scale += 2;
            rot += 2;
            chan += 2;
            if (count == 0) return;
            continue;
        }
        // The scale: a halfword of 0 is a 16-bit field from 0; otherwise
        // its top 12 bits plus a field of its low 4 bits' width (none: as is).
        COV(6);
        uint32_t q = m.lhu(chan + 2);
        chan += 4;
        rot += 2;
        uint32_t ws = 16;
        bool field = true;
        if (q != 0) {
            ws = q & 0xF;
            q ^= ws;
            field = ws != 0;
        } else {
            COV(7);
        }
        if (field) {
            s.need(m, ws);
            uint64_t va, vb;
            s.take(ws, va, vb);
            q += scale_field(va, vb, t9);
        } else {
            COV(8);
        }
        count--;
        m.sh(scale, q);
        scale += 2;
        if (count == 0) return;
    }
}

// 80619F50: an additive layer (an animation slot of its own) blended over
// a pose by weight/256: the rotations and scales its channels give
// replace the pose's in that proportion; a channel whose words are all
// clear (no field, no scale) leaves the bone alone. Its root translation is
// skipped.
void blend_layer(const Rd& m, uint32_t slot, uint32_t hdr, uint32_t rot, uint32_t scale, uint32_t weight) {
    COV(9);
    const uint32_t stride = m.lbu(hdr + 0x13);
    const int32_t t9 = cvt_w_s(m.lwc1(slot + 8) * 1024.0f);
    const uint32_t data = (uint32_t)m.lh(hdr + 6) + hdr;
    const uint32_t skip = m.lbu(hdr + 0xF) + m.lbu(hdr + 0xE) + m.lbu(hdr + 0x10);
    uint32_t count = m.lbu(hdr + 0x11) - 1;
    uint32_t chan = hdr + 0x14;
    Streams s;
    s.init(m, (uint32_t)m.lh(slot + 0xC) * stride + data, (uint32_t)m.lh(slot + 0xE) * stride + data);
    s.used = (int32_t)skip;
    s.a <<= (skip & 63u);
    s.b <<= (skip & 63u);
    do {
        if (((m.lhu(chan) | m.lhu(chan + 2) | m.lhu(chan + 4)) & 0x1F) == 0) {
            COV(10);
            chan += 6;
            rot += 6;
            scale += 6;
            continue;
        }
        COV(11);
        for (int k = 0; k < 3; k++) {
            uint32_t v = m.lhu(chan);
            const uint32_t w = v & 0xF;
            v &= 0xFFF0;
            if (w != 0) {
                s.need(m, w);
                uint64_t va, vb;
                s.take(w, va, vb);
                v += rot_field(va, vb, t9);
            }
            int32_t old = m.lh(rot);
            int32_t nv = (int16_t)v;
            int32_t mix = (int32_t)((uint32_t)(nv - old) * weight);
            m.sh(rot, (uint32_t)((mix >> 8) + old));
            uint32_t q = 0;
            if (nv & 0x10) {
                COV(12);
                q = m.lhu(chan + 2);
                chan += 2;
                uint32_t ws = 16;
                bool field = true;
                if (q != 0) {
                    ws = q & 0xF;
                    q ^= ws;
                    field = ws != 0;
                } else {
                    COV(13);
                }
                if (field) {
                    s.need(m, ws);
                    uint64_t va, vb;
                    s.take(ws, va, vb);
                    q += scale_field(va, vb, t9);
                }
            }
            old = m.lh(scale);
            nv = (int16_t)q;
            mix = (int32_t)((uint32_t)(nv - old) * weight);
            chan += 2;
            rot += 2;
            scale += 2;
            m.sh(scale - 2, (uint32_t)((mix >> 8) + old));
        }
    } while (--count != 0);
}

// 8061A1A0: the animation state's offset list (unk1C, 8-byte entries up to
// a weight of 0): add weight/128 of three angles to a channel's rotation,
// or, for a negative weight, move the rotation toward them by -weight/128.
void apply_offsets(const Rd& m, uint32_t as, uint32_t rot) {
    uint32_t p = m.lw(as + 0x1C);
    if (p == 0) return;
    COV(14);
    for (;;) {
        int32_t k = m.lb(p);
        if (k == 0) return;
        const uint32_t e = rot + m.lbu(p + 1) * 6;
        if (k >= 0) {
            COV(15);
            for (int i = 0; i < 3; i++) {
                const int32_t prod = (int32_t)((uint32_t)k * (uint32_t)m.lh(p + 2 + 2 * i));
                m.sh(e + 2 * i, (uint32_t)((prod >> 7) + m.lh(e + 2 * i)));
            }
        } else {
            COV(16);
            k = -k;
            for (int i = 0; i < 3; i++) {
                const int32_t cur = m.lh(e + 2 * i);
                const int32_t prod = (int32_t)((uint32_t)(m.lh(p + 2 + 2 * i) - cur) * (uint32_t)k);
                m.sh(e + 2 * i, (uint32_t)((prod >> 7) + cur));
            }
        }
        p += 8;
    }
}

// 80619B44's half-angle lookup: a 16-bit angle's top bit picks the half
// turn, the next 9 the table step; c, s are the asm's even and odd
// registers (cos and sin of half the angle).
inline void half_sincos(const Rd& m, int32_t v, float& c, float& s) {
    const uint32_t i = (((uint32_t)v >> 6) << 2) & 0x7FC;
    const uint32_t j = 0x800 - i;
    if (v >= 0) {
        s = m.sin_tab(i);
        c = m.sin_tab(j);
    } else {
        COV(17);
        c = 0.0f - m.sin_tab(i);   // sub.s from zero: +0 stays +0
        s = m.sin_tab(j);
    }
}

// The fast path's full-angle lookup: the top 2 of 11 bits the quadrant.
inline void full_sincos(const Rd& m, uint32_t v, float& c, float& s) {
    const uint32_t t2 = v >> 5;
    const uint32_t quad = t2 & 0x600;
    const uint32_t i = (t2 << 2) & 0x7FC;
    const uint32_t j = 0x800 - i;
    if (quad == 0x600) {
        COV(18);
        c = m.sin_tab(i);
        s = 0.0f - m.sin_tab(j);
    } else if (quad == 0x400) {
        COV(19);
        s = 0.0f - m.sin_tab(i);
        c = 0.0f - m.sin_tab(j);
    } else if (quad == 0x200) {
        COV(20);
        c = 0.0f - m.sin_tab(i);
        s = m.sin_tab(j);
    } else {
        s = m.sin_tab(i);
        c = m.sin_tab(j);
    }
}

// 80619B44: a channel's three angles to the products the quaternion is
// made of (the asm's f6-f13, f18, f19).
struct Euler { float f6, f7, f8, f9, f10, f11, f12, f13, f18, f19; };
inline Euler euler(const Rd& m, uint32_t p) {
    float f0, f1, f2, f3;
    Euler e;
    half_sincos(m, m.lh(p), f0, f1);
    half_sincos(m, m.lh(p + 2), f2, f3);
    e.f6 = f0 * f2;
    e.f7 = f0 * f3;
    e.f8 = f1 * f2;
    e.f9 = f1 * f3;
    half_sincos(m, m.lh(p + 4), e.f18, e.f19);
    e.f10 = e.f6 * e.f18;
    e.f11 = e.f9 * e.f19;
    e.f12 = e.f8 * e.f18;
    e.f13 = e.f7 * e.f19;
    return e;
}

// A row's three elements times a scale halfword's value/32768, when it has
// one, stored at out.
inline void store_row(const Rd& m, uint32_t out, uint32_t sc, float a, float b, float c) {
    if (sc != 0) {
        COV(21);
        const float k = (float)(int32_t)sc * f32_of(0x38000000u);
        a = a * k;
        b = b * k;
        c = c * k;
    }
    m.swc1(out, a);
    m.swc1(out + 4, b);
    m.swc1(out + 8, c);
}

// 806197B8-80619AA4: blend the second pose into the first by w (the root
// translation and the scales linearly, the rotations as quaternions, the
// shorter way round) and write each bone's rotation-scale matrix (and its
// rest position, from the model's bone list) into actor->unk8.
void blend_and_build(const Rd& m, uint32_t actor, const float a_root[3], float root[3], float w) {
    const float one = 1.0f;
    const float f27 = one - w;
    root[0] = (a_root[0] - root[0]) * f27 + root[0];
    root[1] = (a_root[1] - root[1]) * f27 + root[1];
    root[2] = (a_root[2] - root[2]) * f27 + root[2];
    const int32_t t6 = cvt_w_s(1024.0f * w);
    const uint32_t model = m.lw(actor);
    uint32_t nbones = m.lbu(model + 0x20);
    const uint32_t mats = m.lw(actor + 8);
    uint32_t desc = m.lw(model + 8);

    // Scales: an unset one (0) counts as 1 (0x8000) against a set one;
    // two unset stay unset.
    {
        uint32_t pb = kScaleB, pa = kScaleA;
        const uint32_t end = kScaleB + nbones * 6;
        do {
            uint32_t b = m.lhu(pb);
            pb += 2;
            uint32_t a = m.lhu(pa);
            pa += 2;
            if (b == 0 && a == 0) { COV(22); continue; }
            if (b == 0) { COV(23); b = 0x8000; }
            if (a == 0) { COV(24); a = 0x8000; }
            const int32_t mix = (int32_t)((b - a) * (uint32_t)t6);
            m.sh(pa - 2, (uint32_t)((mix >> 10) + (int32_t)a));
        } while (pb != end);
    }

    do {
        const uint32_t off = m.lbu(desc + 2) * 6;
        const uint32_t sc = kScaleA + off;
        // The first pose's quaternion (f20, f21, f25, f26), the second's
        // (f4-f7).
        const Euler ea = euler(m, kRotA + off);
        float f20 = ea.f10 + ea.f11;
        float f21 = ea.f12 - ea.f13;
        float f25 = ea.f7 * ea.f18 + ea.f8 * ea.f19;
        float f26 = ea.f6 * ea.f19 - ea.f9 * ea.f18;
        const Euler eb = euler(m, kRotB + off);
        float f4 = eb.f10 + eb.f11;
        float f5 = eb.f12 - eb.f13;
        float f6 = eb.f7 * eb.f18 + eb.f8 * eb.f19;
        float f7 = eb.f6 * eb.f19 - eb.f9 * eb.f18;
        float dot = f20 * f4;
        dot = dot + f21 * f5;
        dot = dot + f25 * f6;
        dot = dot + f26 * f7;
        f20 = f20 * f27;
        f21 = f21 * f27;
        if (dot < 0.0f) {
            COV(25);
            f4 = 0.0f - f4;
            f5 = 0.0f - f5;
            f6 = 0.0f - f6;
            f7 = 0.0f - f7;
        }
        f4 = f4 * w;
        f5 = f5 * w;
        const float qw = f20 + f4;
        f25 = f25 * f27;
        const float qx = f21 + f5;
        f6 = f6 * w;
        f26 = f26 * f27;
        const float qy = f25 + f6;
        f7 = f7 * w;
        const float qz = f26 + f7;
        float n = qw * qw + qx * qx;
        n = n + qy * qy;
        n = n + qz * qz;
        const float k = 2.0f / n;
        // The quaternion's matrix, unnormalised (2/|q|^2 scales it).
        const float a1 = qy * k, a2 = qz * k, a0 = qx * k;
        const float p4 = qw * a1, p8 = qx * a2, p7 = qx * a1, p5 = qw * a2;
        const float p15 = qy * a1, p17 = qz * a2, p16 = qy * a2;
        const float p3 = qw * a0, p6 = qx * a0;
        const float r00 = one - (p15 + p17), r01 = p7 + p5, r02 = p8 - p4;
        const float r10 = p7 - p5, r11 = one - (p6 + p17), r12 = p16 + p3;
        const float r20 = p8 + p4, r21 = p16 - p3, r22 = one - (p6 + p15);
        const uint32_t out = mats + (m.lbu(desc + 1) << 6);
        store_row(m, out, m.lhu(sc), r00, r01, r02);
        store_row(m, out + 0x10, m.lhu(sc + 2), r10, r11, r12);
        store_row(m, out + 0x20, m.lhu(sc + 4), r20, r21, r22);
        m.sw(out + 0x30, m.lw(desc + 4));
        m.sw(out + 0x34, m.lw(desc + 8));
        m.sw(out + 0x38, m.lw(desc + 0xC));
        desc += 16;
    } while (--nbones != 0);
}

// 8061A2A8: one pose, no blending: each bone's matrix straight from its
// Euler angles (full-angle lookups, x, then z, then y).
void build_direct(const Rd& m, uint32_t actor) {
    COV(26);
    const uint32_t mats = m.lw(actor + 8);
    const uint32_t model = m.lw(actor);
    uint32_t desc = m.lw(model + 8);
    const uint32_t end = (m.lbu(model + 0x20) << 4) + desc;
    do {
        const uint32_t out = (m.lbu(desc + 1) << 6) + mats;
        const uint32_t off = m.lbu(desc + 2) * 6;
        const uint32_t sc = kScaleA + off;
        const uint32_t rp = kRotA + off;
        float f0, f1, f2, f3, f4, f5;
        full_sincos(m, m.lhu(rp), f0, f1);
        full_sincos(m, m.lhu(rp + 4), f4, f5);
        const float f6 = f1 * f5, f7 = f0 * f5, f8 = f1 * f4, f9 = f0 * f4;
        m.sw(out + 0x30, m.lw(desc + 4));
        full_sincos(m, m.lhu(rp + 2), f2, f3);
        m.sw(out + 0x38, m.lw(desc + 0xC));
        m.sw(out + 0x34, m.lw(desc + 8));
        store_row(m, out, m.lhu(sc), f2 * f4, f2 * f5, 0.0f - f3);
        store_row(m, out + 0x10, m.lhu(sc + 2), f8 * f3 - f7, f6 * f3 + f9, f1 * f2);
        store_row(m, out + 0x20, m.lhu(sc + 4), f9 * f3 + f6, f7 * f3 - f8, f0 * f2);
        desc += 16;
    } while (desc != end);
}

// 8061A61C, 8061A758: the animation state's per-bone scale list (unk20)
// and translation list (unk24), 16-byte entries up to a bone of 0xFF: a
// scale multiplies a bone's rotation rows (its second bone gets the
// inverse), a translation adds to a bone's position (the second bone's has
// it taken off).
void apply_bone_scales(const Rd& m, uint32_t actor) {
    uint32_t p = m.lw(m.lw(actor + 4) + 0x20);
    if (p == 0) return;
    COV(27);
    const uint32_t mats = m.lw(actor + 8);
    auto scale_rows = [&](uint32_t mtx, float x, float y, float z) {
        for (int i = 0; i < 3; i++) m.swc1(mtx + 4 * i, m.lwc1(mtx + 4 * i) * x);
        for (int i = 0; i < 3; i++) m.swc1(mtx + 0x10 + 4 * i, m.lwc1(mtx + 0x10 + 4 * i) * y);
        for (int i = 0; i < 3; i++) m.swc1(mtx + 0x20 + 4 * i, m.lwc1(mtx + 0x20 + 4 * i) * z);
    };
    for (;;) {
        const uint32_t i = m.lbu(p);
        if (i == 0xFF) return;
        float x = m.lwc1(p + 4), y = m.lwc1(p + 8), z = m.lwc1(p + 0xC);
        scale_rows(mats + (i << 6), x, y, z);
        const uint32_t j = m.lbu(p + 1);
        if (j != 0xFF) {
            COV(28);
            const float one = 1.0f;
            x = one / x;
            y = one / y;
            z = one / z;
            scale_rows(mats + (j << 6), x, y, z);
        }
        p += 16;
    }
}

void apply_bone_translations(const Rd& m, uint32_t actor) {
    uint32_t p = m.lw(m.lw(actor + 4) + 0x24);
    if (p == 0) return;
    COV(29);
    const uint32_t mats = m.lw(actor + 8);
    for (;;) {
        const uint32_t i = m.lbu(p);
        if (i == 0xFF) return;
        const float x = m.lwc1(p + 4), y = m.lwc1(p + 8), z = m.lwc1(p + 0xC);
        uint32_t mtx = mats + (i << 6);
        m.swc1(mtx + 0x30, m.lwc1(mtx + 0x30) + x);
        m.swc1(mtx + 0x34, m.lwc1(mtx + 0x34) + y);
        m.swc1(mtx + 0x38, m.lwc1(mtx + 0x38) + z);
        const uint32_t j = m.lbu(p + 1);
        if (j != 0xFF) {
            COV(30);
            mtx = mats + (j << 6);
            m.swc1(mtx + 0x30, m.lwc1(mtx + 0x30) - x);
            m.swc1(mtx + 0x34, m.lwc1(mtx + 0x34) - y);
            m.swc1(mtx + 0x38, m.lwc1(mtx + 0x38) - z);
        }
        p += 16;
    }
}

// A 4x3 affine matrix (the fourth column is implied): rows 0-2 the
// rotation-scale, row 3 the translation.
struct Affine { float e[12]; };
inline Affine load_affine(const Rd& m, uint32_t mtx) {
    Affine r;
    for (int row = 0; row < 4; row++)
        for (int c = 0; c < 3; c++) r.e[row * 3 + c] = m.lwc1(mtx + row * 0x10 + c * 4);
    return r;
}

// 8061A938: bone = bone * parent, in place, as the asm sums: per row,
// (a*p0 + b*p3) + c*p6, (c*p7 + a*p1) + b*p4, (a*p2 + b*p5) + c*p8, and
// for the translation row ((x*p0 + p9) + y*p3) + z*p6 and so on.
inline Affine concat(const Rd& m, uint32_t mtx, const Affine& p) {
    Affine r;
    for (int row = 0; row < 3; row++) {
        const float a = m.lwc1(mtx + row * 0x10), b = m.lwc1(mtx + row * 0x10 + 4), c = m.lwc1(mtx + row * 0x10 + 8);
        r.e[row * 3 + 0] = (a * p.e[0] + b * p.e[3]) + c * p.e[6];
        r.e[row * 3 + 1] = (c * p.e[7] + a * p.e[1]) + b * p.e[4];
        r.e[row * 3 + 2] = (a * p.e[2] + b * p.e[5]) + c * p.e[8];
    }
    const float x = m.lwc1(mtx + 0x30), y = m.lwc1(mtx + 0x34), z = m.lwc1(mtx + 0x38);
    r.e[9] = ((x * p.e[0] + p.e[9]) + y * p.e[3]) + z * p.e[6];
    r.e[10] = ((x * p.e[1] + p.e[10]) + y * p.e[4]) + z * p.e[7];
    r.e[11] = ((x * p.e[2] + p.e[11]) + y * p.e[5]) + z * p.e[8];
    for (int row = 0; row < 4; row++)
        for (int c = 0; c < 3; c++) m.swc1(mtx + row * 0x10 + c * 4, r.e[row * 3 + c]);
    return r;
}

// 8061A7F0: the hierarchy. The first bone hangs from the actor's own
// matrix (actor+0xC) moved by the root translation; each next one from its
// parent bone (the previous bone's result as computed when it is the
// parent, from RDRAM otherwise).
void concat_hierarchy(const Rd& m, uint32_t actor, const float root[3]) {
    const uint32_t model = m.lw(actor);
    const uint32_t mats = m.lw(actor + 8);
    uint32_t desc = m.lw(model + 8);
    uint32_t n = m.lbu(model + 0x20);
    Affine p = load_affine(m, actor + 0xC);
    const float k = m.lwc1(kRootScale);
    const float x = root[0] * k, y = root[1] * k, z = root[2] * k;
    p.e[9] = p.e[9] + ((x * p.e[0] + y * p.e[3]) + z * p.e[6]);
    p.e[10] = p.e[10] + ((x * p.e[1] + y * p.e[4]) + z * p.e[7]);
    p.e[11] = p.e[11] + ((x * p.e[2] + y * p.e[5]) + z * p.e[8]);
    uint32_t prev = m.lbu(desc + 1);
    Affine r = concat(m, mats + (prev << 6), p);
    while (--n != 0) {
        desc += 16;
        const uint32_t idx = m.lbu(desc + 1);
        const uint32_t parent = m.lbu(desc);
        if (parent == prev) {
            COV(31);
            p = r;
        } else {
            COV(32);
            p = load_affine(m, mats + (parent << 6));
        }
        prev = idx;
        r = concat(m, mats + (idx << 6), p);
    }
}

// 8061AAA0: each float matrix to an Mtx in place: s15.16 by element, the
// integer halves in the first 32 bytes, the fractions in the next 32, the
// fourth column 0 0 0 1. Rounded by cvt.w.s.
void to_fixed(const Rd& m, uint32_t actor) {
    uint32_t n = m.lbu(m.lw(actor) + 0x20);
    uint32_t mtx = m.lw(actor + 8);
    const float k = 65536.0f;
    do {
        int32_t v[12];
        for (int row = 0; row < 4; row++)
            for (int c = 0; c < 3; c++) v[row * 3 + c] = cvt_w_s(m.lwc1(mtx + row * 0x10 + c * 4) * k);
        for (int row = 0; row < 4; row++) {
            const uint32_t x = (uint32_t)v[row * 3], y = (uint32_t)v[row * 3 + 1], z = (uint32_t)v[row * 3 + 2];
            m.sw(mtx + row * 8, (x & 0xFFFF0000u) | (y >> 16));
            m.sw(mtx + row * 8 + 4, (z & 0xFFFF0000u) + (row == 3 ? 1u : 0u));
            m.sw(mtx + 0x20 + row * 8, (x << 16) | (y & 0xFFFFu));
            m.sw(mtx + 0x20 + row * 8 + 4, z << 16);
        }
        mtx += 0x40;
    } while (--n != 0);
}

}   // namespace

static void bones_build(uint8_t* rdram, recomp_context* ctx) {
    const Rd m{ rdram };
    const uint32_t actor = (uint32_t)ctx->r4;

    // The prologue's saves (ra, s0-s7 at sp-0x28; f20-f31 at sp-0x58 as
    // the caller's FR=0 view sees them: an even register's low and high
    // words) go to RDRAM as the asm's do, and come back at the end.
    const uint32_t sp = (uint32_t)ctx->r29;
    const uint32_t gsave = sp - 0x28, fsave = gsave - 0x30;
    uint32_t saved[9] = { (uint32_t)ctx->r31, (uint32_t)ctx->r16, (uint32_t)ctx->r17, (uint32_t)ctx->r18,
                          (uint32_t)ctx->r19, (uint32_t)ctx->r20, (uint32_t)ctx->r21, (uint32_t)ctx->r22,
                          (uint32_t)ctx->r23 };
    for (int i = 0; i < 9; i++) m.sw(gsave + 4 * i, saved[i]);
    fpr* const fe[6] = { &ctx->f20, &ctx->f22, &ctx->f24, &ctx->f26, &ctx->f28, &ctx->f30 };
    uint32_t fsaved[12];
    for (int i = 0; i < 6; i++) {
        fsaved[2 * i] = fe[i]->u32l;
        fsaved[2 * i + 1] = ctx->f_odd[(20 + 2 * i + 1 - 1) * 2];
        m.sw(fsave + 8 * i, fsaved[2 * i]);
        m.sw(fsave + 8 * i + 4, fsaved[2 * i + 1]);
    }
    const uint32_t status = ctx->status_reg;

    m.sw(kCurActor, actor);
    const uint32_t as = m.lw(actor + 4);

    // The first pose (slot unk0), its layer (unk4, weight unk28), the
    // offsets.
    float root_a[3], root[3];
    uint32_t slot = m.lw(as);
    decode_pose(m, actor, slot, m.lw(slot), false, kRotA, kScaleA, root_a);
    slot = m.lw(as + 4);
    uint32_t hdr = m.lw(slot);
    if (hdr != 0 && m.lhu(as + 0x28) != 0) blend_layer(m, slot, hdr, kRotA, kScaleA, m.lhu(as + 0x28));
    apply_offsets(m, as, kRotA);

    // The second pose: another animation (slot unk8, its layer unkC with
    // weight unk2A) blended in by unk10; or, with a single animation whose
    // header asks for it (flag 0x20), its next keyframe, blended in by the
    // slot's fraction. Otherwise the single pose as it is.
    float w;
    slot = m.lw(as + 8);
    hdr = m.lw(slot);
    bool second = hdr != 0;
    if (!second) {
        slot = m.lw(as);
        hdr = m.lw(slot);
        if (hdr == 0) {
            COV(33);
            second = true;   // an empty slot: decode_pose zeroes the second pose
        } else if ((m.lh(hdr + 4) & 0x20) == 0) {
            build_direct(m, actor);
            goto finish;
        }
    }
    if (second) {
        COV(34);
        decode_pose(m, actor, slot, hdr, false, kRotB, kScaleB, root);
        slot = m.lw(as + 0xC);
        hdr = m.lw(slot);
        if (hdr != 0 && m.lhu(as + 0x2A) != 0) blend_layer(m, slot, hdr, kRotB, kScaleB, m.lhu(as + 0x2A));
        apply_offsets(m, as, kRotB);
        w = m.lwc1(as + 0x10);
    } else {
        COV(35);
        slot = m.lw(as);
        decode_pose(m, actor, slot, m.lw(slot), true, kRotB, kScaleB, root);
        slot = m.lw(as + 4);
        hdr = m.lw(slot);
        if (hdr != 0 && m.lhu(as + 0x28) != 0) blend_layer(m, slot, hdr, kRotB, kScaleB, m.lhu(as + 0x28));
        apply_offsets(m, as, kRotB);
        w = m.lwc1(m.lw(as) + 8);
    }
    blend_and_build(m, actor, root_a, root, w);
    root_a[0] = root[0];
    root_a[1] = root[1];
    root_a[2] = root[2];

finish:
    apply_bone_scales(m, actor);
    apply_bone_translations(m, actor);
    concat_hierarchy(m, actor, root_a);
    to_fixed(m, actor);

    // The epilogue: FR cleared again (the odd singles alias the even
    // registers' high words), the saved registers back.
    ctx->status_reg = status & ~kStatusFR;
    ctx->f_odd = &ctx->f0.u32h;
    ctx->mips3_float_mode = false;
    for (int i = 0; i < 6; i++) {
        fe[i]->u32l = fsaved[2 * i];
        ctx->f_odd[(20 + 2 * i + 1 - 1) * 2] = fsaved[2 * i + 1];
    }
    ctx->r31 = (gpr)(int32_t)saved[0];
    gpr* const s[8] = { &ctx->r16, &ctx->r17, &ctx->r18, &ctx->r19, &ctx->r20, &ctx->r21, &ctx->r22, &ctx->r23 };
    for (int i = 0; i < 8; i++) *s[i] = (gpr)(int32_t)saved[i + 1];
    ctx->r29 = (gpr)(int32_t)sp;
}

// What the recompiled routine's TRACE_ENTRY / TRACE_RETURN did (the game's
// include/trace.h): the preemption check at its entry, and in a
// DK64_GAME_PROFILE build its name for the sampling profile while it runs.
extern "C" unsigned int ultramodern_preempt_pending;
extern "C" void ultramodern_preempt(unsigned char* rdram);
#ifdef DK64_GAME_PROFILE
extern "C" const char* volatile recomp3ds_trace_func;
#endif

extern "C" void func_global_asm_806195D0(uint8_t* rdram, recomp_context* ctx) {
    if (*(volatile unsigned int*)&ultramodern_preempt_pending) {
        ultramodern_preempt(rdram);
    }
#ifdef DK64_GAME_PROFILE
    const char* const prev = recomp3ds_trace_func;
    recomp3ds_trace_func = "func_global_asm_806195D0 (native)";
    bones_build(rdram, ctx);
    recomp3ds_trace_func = prev;
#else
    bones_build(rdram, ctx);
#endif
}
