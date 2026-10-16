/* Update DM state and derive color, tone and output-map parameters. */
#include "dm_commit.h"
#include "dm_commit_abi.h"
#include "byte_utils.h"
#include "dm_commit_tables.inc"
#include "control_target_tables.inc"
#include "control_luminance.h"

typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

static int32_t dm_signed(uint32_t v)
{ return v <= 0x7fffffffu ? (int32_t)v : -1 - (int32_t)~v; }
static int32_t dm_pair(uint16_t lo, uint16_t hi)
{ return dm_signed((uint32_t)lo | ((uint32_t)hi << 16)); }

#define MemCpyByte mem_cpy_byte
#define LToPQ12 control_l_to_pq12
#define CheckMds check_dm_metadata
#define CommitMdsCsc(a,b,c,d) commit_color((a),(b),(c),(d),ops)
#define GetLms2IptM33Fxp(...) ops->lms_to_ipt(ops->opaque, __VA_ARGS__)
#define GetIpt2LmsM33Fxp(...) ops->ipt_to_lms(ops->opaque, __VA_ARGS__)
#define InvM33(...) ops->invert_matrix(ops->opaque, __VA_ARGS__)
#define SigEnv2KsFrmFmt(...) ops->frame_format(ops->opaque, __VA_ARGS__)
#define CommitIptKsIMap(...) ops->ipt_input(ops->opaque, __VA_ARGS__)
#define AssignM33I2I(...) ops->assign_matrix(ops->opaque, __VA_ARGS__)
#define AssignV3I2I(...) ops->assign_vector(ops->opaque, __VA_ARGS__)
#define Yuv2RgbOffsetRgbFxp(...) ops->rgb_offset(ops->opaque, __VA_ARGS__)
#define GetRgb2LmsCtWpM33Fxp(...) ops->rgb_to_lms_crosstalk(ops->opaque, __VA_ARGS__)
#define GetYuv2RgbM33NarrowFxp(...) ops->yuv_to_rgb_narrow(ops->opaque, __VA_ARGS__)
#define GetYuv2RgbM33Fxp(...) ops->yuv_to_rgb(ops->opaque, __VA_ARGS__)
#define GetYuvRgbOffFxp(...) ops->yuv_offset(ops->opaque, __VA_ARGS__)
#define GetRange(...) ops->range(ops->opaque, __VA_ARGS__)
#define calcTaTb(...) ops->ta_tb(ops->opaque, __VA_ARGS__)
#define fxPOW2Taylor(...) ops->power2(ops->opaque, __VA_ARGS__)
#define DeGamma(...) ops->de_gamma(ops->opaque, __VA_ARGS__)
#define GetRgb2LmsByDefM33(...) ops->rgb_to_lms_default(ops->opaque, __VA_ARGS__)
#define GetRgb2LmsByPrimsM33Fxp(...) ops->rgb_to_lms_primaries(ops->opaque, __VA_ARGS__)
#define GetIptNormParam(...) ops->ipt_norm(ops->opaque, __VA_ARGS__)
#define TgtSigEnv2TcCtrl(...) ops->target_curve(ops->opaque, __VA_ARGS__)
#define UpdateTcSigHlgA(...) ops->hlg_signal(ops->opaque, __VA_ARGS__)
#define TmCtrl2TcCtrl(...) ops->tone_control(ops->opaque, __VA_ARGS__)
#define UpdateTcCtrlLevel(...) ops->tone_level(ops->opaque, __VA_ARGS__)
#define CreateToneCurve(...) ops->tone_curve(ops->opaque, __VA_ARGS__)
#define CommitKsOMap(...) ops->output_map(ops->opaque, __VA_ARGS__)
#define CommitUs(...) ops->upsample(ops->opaque, __VA_ARGS__)
#define CommitTrim(...) ops->trim(ops->opaque, __VA_ARGS__)

unsigned int check_dm_metadata(const void *metadata, const void *previous)
{
    if (mem_eq_byte(metadata, previous, 520)) return MDS_CHG_NONE;
    return mem_eq_byte(metadata, previous, 514) ? MDS_CHG_TC : MDS_CHG_CFG;
}


static void commit_color(MdsExt_t *hMdsExt, DmCfgFxp_t *pDmCfg,
                         CscCtrlFxp_t *pCscCtrl, DmKsIMapFxp_t *pKsIMap,
                         const struct dm_commit_ops *ops)

{
  int16_t (*paiVar1) [3];
  int64_t lVar2;
  int32_t crossTalk;
  int32_t iVar3;
  uint8_t bVar4;
  short m33Scale;
  int16_t iVar5;
  uint32_t uVar6;
  uint32_t rng;
  int16_t (*paiVar7) [3];
  int64_t lVar8;
  int16_t (*paiVar9) [3];
  CscCtrlFxp_t *pCVar10;
  uint16_t uVar11;
  uint32_t *puVar12;
  uint32_t *puVar13;
  int32_t v3 [3];
  int16_t m33Lms2Ipt [3] [3];
  int16_t m33Ipt2Lms [3] [3];
  crossTalk = (pDmCfg->srcSigEnv).CrossTalk;
  MemCpyByte(m33Ipt2Lms,pCscCtrl->m33Ipt2Lms,0x12);
  MemCpyByte(m33Lms2Ipt,pCscCtrl->m33Lms2Ipt,0x12);
  iVar3 = pCscCtrl->m33Lms2IptScale2P;
  pKsIMap->clr = hMdsExt->signal_color_space == '\x01';
  if (hMdsExt->signal_color_space == '\x02') {
    pKsIMap->clr = '\0';
  }
  pKsIMap->m33Yuv2RgbScale2P = hMdsExt->m33Yuv2RgbScale2P;
  paiVar7 = hMdsExt->m33Yuv2Rgb;
  paiVar9 = pKsIMap->m33Yuv2Rgb;
  do {
    paiVar1 = paiVar7 + 1;
    (*paiVar9)[0] = (*paiVar7)[0];
    (*paiVar9)[1] = (*paiVar7)[1];
    (*paiVar9)[2] = (*paiVar7)[2];
    paiVar7 = paiVar1;
    paiVar9 = paiVar9 + 1;
  } while (paiVar1 != (int16_t (*) [3])((unsigned char *)hMdsExt + 22));
  Yuv2RgbOffsetRgbFxp(pKsIMap->m33Yuv2Rgb,hMdsExt->v3Yuv2Rgb[0],hMdsExt->v3Yuv2Rgb[1],
                      pKsIMap->v3Yuv2RgbOffInRgb);
  m33Scale = hMdsExt->m33Rgb2WpLmsScale2P;
  pCscCtrl->sM33Rgb2LmsScale2P = (int)m33Scale;
  paiVar7 = hMdsExt->m33Rgb2WpLms;
  pCVar10 = pCscCtrl;
  do {
    paiVar9 = paiVar7 + 1;
    pCVar10->sM33Rgb2Lms[0][0] = (*paiVar7)[0];
    pCVar10->sM33Rgb2Lms[0][1] = (*paiVar7)[1];
    pCVar10->sM33Rgb2Lms[0][2] = (*paiVar7)[2];
    paiVar7 = paiVar9;
    pCVar10 = (CscCtrlFxp_t *)(pCVar10->sM33Rgb2Lms + 1);
  } while (paiVar9 != (int16_t (*) [3])&hMdsExt->signal_eotf);
  if (hMdsExt->signal_color_space == '\x02') {
    v3[0] = 0x7fff;
    v3[1] = 0x7fff;
    v3[2] = 0x7fff;
    iVar5 = GetRgb2LmsCtWpM33Fxp
                      ((int16_t (*) [3])pCscCtrl,m33Scale,crossTalk,v3,0xf,pKsIMap->m33Rgb2Lms);
    pKsIMap->m33Rgb2LmsScale2P = iVar5;
  }
  else {
    iVar5 = GetRgb2LmsCtWpM33Fxp
                      ((int16_t (*) [3])pCscCtrl,m33Scale,crossTalk,pCscCtrl->sV3Wp,
                       (int16_t)pCscCtrl->sV3WpScale2P,pKsIMap->m33Rgb2Lms);
    pKsIMap->m33Rgb2LmsScale2P = iVar5;
  }
  bVar4 = hMdsExt->signal_full_range_flag;
  rng = (uint32_t)bVar4;
  if ((bVar4 != 1) && (rng = CRngNarrow, bVar4 != 0)) {
    rng = CRngSdi;
  }
  GetRange((uint32_t)hMdsExt->signal_bit_depth,rng,&(pKsIMap->eotfParam).rangeMin,
           &(pKsIMap->eotfParam).range);
  if (hMdsExt->signal_bit_depth == 0) {
    (pKsIMap->eotfParam).rangeInv = 0x10000;
    (pKsIMap->eotfParam).bdp = (uint16_t)hMdsExt->signal_bit_depth;
    uVar11 = hMdsExt->signal_eotf;
  }
  else {

    lVar8 = (int64_t)(int)((uint32_t)(pKsIMap->eotfParam).range <<
                       (uint64_t)(0x10 - hMdsExt->signal_bit_depth & 0x1f));
    uVar6 = 0;
    if (lVar8 != 0) {
      uVar6 = (uint32_t)(0xffffffff / lVar8);
    }
    (pKsIMap->eotfParam).rangeInv = uVar6;
    (pKsIMap->eotfParam).bdp = (uint16_t)hMdsExt->signal_bit_depth;
    uVar11 = hMdsExt->signal_eotf;
  }
  if (uVar11 == 0xffff) {
    (pKsIMap->eotfParam).eotf = '\x01';
  }
  else {
    (pKsIMap->eotfParam).eotf = '\0';

    if ((uint16_t)(hMdsExt->source_max_PQ - 0xe6f) < 3) {
      if ((uint16_t)(hMdsExt->source_min_PQ - 0x3c) < 3) {
        lVar8 = 0;
        do {
          lVar2 = lVar8 + 4;
          *(uint32_t *)((int64_t)pKsIMap->g2L + lVar8) =
               *(uint32_t *)((int64_t)g2lPulsar256Lut + lVar8);
          lVar8 = lVar2;
        } while (lVar2 != 0x400);
        goto LAB_00108af4;
      }
    }
    else if (((uint16_t)(hMdsExt->source_max_PQ - 0x820) < 3) &&
            ((uint16_t)(hMdsExt->source_min_PQ - 0x3c) < 3)) {
      lVar8 = 0;
      do {
        lVar2 = lVar8 + 4;
        *(uint32_t *)((int64_t)pKsIMap->g2L + lVar8) = *(uint32_t *)((int64_t)g2lSdr256Lut + lVar8);
        lVar8 = lVar2;
      } while (lVar2 != 0x400);
      goto LAB_00108af4;
    }
    puVar12 = pKsIMap->g2L;
    uVar11 = 0;
    do {

      uVar6 = DeGamma(hMdsExt->signal_eotf_param0,hMdsExt->signal_eotf_param1,hMdsExt->signal_eotf,
                      hMdsExt->signal_eotf_param2,uVar11);
      puVar13 = puVar12 + 1;
      *puVar12 = uVar6;
      puVar12 = puVar13;
      uVar11 = uVar11 + 0x100;
    } while (puVar13 != (uint32_t *)&pKsIMap->m33Rgb2LmsScale2P);
  }
LAB_00108af4:
  AssignM33I2I(m33Lms2Ipt,pKsIMap->m33Lms2Ipt);
  pKsIMap->m33Lms2IptScale2P = (int16_t)iVar3;
  GetIptNormParam((uint32_t)hMdsExt->signal_bit_depth,(uint32_t)(hMdsExt->v3Yuv2Rgb[0] == 0),
                  pKsIMap->v3IptOff,&pKsIMap->iptScale);
  pKsIMap->v3IptOff[0] = (uint32_t)*(uint16_t *)((int64_t)hMdsExt->v3Yuv2Rgb + 2);
  pKsIMap->v3IptOff[1] = (uint32_t)*(uint16_t *)((int64_t)hMdsExt->v3Yuv2Rgb + 6);
  pKsIMap->v3IptOff[2] = (uint32_t)*(uint16_t *)((int64_t)hMdsExt->v3Yuv2Rgb + 10);
  return;
}



