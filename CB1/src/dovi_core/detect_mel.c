/* Identify minimal enhancement layer payloads from their mapping fields. */
#include "detect_mel.h"

typedef char mel_uint32_is_four_bytes[(sizeof(unsigned int) == 4) ? 1 : -1];

static unsigned int load_le16(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static unsigned int load_le32(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

int detect_mel(const struct mel_decoder *decoder)
{
    const unsigned char *payload = decoder->root->payload;
    unsigned int component;

    for (component = 0; component < 3; ++component) {
        const unsigned char *entry = payload + 0x6874u + component * 0x1cu;
        if (load_le16(entry) != 0 ||
            load_le32(entry + 0x0c) != 0 ||
            load_le32(entry + 0x10) != 0 ||
            load_le32(entry + 0x04) != 1 ||
            load_le32(entry + 0x08) != 0 ||
            load_le32(entry + 0x14) != 0 ||
            load_le32(entry + 0x18) != 0)
            return 0;
    }
    return 1;
}
