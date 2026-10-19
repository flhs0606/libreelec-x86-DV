/* Decode display capabilities and luminance from a Dolby VSVDB. */
#include "control_target.h"
#include "control_luminance.h"

static void put_word(void *target, unsigned int offset, unsigned int value)
{
    unsigned char *bytes = (unsigned char *)target + offset;
    bytes[0] = (unsigned char)value;
    bytes[1] = (unsigned char)(value >> 8);
    bytes[2] = (unsigned char)(value >> 16);
    bytes[3] = (unsigned char)(value >> 24);
}

#define PUT(field, value) put_word(target_param, CONTROL_TARGET_##field##_OFFSET, (value))

int vsvdb_to_target_param(const unsigned char *vsvdb_buf, void *target_param)
{
    unsigned int length, version, tmin, tmax, min_pq, max_pq;
    if (vsvdb_buf == (const unsigned char *)0 || vsvdb_buf[1] != 1u ||
        vsvdb_buf[2] != 0x46u || vsvdb_buf[3] != 0xd0u || vsvdb_buf[4] != 0u ||
        (vsvdb_buf[0] >> 5) != 7u)
        return -1;

    length = (vsvdb_buf[0] & 31u) + 1u;
    version = vsvdb_buf[5] >> 5;
    PUT(VSVDB_LENGTH, length);
    PUT(VSVDB_VERSION, version);
    PUT(SINK_DM_VERSION, (vsvdb_buf[5] >> 2) & 7u);

    if (version == 0u) {
        min_pq = ((unsigned int)vsvdb_buf[19] << 4) | (vsvdb_buf[18] >> 4);
        max_pq = ((unsigned int)vsvdb_buf[20] << 4) | (vsvdb_buf[18] & 15u);
        PUT(TMINPQ, min_pq);
        PUT(TMAXPQ, max_pq);
        PUT(TMIN, (unsigned int)((unsigned long long)control_pq12_to_l((unsigned short)min_pq) * 10000u >> 18));
        PUT(TMAX, (unsigned int)((unsigned long long)control_pq12_to_l((unsigned short)max_pq) * 10000u >> 18));
        PUT(RX, (((unsigned int)vsvdb_buf[7] << 4) | (vsvdb_buf[6] >> 4)) << 14);
        PUT(RY, (((unsigned int)vsvdb_buf[8] << 4) | (vsvdb_buf[6] & 15u)) << 14);
        PUT(GX, (((unsigned int)vsvdb_buf[10] << 4) | (vsvdb_buf[9] >> 4)) << 14);
        PUT(GY, (((unsigned int)vsvdb_buf[11] << 4) | (vsvdb_buf[9] & 15u)) << 14);
        PUT(BX, (((unsigned int)vsvdb_buf[13] << 4) | (vsvdb_buf[12] >> 4)) << 14);
        PUT(BY, (((unsigned int)vsvdb_buf[14] << 4) | (vsvdb_buf[12] & 15u)) << 14);
        PUT(WX, (((unsigned int)vsvdb_buf[16] << 4) | (vsvdb_buf[15] >> 4)) << 14);
        PUT(WY, (((unsigned int)vsvdb_buf[17] << 4) | (vsvdb_buf[15] & 15u)) << 14);
        return 0;
    }

    if (version == 1u) {
        tmin = (vsvdb_buf[7] >> 1) * (vsvdb_buf[7] >> 1) * 10000u / 16129u;
        tmax = (vsvdb_buf[6] >> 1) * 500000u + 1000000u;
        PUT(TMIN, tmin);
        PUT(TMAX, tmax);
        PUT(TMINPQ, control_l_to_pq12((unsigned int)(((unsigned long long)tmin << 18) / 10000u)));
        PUT(TMAXPQ, control_l_to_pq12((unsigned int)(((unsigned long long)tmax << 18) / 10000u)));
        if (length == 12u) {
            PUT(V1_LOW_LATENCY, vsvdb_buf[8] & 3u);
            PUT(RX, ((vsvdb_buf[11] >> 3) | 0xa0u) << 18);
            PUT(RY, (((vsvdb_buf[10] & 1u) << 1) | ((vsvdb_buf[11] & 7u) << 2) |
                     (vsvdb_buf[9] & 1u) | 0x40u) << 18);
            PUT(GX, (unsigned int)(vsvdb_buf[9] >> 1) << 18);
            PUT(GY, ((vsvdb_buf[10] >> 1) | 0x80u) << 18);
            PUT(BX, ((vsvdb_buf[8] >> 5) | 0x20u) << 18);
            PUT(BY, (((vsvdb_buf[8] >> 2) & 7u) | 8u) << 18);
        } else if (length == 15u) {
            PUT(RX, (unsigned int)vsvdb_buf[9] << 18);
            PUT(RY, (unsigned int)vsvdb_buf[10] << 18);
            PUT(GX, (unsigned int)vsvdb_buf[11] << 18);
            PUT(GY, (unsigned int)vsvdb_buf[12] << 18);
            PUT(BX, (unsigned int)vsvdb_buf[13] << 18);
            PUT(BY, (unsigned int)vsvdb_buf[14] << 18);
        } else {
            return -1;
        }
    } else if (version == 2u) {
        PUT(V2_INTERFACE, vsvdb_buf[7] & 3u);
        PUT(V2_SUPPORTS_10B_12B_444, ((vsvdb_buf[8] & 1u) << 1) | (vsvdb_buf[9] & 1u));
        min_pq = (vsvdb_buf[6] >> 3) * 20u;
        max_pq = (vsvdb_buf[7] >> 3) * 65u + 2055u;
        PUT(TMINPQ, min_pq);
        PUT(TMAXPQ, max_pq);
        PUT(TMIN, (unsigned int)((unsigned long long)control_pq12_to_l((unsigned short)min_pq) * 10000u >> 18));
        PUT(TMAX, (unsigned int)((unsigned long long)control_pq12_to_l((unsigned short)max_pq) * 10000u >> 18));
        PUT(RX, ((vsvdb_buf[10] >> 3) | 0xa0u) << 18);
        PUT(RY, ((vsvdb_buf[11] >> 3) | 0x40u) << 18);
        PUT(GX, (unsigned int)(vsvdb_buf[8] >> 1) << 18);
        PUT(GY, ((vsvdb_buf[9] >> 1) | 0x80u) << 18);
        PUT(BX, ((vsvdb_buf[10] & 7u) << 18) | 0x800000u);
        PUT(BY, ((vsvdb_buf[11] & 7u) << 18) | 0x200000u);
    } else {
        return -1;
    }
    PUT(WX, 0x140346eu);
    PUT(WY, 0x150e560u);
    return 0;
}

#undef PUT
