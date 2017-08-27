#include "coding.h"
#include "../util.h"

void decode_pcm16LE(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        outbuf[sample_count]=read_16bitLE(stream->offset+i*2,stream->streamfile);
    }
}

void decode_pcm16BE(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        outbuf[sample_count]=read_16bitBE(stream->offset+i*2,stream->streamfile);
    }
}

void decode_pcm8(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        outbuf[sample_count]=read_8bit(stream->offset+i,stream->streamfile)*0x100;
    }
}

void decode_pcm8_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        outbuf[sample_count]=read_8bit(stream->offset+i*channelspacing,stream->streamfile)*0x100;
    }
}

void decode_pcm8_sb_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int16_t v = (uint8_t)read_8bit(stream->offset+i*channelspacing,stream->streamfile);
        if (v&0x80) v = 0-(v&0x7f);
        outbuf[sample_count] = v*0x100;
    }
}

void decode_pcm8_unsigned_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int16_t v = (uint8_t)read_8bit(stream->offset+i*channelspacing,stream->streamfile);
        outbuf[sample_count] = v*0x100 - 0x8000;
    }
}

void decode_pcm8_unsigned(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        int16_t v = (uint8_t)read_8bit(stream->offset+i,stream->streamfile);
        outbuf[sample_count] = v*0x100 - 0x8000;
    }
}

void decode_pcm16_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do, int big_endian) {
    int i, sample_count;
    int16_t (*read_16bit)(off_t,STREAMFILE*) = big_endian ? read_16bitBE : read_16bitLE;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        outbuf[sample_count]=read_16bit(stream->offset+i*2*channelspacing,stream->streamfile);
    }
}

void decode_pcm16LE_XOR_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        outbuf[sample_count]=read_16bitLE(stream->offset+i*2*channelspacing,stream->streamfile)^stream->key_xor;
    }
}

/* decodes u-law (ITU G.711 non-linear PCM) */
void decode_ulaw(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do) {
    int i;
    int32_t sample_count;
    int sign, segment, quantization, sample;
    const int bias = 0x84;


    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        uint8_t ulawbyte = read_8bit(stream->offset+i,stream->streamfile);

        ulawbyte = ~ulawbyte; /* stored in complement */
        sign = (ulawbyte & 0x80);
        segment = (ulawbyte & 0x70) >> 4; /* exponent */
        quantization = ulawbyte & 0x0F; /* mantissa */

        sample = (quantization << 3) + bias; /* add bias */
        sample <<= segment;
        sample = (sign) ? (bias - sample) : (sample - bias); /* remove bias */

#if 0   // the above follows Sun's implementation, but this works too
        {
            static int exp_lut[8] = {0,132,396,924,1980,4092,8316,16764}; /* precalcs from bias */
            sample = exp_lut[segment] + (quantization << (segment + 3));
            if (sign != 0) sample = -sample;
        }
#endif

        outbuf[sample_count] = sample;
    }
}

void decode_pcmfloat(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do, int big_endian) {
    int i, sample_count;
    int32_t (*read_32bit)(off_t,STREAMFILE*) = big_endian ? read_32bitBE : read_32bitLE;

    for (i=first_sample,sample_count=0; i<first_sample+samples_to_do; i++,sample_count+=channelspacing) {
        uint32_t sample_int = read_32bit(stream->offset+i*4,stream->streamfile);
        float* sample_float;
        int sample_pcm;

        sample_float = (float*)&sample_int;
        sample_pcm = floor((*sample_float) * 32767.f + .5f);

        outbuf[sample_count] = clamp16(sample_pcm);
    }
}

size_t pcm_bytes_to_samples(size_t bytes, int channels, int bits_per_sample) {
    return bytes / channels / (bits_per_sample/8);
}
