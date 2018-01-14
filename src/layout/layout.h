#ifndef _LAYOUT_H
#define _LAYOUT_H

#include "../streamtypes.h"
#include "../vgmstream.h"

/* blocked layouts */
void render_vgmstream_blocked(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

void ast_block_update(off_t block_ofset, VGMSTREAM * vgmstream);

void mxch_block_update(off_t block_ofset, VGMSTREAM * vgmstream);

void halpst_block_update(off_t block_ofset, VGMSTREAM * vgmstream);

void xa_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void block_update_ea_schl(off_t block_offset, VGMSTREAM * vgmstream);
void block_update_ea_1snh(off_t block_offset, VGMSTREAM * vgmstream);

void caf_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void wsi_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void str_snds_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void ws_aud_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void matx_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void block_update_dec(off_t block_offset, VGMSTREAM * vgmstream);

void vs_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void emff_ps2_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void emff_ngc_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void gsb_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void xvas_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void thp_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void filp_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void block_update_ivaud(off_t block_offset, VGMSTREAM * vgmstream);

void psx_mgav_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void ps2_adm_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void dsp_bdsp_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void tra_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void ps2_iab_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void ps2_strlr_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void rws_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void hwas_block_update(off_t block_offset, VGMSTREAM * vgmstream);

void block_update_ea_sns(off_t block_offset, VGMSTREAM * vgmstream);
void block_update_awc(off_t block_offset, VGMSTREAM * vgmstream);
void block_update_vgs(off_t block_offset, VGMSTREAM * vgmstream);
void block_update_vawx(off_t block_offset, VGMSTREAM * vgmstream);
void block_update_xvag_subsong(off_t block_offset, VGMSTREAM * vgmstream);

/* other layouts */
void render_vgmstream_interleave(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

void render_vgmstream_nolayout(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

void render_vgmstream_mus_acm(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

void render_vgmstream_aix(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

void render_vgmstream_aax(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

void render_vgmstream_scd_int(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

#endif
