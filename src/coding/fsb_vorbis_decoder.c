#include "coding.h"

#ifdef VGM_USE_VORBIS
#include <vorbis/codec.h>

#define FSB_VORBIS_ON 1 //todo recompile libvorbis with vorbis_* and remove

#define FSB_VORBIS_DEFAULT_BUFFER_SIZE 0x8000 /* should be at least the size of the setup header, ~0x2000 */

#if FSB_VORBIS_ON
static void pcm_convert_float_to_16(vorbis_codec_data * data, sample * outbuf, int samples_to_do, float ** pcm);
static int vorbis_make_header_identification(uint8_t * buf, size_t bufsize, int channels, int sample_rate, int blocksize_short, int blocksize_long);
static int vorbis_make_header_comment(uint8_t * buf, size_t bufsize);
static int vorbis_make_header_setup(uint8_t * buf, size_t bufsize, uint32_t setup_id, STREAMFILE *streamFile);
#endif

/**
 * Inits a raw FSB vorbis stream.
 *
 * Vorbis packets are stored in .ogg, which is divided into ogg pages/packets, and the first packets contain necessary
 * vorbis setup. For raw vorbis the setup is stored it elsewhere (i.e.- in the .exe), presumably to shave off some kb
 * per stream and codec startup time. We'll read the external setup and raw data and decode it with libvorbis.
 *
 * FSB references the external setup with the setup_id, and the raw vorbis packets have mini headers with the block size.
 *
 * Format info and references from python-fsb5 (https://github.com/HearthSim/python-fsb5) and
 *  fsb-vorbis-extractor (https://github.com/tmiasko/fsb-vorbis-extractor).
 * Also from the official docs (https://www.xiph.org/vorbis/doc/libvorbis/overview.html).
 */
vorbis_codec_data * init_fsb_vorbis_codec_data(STREAMFILE *streamfile, off_t start_offset, int channels, int sample_rate, uint32_t setup_id) {
#if FSB_VORBIS_ON
    vorbis_codec_data * data = NULL;

    /* init stuff */
    data = calloc(1,sizeof(vorbis_codec_data));
    if (!data) goto fail;

    data->buffer_size = FSB_VORBIS_DEFAULT_BUFFER_SIZE;
    data->buffer = calloc(sizeof(uint8_t), data->buffer_size);
    if (!data->buffer) goto fail;


    /* init vorbis stream state, using 3 fake Ogg setup packets (info, comments, setup/codebooks)
     * libvorbis expects parsed Ogg pages, but we'll fake them with our raw data instead */
    vorbis_info_init(&data->vi);
    vorbis_comment_init(&data->vc);

    data->op.packet = data->buffer;
    data->op.b_o_s = 1; /* fake headers start */

    data->op.bytes = vorbis_make_header_identification(data->buffer, data->buffer_size, channels, sample_rate, 256, 2048); /* FSB default block sizes */
    if (!data->op.bytes) goto fail;
    if (vorbis_synthesis_headerin(&data->vi, &data->vc, &data->op) != 0) goto fail; /* parse identification header */

    data->op.bytes = vorbis_make_header_comment(data->buffer, data->buffer_size);
    if (!data->op.bytes) goto fail;
    if (vorbis_synthesis_headerin(&data->vi, &data->vc, &data->op) !=0 ) goto fail; /* parse comment header */

    data->op.bytes = vorbis_make_header_setup(data->buffer, data->buffer_size, setup_id, streamfile);
    if (!data->op.bytes) goto fail;
    if (vorbis_synthesis_headerin(&data->vi, &data->vc, &data->op) != 0) goto fail; /* parse setup header */

    data->op.b_o_s = 0; /* end of fake headers */

    /* init vorbis global and block state */
    if (vorbis_synthesis_init(&data->vd,&data->vi) != 0) goto fail;
    if (vorbis_block_init(&data->vd,&data->vb) != 0) goto fail;


    return data;

fail:
    free_fsb_vorbis(data);
#endif
    return NULL;
}

/**
 * Decodes raw FSB vorbis
 */