static int commit_metadata(MdsExt_t *hMdsExt, DmCtxtFxp_t *hDm,
                           const struct dm_commit_ops *ops)

{
  TcCtrlFxp_t *pTcCtrl;
  TcCtrlFxp_t *pTcCtrl_00;
  int16_t (*paiVar1) [3];
  CscCtrlFxp_t *m33;
  TgtSigEnvFxp_t *pTgtSig;
  int64_t lVar2;
  uint32_t *pDmCtrl;
  TmCtrlFxp_t *pTmCtrl;
  uint32_t CVar3;
  uint32_t CVar4;
  uint32_t uVar5;
  uint32_t uVar6;
  uint32_t uVar7;
  uint32_t CVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  uint8_t bVar14;
  uint16_t uVar15;
  uint16_t uVar16;
  uint16_t uVar17;
  uint16_t uVar18;
  uint16_t sA;
  uint8_t KVar21;
  uint16_t uVar23;
  uint16_t uVar24;
  uint16_t uVar25;
  uint16_t uVar26;
  uint32_t uVar27;
  uint32_t uVar28;
  uint32_t uVar29;
  uint32_t MVar30;
  uint32_t uVar31;
  uint32_t CVar32;
  int iVar33;
  int bVar19;
  uint8_t KVar20;
  int16_t iVar22;
  DmKsFxp_t * pDVar34;
  uint32_t CVar35;
  int32_t iVar36;
  int64_t lVar37;
  uint32_t uVar39;
  DmCfgFxp_t * pDVar40;
  DmKsFxp_t * pDVar41;
  uint32_t *puVar42;
  uint32_t *puVar43;
  uint32_t uVar44;
  int16_t (*local_50) [3];
  int local_18;
  int local_14;
  int32_t offset [3];


  pTcCtrl = &hDm->tcCtrl;
  pTcCtrl_00 = &hDm->tc2Ctrl;
  if ((hMdsExt == (MdsExt_t *)0x0) && (hDm->mdsExtFrcCmmt != 0)) {
    hMdsExt = (MdsExt_t *)0x0;
    uVar27 = 0;
    if (hDm->mdsExtActValid != 0) {
      hMdsExt = &hDm->mdsExtAct;
      uVar27 = (uint32_t)(hMdsExt != (MdsExt_t *)0x0);
    }
  }
  else {
    uVar27 = (uint32_t)(hMdsExt != (MdsExt_t *)0x0);
  }
  (hDm->pDmCfgToBe->dmCtrl).MdsFollowed = uVar27;

  pDVar34 = hDm->hDmKs;
  pDVar40 = hDm->pDmCfgToBe;
  if (hDm->dmCfgToBeValid == 0) {
    uVar27 = 0;
    if (hDm->dmCfgActValid == 0) {
      return -1;
    }
  }
  else {
    if ((pDVar40->dmCtrl).Lms2IptExt == 0) {
      iVar22 = GetLms2IptM33Fxp((hDm->cscCtrl).m33Lms2Ipt);
      (hDm->cscCtrl).m33Lms2IptScale2P = (int)iVar22;
      if ((pDVar40->dmCtrl).Ipt2LmsExt == 0) {
        iVar22 = GetIpt2LmsM33Fxp((hDm->cscCtrl).m33Ipt2Lms);
        (hDm->cscCtrl).m33Ipt2LmsScale2P = (int)iVar22;
        goto LAB_00109d04;
      }
LAB_0010974c:
      MemCpyByte((hDm->cscCtrl).m33Ipt2Lms,(pDVar40->dmCtrl).M33Ipt2Lms,0x12);
      (hDm->cscCtrl).m33Ipt2LmsScale2P = (pDVar40->dmCtrl).M33Lms2IptScale2P;
    }
    else {

      paiVar1 = (pDVar40->dmCtrl).M33Lms2Ipt;
      MemCpyByte((hDm->cscCtrl).m33Lms2Ipt,paiVar1,0x12);
      iVar36 = (pDVar40->dmCtrl).M33Lms2IptScale2P;
      (hDm->cscCtrl).m33Lms2IptScale2P = iVar36;
      (hDm->cscCtrl).m33Ipt2LmsScale2P = iVar36;
      if ((pDVar40->dmCtrl).Ipt2LmsExt == 0) {
        iVar22 = InvM33(paiVar1,(hDm->cscCtrl).m33Ipt2Lms,(int16_t)iVar36,(int16_t)iVar36);
        (hDm->cscCtrl).m33Ipt2LmsScale2P = dm_signed((uint32_t)(hDm->cscCtrl).m33Ipt2LmsScale2P - (uint32_t)(int)iVar22);
LAB_00109d04:
        if ((pDVar40->dmCtrl).Ipt2LmsExt != 0) goto LAB_0010974c;
      }
      else {
        MemCpyByte((hDm->cscCtrl).m33Ipt2Lms,(pDVar40->dmCtrl).M33Ipt2Lms,0x12);
        (hDm->cscCtrl).m33Ipt2LmsScale2P = (pDVar40->dmCtrl).M33Lms2IptScale2P;
      }
    }
    local_50 = (hDm->cscCtrl).m33Lms2Ipt;
    (pDVar34->ksDmCtrl).mainIn = 1;

    m33 = &hDm->cscCtrl;
    (pDVar34->ksDmCtrl).secIn = (pDVar40->dmCtrl).SecIn;
    (pDVar34->ksDmCtrl).prf = (pDVar40->dmCtrl).Prf;
    SigEnv2KsFrmFmt((SigEnvCom_t *)&pDVar40->grcSigEnv,&pDVar34->ksFrmFmtG);
    CVar3 = (pDVar40->grcSigEnv).Bdp;
    pDVar41 = hDm->hDmKs;
    (hDm->cscCtrl).gV3Wp[2] = 0x7fff;
    (hDm->cscCtrl).gV3Wp[1] = 0x7fff;
    uVar25 = (uint16_t)CVar3;
    iVar33 = (int)(short)uVar25;
    (hDm->cscCtrl).gV3Wp[0] = 0x7fff;
    (hDm->cscCtrl).gV3WpScale2P = 0xf;
    if ((pDVar40->grcSigEnv).WpExt != 0) {
      MemCpyByte((hDm->cscCtrl).gV3Wp,(pDVar40->grcSigEnv).V3Wp,0xc);
      (hDm->cscCtrl).gV3WpScale2P = (int)(pDVar40->grcSigEnv).WpScale;
    }
    CVar4 = (pDVar40->grcSigEnv).Clr;
    KVar20 = '\0';
    if (CVar4 != CClrYuv) {
      if (CVar4 == CClrRgb) {
        KVar20 = '\x01';
      }
      else {
        KVar20 = '\x02';
        if (CVar4 != CClrRgba) {
          KVar20 = '\0';
        }
      }
    }
    (pDVar41->ksIMapG).clr = KVar20;
    if ((pDVar40->grcSigEnv).Clr == CClrIpt) {
      CommitIptKsIMap((pDVar40->grcSigEnv).Bdp,(pDVar40->grcSigEnv).Rng,&pDVar41->ksIMapG);
      if ((pDVar40->grcSigEnv).Yuv2RgbExt != 0) {
        AssignM33I2I((pDVar40->grcSigEnv).M33Yuv2Rgb,(pDVar41->ksIMapG).m33Yuv2Rgb);
        (pDVar41->ksIMapG).m33Yuv2RgbScale2P = (int16_t)(pDVar40->grcSigEnv).M33Yuv2RgbScale2P;
      }
      if ((pDVar40->grcSigEnv).Yuv2RgbOffExt != 0) {
        AssignV3I2I((pDVar40->grcSigEnv).V3Yuv2RgbOff,offset);
        offset[0] = dm_signed((uint32_t)offset[0] << 16);
        offset[1] = dm_signed((uint32_t)offset[1] << 16);
        Yuv2RgbOffsetRgbFxp((pDVar41->ksIMapG).m33Yuv2Rgb,offset[0],offset[1],
                            (pDVar41->ksIMapG).v3Yuv2RgbOffInRgb);
      }
      if ((pDVar40->grcSigEnv).Rgb2LmsM33Ext != 0) {
        paiVar1 = (hDm->cscCtrl).gM33Rgb2Lms;
        MemCpyByte(paiVar1,(pDVar40->grcSigEnv).M33Rgb2Lms,0x12);
        iVar36 = (pDVar40->grcSigEnv).M33Rgb2LmsScale2P;
        (hDm->cscCtrl).gM33Rgb2LmsScale2P = iVar36;
        iVar22 = GetRgb2LmsCtWpM33Fxp
                           (paiVar1,(int16_t)iVar36,(pDVar40->grcSigEnv).CrossTalk,
                            (hDm->cscCtrl).gV3Wp,(int16_t)(hDm->cscCtrl).gV3WpScale2P,
                            (pDVar41->ksIMapG).m33Rgb2Lms);
        (pDVar41->ksIMapG).m33Rgb2LmsScale2P = iVar22;
      }
    }
    else {
      if ((pDVar40->grcSigEnv).Yuv2RgbExt == 0) {
        paiVar1 = (pDVar41->ksIMapG).m33Yuv2Rgb;
        if ((pDVar40->grcSigEnv).Rng == CRngNarrow) {
          iVar22 = GetYuv2RgbM33NarrowFxp
                             ((pDVar40->grcSigEnv).YuvXferSpec,(pDVar40->grcSigEnv).Bdp,paiVar1);
          (pDVar41->ksIMapG).m33Yuv2RgbScale2P = iVar22;
          goto LAB_00108dfc;
        }
        iVar22 = GetYuv2RgbM33Fxp((pDVar40->grcSigEnv).YuvXferSpec,paiVar1);
        (pDVar41->ksIMapG).m33Yuv2RgbScale2P = iVar22;
        if ((pDVar40->grcSigEnv).Yuv2RgbOffExt == 0) goto LAB_001097c0;
LAB_00108e04:
        AssignV3I2I((pDVar40->grcSigEnv).V3Yuv2RgbOff,offset);
      }
      else {
        AssignM33I2I((pDVar40->grcSigEnv).M33Yuv2Rgb,(pDVar41->ksIMapG).m33Yuv2Rgb);
        (pDVar41->ksIMapG).m33Yuv2RgbScale2P = (int16_t)(pDVar40->grcSigEnv).M33Yuv2RgbScale2P;
LAB_00108dfc:
        if ((pDVar40->grcSigEnv).Yuv2RgbOffExt != 0) goto LAB_00108e04;
LAB_001097c0:
        GetYuvRgbOffFxp((pDVar40->grcSigEnv).Rng,iVar33,offset);
      }
      offset[1] = dm_signed((uint32_t)offset[1] << 16);
      offset[0] = dm_signed((uint32_t)offset[0] << 16);
      Yuv2RgbOffsetRgbFxp((pDVar41->ksIMapG).m33Yuv2Rgb,offset[0],offset[1],
                          (pDVar41->ksIMapG).v3Yuv2RgbOffInRgb);
      CVar35 = CRngFull;
      if ((pDVar40->grcSigEnv).Clr - CClrRgb < 2) {
        CVar35 = (pDVar40->grcSigEnv).Rng;
      }
      GetRange(iVar33,CVar35,&(pDVar41->ksIMapG).eotfParam.rangeMin,
               &(pDVar41->ksIMapG).eotfParam.range);
      uVar28 = 0xffffffff;
      if (iVar33 != 0) {

        lVar37 = (int64_t)(int)((uint32_t)(pDVar41->ksIMapG).eotfParam.range <<
                            (uint64_t)(0x10U - iVar33 & 0x1f));
        uVar28 = 0;
        if (lVar37 != 0) {
          uVar28 = (uint32_t)(0xffffffff / lVar37);
        }
      }
      (pDVar41->ksIMapG).eotfParam.rangeInv = uVar28;
      (pDVar41->ksIMapG).eotfParam.bdp = uVar25;
      (pDVar41->ksIMapG).eotfParam.eotf = (pDVar40->grcSigEnv).Eotf == CEotfPq;
      if ((pDVar40->grcSigEnv).Eotf == CEotfBt1886) {

        calcTaTb((uint32_t)(pDVar40->grcSigEnv).Gamma * 2,(pDVar40->grcSigEnv).Max,
                 (pDVar40->grcSigEnv).Min,&local_18,&local_14);
        local_18 = dm_signed((uint32_t)local_18 + 0x4000u) >> 15;
        local_14 = dm_signed((uint32_t)local_14 + 0x100u) >> 9;
        uVar28 = fxPOW2Taylor((uint32_t)(pDVar40->grcSigEnv).Gamma,0xe,0x12);

        iVar33 = dm_pair((pDVar40->grcSigEnv).MinPq, (pDVar40->grcSigEnv).MaxPq);
        if (iVar33 == 0) {
          uVar25 = LToPQ12((pDVar40->grcSigEnv).Max);
          uVar26 = LToPQ12((pDVar40->grcSigEnv).Min);
        }
        else {
          uVar25 = (pDVar40->grcSigEnv).MaxPq;
          uVar26 = (pDVar40->grcSigEnv).MinPq;
        }
        if ((uint16_t)(uVar25 - 0xe6f) < 3) {
          if ((uint16_t)(uVar26 - 0x3c) < 3) {
            lVar37 = 0;
            do {
              lVar2 = lVar37 + 4;
              *(uint32_t *)((int64_t)(pDVar41->ksIMapG).g2L + lVar37) =
                   *(uint32_t *)((int64_t)g2lPulsar256Lut + lVar37);
              lVar37 = lVar2;
            } while (lVar2 != 0x400);
          }
          else {
LAB_0010998c:
            sA = (uint16_t)local_18;
            uVar25 = 0;
            uVar26 = (uint16_t)local_14;
            puVar42 = (pDVar41->ksIMapG).g2L;
            do {

              uVar31 = DeGamma(sA,uVar26,(pDVar40->grcSigEnv).Gamma,uVar28,uVar25);
              puVar43 = puVar42 + 1;
              *puVar42 = uVar31;
              uVar25 = uVar25 + 0x100;
              puVar42 = puVar43;
            } while (puVar43 != (uint32_t *)&(pDVar41->ksIMapG).m33Rgb2LmsScale2P);
          }
        }
        else {
          if ((2 < (uint16_t)(uVar25 - 0x820)) || (2 < (uint16_t)(uVar26 - 0x3c))) goto LAB_0010998c;
          lVar37 = 0;
          do {
            lVar2 = lVar37 + 4;
            *(uint32_t *)((int64_t)(pDVar41->ksIMapG).g2L + lVar37) =
                 *(uint32_t *)((int64_t)g2lSdr256Lut + lVar37);
            lVar37 = lVar2;
          } while (lVar2 != 0x400);
        }
      }
      if ((pDVar40->grcSigEnv).Rgb2LmsM33Ext == 0) {
        if ((pDVar40->grcSigEnv).Rgb2LmsRgbwExt == 0) {
          iVar22 = GetRgb2LmsByDefM33((pDVar40->grcSigEnv).RgbDef,(hDm->cscCtrl).gM33Rgb2Lms);
          iVar33 = (int)iVar22;
          (hDm->cscCtrl).gM33Rgb2LmsScale2P = iVar33;
        }
        else {
          iVar22 = GetRgb2LmsByPrimsM33Fxp
                             ((pDVar40->grcSigEnv).V8Rgbw[0],(pDVar40->grcSigEnv).V8Rgbw[1],
                              (pDVar40->grcSigEnv).V8Rgbw[2],(pDVar40->grcSigEnv).V8Rgbw[3],
                              (pDVar40->grcSigEnv).V8Rgbw[4],(pDVar40->grcSigEnv).V8Rgbw[5],
                              (pDVar40->grcSigEnv).V8Rgbw[6],(pDVar40->grcSigEnv).V8Rgbw[7],
                              (hDm->cscCtrl).gM33Rgb2Lms);
          iVar33 = (int)iVar22;
          (hDm->cscCtrl).gM33Rgb2LmsScale2P = iVar33;
        }
      }
      else {
        MemCpyByte((hDm->cscCtrl).gM33Rgb2Lms,(pDVar40->grcSigEnv).M33Rgb2Lms,0x12);
        iVar33 = (pDVar40->grcSigEnv).M33Rgb2LmsScale2P;
        (hDm->cscCtrl).gM33Rgb2LmsScale2P = iVar33;
      }
      iVar22 = GetRgb2LmsCtWpM33Fxp
                         ((hDm->cscCtrl).gM33Rgb2Lms,(int16_t)iVar33,(pDVar40->grcSigEnv).CrossTalk,
                          (hDm->cscCtrl).gV3Wp,(int16_t)(hDm->cscCtrl).gV3WpScale2P,
                          (pDVar41->ksIMapG).m33Rgb2Lms);
      (pDVar41->ksIMapG).m33Rgb2LmsScale2P = iVar22;
    }
    (pDVar41->ksIMapG).m33Lms2IptScale2P = (int16_t)(hDm->cscCtrl).m33Lms2IptScale2P;
    AssignM33I2I(local_50,(pDVar41->ksIMapG).m33Lms2Ipt);
    GetIptNormParam((pDVar40->grcSigEnv).Bdp,(pDVar40->grcSigEnv).Rng,(pDVar41->ksIMapG).v3IptOff,
                    &(pDVar41->ksIMapG).iptScale);
    if (((pDVar40->dmCtrl).SeparatedAlpha != 0) && ((pDVar40->grcSigEnv).Clr == CClrRgba)) {
      (hDm->hDmKs->ksIMapG).clr = '\x01';
    }
    SigEnv2KsFrmFmt(&pDVar40->alpSigEnv,&pDVar34->ksFrmFmtA);
    bVar14 = (pDVar34->ksFrmFmtA).bdp;
    if (bVar14 == 0) {
      (pDVar34->ksIMapG).eotfParam.bdp = 0;
      uVar28 = 0x10000;
      pDVar34->rangeInvA = 0x10000;
      (pDVar34->ksIMapG).eotfParam.rangeMin = 0;
    }
    else {
      (pDVar34->ksIMapG).eotfParam.bdp = (uint16_t)bVar14;
      (pDVar34->ksIMapG).eotfParam.rangeMin = 0;

      lVar37 = dm_signed(((1u << (bVar14 & 31u)) - 1u) << ((16u - bVar14) & 31u));
      uVar28 = 0;
      if (lVar37 != 0) {
        uVar28 = (uint32_t)(0xffffffff / lVar37);
      }
      pDVar34->rangeInvA = uVar28;
    }
    (pDVar34->ksIMapG).eotfParam.rangeInv = uVar28;
    SigEnv2KsFrmFmt((SigEnvCom_t *)&pDVar40->srcSigEnv,&pDVar34->ksFrmFmtI);

    pDVar41 = hDm->hDmKs;
    (hDm->cscCtrl).sV3Wp[2] = 0x7fff;
    (hDm->cscCtrl).sV3Wp[1] = 0x7fff;
    (hDm->cscCtrl).sV3Wp[0] = 0x7fff;
    (hDm->cscCtrl).sV3WpScale2P = 0xf;
    if ((pDVar40->dmCtrl).MdsFollowed == 0) {
      iVar33 = (pDVar40->srcSigEnv).Bdp;
      if ((pDVar40->srcSigEnv).WpExt != 0) {
        MemCpyByte((hDm->cscCtrl).sV3Wp,(pDVar40->srcSigEnv).V3Wp,0xc);
        (hDm->cscCtrl).sV3WpScale2P = (int)(pDVar40->srcSigEnv).WpScale;
      }
      CVar4 = (pDVar40->srcSigEnv).Clr;
      KVar20 = '\0';
      if (CVar4 != CClrYuv) {
        if (CVar4 == CClrRgb) {
          KVar20 = '\x01';
        }
        else {
          KVar20 = '\x02';
          if (CVar4 != CClrRgba) {
            KVar20 = '\0';
          }
        }
      }
      (pDVar41->ksIMap).clr = KVar20;
      if ((pDVar40->srcSigEnv).Clr == CClrIpt) {
        CommitIptKsIMap((pDVar40->srcSigEnv).Bdp,(pDVar40->srcSigEnv).Rng,&pDVar41->ksIMap);
        if ((pDVar40->srcSigEnv).Yuv2RgbExt != 0) {
          AssignM33I2I((pDVar40->srcSigEnv).M33Yuv2Rgb,(pDVar41->ksIMap).m33Yuv2Rgb);
          (pDVar41->ksIMap).m33Yuv2RgbScale2P = (int16_t)(pDVar40->srcSigEnv).M33Yuv2RgbScale2P;
        }
        if ((pDVar40->srcSigEnv).Yuv2RgbOffExt != 0) {
          AssignV3I2I((pDVar40->srcSigEnv).V3Yuv2RgbOff,offset);
          offset[0] = dm_signed((uint32_t)offset[0] << 16);
          offset[1] = dm_signed((uint32_t)offset[1] << 16);
          Yuv2RgbOffsetRgbFxp((pDVar41->ksIMap).m33Yuv2Rgb,offset[0],offset[1],
                              (pDVar41->ksIMap).v3Yuv2RgbOffInRgb);
        }
        if ((pDVar40->srcSigEnv).Rgb2LmsM33Ext != 0) {
          MemCpyByte(m33,(pDVar40->srcSigEnv).M33Rgb2Lms,0x12);
          iVar36 = (pDVar40->srcSigEnv).M33Rgb2LmsScale2P;
          (hDm->cscCtrl).sM33Rgb2LmsScale2P = iVar36;
          iVar22 = GetRgb2LmsCtWpM33Fxp
                             ((int16_t (*) [3])m33,(int16_t)iVar36,(pDVar40->srcSigEnv).CrossTalk,
                              (hDm->cscCtrl).sV3Wp,(int16_t)(hDm->cscCtrl).sV3WpScale2P,
                              (pDVar41->ksIMap).m33Rgb2Lms);
          (pDVar41->ksIMap).m33Rgb2LmsScale2P = iVar22;
        }
      }
      else {
        if ((pDVar40->srcSigEnv).Yuv2RgbExt == 0) {
          if ((pDVar40->srcSigEnv).Rng == CRngNarrow) {
            iVar22 = GetYuv2RgbM33NarrowFxp
                               ((pDVar40->srcSigEnv).YuvXferSpec,(pDVar40->srcSigEnv).Bdp,
                                (pDVar41->ksIMap).m33Yuv2Rgb);
            (pDVar41->ksIMap).m33Yuv2RgbScale2P = iVar22;
          }
          else {
            iVar22 = GetYuv2RgbM33Fxp((pDVar40->srcSigEnv).YuvXferSpec,(pDVar41->ksIMap).m33Yuv2Rgb)
            ;
            (pDVar41->ksIMap).m33Yuv2RgbScale2P = iVar22;
          }
        }
        else {
          AssignM33I2I((pDVar40->srcSigEnv).M33Yuv2Rgb,(pDVar41->ksIMap).m33Yuv2Rgb);
          (pDVar41->ksIMap).m33Yuv2RgbScale2P = (int16_t)(pDVar40->srcSigEnv).M33Yuv2RgbScale2P;
        }
        if ((pDVar40->srcSigEnv).Yuv2RgbOffExt == 0) {
          GetYuvRgbOffFxp((pDVar40->srcSigEnv).Rng,iVar33,offset);
        }
        else {
          AssignV3I2I((pDVar40->srcSigEnv).V3Yuv2RgbOff,offset);
        }
        offset[1] = dm_signed((uint32_t)offset[1] << 16);
        offset[0] = dm_signed((uint32_t)offset[0] << 16);
        Yuv2RgbOffsetRgbFxp((pDVar41->ksIMap).m33Yuv2Rgb,offset[0],offset[1],
                            (pDVar41->ksIMap).v3Yuv2RgbOffInRgb);
        CVar35 = CRngFull;
        if ((pDVar40->srcSigEnv).Clr - CClrRgb < 2) {
          CVar35 = (pDVar40->srcSigEnv).Rng;
        }
        GetRange(iVar33,CVar35,&(pDVar41->ksIMap).eotfParam.rangeMin,
                 &(pDVar41->ksIMap).eotfParam.range);
        uVar28 = 0xffffffff;
        if (iVar33 != 0) {

          lVar37 = (int64_t)(int)((uint32_t)(pDVar41->ksIMap).eotfParam.range <<
                              (uint64_t)(0x10U - iVar33 & 0x1f));
          uVar28 = 0;
          if (lVar37 != 0) {
            uVar28 = (uint32_t)(0xffffffff / lVar37);
          }
        }
        (pDVar41->ksIMap).eotfParam.rangeInv = uVar28;
        (pDVar41->ksIMap).eotfParam.bdp = (uint16_t)iVar33;
        (pDVar41->ksIMap).eotfParam.eotf = (pDVar40->srcSigEnv).Eotf == CEotfPq;
        if ((pDVar40->srcSigEnv).Eotf == CEotfBt1886) {
          uVar25 = 0;
          puVar42 = (pDVar41->ksIMap).g2L;
          do {

            uVar28 = DeGamma((pDVar40->srcSigEnv).A,(pDVar40->srcSigEnv).B,
                             (pDVar40->srcSigEnv).Gamma,(pDVar40->srcSigEnv).G,uVar25);
            puVar43 = puVar42 + 1;
            *puVar42 = uVar28;
            uVar25 = uVar25 + 0x100;
            puVar42 = puVar43;
          } while (puVar43 != (uint32_t *)&(pDVar41->ksIMap).m33Rgb2LmsScale2P);
        }
        if ((pDVar40->srcSigEnv).Rgb2LmsM33Ext == 0) {
          if ((pDVar40->srcSigEnv).Rgb2LmsRgbwExt == 0) {
            iVar22 = GetRgb2LmsByDefM33((pDVar40->srcSigEnv).RgbDef,(int16_t (*) [3])m33);
            iVar33 = (int)iVar22;
            (hDm->cscCtrl).sM33Rgb2LmsScale2P = iVar33;
          }
          else {
            iVar22 = GetRgb2LmsByPrimsM33Fxp
                               ((pDVar40->srcSigEnv).V8Rgbw[0],(pDVar40->srcSigEnv).V8Rgbw[1],
                                (pDVar40->srcSigEnv).V8Rgbw[2],(pDVar40->srcSigEnv).V8Rgbw[3],
                                (pDVar40->srcSigEnv).V8Rgbw[4],(pDVar40->srcSigEnv).V8Rgbw[5],
                                (pDVar40->srcSigEnv).V8Rgbw[6],(pDVar40->srcSigEnv).V8Rgbw[7],
                                (int16_t (*) [3])m33);
            iVar33 = (int)iVar22;
            (hDm->cscCtrl).sM33Rgb2LmsScale2P = iVar33;
          }
        }
        else {
          MemCpyByte(m33,(pDVar40->srcSigEnv).M33Rgb2Lms,0x12);
          iVar33 = (pDVar40->srcSigEnv).M33Rgb2LmsScale2P;
          (hDm->cscCtrl).sM33Rgb2LmsScale2P = iVar33;
        }
        iVar22 = GetRgb2LmsCtWpM33Fxp
                           ((int16_t (*) [3])m33,(int16_t)iVar33,(pDVar40->srcSigEnv).CrossTalk,
                            (hDm->cscCtrl).sV3Wp,(int16_t)(hDm->cscCtrl).sV3WpScale2P,
                            (pDVar41->ksIMap).m33Rgb2Lms);
        (pDVar41->ksIMap).m33Rgb2LmsScale2P = iVar22;
      }
    }
    pTgtSig = &pDVar40->tgtSigEnv;
    (pDVar41->ksIMap).m33Lms2IptScale2P = (int16_t)(hDm->cscCtrl).m33Lms2IptScale2P;
    AssignM33I2I(local_50,(pDVar41->ksIMap).m33Lms2Ipt);
    GetIptNormParam((pDVar40->srcSigEnv).Bdp,(pDVar40->srcSigEnv).Rng,(pDVar41->ksIMap).v3IptOff,
                    &(pDVar41->ksIMap).iptScale);
    TgtSigEnv2TcCtrl(pTgtSig,pTcCtrl);
    uVar23 = (hDm->tcCtrl).tcTgtSig.minPq;
    uVar44 = (uint32_t)uVar23;
    uVar24 = (hDm->tcCtrl).tcTgtSig.maxPq;
    uVar27 = (uint32_t)uVar24;
    (hDm->tcCtrl).tcTgtSig.crush = uVar23;
    (hDm->tcCtrl).tcTgtSig.clip = uVar24;
    (hDm->tcCtrl).tcTgtSig.mid = (uint16_t)((int)((uint32_t)uVar23 + (uint32_t)uVar24) >> 1);
    if ((pDVar40->dmCtrl).MdsFollowed == 0) {
      iVar10 = dm_pair((pDVar40->srcSigEnv).MinPq, (pDVar40->srcSigEnv).MaxPq);
      if (iVar10 == 0) {
        uVar25 = (pDVar40->srcSigEnv).DiagSize;
        uVar28 = (pDVar40->srcSigEnv).Max;
        uVar26 = LToPQ12((pDVar40->srcSigEnv).Min);
        uVar39 = (uint32_t)uVar26;
        (hDm->tcCtrl).tcSrcSig.minPq = uVar26;
        uVar26 = LToPQ12(uVar28);
        (hDm->tcCtrl).tcSrcSig.maxPq = uVar26;
        uVar29 = (uint32_t)uVar26;
        (hDm->tcCtrl).tcSrcSig.diagSize = uVar25;
        bVar19 = (pDVar40->srcSigEnv).Eotf == CEotfHlg;
        (hDm->tcCtrl).tcSrcSig.hlgA = (uint16_t)bVar19;
        if (bVar19) {
          UpdateTcSigHlgA((pDVar40->srcSigEnv).Min,(pDVar40->srcSigEnv).Max,&pTcCtrl->tcSrcSig);
          uVar44 = (uint32_t)(hDm->tcCtrl).tcTgtSig.minPq;
          uVar39 = (uint32_t)(hDm->tcCtrl).tcSrcSig.minPq;
          uVar29 = (uint32_t)(hDm->tcCtrl).tcSrcSig.maxPq;
          uVar27 = (uint32_t)(hDm->tcCtrl).tcTgtSig.maxPq;
        }
      }
      else {
        uVar23 = (pDVar40->srcSigEnv).MinPq;
        uVar39 = (uint32_t)uVar23;
        (hDm->tcCtrl).tcSrcSig.minPq = uVar23;
        uVar23 = (pDVar40->srcSigEnv).MaxPq;
        uVar29 = (uint32_t)uVar23;
        (hDm->tcCtrl).tcSrcSig.maxPq = uVar23;
        (hDm->tcCtrl).tcSrcSig.diagSize = (pDVar40->srcSigEnv).DiagSize;
      }
      (hDm->tcCtrl).tcSrcSig.crush = (uint16_t)uVar39;
      CVar32 = CTcLutDirBwd;
      (hDm->tcCtrl).tcSrcSig.clip = (uint16_t)uVar29;
      (hDm->tcCtrl).tcSrcSig.mid = (uint16_t)((int)(uVar39 + uVar29) >> 1);
      if (((uVar27 <= uVar29 + 1) && (CVar32 = CTcLutDirFwd, uVar29 <= uVar27 + 1)) &&
         ((uVar39 + 1 < uVar44 || (CVar32 = CTcLutDirPass, uVar44 + 1 < uVar39)))) {
        CVar32 = (uint32_t)(uVar29 < uVar27);
      }
      (hDm->tcCtrl).tcLutDir = CVar32;
      TmCtrl2TcCtrl(&pDVar40->tmCtrl,(DmCtrlFxp_t *)&(pDVar40->dmCtrl).StbMode,pTcCtrl,0);
      UpdateTcCtrlLevel(&pDVar40->tmCtrl,pTcCtrl);
      uVar27 = (hDm->tcCtrl).level2[2];
      if (0 < (int)uVar27) {
        uVar27 = 0;
      }
      (hDm->tcCtrl).level2[2] = uVar27;
      uVar27 = CreateToneCurve(pTcCtrl,&pDVar34->ksTMap,&hDm->tcCtrlUsed);
      if (uVar27 == 0) goto LAB_00109218;
      uVar44 = 0x11;
      if ((uVar27 >> 0x14 & 1) != 0) {
        uVar44 = 0x100011;
      }
    }
    else {
LAB_00109218:
      uVar44 = 1;
    }

    bVar19 = (pDVar40->grcSigEnv).Eotf == CEotfHlg;
    (hDm->tc2Ctrl).tcSrcSig.hlgA = (uint16_t)bVar19;
    if ((pDVar40->dmCtrl).StbMode == CStbModeVpri) {
      uVar27 = (pDVar40->dmCtrl).PMin;
      uVar29 = (pDVar40->dmCtrl).PMax;
    }
    else {
      uVar29 = (pDVar40->grcSigEnv).Max;
      uVar39 = (pDVar40->tgtSigEnv).Max;
      if (uVar29 < uVar39) {
        uVar5 = (pDVar40->grcSigEnv).Min;
        uVar6 = (pDVar40->dmCtrl).PMin;
        uVar7 = (pDVar40->dmCtrl).PMax;
        uVar27 = (pDVar40->tgtSigEnv).Min;
        if (uVar5 < uVar6) {
          uVar6 = uVar5;
        }
        if (uVar7 < uVar29) {
          uVar7 = uVar29;
        }
        if (uVar27 < uVar6) {
          uVar27 = uVar6;
        }
        uVar29 = uVar7;
        if (uVar39 < uVar7) {
          uVar29 = uVar39;
        }
      }
      else {
        uVar27 = (pDVar40->grcSigEnv).Min;
      }
    }
    uVar25 = (pDVar40->grcSigEnv).DiagSize;
    uVar23 = LToPQ12(uVar27);
    (hDm->tc2Ctrl).tcSrcSig.minPq = uVar23;
    uVar24 = LToPQ12(uVar29);
    (hDm->tc2Ctrl).tcSrcSig.diagSize = uVar25;
    (hDm->tc2Ctrl).tcSrcSig.maxPq = uVar24;
    if (bVar19) {
      UpdateTcSigHlgA((pDVar40->grcSigEnv).Min,(pDVar40->grcSigEnv).Max,&pTcCtrl_00->tcSrcSig);
      uVar23 = (hDm->tc2Ctrl).tcSrcSig.minPq;
      uVar24 = (hDm->tc2Ctrl).tcSrcSig.maxPq;
    }
    (hDm->tc2Ctrl).tcSrcSig.crush = uVar23;
    (hDm->tc2Ctrl).tcSrcSig.clip = uVar24;
    (hDm->tc2Ctrl).tcSrcSig.mid = (uint16_t)((int)((uint32_t)uVar23 + (uint32_t)uVar24) >> 1);
    uVar29 = uVar44;
    if (((pDVar40->dmCtrl).MdsFollowed == 0) || ((pDVar40->dmCtrl).StbMode == CStbModeGpri)) {
      TgtSigEnv2TcCtrl(pTgtSig,pTcCtrl_00);
      uVar23 = (hDm->tc2Ctrl).tcTgtSig.maxPq;
      CVar32 = CTcLutDirBwd;
      uVar24 = (hDm->tc2Ctrl).tcTgtSig.minPq;
      uVar15 = (hDm->tc2Ctrl).tcSrcSig.maxPq;
      (hDm->tc2Ctrl).tcTgtSig.crush = uVar24;
      (hDm->tc2Ctrl).tcTgtSig.mid = (uint16_t)((int)((uint32_t)uVar24 + (uint32_t)uVar23) >> 1);
      (hDm->tc2Ctrl).tcTgtSig.clip = uVar23;
      uVar27 = (uint32_t)uVar23;
      uVar16 = (hDm->tc2Ctrl).tcSrcSig.minPq;
      if (uVar27 <= uVar15 + 1u) {
        CVar32 = CTcLutDirFwd;
        uVar39 = (uint32_t)uVar15;
        if (uVar39 == uVar27 + 1 || (uint32_t)uVar15 < uVar27 + 1) {
          if ((uVar16 + 1u < (uint32_t)uVar24) ||
             (uVar6 = uVar24 + 1, CVar32 = CTcLutDirPass, uVar16 != uVar6 && uVar6 <= uVar16)) {
            CVar32 = (uint32_t)(uVar39 <= uVar27 && uVar23 != uVar39);
          }
        }
      }
      (hDm->tc2Ctrl).tcLutDir = CVar32;
      TmCtrl2TcCtrl(&pDVar40->tmCtrl,(DmCtrlFxp_t *)&(pDVar40->dmCtrl).StbMode,pTcCtrl_00,1);
      UpdateTcCtrlLevel(&pDVar40->tmCtrl,pTcCtrl_00);
      uVar27 = CreateToneCurve(pTcCtrl_00,&pDVar34->ksTMapG,&hDm->tc2CtrlUsed);
      if (uVar27 != 0) {
        uVar29 = uVar44 | 0x200020;
        if ((uVar27 & 0x100000) == 0) {
          uVar29 = uVar44 | 0x20;
        }
      }
    }
    uVar27 = CommitKsOMap(pDVar40,1,hDm);
    uVar27 = uVar27 | uVar29;
    SigEnv2KsFrmFmt((SigEnvCom_t *)pTgtSig,&pDVar34->ksFrmFmtO);

    CVar8 = (pDVar40->tgtSigEnv).Chrm;
    KVar21 = '\0';
    pDVar34 = hDm->hDmKs;
    if ((CVar8 != CChrm420) && (KVar21 = '\x02', CVar8 == CChrm422)) {
      KVar21 = '\x01';
    }
    (pDVar34->ksDs).chrmOut = KVar21;
    (pDVar34->ksDs).minDs = 0;
    uVar44 = (pDVar40->tgtSigEnv).Bdp;
    (pDVar34->ksDs).filterUvColDsRadius = 7;
    (pDVar34->ksDs).filterUvColDsScale2P = 0xc;
    (pDVar34->ksDs).maxDs = (short)(1u << (uint64_t)(uVar44 & 0x1f)) + -1;
    lVar37 = 0;
    do {
      lVar2 = lVar37 + 2;
      *(uint16_t *)((int64_t)(pDVar34->ksDs).filterUvColDs_m + lVar37) =
           *(uint16_t *)((int64_t)FilterUvColDs_m + lVar37);
      lVar37 = lVar2;
    } while (lVar2 != 0x1e);
    CommitUs((pDVar40->srcSigEnv).Chrm,&(pDVar40->srcSigEnv).Bdp,&hDm->hDmKs->ksUs);
    CommitUs((pDVar40->grcSigEnv).Chrm,&(pDVar40->grcSigEnv).Bdp,&hDm->hDmKs->ksUsG);
    hDm->dmCfgToBeValid = 0;
    hDm->pDmCfgToBe = hDm->pDmCfgAct;
    hDm->pDmCfgAct = pDVar40;
    hDm->dmCfgActValid = 1;
  }
  if (hMdsExt == (MdsExt_t *)0x0) {
    return uVar27;
  }
  uVar25 = hMdsExt->mid_PQ;
  uVar23 = hMdsExt->max_PQ;
  uVar24 = hMdsExt->min_PQ;
  pDVar40 = hDm->pDmCfgAct;
  pDVar34 = hDm->hDmKs;
  if ((hDm->mdsExtFrcCmmt == 0) && (hDm->mdsExtActValid != 0)) {
    MVar30 = CheckMds(hMdsExt,&hDm->mdsExtAct);
    if (MVar30 == MDS_CHG_NONE) {
      return uVar27;
    }
    uVar44 = uVar27 | 2;
    if (MVar30 == MDS_CHG_CFG) goto LAB_001094b0;
  }
  else {
LAB_001094b0:
    (pDVar34->ksFrmFmtI).dtp = '\0';
    if (hMdsExt->signal_chroma_format == '\x01') {
      (pDVar34->ksFrmFmtI).weav = '\x02';
      if (hMdsExt->signal_chroma_format == '\0') {
        iVar33 = 6;
      }
      else {
        iVar33 = 4;
        if (hMdsExt->signal_chroma_format != '\x01') {
          iVar33 = 6;
        }
      }
    }
    else {
      (pDVar34->ksFrmFmtI).weav = '\0';

      iVar33 = 2;
    }
    (pDVar34->ksFrmFmtI).colPitch = iVar33;
    (pDVar34->ksUs).minUs = 0;
    (pDVar34->ksUs).maxUs = (short)(1u << (uint64_t)(hMdsExt->signal_bit_depth & 0x1f)) + -1;
    KVar21 = hMdsExt->signal_chroma_format;
    if ((KVar21 != '\0') && (KVar21 != '\x01')) {
      KVar21 = '\x02';
    }
    (pDVar34->ksUs).chrmIn = KVar21;
    (pDVar34->ksFrmFmtI).chrm = KVar21;
    uVar44 = uVar27 | 6;
    CommitMdsCsc(hMdsExt,pDVar40,&hDm->cscCtrl,&pDVar34->ksIMap);
    (pDVar34->ksFrmFmtI).bdp = hMdsExt->signal_bit_depth;
  }
  uVar15 = hMdsExt->source_min_PQ;
  (hDm->tcCtrl).tcSrcSig.minPq = uVar15;
  uVar16 = hMdsExt->source_max_PQ;
  (hDm->tcCtrl).tcSrcSig.maxPq = uVar16;
  uVar26 = hMdsExt->source_diagonal;
  (hDm->tcCtrl).tcSrcSig.diagSize = uVar26;
  uVar27 = (uint32_t)uVar15;
  if ((pDVar40->dmCtrl).StbMode == CStbModeVpri) {
    (hDm->tcCtrl).tcTgtSig.minPq = uVar15;
    (hDm->tcCtrl).tcTgtSig.maxPq = uVar16;
    (hDm->tcCtrl).tcTgtSig.diagSize = uVar26;
    uVar29 = uVar27;
    uVar15 = uVar16;
  }
  else {
    uVar29 = (uint32_t)(hDm->tcCtrl).tcTgtSig.minPq;
    uVar15 = (hDm->tcCtrl).tcTgtSig.maxPq;
  }
  uVar39 = (uint32_t)uVar15;
  CVar32 = CTcLutDirBwd;
  if (uVar39 <= uVar16 + 1u) {
    CVar32 = CTcLutDirFwd;
    if (((uint32_t)uVar16 <= uVar39 + 1) &&
       ((uVar27 + 1 < uVar29 || (CVar32 = CTcLutDirPass, uVar29 + 1 < uVar27)))) {
      CVar32 = (uint32_t)(uVar16 < uVar39);
    }
  }
  pDmCtrl = &(pDVar40->dmCtrl).StbMode;
  pTmCtrl = &pDVar40->tmCtrl;
  (hDm->tcCtrl).tcLutDir = CVar32;
  TmCtrl2TcCtrl(pTmCtrl,(DmCtrlFxp_t *)pDmCtrl,pTcCtrl,0);
  if ((hDm->tcCtrl).tcLutDir == CTcLutDirBwd) {
    uVar26 = hMdsExt->source_max_PQ;
  }
  else {
    uVar26 = (hDm->tcCtrl).tcTgtSig.maxPq;
  }
  CommitTrim(uVar26,hMdsExt,(DmCtxtFxp_t *)hDm->pDmCfgAct,pTcCtrl);
  if (((pDVar40->dmCtrl).SecIn != 0) && ((pDVar40->dmCtrl).StbMode == CStbModeVpri)) {
    uVar26 = LToPQ12((pDVar40->grcSigEnv).Max);
    uVar15 = uVar23;
    if (uVar23 <= uVar26) {
      uVar15 = uVar26;
    }
    uVar26 = LToPQ12((pDVar40->grcSigEnv).Min);
    uVar17 = (hDm->tcCtrl).tcSrcSig.minPq;
    uVar16 = uVar24;
    if (uVar26 <= uVar24) {
      uVar16 = uVar26;
    }
    uVar23 = (hDm->tcCtrl).tcSrcSig.maxPq;
    if (uVar15 <= uVar23) {
      uVar23 = uVar15;
    }
    uVar24 = uVar17;
    if (uVar17 <= uVar16) {
      uVar24 = uVar16;
    }
  }
  (hDm->tcCtrl).tcSrcSig.crush = uVar24;
  (hDm->tcCtrl).tcSrcSig.clip = uVar23;
  (hDm->tcCtrl).tcSrcSig.mid = uVar25;
  uVar15 = (hDm->tcCtrl).tcTgtSig.minPq;
  uVar16 = (hDm->tcCtrl).tcTgtSig.maxPq;
  (hDm->tcCtrl).tcTgtSig.crush = uVar15;
  (hDm->tcCtrl).tcTgtSig.clip = uVar16;
  uVar27 = (int)((uint32_t)uVar15 + (uint32_t)uVar16) >> 1;
  (hDm->tcCtrl).tcTgtSig.mid = (uint16_t)uVar27;
  if ((hMdsExt->lvl5AoiAvail != 0) &&
     ((iVar9 = dm_pair(hMdsExt->active_area_right_offset, hMdsExt->active_area_top_offset), iVar9 != 0 ||
      (hMdsExt->active_area_bottom_offset != 0)))) {
    iVar33 = (hDm->tcCtrl).level2[2];
    if (0 < iVar33) {
      iVar33 = 0;
    }
    (hDm->tcCtrl).level2[2] = iVar33;
  }
  uVar29 = CreateToneCurve(pTcCtrl,&pDVar34->ksTMap,&hDm->tcCtrlUsed);
  uVar27 = uVar44;
  if (uVar29 != 0) {
    uVar27 = uVar44 | 0x100010;
    if ((uVar29 & 0x100000) == 0) {
      uVar27 = uVar44 | 0x10;
    }
  }
  uVar44 = uVar27;
  if ((pDVar40->dmCtrl).SecIn != 0) {
    if ((pDVar40->dmCtrl).StbMode == CStbModeVpri) {
      uVar15 = (hDm->tcCtrl).tcTgtSig.minPq;
      (hDm->tc2Ctrl).tcTgtSig.minPq = uVar15;
      uVar16 = (hDm->tcCtrl).tcTgtSig.maxPq;
      (hDm->tc2Ctrl).tcTgtSig.maxPq = uVar16;
      (hDm->tc2Ctrl).tcTgtSig.diagSize = (hDm->tcCtrl).tcTgtSig.diagSize;
      uVar17 = (hDm->tc2Ctrl).tcSrcSig.maxPq;
      uVar18 = (hDm->tc2Ctrl).tcSrcSig.minPq;
      CVar32 = CTcLutDirBwd;
      if ((uint32_t)uVar16 <= uVar17 + 1u) {
        uVar29 = uVar16 + 1;
        CVar32 = CTcLutDirFwd;
        if ((uint32_t)uVar17 == uVar29 || (uint32_t)uVar17 < uVar29) {
          if ((uVar18 + 1u < (uint32_t)uVar15) ||
             (CVar32 = CTcLutDirPass, (uint32_t)uVar18 != uVar15 + 1u && uVar15 + 1u <= (uint32_t)uVar18)) {
            CVar32 = (uint32_t)((uint32_t)uVar17 < (uint32_t)uVar16);
          }
        }
      }
      (hDm->tc2Ctrl).tcLutDir = CVar32;
      TmCtrl2TcCtrl(pTmCtrl,(DmCtrlFxp_t *)pDmCtrl,pTcCtrl_00,1);
      if (((pDVar40->dmCtrl).StbMode != CStbModeVpri) || ((hDm->tc2Ctrl).tcLutDir != CTcLutDirBwd))
      goto LAB_00109670;
      CommitTrim((hDm->tc2Ctrl).tcSrcSig.maxPq,hMdsExt,(DmCtxtFxp_t *)hDm->pDmCfgAct,pTcCtrl_00);
      if ((hMdsExt->lvl5AoiAvail != 0) &&
         ((iVar11 = dm_pair(hMdsExt->active_area_right_offset, hMdsExt->active_area_top_offset), iVar11 != 0 ||
          (hMdsExt->active_area_bottom_offset != 0)))) {
        iVar33 = (hDm->tc2Ctrl).level2[2];
        if (0 < iVar33) {
          iVar33 = 0;
        }
        (hDm->tc2Ctrl).level2[2] = iVar33;
      }
    }
    else {
LAB_00109670:
      UpdateTcCtrlLevel(pTmCtrl,pTcCtrl_00);
    }
    if ((pDVar40->dmCtrl).StbMode == CStbModeVpri) {
      (hDm->tc2Ctrl).tcTgtSig.crush = uVar24;
      (hDm->tc2Ctrl).tcTgtSig.clip = uVar23;
      (hDm->tc2Ctrl).tcTgtSig.mid = uVar25;
    }
    else {
      uVar23 = (hDm->tc2Ctrl).tcTgtSig.minPq;
      uVar24 = (hDm->tc2Ctrl).tcTgtSig.maxPq;
      (hDm->tc2Ctrl).tcTgtSig.crush = uVar23;
      (hDm->tc2Ctrl).tcTgtSig.clip = uVar24;
      (hDm->tc2Ctrl).tcTgtSig.mid = (uint16_t)((int)((uint32_t)uVar23 + (uint32_t)uVar24) >> 1);
    }
    uVar29 = CreateToneCurve(pTcCtrl_00,&pDVar34->ksTMapG,&hDm->tc2CtrlUsed);
    if (uVar29 != 0) {
      uVar44 = uVar27 | 0x200020;
      if ((uVar29 & 0x100000) == 0) {
        uVar44 = uVar27 | 0x20;
      }
    }
  }
  iVar36 = 1;
  if ((pDVar40->dmCtrl).StbMode == CStbModeVpri) {
    if (((hDm->mdsExtActValid != 0) && (hDm->mdsExtFrcCmmt == 0)) &&
       (iVar12 = dm_pair((hDm->mdsExtAct).source_min_PQ, (hDm->mdsExtAct).source_max_PQ), iVar13 = dm_pair(hMdsExt->source_min_PQ, hMdsExt->source_max_PQ),
       iVar12 == iVar13)) goto LAB_001096e8;

    uVar23 = (hDm->tcCtrl).tcSrcSig.minPq;
    uVar27 = (uint32_t)uVar23;
    if (0xfff < uVar23) {
      uVar27 = 0xfff;
    }
    uVar29 = uVar27 >> 2;
    uVar28 = control_depq_lut[(int)uVar29];
    if (uVar29 != 0x3ff) {
      uVar28 = uVar28 + (int)((int64_t)((uint64_t)(control_depq_lut[(int)(uVar29 + 1)] - uVar28) *
                                    ((uint64_t)uVar27 & 3)) >> 2);
    }

    uVar23 = (hDm->tcCtrl).tcSrcSig.maxPq;
    uVar27 = (uint32_t)uVar23;
    if (0xfff < uVar23) {
      uVar27 = 0xfff;
    }
    uVar29 = uVar27 >> 2;
    uVar31 = control_depq_lut[(int)uVar29];
    if (uVar29 != 0x3ff) {
      uVar31 = uVar31 + (int)((int64_t)((uint64_t)(control_depq_lut[(int)(uVar29 + 1)] - uVar31) *
                                    ((uint64_t)uVar27 & 3)) >> 2);
    }
    (pDVar34->ksOMap).oetfParam.min = uVar28;
    (pDVar34->ksOMap).oetfParam.max = uVar31;
    if (((pDVar40->tgtSigEnv).Eotf == CEotfPq) || ((pDVar40->tgtSigEnv).Clr == CClrIpt)) {
      (pDVar34->ksOMap).oetfParam.min = 0;
      (pDVar34->ksOMap).oetfParam.max = 0x9c400000;
      iVar36 = 0;
    }
    else {

      uVar23 = (hDm->tcCtrl).tcSrcSig.minPq;
      uVar27 = (uint32_t)uVar23;
      if (0xfff < uVar23) {
        uVar27 = 0xfff;
      }
      uVar29 = uVar27 >> 2;
      uVar28 = control_depq_lut[(int)uVar29];
      if (uVar29 != 0x3ff) {
        uVar28 = uVar28 + (int)((int64_t)((uint64_t)(control_depq_lut[(int)(uVar29 + 1)] - uVar28) *
                                      ((uint64_t)uVar27 & 3)) >> 2);
      }

      uVar23 = (hDm->tcCtrl).tcSrcSig.maxPq;
      uVar27 = (uint32_t)uVar23;
      if (0xfff < uVar23) {
        uVar27 = 0xfff;
      }
      uVar29 = uVar27 >> 2;
      uVar31 = control_depq_lut[(int)uVar29];
      if (uVar29 != 0x3ff) {
        uVar31 = uVar31 + (int)((int64_t)((uint64_t)(control_depq_lut[(int)(uVar29 + 1)] - uVar31) *
                                      ((uint64_t)uVar27 & 3)) >> 2);
      }
      (pDVar34->ksOMap).oetfParam.min = uVar28;
      iVar36 = 0;
      (pDVar34->ksOMap).oetfParam.max = uVar31;
    }
  }
  if (hDm->mdsExtFrcCmmt != 0) {
    CommitKsOMap(pDVar40,iVar36,hDm);
  }
LAB_001096e8:
  MemCpyByte(&hDm->mdsExtAct,hMdsExt,0x208);
  hDm->mdsExtFrcCmmt = 0;
  hDm->mdsExtActValid = 1;
  return uVar44;
}


void commit_dm_color_with_ops(void *metadata, void *configuration, void *color,
                              void *input_map, const struct dm_commit_ops *ops)
{ commit_color(metadata, configuration, color, input_map, ops); }
int commit_dm_metadata_with_ops(void *metadata, void *dm,
                                const struct dm_commit_ops *ops)
{ return commit_metadata(metadata, dm, ops); }