void decode_fsb_vorbis(VGMSTREAM * vgmstream, sample * outbuf, int32_t samples_to_do, int channels) {
#if FSB_VORBIS_ON
    VGMSTREAMCHANNEL *stream = &vgmstream->ch[0];
    vorbis_codec_data * data = vgmstream->codec_data;
    size_t stream_size =  get_streamfile_size(stream->streamfile);
    //data->op.packet = data->buffer;/* implicit from init */
    int samples_done = 0;

    while (samples_done < samples_to_do) {

        /* extra EOF check for edge cases */
        if (stream->offset > stream_size) {
            memset(outbuf + samples_done * channels, 0, (samples_to_do - samples_done) * sizeof(sample));
            break;
        }


        if (data->samples_full) {  /* read more samples */
            int samples_to_get;
            float **pcm;

            /* get PCM samples from libvorbis buffers */
            samples_to_get = vorbis_synthesis_pcmout(&data->vd, &pcm);
            if (!samples_to_get) {
                data->samples_full = 0; /* request more if empty*/
                continue;
            }

            if (data->samples_to_discard) {
                /* discard samples for looping */
                if (samples_to_get > data->samples_to_discard)
                    samples_to_get = data->samples_to_discard;
                data->samples_to_discard -= samples_to_get;
            }
            else {
                /* get max samples and convert from Vorbis float pcm to 16bit pcm */
                if (samples_to_get > samples_to_do - samples_done)
                    samples_to_get = samples_to_do - samples_done;
                samples_done += samples_to_get;
                pcm_convert_float_to_16(data, outbuf + samples_done * channels, samples_to_get, pcm);
            }

            /* mark consumed samples from the buffer
             * (non-consumed samples are returned in next vorbis_synthesis_pcmout calls) */
            vorbis_synthesis_read(&data->vd, samples_to_get);
        }
        else { /* read more data */
            int rc;

            /* this is not actually needed, but feels nicer */
            data->op.granulepos += samples_to_do;
            data->op.packetno++;

            /* get next packet size from the FSB 16b header (doesn't count this 16b) */
            data->op.bytes = (uint16_t)read_16bitLE(stream->offset, stream->streamfile);
            stream->offset += 2;
            if (data->op.bytes == 0 || data->op.bytes == 0xFFFF) {
                goto decode_fail; /* eof or FSB end padding */
            }

            /* read raw block */
            if (read_streamfile(data->buffer,stream->offset, data->op.bytes,stream->streamfile) != data->op.bytes) {
                goto decode_fail; /* wrong packet? */
            }
            stream->offset += data->op.bytes;

            /* parse the fake ogg packet into a logical vorbis block */
            rc = vorbis_synthesis(&data->vb,&data->op);
            if (rc == OV_ENOTAUDIO) {
                VGM_LOG("vorbis_synthesis: not audio packet @ %lx\n",stream->offset); getchar();
                continue; /* not tested */
            } else if (rc != 0) {
                goto decode_fail;
            }

            /* finally decode the logical block into samples */
            rc = vorbis_synthesis_blockin(&data->vd,&data->vb);
            if (rc != 0)  {
                goto decode_fail; /* ? */
            }

            data->samples_full = 1;
        }
    }

    return;

decode_fail:
    /* on error just put some 0 samples */
    memset(outbuf + samples_done * channels, 0, (samples_to_do - samples_done) * sizeof(sample));
#endif
}

#if FSB_VORBIS_ON

static void pcm_convert_float_to_16(vorbis_codec_data * data, sample * outbuf, int samples_to_do, float ** pcm) {
    /* mostly from Xiph's decoder_example.c */
    int i,j;

    /* convert float PCM (multichannel float array, with pcm[0]=ch0, pcm[1]=ch1, pcm[2]=ch0, etc)
     * to 16 bit signed PCM ints (host order) and interleave + fix clipping */
    for (i = 0; i < data->vi.channels; i++) {
        sample *ptr = outbuf + i;
        float *mono = pcm[i];
        for (j = 0; j < samples_to_do; j++) {
            int val = floor(mono[j] * 32767.f + .5f);
            if (val > 32767) val = 32767;
            if (val < -32768) val = -32768;

            *ptr = val;
            ptr += data->vi.channels;
        }
    }
}

static int vorbis_make_header_identification(uint8_t * buf, size_t bufsize, int channels, int sample_rate, int blocksize_short, int blocksize_long) {
    int bytes = 0x1e;
    uint8_t blocksizes, exp_blocksize_0, exp_blocksize_1;

    if (bytes > bufsize) return 0;

    /* guetto log2 for allowed blocksizes (2-exp), could be improved */
    switch(blocksize_long) {
        case 64:   exp_blocksize_0 = 6;  break;
        case 128:  exp_blocksize_0 = 7;  break;
        case 256:  exp_blocksize_0 = 8;  break;
        case 512:  exp_blocksize_0 = 9;  break;
        case 1024: exp_blocksize_0 = 10; break;
        case 2048: exp_blocksize_0 = 11; break;
        case 4096: exp_blocksize_0 = 12; break;
        case 8192: exp_blocksize_0 = 13; break;
        default: return 0;
    }
    switch(blocksize_short) {
        case 64:   exp_blocksize_1 = 6;  break;
        case 128:  exp_blocksize_1 = 7;  break;
        case 256:  exp_blocksize_1 = 8;  break;
        case 512:  exp_blocksize_1 = 9;  break;
        case 1024: exp_blocksize_1 = 10; break;
        case 2048: exp_blocksize_1 = 11; break;
        case 4096: exp_blocksize_1 = 12; break;
        case 8192: exp_blocksize_1 = 13; break;
        default: return 0;
    }
    blocksizes = (exp_blocksize_0 << 4) | (exp_blocksize_1);

    put_8bit   (buf+0x00, 0x01);            /* packet_type (id) */
    memcpy     (buf+0x01, "vorbis", 6);     /* id */
    put_32bitLE(buf+0x07, 0x00);            /* vorbis_version (fixed) */
    put_8bit   (buf+0x0b, channels);        /* audio_channels */
    put_32bitLE(buf+0x0c, sample_rate);     /* audio_sample_rate */
    put_32bitLE(buf+0x10, 0x00);            /* bitrate_maximum (optional hint) */
    put_32bitLE(buf+0x14, 0x00);            /* bitrate_nominal (optional hint) */
    put_32bitLE(buf+0x18, 0x00);            /* bitrate_minimum (optional hint) */
    put_8bit   (buf+0x1c, blocksizes);      /* blocksize_0 + blocksize_1 nibbles */
    put_8bit   (buf+0x1d, 0x01);            /* framing_flag (fixed) */

    return bytes;
}

static int vorbis_make_header_comment(uint8_t * buf, size_t bufsize) {
    int bytes = 0x19;

    if (bytes > bufsize) return 0;

    put_8bit   (buf+0x00, 0x03);            /* packet_type (comments) */
    memcpy     (buf+0x01, "vorbis", 6);     /* id */
    put_32bitLE(buf+0x07, 0x09);            /* vendor_length */
    memcpy     (buf+0x0b, "vgmstream", 9);  /* vendor_string */
    put_32bitLE(buf+0x14, 0x00);            /* user_comment_list_length */
    put_8bit   (buf+0x18, 0x01);            /* framing_flag (fixed) */

    return bytes;
}

static int vorbis_make_header_setup(uint8_t * buf, size_t bufsize, uint32_t setup_id, STREAMFILE *streamFile) {
    char setupname[PATH_LIMIT];
    char pathname[PATH_LIMIT];
    char *path;
    STREAMFILE * streamFileSetup = NULL;
    size_t bytes = 0;


    streamFile->get_name(streamFile,pathname,sizeof(pathname));

    /* try to get setup packet from external file first, using "(dir/).vorbis_{setup_id}" */
    path = strrchr(pathname,DIR_SEPARATOR);
    if (path) {
        *(path+1) = '\0';
    } else {
        pathname[0] = '\0';
    }
    snprintf(setupname,PATH_LIMIT,"%s.vorbis_%08x", pathname, setup_id);

    streamFileSetup = streamFile->open(streamFile,setupname,STREAMFILE_DEFAULT_BUFFER_SIZE);
    if (streamFileSetup) {
        /* file found, get contents into the buffer */
        bytes = streamFileSetup->get_size(streamFileSetup);
        if (bytes > bufsize) goto fail;

        if (read_streamfile(buf, 0, bufsize, streamFileSetup) != bytes)
            goto fail;

        streamFileSetup->close(streamFileSetup);
    }
    else {
        /* check the index list */
        VGM_LOG("FSB Vorbis: setup_id %08x not found\n", setup_id);
        goto fail;
    }


    return bytes;

fail:
    if (streamFileSetup) streamFileSetup->close(streamFileSetup);
    return 0;
}

#endif


void free_fsb_vorbis(vorbis_codec_data * data) {
#if FSB_VORBIS_ON
    if (!data)
        return;

    /* internal decoder cleanp */
    vorbis_info_clear(&data->vi);
    vorbis_comment_clear(&data->vc);
    vorbis_dsp_clear(&data->vd);

    free(data->buffer);
    free(data);
#endif
}

void reset_fsb_vorbis(VGMSTREAM *vgmstream) {
    seek_fsb_vorbis(vgmstream, 0);
}

void seek_fsb_vorbis(VGMSTREAM *vgmstream, int32_t num_sample) {
#if FSB_VORBIS_ON
    vorbis_codec_data *data = vgmstream->codec_data;

    /* Seeking is provided by the Ogg layer, so with raw vorbis we need seek tables instead.
     * To avoid having to parse different formats we'll just discard until the expected sample */
    vorbis_synthesis_restart(&data->vd);
    data->samples_to_discard = num_sample;
    vgmstream->loop_ch[0].offset = vgmstream->loop_ch[0].channel_start_offset;
#endif
}

#endif
