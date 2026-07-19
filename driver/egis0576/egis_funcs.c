#include "egis_rt.h"
#include "egis_decls.h"
#include "egis_dat.h"
#include "egis_intrin.h"

/* ===== batch 0 ===== */


/* ================ FUN_18002b720 (debug logger) ================
 * In the Windows binary this vsprintf_s/OutputDebugStringA logger.
 * In the emulated/Linux runtime it is a no-op with no observable
 * effect on the validated data path. Kept as a faithful no-op. */
void FUN_18002b720(undefined8 param_1,char *param_2,undefined8 param_3,undefined8 param_4)
{
  (void)param_1; (void)param_2; (void)param_3; (void)param_4;
  return;
}

/* ================ FUN_18002b7b0 ================ */
undefined8 FUN_18002b7b0(char *param_1,byte param_2,undefined8 param_3,undefined8 param_4)
{
  ulonglong uVar1;

  uVar1 = (ulonglong)param_2;
  FUN_18002b720(3,"993501==>%d",uVar1,param_4);
  if (param_1 == (char *)0x0) {
    return 0xfffffbfc;
  }
  if (((*param_1 == 'E') && (param_1[1] == '\x01')) && ((uint)(*(ushort *)(param_1 + 2) - 1) < 0x28)) {
    param_1[0xc] = (char)param_2;
    FUN_18002b720(3,"9935FF==>",uVar1,param_4);
    return 0;
  }
  return 0xfffffc0e;
}

/* ================ FUN_18002b840 ================ */
undefined8 FUN_18002b840(longlong param_1,byte *param_2,undefined8 param_3,undefined8 param_4)
{
  byte bVar1;

  FUN_18002b720(3,"993601==>",param_3,param_4);
  if (param_1 == 0) {
    return 0xfffffbfc;
  }
  bVar1 = *(byte *)(param_1 + 0xc);
  *param_2 = bVar1;
  FUN_18002b720(3,"9936FF==>%d",(ulonglong)bVar1,param_4);
  return 0;
}

/* ================ FUN_18002c6f0 ================ */
uint FUN_18002c6f0(longlong param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4)
{
  int iVar1;
  int *piVar2;
  uint *puVar3;
  longlong lVar4;
  uint uVar5;
  ulonglong uVar6;
  uint uVar7;
  int iVar8;
  ulonglong uVar10;
  ulonglong uVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  ulonglong uVar15;
  ulonglong uVar9;

  uVar15 = 0;
  FUN_18002b720(3,"991291==>",param_3,param_4);
  lVar4 = DAT_18009d018;
  piVar2 = *(int **)(*(longlong *)(param_1 + 0x18) + 0x78);
  uVar5 = 0;
  if (0 < *piVar2) {
    uVar6 = uVar15;
    uVar9 = uVar15;
    uVar10 = uVar15;
    uVar11 = uVar15;
    do {
      if ((int)uVar10 < (int)**(uint **)(uVar6 + *(longlong *)(piVar2 + 2))) {
        uVar10 = (ulonglong)**(uint **)(uVar6 + *(longlong *)(piVar2 + 2));
        uVar11 = uVar9;
      }
      uVar5 = (uint)uVar11;
      uVar7 = (int)uVar9 + 1;
      uVar9 = (ulonglong)uVar7;
      uVar6 = uVar6 + 8;
    } while ((int)uVar7 < *piVar2);
  }
  iVar12 = -0x400;
  puVar3 = *(uint **)(*(longlong *)(piVar2 + 2) + (longlong)(int)uVar5 * 8);
  iVar14 = 0x400;
  iVar8 = -0x400;
  uVar5 = *puVar3;
  iVar13 = 0x400;
  if (0 < (int)uVar5) {
    uVar6 = (ulonglong)uVar5;
    do {
      piVar2 = *(int **)(uVar15 + *(longlong *)(puVar3 + 6));
      uVar15 = uVar15 + 8;
      iVar1 = *piVar2;
      if (iVar1 < iVar14) {
        iVar14 = iVar1;
      }
      if (iVar12 < iVar1) {
        iVar12 = iVar1;
      }
      iVar1 = piVar2[1];
      if (iVar1 < iVar13) {
        iVar13 = iVar1;
      }
      if (iVar8 < iVar1) {
        iVar8 = iVar1;
      }
      uVar6 = uVar6 - 1;
    } while (uVar6 != 0);
  }
  if (*(int *)(DAT_18009d018 + 0x70) - *(int *)(param_1 + 4) == *(int *)(DAT_18009d018 + 0x6c) + 2)
  {
    *(int *)(*(longlong *)(param_1 + 0x18) + 0x18) = iVar8 - iVar13;
  }
  uVar5 = (((iVar8 - iVar13) - *(int *)(*(longlong *)(param_1 + 0x18) + 0x18)) * 10) /
          *(int *)(param_1 + 0x24);
  if (iVar12 - iVar14 < *(int *)(param_1 + 0x28)) {
    if ((int)uVar5 < 100) {
      uVar5 = (int)(uVar5 * 9) / 10;
    }
    else {
      uVar5 = 0x5a;
    }
  }
  uVar15 = (longlong)((*(int *)(lVar4 + 0x70) - *(int *)(param_1 + 4)) * 100) /
           (longlong)*(int *)(lVar4 + 0x70);
  uVar7 = (uint)uVar15;
  uVar15 = uVar15 & 0xffffffff;
  FUN_18002b720(3,"991292==>%d %d",(ulonglong)uVar5,uVar15);
  if ((int)uVar7 < (int)uVar5) {
    uVar7 = uVar5;
  }
  if ((int)uVar7 < 0) {
    uVar7 = 0;
  }
  FUN_18002b720(3,"991293==>%d",(ulonglong)uVar7,uVar15);
  return uVar7;
}

/* ================ FUN_18002cf00 ================ */
void FUN_18002cf00(undefined8 param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4)
{
  int *_Memory;
  void *_Memory_00;
  uint uVar1;
  ulonglong uVar2;
  ulonglong uVar3;
  ulonglong uVar4;
  undefined8 *puVar5;
  ulonglong uVar6;

  FUN_18002b720(3,"991501==>",param_3,param_4);
  uVar4 = 0;
  if (0 < DAT_1800a0464) {
    puVar5 = &DAT_1800a0470;
    uVar6 = uVar4;
    do {
      _Memory = (int *)*puVar5;
      if (_Memory != (int *)0x0) {
        uVar2 = uVar4;
        uVar3 = uVar4;
        if (0 < *_Memory) {
          do {
            _Memory_00 = *(void **)(uVar3 + *(longlong *)(_Memory + 2));
            if (_Memory_00 != (void *)0x0) {
              free(*(void **)((longlong)_Memory_00 + 0x10));
              free(_Memory_00);
            }
            uVar1 = (int)uVar2 + 1;
            uVar2 = (ulonglong)uVar1;
            uVar3 = uVar3 + 8;
          } while ((int)uVar1 < *_Memory);
        }
        free(_Memory);
        *puVar5 = 0;
      }
      *(undefined4 *)(puVar5 + -1) = 0;
      *(undefined1 *)(puVar5 + 1) = 0;
      uVar1 = (int)uVar6 + 1;
      uVar6 = (ulonglong)uVar1;
      puVar5 = puVar5 + 3;
    } while ((int)uVar1 < DAT_1800a0464);
  }
  FUN_18002b720(3,"9915FF==>",param_3,param_4);
  return;
}

/* ================ FUN_18002d250 ================
 * The CONCAT17(...CONCAT11...) tree in the decomp assembles an 8-byte
 * value where byte i = (local_20 byte i) ^ (uStack_18 byte i); this is
 * exactly (local_20 ^ uStack_18) as a 64-bit value. local_20/uStack_18
 * form one contiguous 16-byte key buffer that the loop XORs across, so
 * they are represented as a single 16-byte array. */
undefined8 FUN_18002d250(longlong param_1,int param_2)
{
  ulonglong uVar1;
  byte *pbVar2;
  ulonglong uVar3;
  uint uVar4;
  uint8_t kbuf[16];

  uVar3 = 0;
  uVar1 = uVar3;
  while (*(char *)(param_1 + 4 + uVar1) == '\0') {
    uVar1 = uVar1 + 1;
    if (7 < (longlong)uVar1) {
      return 0;
    }
  }
  *(uint64_t *)kbuf = _DAT_18007d2d8;
  *(uint64_t *)(kbuf + 8) = _UNK_18007d2e0;
  uVar1 = uVar3;
  if (0 < param_2 + -0x20) {
    do {
      uVar4 = (uint)uVar1;
      uVar3 = uVar3 + 1;
      uVar1 = (ulonglong)(uVar4 + 1);
      pbVar2 = &kbuf[uVar4 & 0xf];
      *pbVar2 = *pbVar2 ^ *(byte *)(param_1 + 0x1f + uVar3);
    } while ((longlong)uVar3 < (longlong)(param_2 + -0x20));
  }
  if ((*(uint64_t *)kbuf ^ *(uint64_t *)(kbuf + 8)) == (uint64_t)*(longlong *)(param_1 + 4)) {
    return 0;
  }
  return 0xfffffbff;
}

/* ================ FUN_18002e460 ================ */
void * FUN_18002e460(int param_1,int param_2)
{
  void *pvVar1;
  longlong lVar2;
  longlong lVar3;

  pvVar1 = malloc((longlong)(param_1 * param_2 + param_2 * 8));
  if (pvVar1 != (void *)0x0) {
    lVar2 = 0;
    lVar3 = (longlong)pvVar1 + (longlong)(param_2 * 8);
    if (0 < param_2) {
      do {
        *(longlong *)((longlong)pvVar1 + lVar2 * 8) = lVar3;
        lVar2 = lVar2 + 1;
        lVar3 = lVar3 + param_1;
      } while (lVar2 < param_2);
    }
    return pvVar1;
  }
  return (void *)0x0;
}

/* ================ FUN_18002e4f0 ================ */
void FUN_18002e4f0(int param_1,int param_2,longlong param_3,int param_4)
{
  int iVar1;
  longlong lVar2;
  void *_Memory;
  int *_Memory_00;
  void *pvVar3;
  longlong lVar4;
  longlong lVar5;
  size_t _Count;
  longlong lVar6;
  int iVar7;
  longlong lVar8;
  longlong *plVar9;
  int iVar10;
  longlong lVar11;
  longlong lVar12;
  int iVar13;
  int *piVar14;
  longlong lVar15;
  int iVar16;

  lVar2 = (longlong)param_2;
  _Count = (size_t)param_1;
  iVar16 = param_4 * 2 + 1;
  iVar1 = param_2;
  if (iVar16 < param_2) {
    iVar1 = iVar16;
  }
  iVar7 = 0;
  _Memory = malloc(lVar2 * 8);
  _Memory_00 = (int *)calloc(_Count,4);
  lVar8 = 0;
  if (0 < (longlong)iVar1) {
    do {
      pvVar3 = malloc(_Count);
      lVar8 = lVar8 + 1;
      *(void **)((longlong)_Memory + lVar8 * 8 + -8) = pvVar3;
    } while (lVar8 < iVar1);
  }
  lVar8 = (longlong)-param_4;
  if (lVar8 < lVar2) {
    lVar11 = (longlong)-iVar16;
    plVar9 = (longlong *)(lVar11 * 8 + (longlong)_Memory);
    lVar6 = lVar8;
    do {
      lVar12 = (lVar11 - lVar8) + lVar6;
      if (-1 < lVar12) {
        lVar15 = *plVar9;
        lVar4 = 0;
        if (0 < (longlong)_Count) {
          do {
            lVar5 = lVar4 + 1;
            _Memory_00[lVar4] = _Memory_00[lVar4] - (uint)*(byte *)(lVar4 + lVar15);
            lVar4 = lVar5;
          } while (lVar5 < (longlong)_Count);
        }
        iVar7 = iVar7 + -1;
      }
      if (lVar6 - lVar8 < lVar2) {
        if (-1 < lVar12) {
          plVar9[-lVar11] = *plVar9;
        }
        FUN_18004d200((undefined8 *)plVar9[-lVar11],
                      *(undefined8 **)
                       (((param_3 + lVar11 * -8) - (longlong)_Memory) + (longlong)plVar9),_Count);
        lVar12 = plVar9[-lVar11];
        lVar15 = 0;
        if (0 < (longlong)_Count) {
          do {
            lVar4 = lVar15 + 1;
            _Memory_00[lVar15] = _Memory_00[lVar15] + (uint)*(byte *)(lVar15 + lVar12);
            lVar15 = lVar4;
          } while (lVar4 < (longlong)_Count);
        }
        iVar7 = iVar7 + 1;
      }
      if (-1 < lVar6) {
        lVar12 = *(longlong *)
                  (((lVar8 - lVar11) * 8 - (longlong)_Memory) + param_3 + (longlong)plVar9);
        iVar10 = 0;
        lVar15 = -(longlong)param_4;
        iVar13 = 0;
        if (lVar15 < (longlong)_Count) {
          lVar4 = 0;
          piVar14 = _Memory_00;
          do {
            if (-1 < lVar4 - iVar16) {
              iVar10 = iVar10 - piVar14[-(longlong)iVar16];
              iVar13 = iVar13 - iVar7;
            }
            if (lVar4 < (longlong)_Count) {
              iVar10 = iVar10 + *piVar14;
              iVar13 = iVar13 + iVar7;
            }
            if (-1 < lVar4 + lVar15) {
              *(char *)((lVar12 - param_4) + lVar4) = (char)(iVar10 / iVar13);
            }
            lVar4 = lVar4 + 1;
            piVar14 = piVar14 + 1;
          } while (lVar4 + lVar15 < (longlong)_Count);
        }
      }
      lVar6 = lVar6 + 1;
      plVar9 = plVar9 + 1;
    } while (lVar6 < lVar2);
  }
  for (lVar8 = (longlong)(param_2 - iVar1); lVar8 < lVar2; lVar8 = lVar8 + 1) {
    free(*(void **)((longlong)_Memory + lVar8 * 8));
  }
  free(_Memory);
  free(_Memory_00);
  return;
}

/* ================ FUN_18002e7a0 ================ */
void FUN_18002e7a0(int param_1,int param_2,longlong param_3,longlong param_4)
{
  longlong lVar1;
  longlong lVar2;
  longlong lVar3;
  int iVar4;
  byte bVar5;
  int iVar6;
  void *_Memory;
  int *_Memory_00;
  void *pvVar7;
  byte *pbVar8;
  byte bVar9;
  int *piVar10;
  size_t _Count;
  longlong lVar11;
  longlong lVar12;
  uint uVar13;
  byte bVar14;
  size_t sVar15;
  int iVar16;
  longlong lVar17;
  longlong lVar18;
  longlong *plVar19;
  int iVar20;
  longlong local_70;

  lVar18 = (longlong)param_2;
  _Count = (size_t)param_1;
  iVar4 = param_2;
  if (0x11 < param_2) {
    iVar4 = 0x11;
  }
  iVar20 = 0;
  _Memory = malloc(lVar18 * 8);
  _Memory_00 = (int *)calloc(_Count,4);
  lVar11 = 0;
  if (0 < (longlong)iVar4) {
    do {
      pvVar7 = malloc(_Count);
      lVar11 = lVar11 + 1;
      *(void **)((longlong)_Memory + lVar11 * 8 + -8) = pvVar7;
    } while (lVar11 < iVar4);
  }
  if (-8 < lVar18) {
    plVar19 = (longlong *)((longlong)_Memory + -0x88);
    local_70 = -8;
    lVar11 = param_3 - (longlong)_Memory;
    do {
      if (-1 < local_70 + -9) {
        pbVar8 = *(byte **)(lVar11 + (longlong)plVar19);
        if (0 < (longlong)_Count) {
          lVar17 = *plVar19 - (longlong)pbVar8;
          piVar10 = _Memory_00;
          sVar15 = _Count;
          do {
            iVar6 = (uint)pbVar8[lVar17] - (uint)*pbVar8;
            if (iVar6 < 0) {
              iVar6 = (uint)*pbVar8 - (uint)pbVar8[lVar17];
            }
            *piVar10 = *piVar10 - iVar6;
            piVar10 = piVar10 + 1;
            pbVar8 = pbVar8 + 1;
            sVar15 = sVar15 - 1;
          } while (sVar15 != 0);
        }
        iVar20 = iVar20 + -1;
      }
      if (local_70 + 8 < lVar18) {
        if (-1 < local_70 + -9) {
          plVar19[0x11] = *plVar19;
        }
        FUN_18004d200((undefined8 *)plVar19[0x11],
                      *(undefined8 **)((param_4 - (longlong)_Memory) + 0x88 + (longlong)plVar19),
                      _Count);
        pbVar8 = *(byte **)(lVar11 + 0x88 + (longlong)plVar19);
        if (0 < (longlong)_Count) {
          lVar17 = plVar19[0x11] - (longlong)pbVar8;
          piVar10 = _Memory_00;
          sVar15 = _Count;
          do {
            iVar6 = (uint)pbVar8[lVar17] - (uint)*pbVar8;
            if (iVar6 < 0) {
              iVar6 = (uint)*pbVar8 - (uint)pbVar8[lVar17];
            }
            *piVar10 = *piVar10 + iVar6;
            piVar10 = piVar10 + 1;
            pbVar8 = pbVar8 + 1;
            sVar15 = sVar15 - 1;
          } while (sVar15 != 0);
        }
        iVar20 = iVar20 + 1;
      }
      if (-1 < local_70) {
        lVar17 = *(longlong *)(lVar11 + 0x48 + (longlong)plVar19);
        lVar2 = plVar19[9];
        lVar3 = *(longlong *)((param_4 - (longlong)_Memory) + 0x48 + (longlong)plVar19);
        iVar16 = 0;
        iVar6 = 0;
        if (-8 < (longlong)_Count) {
          piVar10 = _Memory_00;
          lVar12 = 0;
          do {
            if (-1 < lVar12 + -0x11) {
              iVar16 = iVar16 - piVar10[-0x11];
              iVar6 = iVar6 - iVar20;
            }
            if (lVar12 < (longlong)_Count) {
              iVar16 = iVar16 + *piVar10;
              iVar6 = iVar6 + iVar20;
            }
            if (-1 < lVar12 + -8) {
              bVar5 = (byte)((longlong)iVar16 / (longlong)iVar6);
              bVar9 = *(byte *)(lVar12 + lVar2 + -8);
              if (bVar9 < bVar5) {
                uVar13 = 0;
              }
              else {
                uVar13 = (uint)(byte)(bVar9 - bVar5);
              }
              if ((int)(0xff - ((uint)((longlong)iVar16 / (longlong)iVar6) & 0xff)) <
                  (int)(uint)bVar9) {
                bVar9 = 0xff;
              }
              else {
                bVar9 = bVar9 + bVar5;
              }
              bVar5 = *(byte *)(lVar12 + lVar17 + -8);
              bVar14 = bVar9 - (byte)uVar13;
              if (bVar14 != 0) {
                if ((byte)uVar13 < bVar5) {
                  if (bVar5 < bVar9) {
                    bVar5 = (byte)((int)((bVar5 - uVar13) * 0xff) / (int)(uint)bVar14);
                  }
                  else {
                    bVar5 = 0xff;
                  }
                }
                else {
                  bVar5 = 0;
                }
              }
              *(byte *)(lVar12 + lVar3 + -8) = bVar5;
            }
            piVar10 = piVar10 + 1;
            lVar1 = lVar12 + -7;
            lVar12 = lVar12 + 1;
          } while (lVar1 < (longlong)_Count);
        }
      }
      local_70 = local_70 + 1;
      plVar19 = plVar19 + 1;
    } while (local_70 < lVar18);
  }
  for (lVar11 = (longlong)(param_2 - iVar4); lVar11 < lVar18; lVar11 = lVar11 + 1) {
    free(*(void **)((longlong)_Memory + lVar11 * 8));
  }
  free(_Memory);
  free(_Memory_00);
  return;
}

/* ================ FUN_18002fae0 (AES-like MixColumns over GF(2^8)) ================
 * local_38[4] and the 12 individual byte locals local_34..local_29 form one
 * contiguous 16-byte state; represented here as st[16]. */
void FUN_18002fae0(longlong param_1,undefined8 param_2,char param_3)
{
  byte bVar1;
  byte *pbVar2;
  byte bVar3;
  byte *pbVar4;
  int iVar5;
  longlong lVar6;
  longlong lVar7;
  byte *pbVar8;
  longlong lVar9;
  byte bVar10;
  longlong lVar11;
  ulonglong uVar12;
  undefined8 *puVar13;
  byte local_res20[8];
  byte st[16];

  if (4 < (byte)(param_3 << 2)) {
    puVar13 = (undefined8 *)(param_1 + 0x20);
    uVar12 = (ulonglong)(uint)((((byte)(param_3 << 2) - 5) >> 2) + 1);
    do {
      lVar7 = 4;
      do {
        pbVar2 = (byte *)*puVar13;
        lVar11 = 0;
        pbVar8 = pbVar2;
        do {
          local_res20[0] = *pbVar8;
          lVar9 = 4;
          do {
            bVar1 = (&DAT_18007ba90)[lVar11];
            bVar10 = 0;
            bVar3 = bVar1;
            if ((local_res20[0] != 1) && (bVar3 = local_res20[0], bVar1 != 1)) {
              pbVar4 = local_res20;
              lVar6 = 7;
              do {
                bVar3 = *pbVar4;
                pbVar4 = pbVar4 + 1;
                if (bVar3 < 0x80) {
                  bVar3 = bVar3 * '\x02';
                }
                else {
                  bVar3 = bVar3 * '\x02' ^ 0x1b;
                }
                *pbVar4 = bVar3;
                lVar6 = lVar6 + -1;
              } while (lVar6 != 0);
              pbVar4 = local_res20;
              iVar5 = 0;
              do {
                if ((bVar1 >> ((byte)iVar5 & 0x1f) & 1) != 0) {
                  bVar10 = bVar10 ^ *pbVar4;
                }
                iVar5 = iVar5 + 1;
                pbVar4 = pbVar4 + 1;
                bVar3 = bVar10;
              } while (iVar5 < 8);
            }
            st[lVar11] = bVar3;
            lVar11 = lVar11 + 1;
            lVar9 = lVar9 + -1;
          } while (lVar9 != 0);
          pbVar8 = pbVar8 + 1;
        } while (lVar11 < 0x10);
        puVar13 = puVar13 + 1;
        *pbVar2 = st[0] ^ st[12] ^ st[8] ^ st[4];
        pbVar2[1] = st[5] ^ st[9] ^ st[13] ^ st[1];
        pbVar2[2] = st[6] ^ st[10] ^ st[14] ^ st[2];
        pbVar2[3] = st[7] ^ st[11] ^ st[15] ^ st[3];
        lVar7 = lVar7 + -1;
      } while (lVar7 != 0);
      uVar12 = uVar12 - 1;
    } while (uVar12 != 0);
  }
  return;
}

/* ================ FUN_18002fc80 (AES key expansion) ================
 * local_44 (undefined4) accessed byte-wise via _N_1_ fields -> uint8_t[4]. */
void FUN_18002fc80(longlong param_1,longlong *param_2,uint param_3,int param_4)
{
  undefined1 *puVar1;
  byte bVar2;
  uint32_t uVar3;
  ulonglong uVar4;
  undefined1 *puVar5;
  ulonglong uVar6;
  uint uVar7;
  int iVar8;
  ulonglong uVar9;
  longlong lVar10;
  longlong lVar11;
  longlong *plVar12;
  longlong *plVar13;
  uint8_t local_44[4];

  uVar4 = ((ulonglong)param_3 & 0xff) >> 2;
  uVar7 = (uint)uVar4;
  uVar6 = 0x180000000;
  bVar2 = (&DAT_18007caa0)[((longlong)(int)(uVar7 / 2) + -2) * 3];
  if (uVar7 != 0) {
    puVar5 = (undefined1 *)(param_1 + 2);
    lVar10 = 0;
    do {
      lVar11 = lVar10 + 1;
      *(undefined1 *)param_2[lVar10] = puVar5[-2];
      *(undefined1 *)(param_2[lVar10] + 1) = puVar5[-1];
      *(undefined1 *)(param_2[lVar10] + 2) = *puVar5;
      uVar6 = param_2[lVar10];
      puVar1 = puVar5 + 1;
      puVar5 = puVar5 + 4;
      *(undefined1 *)(uVar6 + 3) = *puVar1;
      lVar10 = lVar11;
    } while (lVar11 < (longlong)uVar4);
  }
  uVar9 = (ulonglong)(int)((uint)bVar2 * 4 + 4);
  if (uVar4 < uVar9) {
    lVar10 = uVar9 - uVar4;
    uVar9 = uVar4;
    plVar12 = param_2;
    do {
      uVar3 = *(uint32_t *)plVar12[uVar4 - 1];
      iVar8 = (int)uVar9;
      local_44[0] = (byte)uVar3;
      local_44[2] = (byte)((uint)uVar3 >> 0x10);
      local_44[3] = (byte)((uint)uVar3 >> 0x18);
      local_44[1] = (byte)((uint)uVar3 >> 8);
      if (iVar8 % (int)uVar7 == 0) {
        uVar9 = (ulonglong)local_44[1];
        uVar6 = (ulonglong)local_44[0];
        local_44[1] = (&DAT_18007cab0)[local_44[2]];
        local_44[0] =
             (&DAT_18007cab0)[uVar9] ^ (&DAT_18007a94c)[(longlong)(iVar8 / (int)uVar7) * 4];
        local_44[2] = (&DAT_18007cab0)[local_44[3]];
        local_44[3] = (&DAT_18007cab0)[uVar6];
      }
      else if ((6 < uVar7) && (iVar8 % (int)uVar7 == 4)) {
        local_44[0] = (&DAT_18007cab0)[local_44[0]];
        local_44[1] = (&DAT_18007cab0)[local_44[1]];
        local_44[2] = (&DAT_18007cab0)[local_44[2]];
        local_44[3] = (&DAT_18007cab0)[local_44[3]];
      }
      uVar9 = (ulonglong)(iVar8 + 1);
      plVar13 = plVar12 + 1;
      *(byte *)plVar13[uVar4 - 1] = *(byte *)*plVar12 ^ local_44[0];
      *(byte *)(plVar13[uVar4 - 1] + 1) = *(byte *)(*plVar12 + 1) ^ local_44[1];
      *(byte *)(plVar13[uVar4 - 1] + 2) = *(byte *)(*plVar12 + 2) ^ local_44[2];
      local_44[3] = *(byte *)(*plVar12 + 3) ^ local_44[3];
      uVar6 = (ulonglong)local_44[3];
      *(byte *)(plVar13[uVar4 - 1] + 3) = local_44[3];
      lVar10 = lVar10 + -1;
      plVar12 = plVar13;
      *(uint32_t *)local_44 = uVar3;
    } while (lVar10 != 0);
  }
  if (param_4 != 0) {
    FUN_18002fae0((longlong)param_2,uVar6,(char)bVar2);
  }
  return;
}

/* ================ FUN_18002fec0 (AES-like block encrypt) ================
 * NOTE: Ghidra rendered a small .rdata index table as the empty string
 * literal ""; the code reads 4 consecutive bytes from it (pcVar11[0..3]).
 * The real bytes live in mapped .rdata but their address was lost in the
 * decomp, so "" is kept verbatim. See notes. */
void FUN_18002fec0(byte *param_1,longlong *param_2,int param_3,undefined8 param_4,undefined8 param_5,
                   int param_6)
{
  char cVar1;
  char cVar2;
  int iVar3;
  uint uVar4;
  ulonglong uVar5;
  longlong lVar6;
  longlong lVar7;
  byte *pbVar8;
  int iVar9;
  longlong lVar10;
  char *pcVar11;
  byte *pbVar12;
  byte *pbVar13;
  byte **ppbVar14;
  byte bVar15;
  longlong *plVar16;
  byte *pbVar17;
  ulonglong uVar18;
  ulonglong local_80;
  byte *local_68[4];
  byte local_48[16];

  (void)param_4; (void)param_5;
  if (param_3 != 0) {
    local_80 = (ulonglong)(((param_3 - 1U) >> 4) + 1);
    do {
      uVar5 = 4;
      pbVar13 = &DAT_18007a988;
      lVar10 = 4;
      plVar16 = param_2;
      do {
        bVar15 = *pbVar13;
        lVar7 = *plVar16;
        lVar6 = 4;
        pbVar12 = local_48 + bVar15;
        do {
          *pbVar12 = pbVar12[(lVar7 - (ulonglong)bVar15) - (longlong)local_48] ^
                     pbVar12[(longlong)param_1 - (longlong)local_48];
          lVar6 = lVar6 + -1;
          pbVar12 = pbVar12 + 1;
        } while (lVar6 != 0);
        pbVar13 = pbVar13 + 1;
        plVar16 = plVar16 + 1;
        lVar10 = lVar10 + -1;
      } while (lVar10 != 0);
      if (1 < param_6) {
        uVar18 = (ulonglong)(param_6 - 1);
        pbVar13 = local_48;
        pbVar12 = param_1;
        do {
          pbVar8 = pbVar13;
          iVar9 = 0;
          plVar16 = param_2 + uVar5;
          do {
            iVar3 = 0;
            pcVar11 = "";
            bVar15 = (char)iVar9 * '\x04';
            lVar10 = 0;
            ppbVar14 = local_68;
            do {
              cVar1 = *pcVar11;
              pcVar11 = pcVar11 + 1;
              cVar2 = (char)iVar3;
              iVar3 = iVar3 + 1;
              lVar7 = (ulonglong)pbVar8[(byte)(&DAT_18007a9f0)[(byte)(cVar1 + cVar2 + bVar15)]] +
                      lVar10;
              lVar10 = lVar10 + 0x100;
              *ppbVar14 = &DAT_18007aa70 + lVar7 * 4;
              ppbVar14 = ppbVar14 + 1;
            } while (iVar3 < 4);
            pbVar13 = (byte *)*plVar16;
            iVar9 = iVar9 + 1;
            pbVar17 = pbVar12 + bVar15;
            plVar16 = plVar16 + 1;
            *pbVar17 = *local_68[0] ^ *local_68[1] ^ *local_68[2] ^ *pbVar13 ^ *local_68[3];
            pbVar17[1] = pbVar13[1] ^ local_68[0][1] ^ local_68[1][1] ^ local_68[2][1] ^
                         local_68[3][1];
            pbVar17[2] = pbVar13[2] ^ local_68[0][2] ^ local_68[1][2] ^ local_68[2][2] ^
                         local_68[3][2];
            pbVar17[3] = pbVar13[3] ^ local_68[0][3] ^ local_68[1][3] ^ local_68[2][3] ^
                         local_68[3][3];
          } while (iVar9 < 4);
          uVar5 = (ulonglong)(byte)((char)uVar5 + 4);
          uVar18 = uVar18 - 1;
          pbVar13 = pbVar12;
          pbVar12 = pbVar8;
        } while (uVar18 != 0);
      }
      pbVar13 = &DAT_18007a988;
      lVar10 = 4;
      pbVar12 = &DAT_18007a988;
      do {
        bVar15 = *pbVar12;
        iVar9 = 0;
        do {
          uVar4 = (uint)bVar15 + iVar9;
          iVar9 = iVar9 + 1;
          uVar4 = uVar4 & 0xff;
          local_48[uVar4] = (&DAT_18007cab0)[param_1[uVar4]];
        } while (iVar9 < 4);
        pbVar12 = pbVar12 + 1;
        lVar10 = lVar10 + -1;
      } while (lVar10 != 0);
      iVar9 = 0;
      lVar10 = 0;
      do {
        uVar5 = (ulonglong)*pbVar13;
        lVar6 = 4;
        lVar7 = param_2[iVar9 + (uint)(byte)((char)param_6 << 2) & 0xff];
        pbVar12 = param_1 + uVar5;
        do {
          *pbVar12 = local_48[pbVar12[(longlong)
                                      (&DAT_18007aa10 + ((lVar10 - uVar5) - (longlong)param_1))]] ^
                     pbVar12[(lVar7 - uVar5) - (longlong)param_1];
          lVar6 = lVar6 + -1;
          pbVar12 = pbVar12 + 1;
        } while (lVar6 != 0);
        iVar9 = iVar9 + 1;
        pbVar13 = pbVar13 + 1;
        lVar10 = lVar10 + 4;
      } while (iVar9 < 4);
      param_1 = param_1 + 0x10;
      local_80 = local_80 - 1;
    } while (local_80 != 0);
  }
  return;
}

/* ================ FUN_1800301f0 (AES-like block decrypt) ================
 * Same "" .rdata-table artifact as FUN_18002fec0; see notes. */
void FUN_1800301f0(byte *param_1,longlong *param_2,int param_3,undefined8 param_4,undefined8 param_5,
                   int param_6)
{
  char cVar1;
  char cVar2;
  int iVar3;
  uint uVar4;
  longlong lVar5;
  longlong lVar6;
  byte *pbVar7;
  longlong *plVar8;
  int iVar9;
  longlong lVar10;
  longlong lVar11;
  char *pcVar12;
  byte *pbVar13;
  byte *pbVar14;
  byte **ppbVar15;
  byte bVar16;
  byte *pbVar17;
  ulonglong uVar18;
  ulonglong uVar19;
  byte bVar20;
  byte *local_68[4];
  byte local_48[16];

  (void)param_4; (void)param_5;
  if (param_3 != 0) {
    uVar19 = (ulonglong)(((param_3 - 1U) >> 4) + 1);
    do {
      pbVar14 = &DAT_18007a988;
      lVar10 = 4;
      plVar8 = param_2 + (byte)((char)param_6 << 2);
      do {
        bVar20 = *pbVar14;
        lVar6 = *plVar8;
        lVar5 = 4;
        pbVar13 = local_48 + bVar20;
        do {
          *pbVar13 = pbVar13[(lVar6 - (ulonglong)bVar20) - (longlong)local_48] ^
                     (pbVar13 + 1)[(longlong)(param_1 + (-1 - (longlong)local_48))];
          lVar5 = lVar5 + -1;
          pbVar13 = pbVar13 + 1;
        } while (lVar5 != 0);
        plVar8 = plVar8 + 1;
        pbVar14 = pbVar14 + 1;
        lVar10 = lVar10 + -1;
      } while (lVar10 != 0);
      if (1 < param_6) {
        uVar18 = (ulonglong)(param_6 - 1);
        pbVar14 = local_48;
        pbVar13 = param_1;
        bVar20 = ((char)param_6 + -1) * '\x04';
        do {
          pbVar7 = pbVar14;
          iVar9 = 0;
          plVar8 = param_2 + bVar20;
          do {
            iVar3 = 0;
            pcVar12 = "";
            bVar16 = (char)iVar9 * '\x04';
            lVar10 = 0;
            ppbVar15 = local_68;
            do {
              cVar2 = (char)iVar3;
              iVar3 = iVar3 + 1;
              cVar1 = *pcVar12;
              pcVar12 = pcVar12 + 1;
              lVar6 = (ulonglong)pbVar7[(byte)(&DAT_18007a9f0)[(byte)(bVar16 + cVar2 + cVar1)]] +
                      lVar10;
              lVar10 = lVar10 + 0x100;
              *ppbVar15 = &DAT_18007baa0 + lVar6 * 4;
              ppbVar15 = ppbVar15 + 1;
            } while (iVar3 < 4);
            pbVar14 = (byte *)*plVar8;
            iVar9 = iVar9 + 1;
            pbVar17 = pbVar13 + bVar16;
            plVar8 = plVar8 + 1;
            *pbVar17 = *local_68[0] ^ *local_68[1] ^ *local_68[2] ^ *local_68[3] ^ *pbVar14;
            pbVar17[1] = pbVar14[1] ^ local_68[0][1] ^ local_68[1][1] ^ local_68[2][1] ^
                         local_68[3][1];
            pbVar17[2] = pbVar14[2] ^ local_68[0][2] ^ local_68[1][2] ^ local_68[2][2] ^
                         local_68[3][2];
            pbVar17[3] = pbVar14[3] ^ local_68[0][3] ^ local_68[1][3] ^ local_68[2][3] ^
                         local_68[3][3];
          } while (iVar9 < 4);
          bVar20 = bVar20 - 4;
          uVar18 = uVar18 - 1;
          pbVar14 = pbVar13;
          pbVar13 = pbVar7;
        } while (uVar18 != 0);
      }
      pbVar14 = &DAT_18007a988;
      lVar10 = 4;
      pbVar13 = &DAT_18007a988;
      do {
        bVar20 = *pbVar13;
        iVar9 = 0;
        do {
          uVar4 = (uint)bVar20 + iVar9;
          iVar9 = iVar9 + 1;
          uVar4 = uVar4 & 0xff;
          local_48[uVar4] = (&DAT_18007cbb0)[param_1[uVar4]];
        } while (iVar9 < 4);
        pbVar13 = pbVar13 + 1;
        lVar10 = lVar10 + -1;
      } while (lVar10 != 0);
      lVar6 = 0;
      lVar10 = 4;
      plVar8 = param_2;
      do {
        uVar18 = (ulonglong)*pbVar14;
        lVar5 = *plVar8;
        lVar11 = 4;
        pbVar13 = param_1 + uVar18;
        do {
          *pbVar13 = local_48[pbVar13[(longlong)
                                      (&DAT_18007a990 + ((lVar6 - uVar18) - (longlong)param_1))]] ^
                     pbVar13[(lVar5 - uVar18) - (longlong)param_1];
          lVar11 = lVar11 + -1;
          pbVar13 = pbVar13 + 1;
        } while (lVar11 != 0);
        pbVar14 = pbVar14 + 1;
        plVar8 = plVar8 + 1;
        lVar6 = lVar6 + 4;
        lVar10 = lVar10 + -1;
      } while (lVar10 != 0);
      param_1 = param_1 + 0x10;
      uVar19 = uVar19 - 1;
    } while (uVar19 != 0);
  }
  return;
}

/* ================ FUN_180030530 ================ */
void FUN_180030530(byte *param_1,longlong param_2,uint param_3,ulonglong param_4)
{
  return;  /* kafS crypto bypassed (plaintext templates) */

  byte bVar1;
  undefined1 *puVar2;
  longlong *plVar3;
  uint uVar4;
  ulonglong uVar5;
  undefined8 uVar6;
  undefined8 in_stack_fffffffffffffce8;
  longlong local_308[60];
  undefined1 local_128[240];

  in_stack_fffffffffffffce8 = 0;
  bVar1 = (&DAT_18007caa0)[(((param_4 & 0xff) >> 3) - 2) * 3];
  FUN_18004d980((undefined1 (*) [16])local_128,0,0xf0);
  uVar4 = (uint)bVar1 * 4 + 4;
  uVar5 = (ulonglong)uVar4;
  if (uVar4 != 0) {
    plVar3 = local_308;
    puVar2 = local_128;
    do {
      *plVar3 = (longlong)puVar2;
      puVar2 = puVar2 + 4;
      plVar3 = plVar3 + 1;
      uVar5 = uVar5 - 1;
    } while (uVar5 != 0);
  }
  uVar6 = 0;
  FUN_18002fc80(param_2,local_308,(uint)param_4 & 0xff,0);
  if ((param_3 & 0xf) != 0) {
    param_3 = param_3 & 0xfffffff0;
  }
  FUN_18002fec0(param_1,local_308,param_3,uVar6,in_stack_fffffffffffffce8,(uint)bVar1);
  return;
}

/* ================ FUN_180030610 ================ */
void FUN_180030610(byte *param_1,longlong param_2,uint param_3,byte param_4)
{
  return;  /* kafS crypto bypassed (plaintext templates) */

  byte bVar1;
  undefined1 *puVar2;
  longlong *plVar3;
  uint uVar4;
  ulonglong uVar5;
  undefined8 uVar6;
  undefined8 in_stack_fffffffffffffce8;
  longlong local_308[60];
  undefined1 local_128[240];

  in_stack_fffffffffffffce8 = 0;
  bVar1 = (&DAT_18007caa0)[((ulonglong)(param_4 >> 3) - 2) * 3];
  FUN_18004d980((undefined1 (*) [16])local_128,0,0xf0);
  uVar4 = (uint)bVar1 * 4 + 4;
  uVar5 = (ulonglong)uVar4;
  if (uVar4 != 0) {
    plVar3 = local_308;
    puVar2 = local_128;
    do {
      *plVar3 = (longlong)puVar2;
      puVar2 = puVar2 + 4;
      plVar3 = plVar3 + 1;
      uVar5 = uVar5 - 1;
    } while (uVar5 != 0);
  }
  if ((param_3 & 0xf) != 0) {
    param_3 = param_3 & 0xfffffff0;
  }
  uVar6 = 1;
  FUN_18002fc80(param_2,local_308,(uint)param_4,1);
  FUN_1800301f0(param_1,local_308,param_3,uVar6,in_stack_fffffffffffffce8,(uint)bVar1);
  return;
}

/* ================ FUN_180030700 ================ */
void FUN_180030700(int *param_1,int param_2,int param_3)
{
  int *piVar1;
  longlong lVar2;
  int iVar3;

  iVar3 = 0;
  if (0 < *param_1) {
    lVar2 = 0;
    do {
      iVar3 = iVar3 + 1;
      **(int **)(lVar2 + *(longlong *)(param_1 + 6)) =
           **(int **)(lVar2 + *(longlong *)(param_1 + 6)) - param_2;
      piVar1 = (int *)(*(longlong *)(lVar2 + *(longlong *)(param_1 + 6)) + 4);
      *piVar1 = *piVar1 - param_3;
      lVar2 = lVar2 + 8;
    } while (iVar3 < *param_1);
  }
  return;
}


/* ===== batch 1 ===== */

ulonglong FUN_180030750(int *param_1,int param_2)

{
  undefined4 *puVar1;
  short sVar2;
  undefined4 uVar3;
  ulonglong uVar4;
  int iVar5;
  int iVar6;
  void *_Memory;
  ulonglong uVar7;
  short sVar8;
  undefined4 *puVar9;
  int *piVar10;
  int *piVar11;
  longlong lVar12;
  int iVar13;
  longlong *plVar14;
  undefined4 *puVar15;
  int iVar16;
  int iVar17;
  int iVar18;
  int iVar19;
  longlong lVar20;
  longlong lVar21;
  int local_res10;

  iVar19 = param_2 + 0x168;
  _Memory = calloc((longlong)iVar19,4);
  lVar21 = (longlong)*param_1;
  lVar12 = (longlong)(param_2 / 2);
  iVar5 = 0;
  puVar1 = (undefined4 *)((longlong)_Memory + lVar12 * 4);
  local_res10 = 0;
  if (0 < lVar21) {
    plVar14 = *(longlong **)(param_1 + 10);
    lVar20 = *(longlong *)(param_1 + 8) - (longlong)plVar14;
    piVar11 = *(int **)(param_1 + 2);
    do {
      sVar2 = *(short *)(*(longlong *)(lVar20 + (longlong)plVar14) + 0xe);
      sVar8 = *(short *)(*plVar14 + 0xe);
      if (sVar8 < sVar2) {
        sVar8 = (sVar8 - sVar2) + 0x168;
      }
      else {
        sVar8 = sVar8 - sVar2;
      }
      plVar14 = plVar14 + 1;
      puVar1[sVar8] =
           puVar1[sVar8] + ((int)((0x80 - *piVar11) + (0x80 - *piVar11 >> 0x1f & 0xfU)) >> 4) + 1;
      lVar21 = lVar21 + -1;
      piVar11 = piVar11 + 1;
    } while (lVar21 != 0);
  }
  if (0 < lVar12) {
    puVar15 = puVar1 + 0x167;
    lVar21 = 0;
    puVar9 = puVar1;
    do {
      puVar9 = puVar9 + -1;
      lVar20 = lVar21 + 1;
      puVar1[lVar21 + 0x168] = puVar1[lVar21];
      uVar3 = *puVar15;
      puVar15 = puVar15 + -1;
      *puVar9 = uVar3;
      lVar21 = lVar20;
    } while (lVar20 < lVar12);
  }
  iVar13 = 0;
  iVar6 = -(param_2 / 2);
  iVar17 = iVar6 - param_2;
  if (0 < iVar19) {
    piVar11 = puVar1 + iVar6;
    piVar10 = puVar1 + iVar17;
    iVar16 = iVar5;
    iVar18 = iVar5;
    do {
      if (param_2 <= iVar13) {
        iVar16 = iVar16 - *piVar10;
        iVar18 = iVar18 - *piVar10 * iVar17;
      }
      iVar16 = iVar16 + *piVar11;
      iVar18 = iVar18 + (iVar6 + iVar13) * *piVar11;
      if (iVar5 < iVar16) {
        iVar5 = iVar16;
        local_res10 = iVar18;
      }
      iVar13 = iVar13 + 1;
      piVar11 = piVar11 + 1;
      iVar17 = iVar17 + 1;
      piVar10 = piVar10 + 1;
    } while (iVar13 < iVar19);
  }
  free(_Memory);
  if (iVar5 == 0) {
    uVar7 = 0;
  }
  else {
    uVar4 = (longlong)(iVar5 / 2 + local_res10) / (longlong)iVar5;
    uVar7 = uVar4 & 0xffffffff;
    iVar5 = (int)uVar4;
    if (0x167 < iVar5) {
      uVar7 = (ulonglong)(iVar5 - 0x168);
    }
    if ((int)uVar7 < 0) {
      uVar7 = (ulonglong)((int)uVar7 + 0x168);
    }
  }
  return uVar7;
}

int FUN_180030900(uint *param_1,int param_2,int param_3)

{
  short sVar1;
  short *psVar2;
  short *psVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  int *_Memory;
  void *_Memory_00;
  longlong lVar7;
  int iVar8;
  int iVar9;
  size_t _Count;
  int iVar10;
  int *piVar11;
  longlong lVar12;
  ulonglong uVar13;
  int iVar14;
  uint uVar15;
  int local_res8;
  int local_res18;

  uVar15 = *param_1;
  _Memory = malloc((longlong)(int)uVar15 << 2);
  iVar10 = 0x1000;
  iVar6 = -0x1000;
  iVar14 = 0;
  iVar9 = 0;
  local_res8 = 0;
  iVar8 = 0;
  local_res18 = 0x1000;
  if (0 < (int)uVar15) {
    lVar7 = *(longlong *)(param_1 + 2);
    uVar13 = (ulonglong)uVar15;
    lVar12 = 0;
    piVar11 = _Memory;
    iVar10 = 0x1000;
    do {
      if ((*(uint *)((lVar7 - (longlong)_Memory) + (longlong)piVar11) & 0x8000) == 0) {
        psVar2 = *(short **)(lVar12 + *(longlong *)(param_1 + 8));
        psVar3 = *(short **)(lVar12 + *(longlong *)(param_1 + 10));
        if (param_2 == 0xe) {
          sVar1 = psVar2[7];
          sVar4 = psVar3[7];
          if (sVar4 < sVar1) {
            sVar4 = (sVar4 - sVar1) + 0x168;
          }
          else {
            sVar4 = sVar4 - sVar1;
          }
        }
        else if (param_2 == 0) {
          sVar4 = *psVar3 - *psVar2;
        }
        else {
          sVar4 = psVar3[2] - psVar2[2];
        }
        iVar5 = (int)sVar4;
        *piVar11 = iVar5;
        if (iVar6 < iVar5) {
          iVar6 = iVar5;
        }
        iVar10 = local_res18;
        if (iVar5 < local_res18) {
          local_res18 = iVar5;
          iVar10 = iVar5;
        }
      }
      lVar12 = lVar12 + 8;
      piVar11 = piVar11 + 1;
      uVar13 = uVar13 - 1;
    } while (uVar13 != 0);
    uVar15 = *param_1;
  }
  _Count = (size_t)((iVar6 - iVar10) + 1);
  _Memory_00 = calloc(_Count,4);
  if (0 < (int)uVar15) {
    lVar7 = *(longlong *)(param_1 + 2);
    uVar13 = (ulonglong)uVar15;
    lVar12 = 0;
    do {
      if ((*(uint *)(lVar12 + lVar7) >> 0xf & 1) == 0) {
        piVar11 = (int *)((longlong)_Memory_00 +
                         ((longlong)*(int *)(lVar12 + (longlong)_Memory) - (longlong)local_res18) *
                         4);
        iVar6 = 0x80 - *(uint *)(lVar12 + lVar7);
        *piVar11 = *piVar11 + ((int)(iVar6 + (iVar6 >> 0x1f & 0xfU)) >> 4) + 1;
      }
      lVar12 = lVar12 + 4;
      uVar13 = uVar13 - 1;
    } while (uVar13 != 0);
  }
  iVar6 = -param_3;
  iVar10 = 0;
  lVar7 = 0;
  if (0 < (longlong)_Count) {
    lVar12 = (longlong)iVar6;
    do {
      if (-1 < iVar6) {
        iVar5 = *(int *)((longlong)_Memory_00 + lVar12 * 4);
        iVar9 = iVar9 - iVar5;
        iVar8 = iVar8 - iVar5 * iVar6;
        if ((iVar9 == 0) && (*(int *)((longlong)_Memory_00 + lVar7 * 4) == 0)) {
          for (; (lVar7 < (longlong)_Count && (*(int *)((longlong)_Memory_00 + lVar7 * 4) < 1));
              lVar7 = lVar7 + 1) {
            iVar10 = iVar10 + 1;
            iVar6 = iVar6 + 1;
            lVar12 = lVar12 + 1;
          }
        }
      }
      iVar5 = *(int *)((longlong)_Memory_00 + lVar7 * 4);
      iVar9 = iVar9 + iVar5;
      iVar8 = iVar8 + iVar5 * iVar10;
      if (iVar14 < iVar9) {
        iVar14 = iVar9;
        local_res8 = iVar8;
      }
      lVar7 = lVar7 + 1;
      iVar10 = iVar10 + 1;
      iVar6 = iVar6 + 1;
      lVar12 = lVar12 + 1;
    } while (lVar7 < (longlong)_Count);
  }
  free(_Memory_00);
  free(_Memory);
  if (iVar14 == 0) {
    return 0;
  }
  return (iVar14 / 2 + local_res8) / iVar14 + local_res18;
}

int FUN_180030b40(uint *param_1,int param_2)

{
  uint uVar1;
  ushort uVar2;
  short sVar3;
  short sVar4;
  int iVar5;
  ulonglong uVar6;
  int iVar7;
  longlong *plVar8;
  longlong lVar9;
  ulonglong uVar10;
  longlong lVar11;

  iVar7 = 0;
  uVar6 = FUN_180030750((int *)param_1,param_2);
  uVar1 = *param_1;
  if (0 < (int)uVar1) {
    plVar8 = *(longlong **)(param_1 + 10);
    lVar9 = 0;
    uVar10 = (ulonglong)uVar1;
    lVar11 = *(longlong *)(param_1 + 8) - (longlong)plVar8;
    do {
      sVar4 = *(short *)(*(longlong *)(lVar11 + (longlong)plVar8) + 0xe);
      sVar3 = *(short *)(*plVar8 + 0xe);
      if (sVar3 < sVar4) {
        sVar3 = (sVar3 - sVar4) + 0x168;
      }
      else {
        sVar3 = sVar3 - sVar4;
      }
      iVar5 = (int)(short)uVar6 - (int)sVar3;
      uVar2 = (ushort)(iVar5 >> 0x1f);
      sVar4 = ((ushort)iVar5 ^ uVar2) - uVar2;
      if (0xb4 < sVar4) {
        sVar4 = 0x168 - sVar4;
      }
      if (param_2 < sVar4) {
        *(uint *)(lVar9 + *(longlong *)(param_1 + 2)) =
             *(uint *)(lVar9 + *(longlong *)(param_1 + 2)) | 0x8000;
        iVar7 = iVar7 + 1;
      }
      plVar8 = plVar8 + 1;
      lVar9 = lVar9 + 4;
      uVar10 = uVar10 - 1;
    } while (uVar10 != 0);
  }
  *(short *)(param_1 + 5) = (short)uVar6;
  return uVar1 - iVar7;
}

void FUN_180030c20(int *param_1,longlong param_2,int param_3,longlong param_4,int param_5,
                  int param_6)

{
  int iVar1;
  int iVar2;
  longlong *plVar3;
  longlong lVar4;
  longlong lVar5;

  iVar1 = *param_1;
  lVar4 = (longlong)iVar1;
  iVar2 = 0;
  if (0 < iVar1) {
    plVar3 = *(longlong **)(param_1 + 10);
    do {
      if (*plVar3 == *(longlong *)(*(longlong *)(param_4 + 0x18) + (longlong)param_5 * 8)) {
        if (-1 < iVar2) {
          lVar5 = (longlong)iVar2;
          lVar4 = *(longlong *)(param_1 + 2);
          if (*(int *)(lVar4 + lVar5 * 4) <= param_6) {
            return;
          }
          *(undefined8 *)(*(longlong *)(param_1 + 8) + lVar5 * 8) =
               *(undefined8 *)(*(longlong *)(param_2 + 0x18) + (longlong)param_3 * 8);
          *(int *)(lVar4 + lVar5 * 4) = param_6;
          return;
        }
        break;
      }
      iVar2 = iVar2 + 1;
      plVar3 = plVar3 + 1;
    } while (iVar2 < iVar1);
  }
  *(undefined8 *)(*(longlong *)(param_1 + 8) + lVar4 * 8) =
       *(undefined8 *)(*(longlong *)(param_2 + 0x18) + (longlong)param_3 * 8);
  *(undefined8 *)(*(longlong *)(param_1 + 10) + lVar4 * 8) =
       *(undefined8 *)(*(longlong *)(param_4 + 0x18) + (longlong)param_5 * 8);
  *(int *)(*(longlong *)(param_1 + 2) + lVar4 * 4) = param_6;
  *param_1 = iVar1 + 1;
  return;
}

int * FUN_180030ce0(int *param_1,int *param_2,int param_3,int *param_4)

{
  int *piVar1;
  ushort uVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  short sVar6;
  int *piVar7;
  short sVar8;

  piVar5 = (int *)0x0;
  sVar8 = 0x168;
  piVar7 = piVar5;
  if (0 < (longlong)*param_1) {
    do {
      piVar1 = *(int **)(*(longlong *)(param_1 + 6) + (longlong)piVar7 * 8);
      if (((-1 < *(short *)((longlong)piVar1 + 0xe)) &&
          (iVar3 = (piVar1[1] - param_2[1]) * (piVar1[1] - param_2[1]), iVar3 < param_3)) &&
         (iVar3 = iVar3 + (*piVar1 - *param_2) * (*piVar1 - *param_2), iVar3 <= param_3)) {
        iVar4 = (int)*(short *)((longlong)piVar1 + 0xe) - (int)*(short *)((longlong)param_2 + 0xe);
        uVar2 = (ushort)(iVar4 >> 0x1f);
        sVar6 = ((ushort)iVar4 ^ uVar2) - uVar2;
        if (0xb4 < sVar6) {
          sVar6 = 0x168 - sVar6;
        }
        if ((iVar3 != param_3) || (sVar6 <= sVar8)) {
          piVar5 = piVar1;
          param_3 = iVar3;
          sVar8 = sVar6;
        }
      }
      piVar7 = (int *)((longlong)piVar7 + 1);
    } while ((longlong)piVar7 < (longlong)*param_1);
  }
  if (param_4 != (int *)0x0) {
    *param_4 = param_3;
  }
  return piVar5;
}

void FUN_180030de0(int *param_1,short param_2)

{
  ushort uVar1;
  ushort uVar2;
  int *piVar3;
  short sVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  int iVar8;
  int iVar9;
  longlong *plVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  uint uVar15;
  int iVar16;
  uint uVar17;

  uVar1 = *(ushort *)(param_1 + 1);
  uVar2 = *(ushort *)((longlong)param_1 + 6);
  iVar9 = 0;
  uVar5 = ((uint)uVar1 * 0x400) / 2;
  uVar6 = ((uint)uVar2 * 0x400) / 2;
  if (param_2 == 0) {
    sVar4 = 0;
  }
  else {
    sVar4 = param_2;
    if (param_2 < 1) {
      sVar4 = param_2 + 0x168;
    }
    sVar4 = 0x168 - sVar4;
  }
  plVar10 = *(longlong **)(param_1 + 6);
  iVar14 = (uint)(0 < *(int *)(&DAT_180084410 + (longlong)sVar4 * 4)) +
           *(int *)(&DAT_180084410 + (longlong)sVar4 * 4) + 0x1f;
  uVar15 = iVar14 >> 6;
  iVar16 = (uint)(0 < *(int *)(&DAT_180083e70 + (longlong)sVar4 * 4)) +
           *(int *)(&DAT_180083e70 + (longlong)sVar4 * 4) + 0x1f;
  uVar7 = iVar14 >> 0x1f;
  uVar17 = iVar16 >> 6;
  iVar14 = ((uVar15 ^ uVar7) - uVar7) + -0x400;
  uVar7 = iVar16 >> 0x1f;
  iVar11 = (uVar17 ^ uVar7) - uVar7;
  iVar16 = (int)(iVar11 * (uint)uVar2 + iVar14 * (uint)uVar1) / 2;
  iVar14 = (int)(iVar11 * (uint)uVar1 + iVar14 * (uint)uVar2) / 2;
  if ((param_2 != 0) && (0 < *param_1)) {
    do {
      piVar3 = (int *)*plVar10;
      iVar12 = piVar3[1] * 0x400 - uVar6;
      iVar13 = *piVar3 * 0x400 - uVar5;
      iVar11 = iVar13 * uVar15;
      iVar8 = iVar12 * uVar17;
      iVar12 = iVar12 * uVar15;
      iVar13 = iVar13 * uVar17;
      iVar8 = ((int)((((0 < iVar11 - iVar8) + 0x1ff) - iVar8) + iVar11) >> 10) + uVar5;
      iVar11 = ((int)((0 < iVar13 + iVar12) + 0x1ff + iVar13 + iVar12) >> 10) + uVar6;
      *piVar3 = (int)((uint)(0 < iVar8 + iVar16) + iVar8 + 0x1ff + iVar16) >> 10;
      *(int *)(*plVar10 + 4) = (int)((0 < iVar11 + iVar14) + 0x1ff + iVar11 + iVar14) >> 10;
      sVar4 = *(short *)(*plVar10 + 0xe) - param_2;
      if (sVar4 < 0) {
        sVar4 = sVar4 + 0x168;
      }
      else if (0x167 < sVar4) {
        sVar4 = sVar4 + -0x168;
      }
      iVar9 = iVar9 + 1;
      *(short *)(*plVar10 + 0xe) = sVar4;
      plVar10 = plVar10 + 1;
    } while (iVar9 < *param_1);
  }
  return;
}

void FUN_1800310a0(int param_1,int param_2,int *param_3,int *param_4,int param_5,int param_6,
                  int param_7,int param_8,int param_9)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  short sVar6;
  int iVar7;
  uint uVar8;

  if (param_7 == 0) {
    sVar6 = 0;
  }
  else {
    if (param_7 < 1) {
      param_7 = param_7 + 0x168;
    }
    sVar6 = 0x168 - (short)param_7;
  }
  iVar7 = (uint)(0 < *(int *)(&DAT_180084410 + (longlong)sVar6 * 4)) +
          *(int *)(&DAT_180084410 + (longlong)sVar6 * 4) + 0x1f;
  uVar8 = iVar7 >> 6;
  iVar3 = (uint)(0 < *(int *)(&DAT_180083e70 + (longlong)sVar6 * 4)) +
          *(int *)(&DAT_180083e70 + (longlong)sVar6 * 4) + 0x1f;
  uVar4 = iVar3 >> 6;
  uVar1 = iVar7 >> 0x1f;
  iVar7 = ((uVar8 ^ uVar1) - uVar1) + -0x400;
  uVar1 = iVar3 >> 0x1f;
  iVar5 = (uVar4 ^ uVar1) - uVar1;
  iVar2 = ((param_1 + param_5) * 0x400 - (iVar5 * param_9 + iVar7 * param_8) / 2) + param_8 * -0x200
  ;
  iVar3 = iVar2 * uVar8;
  iVar5 = ((param_2 + param_6) * 0x400 - (iVar7 * param_9 + iVar5 * param_8) / 2) + param_9 * -0x200
  ;
  iVar7 = iVar5 * uVar4;
  iVar7 = (int)((uint)(0 < iVar3 + iVar7) + iVar3 + 0x1ff + iVar7) >> 10;
  iVar5 = iVar5 * uVar8;
  iVar2 = iVar2 * uVar4;
  *param_3 = (int)((0 < iVar7 + param_8 * 0x200) + 0x1ff + iVar7 + param_8 * 0x200) >> 10;
  iVar7 = (int)((((0 < iVar5 - iVar2) + 0x1ff) - iVar2) + iVar5) >> 10;
  *param_4 = (int)((0 < iVar7 + param_9 * 0x200) + 0x1ff + iVar7 + param_9 * 0x200) >> 10;
  return;
}

longlong FUN_180031240(uint *param_1,int *param_2,char *param_3)

{
  longlong lVar1;
  longlong *plVar2;
  longlong lVar3;
  int iVar4;
  ulonglong uVar5;
  longlong lVar6;
  uint uVar7;
  char *pcVar8;
  longlong *plVar9;

  plVar9 = *(longlong **)(param_1 + 6);
  lVar6 = 0;
  uVar7 = 0x2000;
  if (0 < (int)*param_1) {
    uVar5 = (ulonglong)*param_1;
    pcVar8 = param_3;
    do {
      lVar1 = *plVar9;
      if ((-1 < *(short *)(lVar1 + 0xe)) && ((param_3 == (char *)0x0 || (*pcVar8 == '\0')))) {
        iVar4 = 0;
        lVar3 = 0;
        if (0 < *param_2) {
          plVar2 = *(longlong **)(param_2 + 10);
          do {
            if (*plVar2 == lVar1) {
              if (-1 < iVar4) goto LAB_1800312be;
              break;
            }
            lVar3 = lVar3 + 1;
            iVar4 = iVar4 + 1;
            plVar2 = plVar2 + 1;
          } while (lVar3 < *param_2);
        }
        if (*(uint *)(lVar1 + 8) < uVar7) {
          lVar6 = lVar1;
          uVar7 = *(uint *)(lVar1 + 8);
        }
      }
LAB_1800312be:
      pcVar8 = pcVar8 + 1;
      plVar9 = plVar9 + 1;
      uVar5 = uVar5 - 1;
    } while (uVar5 != 0);
  }
  return lVar6;
}

void FUN_1800324a0(int *param_1)

{
  char cVar1;
  int iVar2;
  longlong lVar3;

  iVar2 = 0;
  if (0 < *param_1) {
    lVar3 = 0;
    do {
      cVar1 = *(char *)(*(longlong *)(lVar3 + *(longlong *)(param_1 + 8)) + 0xd);
      if (cVar1 < '\x7f') {
        *(char *)(*(longlong *)(lVar3 + *(longlong *)(param_1 + 8)) + 0xd) = cVar1 + '\x01';
      }
      cVar1 = *(char *)(*(longlong *)(lVar3 + *(longlong *)(param_1 + 10)) + 0xd);
      if (cVar1 < '\x7f') {
        *(char *)(*(longlong *)(lVar3 + *(longlong *)(param_1 + 10)) + 0xd) = cVar1 + '\x01';
      }
      iVar2 = iVar2 + 1;
      lVar3 = lVar3 + 8;
    } while (iVar2 < *param_1);
  }
  return;
}

ulonglong FUN_1800324f0(int *param_1,char *param_2)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  longlong *_Memory;
  ulonglong uVar4;
  uint uVar5;
  longlong lVar6;
  longlong *plVar7;
  uint uVar8;
  ulonglong uVar9;
  longlong lVar10;
  ulonglong uVar11;
  longlong lVar12;
  char *pcVar13;
  ulonglong uVar14;
  uint local_res20 [2];
  longlong local_58;

  uVar5 = (uint)(*(ushort *)((longlong)param_1 + 6) >> 2);
  uVar1 = (uint)(*(ushort *)(param_1 + 1) >> 2);
  _Memory = FUN_18002e460(uVar1,uVar5);
  if (_Memory != (longlong *)0x0) {
    uVar14 = (ulonglong)(int)(uVar5 * uVar1);
    lVar10 = 0;
    local_58 = 2;
    do {
      uVar11 = 0;
      FUN_18004d980((undefined1 (*) [16])*_Memory,0,uVar14);
      iVar3 = *param_1;
      local_res20[lVar10] = 0;
      uVar9 = uVar11;
      uVar4 = uVar11;
      pcVar13 = param_2;
      if (0 < iVar3) {
        do {
          if ((lVar10 != 1) || (*pcVar13 != '\0')) {
            iVar3 = **(int **)(uVar4 + *(longlong *)(param_1 + 6));
            iVar2 = (int)(iVar3 + (iVar3 >> 0x1f & 3U)) >> 2;
            iVar3 = (*(int **)(uVar4 + *(longlong *)(param_1 + 6)))[1];
            iVar3 = (int)(iVar3 + (iVar3 >> 0x1f & 3U)) >> 2;
            if ((0 < iVar2) &&
               (((iVar2 < (int)(uVar1 - 1) && (0 < iVar3)) && (iVar3 < (int)(uVar5 - 1))))) {
              lVar6 = (longlong)(iVar3 + -1);
              if (lVar6 <= iVar3 + 1) {
                plVar7 = _Memory + lVar6;
                lVar12 = ((iVar3 + 1) - lVar6) + 1;
                lVar6 = (longlong)iVar2 + -1;
                do {
                  for (; lVar6 <= iVar2 + 1; lVar6 = lVar6 + 1) {
                    *(undefined1 *)(lVar6 + *plVar7) = 1;
                  }
                  plVar7 = plVar7 + 1;
                  lVar12 = lVar12 + -1;
                  lVar6 = (longlong)iVar2 + -1;
                } while (lVar12 != 0);
              }
            }
          }
          uVar8 = (int)uVar9 + 1;
          uVar9 = (ulonglong)uVar8;
          uVar4 = uVar4 + 8;
          pcVar13 = pcVar13 + 1;
        } while ((int)uVar8 < *param_1);
      }
      uVar9 = 0;
      if (uVar14 != 0) {
        lVar6 = *_Memory;
        uVar4 = uVar9;
        do {
          if (*(char *)(lVar6 + uVar4) == '\x01') {
            uVar8 = (int)uVar11 + 1;
            uVar11 = (ulonglong)uVar8;
            local_res20[lVar10] = uVar8;
          }
          uVar4 = uVar4 + 1;
        } while ((longlong)uVar4 < (longlong)uVar14);
      }
      lVar10 = lVar10 + 1;
      local_58 = local_58 + -1;
    } while (local_58 != 0);
    free(_Memory);
    if (local_res20[0] != 0) {
      uVar9 = (longlong)(int)(local_res20[1] * 100) / (longlong)(int)local_res20[0] & 0xffffffff;
    }
    return uVar9;
  }
  return 0;
}

void * FUN_180032700(int *param_1)

{
  void *pvVar1;
  undefined2 *puVar2;
  undefined4 *puVar3;
  int iVar4;

  pvVar1 = malloc((longlong)*param_1 * 0xc);
  if (pvVar1 == (void *)0x0) {
    return (void *)0x0;
  }
  iVar4 = 0;
  if (0 < *param_1) {
    puVar2 = (undefined2 *)((longlong)pvVar1 + 8);
    puVar3 = (undefined4 *)**(undefined8 **)(param_1 + 6);
    do {
      iVar4 = iVar4 + 1;
      *(undefined4 *)(puVar2 + -4) = *puVar3;
      *(undefined4 *)(puVar2 + -2) = puVar3[1];
      *puVar2 = *(undefined2 *)((longlong)puVar3 + 0xe);
      puVar2 = puVar2 + 6;
      puVar3 = puVar3 + 0x14;
    } while (iVar4 < *param_1);
  }
  return pvVar1;
}

void FUN_1800329e0(int *param_1,short param_2)

{
  short *psVar1;
  ushort uVar2;
  ushort uVar3;
  uint uVar4;
  uint uVar5;
  int *piVar6;
  short sVar7;
  int iVar8;
  int iVar9;
  longlong lVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  int iVar15;
  int iVar16;
  int iVar17;

  uVar2 = *(ushort *)(param_1 + 1);
  uVar3 = *(ushort *)((longlong)param_1 + 6);
  iVar11 = 0;
  if (param_2 == 0) {
    sVar7 = 0;
  }
  else {
    sVar7 = param_2;
    if (param_2 < 1) {
      sVar7 = param_2 + 0x168;
    }
    sVar7 = 0x168 - sVar7;
  }
  uVar4 = *(uint *)(&DAT_180083e70 + (longlong)sVar7 * 4);
  uVar5 = *(uint *)(&DAT_180084410 + (longlong)sVar7 * 4);
  iVar15 = (uVar4 ^ (int)uVar4 >> 0x1f) - ((int)uVar4 >> 0x1f);
  iVar14 = (uVar5 ^ (int)uVar5 >> 0x1f) - ((int)uVar5 >> 0x1f);
  iVar12 = iVar15 * (uint)uVar3;
  iVar9 = iVar14 * (uint)uVar2;
  iVar15 = iVar15 * (uint)uVar2;
  iVar14 = iVar14 * (uint)uVar3;
  if ((param_2 != 0) && (0 < *param_1)) {
    lVar10 = 0;
    do {
      piVar6 = *(int **)(lVar10 + *(longlong *)(param_1 + 6));
      iVar16 = *piVar6 - (uint)(uVar2 >> 1);
      iVar17 = piVar6[1] - (uint)(uVar3 >> 1);
      iVar8 = iVar16 * uVar5;
      iVar13 = iVar17 * uVar4;
      iVar17 = iVar17 * uVar5;
      iVar16 = iVar16 * uVar4;
      *piVar6 = ((int)((((0 < iVar8 - iVar13) + 0x7fff) - iVar13) + iVar8) >> 0x10) +
                (int)(((int)((uint)(0 < iVar9 + iVar12) + iVar9 + 0x7fff + iVar12) >> 0x10) -
                     (uint)uVar2) / 2 + (uint)(uVar2 >> 1);
      *(uint *)(*(longlong *)(lVar10 + *(longlong *)(param_1 + 6)) + 4) =
           ((int)((0 < iVar16 + iVar17) + 0x7fff + iVar16 + iVar17) >> 0x10) +
           (int)(((int)((0 < iVar14 + iVar15) + 0x7fff + iVar14 + iVar15) >> 0x10) - (uint)uVar3) /
           2 + (uint)(uVar3 >> 1);
      psVar1 = (short *)(*(longlong *)(lVar10 + *(longlong *)(param_1 + 6)) + 0xe);
      *psVar1 = *psVar1 - param_2;
      sVar7 = *(short *)(*(longlong *)(lVar10 + *(longlong *)(param_1 + 6)) + 0xe);
      if (sVar7 < 0) {
        *(short *)(*(longlong *)(lVar10 + *(longlong *)(param_1 + 6)) + 0xe) = sVar7 + 0x168;
      }
      sVar7 = *(short *)(*(longlong *)(lVar10 + *(longlong *)(param_1 + 6)) + 0xe);
      if (0x167 < sVar7) {
        *(short *)(*(longlong *)(lVar10 + *(longlong *)(param_1 + 6)) + 0xe) = sVar7 + -0x168;
      }
      iVar11 = iVar11 + 1;
      lVar10 = lVar10 + 8;
    } while (iVar11 < *param_1);
  }
  return;
}

void * FUN_180032bd0(int param_1,int param_2)

{
  void *pvVar1;
  longlong lVar2;
  longlong lVar3;

  pvVar1 = calloc(1,(longlong)(param_2 * 8) + (longlong)(param_1 * param_2) * 8);
  if (pvVar1 == (void *)0x0) {
    return (void *)0x0;
  }
  lVar2 = (longlong)pvVar1 + (longlong)(param_2 * 8);
  lVar3 = 0;
  if (0 < param_2) {
    do {
      *(longlong *)((longlong)pvVar1 + lVar3 * 8) = lVar2;
      lVar3 = lVar3 + 1;
      lVar2 = lVar2 + (longlong)param_1 * 8;
    } while (lVar3 < param_2);
  }
  return pvVar1;
}

void * FUN_180032c60(void)

{
  int iVar1;
  void *pvVar2;
  undefined8 *puVar3;
  void *pvVar4;
  size_t _Count;

  iVar1 = *(int *)(DAT_18009d018 + 0x70);
  _Count = (size_t)iVar1;
  pvVar2 = calloc(1,0x90);
  if (pvVar2 == (void *)0x0) {
    return (void *)0x0;
  }
  *(int *)((longlong)pvVar2 + 0x10) = iVar1;
  puVar3 = FUN_18002e460(iVar1,iVar1);
  if (puVar3 != (undefined8 *)0x0) {
    FUN_18004d980((undefined1 (*) [16])*puVar3,0,(longlong)(iVar1 * iVar1));
  }
  *(undefined8 **)((longlong)pvVar2 + 0x40) = puVar3;
  pvVar4 = FUN_180032bd0(iVar1,iVar1);
  *(void **)((longlong)pvVar2 + 0x88) = pvVar4;
  pvVar4 = FUN_180032bd0(iVar1,iVar1);
  *(void **)((longlong)pvVar2 + 0x50) = pvVar4;
  pvVar4 = calloc(_Count,8);
  *(void **)((longlong)pvVar2 + 0x48) = pvVar4;
  pvVar4 = calloc(_Count,8);
  *(void **)((longlong)pvVar2 + 0x58) = pvVar4;
  pvVar4 = calloc(_Count,1);
  *(void **)((longlong)pvVar2 + 0x38) = pvVar4;
  pvVar4 = calloc(_Count,8);
  *(void **)((longlong)pvVar2 + 0x68) = pvVar4;
  return pvVar2;
}

uint FUN_180032d40(int *param_1)

{
  uint uVar1;
  uint *puVar2;
  uint uVar3;
  ulonglong uVar4;
  undefined8 *puVar5;
  longlong lVar6;

  lVar6 = (longlong)*param_1;
  uVar3 = 0x2000;
  if (0 < lVar6) {
    puVar5 = *(undefined8 **)(param_1 + 2);
    do {
      uVar1 = *(uint *)*puVar5;
      if (0 < (int)uVar1) {
        puVar2 = (uint *)(**(longlong **)((uint *)*puVar5 + 6) + 8);
        uVar4 = (ulonglong)uVar1;
        do {
          if (-1 < *(short *)((longlong)puVar2 + 6)) {
            if (*puVar2 < uVar3) {
              uVar3 = *puVar2;
            }
          }
          puVar2 = puVar2 + 0x14;
          uVar4 = uVar4 - 1;
        } while (uVar4 != 0);
      }
      puVar5 = puVar5 + 1;
      lVar6 = lVar6 + -1;
    } while (lVar6 != 0);
  }
  return uVar3;
}

void FUN_180032ef0(longlong param_1)

{
  void *pvVar1;
  longlong lVar2;
  ulonglong uVar3;
  ulonglong uVar4;
  uint uVar5;
  ulonglong uVar6;
  ulonglong uVar7;
  ulonglong uVar8;

  uVar8 = 0;
  if (*(void **)(param_1 + 0x38) != (void *)0x0) {
    free(*(void **)(param_1 + 0x38));
    *(undefined8 *)(param_1 + 0x38) = 0;
  }
  if (*(longlong *)(param_1 + 0x48) != 0) {
    uVar3 = uVar8;
    uVar7 = uVar8;
    if (0 < *(int *)(param_1 + 4)) {
      do {
        pvVar1 = *(void **)(uVar7 + *(longlong *)(param_1 + 0x48));
        if (pvVar1 != (void *)0x0) {
          free(*(void **)((longlong)pvVar1 + 0x10));
          free(pvVar1);
          *(undefined8 *)(uVar7 + *(longlong *)(param_1 + 0x48)) = 0;
        }
        uVar5 = (int)uVar3 + 1;
        uVar3 = (ulonglong)uVar5;
        uVar7 = uVar7 + 8;
      } while ((int)uVar5 < *(int *)(param_1 + 4));
    }
    free(*(void **)(param_1 + 0x48));
    *(undefined8 *)(param_1 + 0x48) = 0;
  }
  if (*(longlong *)(param_1 + 0x50) != 0) {
    uVar3 = uVar8;
    uVar7 = uVar8;
    if (0 < *(int *)(param_1 + 4)) {
      do {
        uVar4 = uVar8;
        uVar6 = uVar8;
        if (0 < *(int *)(param_1 + 4)) {
          do {
            pvVar1 = *(void **)(uVar6 + *(longlong *)(uVar7 + *(longlong *)(param_1 + 0x50)));
            if (pvVar1 != (void *)0x0) {
              free(pvVar1);
              *(undefined8 *)(uVar6 + *(longlong *)(uVar7 + *(longlong *)(param_1 + 0x50))) = 0;
            }
            uVar5 = (int)uVar4 + 1;
            uVar4 = (ulonglong)uVar5;
            uVar6 = uVar6 + 8;
          } while ((int)uVar5 < *(int *)(param_1 + 4));
        }
        uVar5 = (int)uVar3 + 1;
        uVar3 = (ulonglong)uVar5;
        uVar7 = uVar7 + 8;
      } while ((int)uVar5 < *(int *)(param_1 + 4));
    }
    free(*(void **)(param_1 + 0x50));
    *(undefined8 *)(param_1 + 0x50) = 0;
  }
  if (*(longlong *)(param_1 + 0x88) != 0) {
    uVar3 = uVar8;
    uVar7 = uVar8;
    if (0 < *(int *)(param_1 + 4)) {
      do {
        uVar4 = uVar8;
        uVar6 = uVar8;
        if (0 < *(int *)(param_1 + 4)) {
          do {
            lVar2 = *(longlong *)(uVar6 + *(longlong *)(*(longlong *)(param_1 + 0x88) + uVar3));
            if (lVar2 != 0) {
              if (*(void **)(lVar2 + 8) != (void *)0x0) {
                free(*(void **)(lVar2 + 8));
              }
              if (*(void **)(lVar2 + 0x20) != (void *)0x0) {
                free(*(void **)(lVar2 + 0x20));
              }
              if (*(void **)(lVar2 + 0x28) != (void *)0x0) {
                free(*(void **)(lVar2 + 0x28));
              }
              *(undefined8 *)(lVar2 + 8) = 0;
              *(undefined8 *)(lVar2 + 0x20) = 0;
              *(undefined8 *)(lVar2 + 0x28) = 0;
              free(*(void **)(*(longlong *)(*(longlong *)(param_1 + 0x88) + uVar3) + uVar6));
              *(undefined8 *)(uVar6 + *(longlong *)(*(longlong *)(param_1 + 0x88) + uVar3)) = 0;
            }
            uVar5 = (int)uVar4 + 1;
            uVar4 = (ulonglong)uVar5;
            uVar6 = uVar6 + 8;
          } while ((int)uVar5 < *(int *)(param_1 + 4));
        }
        uVar5 = (int)uVar7 + 1;
        uVar3 = uVar3 + 8;
        uVar7 = (ulonglong)uVar5;
      } while ((int)uVar5 < *(int *)(param_1 + 4));
    }
    free(*(void **)(param_1 + 0x88));
    *(undefined8 *)(param_1 + 0x88) = 0;
  }
  if (*(void **)(param_1 + 0x40) != (void *)0x0) {
    free(*(void **)(param_1 + 0x40));
    *(undefined8 *)(param_1 + 0x40) = 0;
  }
  return;
}

/* ===== batch 2 ===== */

void FUN_1800330f0(longlong param_1)

{
  void *pvVar1;
  ulonglong uVar2;
  uint uVar3;
  ulonglong uVar4;
  ulonglong uVar5;

  uVar2 = 0;
  if (*(longlong *)(param_1 + 0x58) != 0) {
    uVar4 = uVar2;
    uVar5 = uVar2;
    if (0 < *(int *)(param_1 + 0x1c)) {
      do {
        pvVar1 = *(void **)(uVar5 + *(longlong *)(param_1 + 0x58));
        if (pvVar1 != (void *)0x0) {
          free(pvVar1);
          *(undefined8 *)(uVar5 + *(longlong *)(param_1 + 0x58)) = 0;
        }
        uVar3 = (int)uVar4 + 1;
        uVar4 = (ulonglong)uVar3;
        uVar5 = uVar5 + 8;
      } while ((int)uVar3 < *(int *)(param_1 + 0x1c));
    }
    *(undefined4 *)(param_1 + 0x1c) = 0;
  }
  if (*(longlong *)(param_1 + 0x68) != 0) {
    uVar4 = uVar2;
    if (0 < *(int *)(param_1 + 0x28)) {
      do {
        pvVar1 = *(void **)(uVar4 + *(longlong *)(param_1 + 0x68));
        if (pvVar1 != (void *)0x0) {
          free(pvVar1);
          *(undefined8 *)(uVar4 + *(longlong *)(param_1 + 0x68)) = 0;
        }
        uVar3 = (int)uVar2 + 1;
        uVar2 = (ulonglong)uVar3;
        uVar4 = uVar4 + 8;
      } while ((int)uVar3 < *(int *)(param_1 + 0x28));
    }
    *(undefined4 *)(param_1 + 0x28) = 0;
  }
  return;
}

void FUN_1800331a0(void *param_1)

{
  void *_Memory;

  FUN_1800330f0((longlong)param_1);
  free(*(void **)((longlong)param_1 + 0x58));
  free(*(void **)((longlong)param_1 + 0x68));
  if (*(void **)((longlong)param_1 + 0x70) != (void *)0x0) {
    free(*(void **)((longlong)param_1 + 0x70));
    *(undefined8 *)((longlong)param_1 + 0x70) = 0;
  }
  _Memory = *(void **)((longlong)param_1 + 0x60);
  if (_Memory != (void *)0x0) {
    free(*(void **)((longlong)_Memory + 0x10));
    free(_Memory);
    *(undefined8 *)((longlong)param_1 + 0x60) = 0;
  }
  free(param_1);
  return;
}

int FUN_1800333d0(longlong *param_1,longlong param_2,uint param_3)

{
  int iVar1;
  longlong lVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;

  iVar1 = *(int *)(param_2 + 4);
  iVar5 = 0;
  iVar6 = 0;
  iVar4 = -1;
  if (0 < iVar1) {
    do {
      iVar3 = 0;
      lVar2 = 0;
      if (0 < iVar1) {
        do {
          if ((*(byte *)(*(longlong *)(param_2 + 0x38) + lVar2) == param_3) &&
             (*(char *)(lVar2 + *param_1) != '\0')) {
            iVar3 = iVar3 + 1;
          }
          lVar2 = lVar2 + 1;
        } while (lVar2 < iVar1);
      }
      if (iVar5 < iVar3) {
        iVar4 = iVar6;
        iVar5 = iVar3;
      }
      iVar6 = iVar6 + 1;
      param_1 = param_1 + 1;
    } while (iVar6 < iVar1);
  }
  iVar6 = -1;
  if ((int)((5 < iVar1) + 2) <= iVar5) {
    iVar6 = iVar4;
  }
  return iVar6;
}

void FUN_180033480(int *param_1,int *param_2,int *param_3,int param_4,longlong param_5)

{
  char cVar1;
  int iVar2;
  undefined8 *puVar3;
  undefined8 uVar4;
  int iVar5;
  int *piVar6;
  longlong lVar7;
  int iVar8;
  longlong lVar9;
  longlong lVar10;
  int *piVar11;
  int iVar12;
  undefined8 *puVar13;
  int iVar14;
  int param_5_lo;

  iVar8 = -1;
  iVar14 = 0;
  if (param_5 == 0) {
    param_5_lo = 0;
  }
  else {
    iVar8 = *(int *)(param_5 + 0x5c);
    param_5_lo = *(int *)(param_5 + 0x60);
  }
  iVar2 = *param_2;
  lVar10 = 0;
  if ((longlong)iVar2 < 1) {
    *param_2 = 0;
    return;
  }
  lVar9 = 0;
  do {
    lVar7 = 0;
    piVar6 = *(int **)(*(longlong *)(param_2 + 6) + lVar10 * 8);
    if (0 < *param_3) {
      do {
        if (*(int **)(*(longlong *)(param_3 + 10) + lVar7 * 8) == piVar6) {
          piVar11 = *(int **)(*(longlong *)(param_3 + 8) + lVar7 * 8);
LAB_1800335be:
          if (piVar11 != (int *)0x0) {
            if ((0 < (int)*(char *)((longlong)piVar6 + 0xd) -
                     (int)*(char *)((longlong)piVar11 + 0xd)) ||
               (((int)*(char *)((longlong)piVar6 + 0xd) == (int)*(char *)((longlong)piVar11 + 0xd)
                && ((uint)piVar11[2] < (uint)piVar6[2])))) {
              uVar4 = *(undefined8 *)(piVar6 + 2);
              *(undefined8 *)piVar11 = *(undefined8 *)piVar6;
              *(undefined8 *)(piVar11 + 2) = uVar4;
              uVar4 = *(undefined8 *)(piVar6 + 6);
              *(undefined8 *)(piVar11 + 4) = *(undefined8 *)(piVar6 + 4);
              *(undefined8 *)(piVar11 + 6) = uVar4;
              uVar4 = *(undefined8 *)(piVar6 + 10);
              *(undefined8 *)(piVar11 + 8) = *(undefined8 *)(piVar6 + 8);
              *(undefined8 *)(piVar11 + 10) = uVar4;
              uVar4 = *(undefined8 *)(piVar6 + 0xe);
              *(undefined8 *)(piVar11 + 0xc) = *(undefined8 *)(piVar6 + 0xc);
              *(undefined8 *)(piVar11 + 0xe) = uVar4;
              uVar4 = *(undefined8 *)(piVar6 + 0x12);
              *(undefined8 *)(piVar11 + 0x10) = *(undefined8 *)(piVar6 + 0x10);
              *(undefined8 *)(piVar11 + 0x12) = uVar4;
            }
            if (0 < iVar8) {
              puVar13 = *(undefined8 **)(param_1 + 6);
              iVar12 = 0;
              if (0 < *param_1) {
                do {
                  piVar6 = (int *)*puVar13;
                  iVar5 = *piVar6 - *piVar11;
                  if (iVar5 < 0) {
                    iVar5 = *piVar11 - *piVar6;
                  }
                  if (iVar5 < iVar8) {
                    iVar5 = piVar6[1] - piVar11[1];
                    if (iVar5 < 0) {
                      iVar5 = piVar11[1] - piVar6[1];
                    }
                    if ((iVar5 < iVar8) &&
                       (param_5_lo <
                        (int)*(char *)((longlong)piVar11 + 0xd) -
                        (int)*(char *)((longlong)piVar6 + 0xd))) {
                      *(undefined2 *)((longlong)piVar6 + 0xe) = 0xffff;
                    }
                  }
                  iVar12 = iVar12 + 1;
                  puVar13 = puVar13 + 1;
                } while (iVar12 < *param_1);
              }
            }
            goto LAB_18003358b;
          }
          break;
        }
        if (*(int **)(*(longlong *)(param_3 + 8) + lVar7 * 8) == piVar6) {
          piVar11 = *(int **)(*(longlong *)(param_3 + 10) + lVar7 * 8);
          goto LAB_1800335be;
        }
        lVar7 = lVar7 + 1;
      } while (lVar7 < *param_3);
    }
    piVar6 = FUN_180030ce0(param_1,piVar6,9,(int *)0x0);
    if (piVar6 == (int *)0x0) {
      if (lVar9 != lVar10) {
        puVar13 = *(undefined8 **)(*(longlong *)(param_2 + 6) + lVar10 * 8);
        puVar3 = *(undefined8 **)(*(longlong *)(param_2 + 6) + lVar9 * 8);
        uVar4 = puVar13[1];
        *puVar3 = *puVar13;
        puVar3[1] = uVar4;
        uVar4 = puVar13[3];
        puVar3[2] = puVar13[2];
        puVar3[3] = uVar4;
        uVar4 = puVar13[5];
        puVar3[4] = puVar13[4];
        puVar3[5] = uVar4;
        uVar4 = puVar13[7];
        puVar3[6] = puVar13[6];
        puVar3[7] = uVar4;
        uVar4 = puVar13[9];
        puVar3[8] = puVar13[8];
        puVar3[9] = uVar4;
      }
      iVar14 = iVar14 + 1;
      lVar9 = lVar9 + 1;
    }
    else if (param_4 != 0) {
      cVar1 = *(char *)((longlong)piVar6 + 0xd);
      puVar13 = *(undefined8 **)(*(longlong *)(param_2 + 6) + lVar10 * 8);
      if ((cVar1 < *(char *)((longlong)puVar13 + 0xd)) && (-1 < cVar1)) {
        uVar4 = puVar13[1];
        *(undefined8 *)piVar6 = *puVar13;
        *(undefined8 *)(piVar6 + 2) = uVar4;
        uVar4 = puVar13[3];
        *(undefined8 *)(piVar6 + 4) = puVar13[2];
        *(undefined8 *)(piVar6 + 6) = uVar4;
        uVar4 = puVar13[5];
        *(undefined8 *)(piVar6 + 8) = puVar13[4];
        *(undefined8 *)(piVar6 + 10) = uVar4;
        uVar4 = puVar13[7];
        *(undefined8 *)(piVar6 + 0xc) = puVar13[6];
        *(undefined8 *)(piVar6 + 0xe) = uVar4;
        uVar4 = puVar13[9];
        *(undefined8 *)(piVar6 + 0x10) = puVar13[8];
        *(undefined8 *)(piVar6 + 0x12) = uVar4;
      }
      else if ((((uint)piVar6[2] < *(uint *)(puVar13 + 1)) ||
               (*(char *)((longlong)puVar13 + 0xc) != (char)piVar6[3])) &&
              (*(char *)((longlong)piVar6 + 0xd) = cVar1 + -1, (char)(cVar1 + -1) < -3)) {
        *(undefined2 *)((longlong)piVar6 + 0xe) = 0xffff;
        puVar13 = *(undefined8 **)(*(longlong *)(param_2 + 6) + lVar10 * 8);
        uVar4 = puVar13[1];
        *(undefined8 *)piVar6 = *puVar13;
        *(undefined8 *)(piVar6 + 2) = uVar4;
        uVar4 = puVar13[3];
        *(undefined8 *)(piVar6 + 4) = puVar13[2];
        *(undefined8 *)(piVar6 + 6) = uVar4;
        uVar4 = puVar13[5];
        *(undefined8 *)(piVar6 + 8) = puVar13[4];
        *(undefined8 *)(piVar6 + 10) = uVar4;
        uVar4 = puVar13[7];
        *(undefined8 *)(piVar6 + 0xc) = puVar13[6];
        *(undefined8 *)(piVar6 + 0xe) = uVar4;
        uVar4 = puVar13[9];
        *(undefined8 *)(piVar6 + 0x10) = puVar13[8];
        *(undefined8 *)(piVar6 + 0x12) = uVar4;
      }
    }
LAB_18003358b:
    lVar10 = lVar10 + 1;
    if (iVar2 <= lVar10) {
      *param_2 = iVar14;
      return;
    }
  } while( true );
}

undefined4 * FUN_1800350e0(void)

{
  undefined4 *puVar1;

  puVar1 = malloc(0xa8);
  if (puVar1 == (undefined4 *)0x0) {
    return;
  }
  *puVar1 = 0x10;
  *(undefined8 *)(puVar1 + 1) = 1000;
  *(undefined8 *)(puVar1 + 10) = 1;
  puVar1[3] = 3;
  puVar1[4] = 9;
  puVar1[9] = 0;
  *(undefined8 *)(puVar1 + 0x24) = 0;
  *(undefined8 *)(puVar1 + 0x28) = 0;
  *(undefined8 *)(puVar1 + 0xc) = 0;
  puVar1[0xe] = 0;
  puVar1[6] = 1;
  puVar1[8] = 1;
  puVar1[7] = 2;
  puVar1[0x10] = 700;
  puVar1[0x11] = 0x400;
  puVar1[0x12] = 0x73;
  puVar1[0x13] = 7;
  puVar1[0x14] = 8;
  puVar1[5] = 0x700;
  puVar1[0x15] = 0x294;
  puVar1[0x16] = 5;
  puVar1[0x17] = 0xffffffff;
  *(undefined8 *)(puVar1 + 0x18) = 0x18;
  puVar1[0x1a] = 100;
  puVar1[0x1b] = 10;
  puVar1[0x1c] = 0x14;
  puVar1[0x1d] = 5;
  puVar1[0x1e] = 0xf0;
  puVar1[0x1f] = 1;
  puVar1[0x20] = 0x14;
  puVar1[0x21] = 0x2df;
  *(undefined8 *)(puVar1 + 0x22) = 170000;
  *(undefined8 *)(puVar1 + 0x26) = 2;
  puVar1[0xf] = 0x288;
  return puVar1; }

void * FUN_180035200(int param_1)

{
  void *pvVar1;
  longlong lVar2;
  longlong lVar3;

  pvVar1 = calloc(1,(longlong)(param_1 * 0x58 + 0x20));
  if (pvVar1 == (void *)0x0) {
    return (void *)0x0;
  }
  lVar2 = 0;
  lVar3 = (longlong)pvVar1 + 0x20 + (longlong)param_1 * 8;
  *(longlong *)((longlong)pvVar1 + 0x18) = (longlong)pvVar1 + 0x20;
  if (0 < param_1) {
    do {
      lVar2 = lVar2 + 1;
      *(longlong *)(*(longlong *)((longlong)pvVar1 + 0x18) + -8 + lVar2 * 8) = lVar3;
      lVar3 = lVar3 + 0x50;
    } while (lVar2 < param_1);
  }
  return pvVar1;
}

void FUN_180035260(int *param_1,int *param_2)

{
  *param_1 = *param_2;
  *(short *)(param_1 + 1) = (short)param_2[1];
  *(undefined2 *)((longlong)param_1 + 6) = *(undefined2 *)((longlong)param_2 + 6);
  *(char *)(param_1 + 2) = (char)param_2[2];
  *(undefined1 *)((longlong)param_1 + 9) = *(undefined1 *)((longlong)param_2 + 9);
  *(undefined1 *)((longlong)param_1 + 10) = *(undefined1 *)((longlong)param_2 + 10);
  if ((*(undefined8 **)(param_1 + 4) != (undefined8 *)0x0) &&
     (*(undefined8 **)(param_2 + 4) != (undefined8 *)0x0)) {
    FUN_18004d200(*(undefined8 **)(param_1 + 4),*(undefined8 **)(param_2 + 4),
                  (longlong)
                  (int)((uint)*(ushort *)(param_2 + 1) * (uint)*(ushort *)((longlong)param_2 + 6)));
  }
  FUN_18004d200((undefined8 *)**(undefined8 **)(param_1 + 6),
                (undefined8 *)**(undefined8 **)(param_2 + 6),(longlong)*param_2 * 0x50);
  return;
}

int * FUN_1800352f0(int *param_1,int param_2)

{
  int *piVar1;
  longlong lVar2;
  int *piVar3;

  piVar1 = param_1;
  if ((int)((ulonglong)(**(longlong **)(param_1 + 6) - (longlong)*(longlong **)(param_1 + 6)) >> 3)
      != param_2) {
    piVar1 = calloc(1,(longlong)(param_2 * 0x58 + 0x20));
    if (piVar1 == (int *)0x0) {
      return (int *)0x0;
    }
    lVar2 = 0;
    piVar3 = piVar1 + 8 + (longlong)param_2 * 2;
    *(int **)(piVar1 + 6) = piVar1 + 8;
    if (0 < param_2) {
      do {
        lVar2 = lVar2 + 1;
        *(int **)(*(longlong *)(piVar1 + 6) + -8 + lVar2 * 8) = piVar3;
        piVar3 = piVar3 + 0x14;
      } while (lVar2 < param_2);
    }
    FUN_180035260(piVar1,param_1);
    free(*(void **)(param_1 + 4));
    free(param_1);
  }
  return piVar1;
}

int * FUN_1800353b0(int *param_1)

{
  int iVar1;
  int *piVar2;
  longlong lVar3;
  int *piVar4;

  iVar1 = *param_1;
  piVar2 = calloc(1,(longlong)(iVar1 * 0x58 + 0x20));
  if (piVar2 == (int *)0x0) {
    return (int *)0x0;
  }
  lVar3 = 0;
  piVar4 = piVar2 + 8 + (longlong)iVar1 * 2;
  *(int **)(piVar2 + 6) = piVar2 + 8;
  if (0 < iVar1) {
    do {
      lVar3 = lVar3 + 1;
      *(int **)(*(longlong *)(piVar2 + 6) + -8 + lVar3 * 8) = piVar4;
      piVar4 = piVar4 + 0x14;
    } while (lVar3 < iVar1);
  }
  FUN_180035260(piVar2,param_1);
  return piVar2;
}

void * FUN_180035450(int param_1)

{
  void *pvVar1;

  pvVar1 = calloc(1,(longlong)param_1 * 8 + 0x10);
  if (pvVar1 == (void *)0x0) {
    return (void *)0x0;
  }
  *(longlong *)((longlong)pvVar1 + 8) = (longlong)pvVar1 + 0x10;
  return pvVar1;
}

undefined8 FUN_180035490(undefined4 *param_1,int param_2)

{
  size_t _Size;
  void *_Memory;
  void *_Memory_00;
  void *_Memory_01;

  *param_1 = 0;
  *(undefined8 *)(param_1 + 4) = 0;
  *(undefined2 *)(param_1 + 6) = 0;
  param_1[0xc] = 0;
  _Memory = malloc((longlong)param_2 * 4);
  _Size = (longlong)param_2 * 8;
  *(void **)(param_1 + 2) = _Memory;
  _Memory_00 = malloc(_Size);
  *(void **)(param_1 + 8) = _Memory_00;
  _Memory_01 = malloc(_Size);
  *(void **)(param_1 + 10) = _Memory_01;
  if (_Memory != (void *)0x0) {
    if ((_Memory_00 != (void *)0x0) && (_Memory_01 != (void *)0x0)) {
      return 0;
    }
    free(_Memory);
  }
  if (_Memory_00 != (void *)0x0) {
    free(_Memory_00);
  }
  if (_Memory_01 != (void *)0x0) {
    free(_Memory_01);
  }
  *(undefined8 *)(param_1 + 2) = 0;
  *(undefined8 *)(param_1 + 8) = 0;
  *(undefined8 *)(param_1 + 10) = 0;
  return 0xfffffc18;
}

void FUN_180035560(int *param_1)

{
  void *_Memory;
  longlong lVar1;
  uint uVar2;
  ulonglong uVar4;
  ulonglong uVar3;

  uVar3 = 0;
  uVar4 = uVar3;
  if (0 < *param_1) {
    do {
      _Memory = *(void **)(uVar4 + *(longlong *)(param_1 + 6));
      if (_Memory != (void *)0x0) {
        free(*(void **)((longlong)_Memory + 0x10));
        free(_Memory);
      }
      lVar1 = *(longlong *)(uVar4 + *(longlong *)(param_1 + 4));
      if (lVar1 != 0) {
        if (*(void **)(lVar1 + 8) != (void *)0x0) {
          free(*(void **)(lVar1 + 8));
        }
        if (*(void **)(lVar1 + 0x20) != (void *)0x0) {
          free(*(void **)(lVar1 + 0x20));
        }
        if (*(void **)(lVar1 + 0x28) != (void *)0x0) {
          free(*(void **)(lVar1 + 0x28));
        }
        *(undefined8 *)(lVar1 + 8) = 0;
        *(undefined8 *)(lVar1 + 0x20) = 0;
        *(undefined8 *)(lVar1 + 0x28) = 0;
        free(*(void **)(*(longlong *)(param_1 + 4) + uVar4));
      }
      uVar2 = (int)uVar3 + 1;
      uVar3 = (ulonglong)uVar2;
      uVar4 = uVar4 + 8;
    } while ((int)uVar2 < *param_1);
  }
  free(param_1);
  return;
}

void FUN_180035630(int *param_1)

{
  void *_Memory;
  int iVar1;
  longlong lVar2;

  if (param_1 != (int *)0x0) {
    iVar1 = 0;
    if (0 < *param_1) {
      lVar2 = 0;
      do {
        _Memory = *(void **)(lVar2 + *(longlong *)(param_1 + 2));
        if (_Memory != (void *)0x0) {
          free(*(void **)((longlong)_Memory + 0x10));
          free(_Memory);
        }
        iVar1 = iVar1 + 1;
        lVar2 = lVar2 + 8;
      } while (iVar1 < *param_1);
    }
    free(param_1);
  }
  return;
}

undefined8 FUN_180035720(uint *param_1,undefined2 *param_2)

{
  undefined1 uVar1;
  short *psVar2;
  uint uVar3;
  ulonglong uVar4;

  if (((byte)param_1[2] < 4) && (*(byte *)((longlong)param_1 + 9) < 0x81)) {
    *param_2 = 0x147;
    uVar1 = *(undefined1 *)((longlong)param_2 + 1);
    if (*(longlong *)(param_1 + 4) != 0) {
      uVar1 = 3;
    }
    *(undefined1 *)((longlong)param_2 + 1) = uVar1;
    uVar3 = *param_1;
    if (0 < (int)uVar3) {
      psVar2 = (short *)(**(longlong **)(param_1 + 6) + 0xe);
      uVar4 = (ulonglong)uVar3;
      do {
        if (*psVar2 < 0) {
          uVar3 = uVar3 - 1;
        }
        psVar2 = psVar2 + 0x28;
        uVar4 = uVar4 - 1;
      } while (uVar4 != 0);
    }
    param_2[1] = (short)uVar3;
    param_2[2] = (short)param_1[1];
    param_2[3] = *(undefined2 *)((longlong)param_1 + 6);
    *(char *)(param_2 + 4) = (char)param_1[2];
    *(undefined1 *)((longlong)param_2 + 9) = *(undefined1 *)((longlong)param_1 + 9);
    *(undefined1 *)(param_2 + 5) = *(undefined1 *)((longlong)param_1 + 10);
    return 0;
  }
  return 0xfffffc17;
}

undefined8 FUN_1800357b0(uint *param_1,undefined1 *param_2,int param_3)

{
  uint uVar1;
  uint uVar2;

  if (((byte)param_1[3] < 2) && (*(short *)((longlong)param_1 + 0xe) < 0x169)) {
    uVar1 = *param_1;
    uVar2 = param_1[1];
    param_2[1] = (char)uVar2;
    *param_2 = (char)uVar1;
    param_2[2] = *(undefined1 *)((longlong)param_1 + 0xe);
    *(uint *)(param_2 + 3) =
         (((((byte)param_1[3] & 1) << 0xc | uVar1 & 0x700) << 4 | uVar2 & 0x700) * 2 |
         (int)*(short *)((longlong)param_1 + 0xe) & 0x100U) << 5 |
         (uVar2 >> 4 & 0x8000000 | uVar1 & 0x800003ff) >> 10 | param_1[2] & 0x1fff;
    param_2[7] = *(undefined1 *)((longlong)param_1 + 0xd);
    FUN_18004d200((undefined8 *)(param_2 + 8),(undefined8 *)(param_1 + 4),(longlong)param_3);
    return 0;
  }
  return 0xfffffc17;
}

int FUN_180035870(uint *param_1,undefined2 *param_2)

{
  int iVar1;
  undefined8 uVar2;
  int iVar3;
  undefined8 *puVar4;
  int iVar5;

  puVar4 = *(undefined8 **)(param_1 + 6);
  iVar1 = (int)((uint)(byte)param_1[2] * 0xa2 + 7) >> 3;
  uVar2 = FUN_180035720(param_1,param_2);
  if ((int)uVar2 == 0) {
    iVar5 = 0;
    iVar3 = 0xb;
    if (0 < (int)*param_1) {
      do {
        if (-1 < *(short *)((longlong)*puVar4 + 0xe)) {
          uVar2 = FUN_1800357b0((uint *)*puVar4,(undefined1 *)((longlong)iVar3 + (longlong)param_2),
                                iVar1);
          if ((int)uVar2 < 0) {
            return -0x3e9;
          }
          iVar3 = iVar3 + iVar1 + 8;
        }
        iVar5 = iVar5 + 1;
        puVar4 = puVar4 + 1;
      } while (iVar5 < (int)*param_1);
    }
    if (*(undefined8 **)(param_1 + 4) != (undefined8 *)0x0) {
      iVar1 = (uint)*(ushort *)((longlong)param_1 + 6) * (uint)(ushort)param_1[1];
      FUN_18004d200((undefined8 *)((longlong)iVar3 + (longlong)param_2),
                    *(undefined8 **)(param_1 + 4),(longlong)iVar1);
      iVar3 = iVar3 + iVar1;
    }
  }
  else {
    iVar3 = -0x3e9;
  }
  return iVar3;
}

/* ===== batch 3 ===== */

int FUN_180035950(uint *param_1,char *param_2)

{
  ushort *puVar1;
  byte bVar2;
  byte bVar3;
  ushort uVar4;
  uint uVar5;
  undefined8 *puVar6;
  int iVar7;
  uint uVar8;
  int iVar10;
  uint uVar11;
  uint uVar12;
  ulonglong uVar13;
  ulonglong uVar9;

  if ((param_2 == (char *)0x0) || (param_1 == (uint *)0x0)) {
    return -0x3ea;
  }
  if (*param_2 == 'G') {
    uVar4 = *(ushort *)(param_2 + 2);
    if (uVar4 < 0x1001) {
      *param_1 = (uint)uVar4;
      *(undefined2 *)(param_1 + 1) = *(undefined2 *)(param_2 + 4);
      iVar10 = 0xb;
      *(undefined2 *)((longlong)param_1 + 6) = *(undefined2 *)(param_2 + 6);
      bVar2 = param_2[8];
      *(byte *)(param_1 + 2) = bVar2;
      bVar3 = param_2[9];
      *(byte *)((longlong)param_1 + 9) = bVar3;
      *(char *)((longlong)param_1 + 10) = param_2[10];
      if ((3 < bVar2) || (0x80 < bVar3)) {
        return -0x3e9;
      }
      uVar9 = 0;
      iVar7 = 0;
      uVar5 = (int)((uint)bVar2 * 0xa2 + 7) >> 3;
      uVar13 = uVar9;
      if (uVar4 != 0) {
        do {
          bVar2 = param_2[iVar10];
          bVar3 = param_2[iVar10 + 1];
          *(ushort *)(*(longlong *)(uVar13 + *(longlong *)(param_1 + 6)) + 0xe) =
               (ushort)(byte)param_2[iVar10 + 2];
          uVar8 = *(uint *)(param_2 + (iVar10 + 3));
          uVar11 = (uint)bVar2 | (int)uVar8 >> 10 & 0x700U;
          *(byte *)(*(longlong *)(uVar13 + *(longlong *)(param_1 + 6)) + 0xc) =
               (byte)((int)uVar8 >> 0x16) & 1;
          uVar12 = (uint)bVar3 | (int)uVar8 >> 6 & 0x700U;
          if ((uVar8 >> 0x15 & 1) != 0) {
            uVar11 = uVar11 | 0xfffff800;
          }
          if ((uVar8 >> 0x11 & 1) != 0) {
            uVar12 = uVar12 | 0xfffff800;
          }
          **(uint **)(uVar13 + *(longlong *)(param_1 + 6)) = uVar11;
          *(uint *)(*(longlong *)(uVar13 + *(longlong *)(param_1 + 6)) + 4) = uVar12;
          puVar1 = (ushort *)(*(longlong *)(uVar13 + *(longlong *)(param_1 + 6)) + 0xe);
          *puVar1 = *puVar1 | (ushort)((int)uVar8 >> 5) & 0x100;
          if (0x169 < (ushort)(*(short *)(*(longlong *)(uVar13 + *(longlong *)(param_1 + 6)) + 0xe)
                              + 1U)) {
            return -0x3e9;
          }
          *(uint *)(*(longlong *)(uVar13 + *(longlong *)(param_1 + 6)) + 8) = uVar8 & 0x1fff;
          *(char *)(*(longlong *)(uVar13 + *(longlong *)(param_1 + 6)) + 0xd) = param_2[iVar10 + 7];
          FUN_18004d200((undefined8 *)(*(longlong *)(uVar13 + *(longlong *)(param_1 + 6)) + 0x10),
                        (undefined8 *)(param_2 + (iVar10 + 8)),(ulonglong)uVar5);
          uVar8 = (int)uVar9 + 1;
          uVar9 = (ulonglong)uVar8;
          iVar10 = iVar10 + 8 + uVar5;
          uVar13 = uVar13 + 8;
        } while ((int)uVar8 < (int)*param_1);
      }
      if ((param_2[1] & 2U) != 0) {
        iVar7 = (uint)*(ushort *)((longlong)param_1 + 6) * (uint)(ushort)param_1[1];
        puVar6 = malloc((longlong)iVar7);
        *(undefined8 **)(param_1 + 4) = puVar6;
        if (puVar6 != (undefined8 *)0x0) {
          FUN_18004d200(puVar6,(undefined8 *)(param_2 + iVar10),(longlong)iVar7);
        }
      }
      return (uVar5 + 8) * *param_1 + 0xb + iVar7;
    }
  }
  return -0x3e9;
}

ulonglong FUN_180035bd0(int *param_1,short *param_2,int param_3)

{
  uint *puVar1;
  short *psVar2;
  uint uVar3;
  ulonglong uVar4;
  uint uVar5;
  int iVar6;
  ulonglong uVar7;
  short sVar8;
  ulonglong uVar9;
  ulonglong uVar10;
  undefined8 *puVar11;

  sVar8 = (short)*param_1;
  uVar7 = 0;
  uVar9 = uVar7;
  if (0 < *param_1) {
    puVar11 = *(undefined8 **)(param_1 + 2);
    do {
      puVar1 = (uint *)*puVar11;
      if (puVar1 == (uint *)0x0) {
        return 0xfffffc17;
      }
      uVar3 = *puVar1;
      uVar4 = (ulonglong)uVar3;
      if (0x1000 < uVar3) {
        return 0xfffffc17;
      }
      uVar5 = uVar3;
      if (0 < (int)uVar3) {
        psVar2 = (short *)(**(longlong **)(puVar1 + 6) + 0xe);
        uVar10 = (ulonglong)uVar3;
        do {
          if (*psVar2 < 0) {
            uVar5 = uVar5 - 1;
          }
          psVar2 = psVar2 + 0x28;
          uVar10 = uVar10 - 1;
        } while (uVar10 != 0);
      }
      if ((int)uVar5 < param_3) {
        sVar8 = sVar8 + -1;
      }
      else {
        if (0 < (int)uVar3) {
          psVar2 = (short *)(**(longlong **)(puVar1 + 6) + 0xe);
          uVar10 = uVar4;
          do {
            if (*psVar2 < 0) {
              uVar4 = (ulonglong)((int)uVar4 - 1);
            }
            uVar3 = (uint)uVar4;
            psVar2 = psVar2 + 0x28;
            uVar10 = uVar10 - 1;
          } while (uVar10 != 0);
        }
        iVar6 = (((int)((uint)(byte)puVar1[2] * 0xa2 + 7) >> 3) + 8) * uVar3 + 0xb;
        if (*(longlong *)(puVar1 + 4) != 0) {
          iVar6 = iVar6 + (uint)*(ushort *)((longlong)puVar1 + 6) * (uint)(ushort)puVar1[1];
        }
        uVar9 = (ulonglong)(uint)((int)uVar9 + iVar6);
      }
      uVar7 = uVar7 + 1;
      puVar11 = puVar11 + 1;
    } while ((longlong)uVar7 < (longlong)*param_1);
  }
  *param_2 = sVar8;
  return uVar9;
}

int FUN_180035cf0(int *param_1,undefined8 *param_2,int *param_3,longlong param_4)

{
  uint *puVar1;
  int iVar2;
  ulonglong uVar3;
  undefined8 *puVar4;
  short *psVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  longlong lVar9;
  short local_res18 [4];

  if (param_3 == (int *)0x0) {
    return -0x3ea;
  }
  if ((*param_1 - 1U < 0x28) && (*(longlong *)(param_1 + 2) != 0)) {
    uVar3 = FUN_180035bd0(param_1,local_res18,*(int *)(param_4 + 0x58));
    iVar7 = (int)uVar3;
    if (-1 < iVar7) {
      *param_3 = iVar7 + 0x20;
      if (param_2 == (undefined8 *)0x0) {
        return 0;
      }
      puVar4 = malloc((longlong)(iVar7 + 0x28));
      *param_2 = puVar4;
      if (puVar4 == (undefined8 *)0x0) {
        iVar2 = -1000;
LAB_180035d95:
        if ((void *)*param_2 != (void *)0x0) {
          free((void *)*param_2);
          *param_2 = 0;
        }
      }
      else {
        iVar8 = 0;
        iVar7 = 0x20;
        *puVar4 = 0;
        puVar4[1] = 0;
        puVar4[2] = 0;
        puVar4[3] = 0;
        *(undefined2 *)puVar4 = 0x145;
        *(short *)((longlong)puVar4 + 2) = local_res18[0];
        if (0 < *param_1) {
          lVar9 = 0;
          do {
            puVar1 = *(uint **)(lVar9 + *(longlong *)(param_1 + 2));
            uVar6 = *puVar1;
            if (0 < (int)uVar6) {
              psVar5 = (short *)(**(longlong **)(puVar1 + 6) + 0xe);
              uVar3 = (ulonglong)uVar6;
              do {
                if (*psVar5 < 0) {
                  uVar6 = uVar6 - 1;
                }
                psVar5 = psVar5 + 0x28;
                uVar3 = uVar3 - 1;
              } while (uVar3 != 0);
            }
            if (*(int *)(param_4 + 0x58) <= (int)uVar6) {
              iVar2 = FUN_180035870(puVar1,(undefined2 *)((longlong)iVar7 + (longlong)puVar4));
              if (iVar2 < 0) goto LAB_180035d95;
              iVar7 = iVar7 + iVar2;
            }
            iVar8 = iVar8 + 1;
            lVar9 = lVar9 + 8;
          } while (iVar8 < *param_1);
        }
        iVar2 = 0;
      }
      return iVar2;
    }
  }
  return -0x3e9;
}

int FUN_180035e80(uint *param_1,char *param_2)

{
  ushort uVar1;
  int iVar2;
  void *pvVar3;
  uint uVar4;
  ulonglong uVar5;
  longlong lVar6;
  int iVar7;
  ulonglong uVar8;
  int iVar9;

  if ((param_2 == (char *)0x0) || (param_1 == (uint *)0x0)) {
    return -0x3ea;
  }
  if ((*param_2 != 'E') || (uVar1 = *(ushort *)(param_2 + 2), 0x27 < uVar1 - 1)) {
    return -0x3e9;
  }
  uVar5 = 0;
  iVar9 = 0x20;
  *param_1 = (uint)uVar1;
  uVar8 = uVar5;
  if (uVar1 != 0) {
    do {
      iVar7 = (int)uVar8;
      uVar4 = (uint)*(ushort *)(param_2 + iVar9 + 2);
      if (0x1000 < uVar4) {
        return -0x3e9;
      }
      pvVar3 = FUN_180035200(uVar4);
      *(void **)(uVar5 + *(longlong *)(param_1 + 2)) = pvVar3;
      if (*(uint **)(uVar5 + *(longlong *)(param_1 + 2)) == (uint *)0x0) {
        iVar2 = -1000;
        iVar7 = iVar7 + -1;
LAB_180035f65:
        lVar6 = (longlong)iVar7;
        if (iVar7 < 0) {
          return iVar2;
        }
        do {
          pvVar3 = *(void **)(*(longlong *)(param_1 + 2) + lVar6 * 8);
          if (pvVar3 != (void *)0x0) {
            free(*(void **)((longlong)pvVar3 + 0x10));
            free(pvVar3);
          }
          lVar6 = lVar6 + -1;
          *(undefined8 *)(*(longlong *)(param_1 + 2) + 8 + lVar6 * 8) = 0;
        } while (-1 < lVar6);
        return iVar2;
      }
      iVar2 = FUN_180035950(*(uint **)(uVar5 + *(longlong *)(param_1 + 2)),param_2 + iVar9);
      if (iVar2 < 0) goto LAB_180035f65;
      iVar9 = iVar9 + iVar2;
      uVar5 = uVar5 + 8;
      uVar8 = (ulonglong)(iVar7 + 1U);
    } while ((int)(iVar7 + 1U) < (int)*param_1);
  }
  return 0;
}

int * FUN_180035fc0(int param_1,int param_2)

{
  int *piVar1;
  int *piVar2;
  longlong lVar3;
  longlong lVar4;

  piVar1 = calloc((longlong)(param_2 * 8) + ((longlong)(param_1 * param_2) + 4) * 4,1);
  if (piVar1 == (int *)0x0) {
    return (int *)0x0;
  }
  *piVar1 = param_1;
  piVar1[1] = param_2;
  piVar2 = piVar1 + 4;
  *(int **)(piVar1 + 2) = piVar2;
  lVar3 = (longlong)piVar2 + (longlong)(param_2 * 8);
  lVar4 = 0;
  if (0 < param_2) {
    do {
      *(longlong *)(piVar2 + lVar4 * 2) = lVar3;
      lVar4 = lVar4 + 1;
      lVar3 = lVar3 + (longlong)param_1 * 4;
    } while (lVar4 < param_2);
  }
  return piVar1;
}

void FUN_180036060(longlong param_1,int param_2,int param_3,uint *param_4)

{
  uint uVar1;
  longlong lVar2;
  int iVar3;

  uVar1 = *param_4;
  if (((uVar1 & 0x3fffffff) != 0) && (iVar3 = 0, 0 < (int)param_4[1])) {
    lVar2 = 0;
    do {
      FUN_18004d200(*(undefined8 **)(*(longlong *)(param_4 + 2) + lVar2),
                    (undefined8 *)
                    (*(longlong *)(*(longlong *)(param_1 + 8) + (longlong)param_3 * 8 + lVar2) +
                    (longlong)param_2 * 4),(longlong)(int)(uVar1 << 2));
      iVar3 = iVar3 + 1;
      lVar2 = lVar2 + 8;
    } while (iVar3 < (int)param_4[1]);
  }
  return;
}

void FUN_180036100(int *param_1)

{
  undefined8 *_Memory;
  longlong lVar1;
  longlong lVar2;
  int iVar3;
  ulonglong uVar4;

  iVar3 = *param_1 * 4;
  if ((iVar3 != 0) && (_Memory = malloc((longlong)*param_1 << 2), _Memory != (undefined8 *)0x0)) {
    lVar1 = 0;
    lVar2 = (longlong)(param_1[1] + -1);
    if (0 < param_1[1] + -1) {
      uVar4 = (ulonglong)iVar3;
      do {
        FUN_18004d200(_Memory,*(undefined8 **)(*(longlong *)(param_1 + 2) + lVar1 * 8),uVar4);
        FUN_18004d200(*(undefined8 **)(*(longlong *)(param_1 + 2) + lVar1 * 8),
                      *(undefined8 **)(*(longlong *)(param_1 + 2) + lVar2 * 8),uVar4);
        FUN_18004d200(*(undefined8 **)(*(longlong *)(param_1 + 2) + lVar2 * 8),_Memory,uVar4);
        lVar1 = lVar1 + 1;
        lVar2 = lVar2 + -1;
      } while (lVar1 < lVar2);
    }
    free(_Memory);
  }
  return;
}

void FUN_1800361d0(longlong param_1,int param_2,int param_3,int *param_4)

{
  int iVar1;
  longlong lVar2;
  int iVar3;

  iVar1 = *param_4;
  iVar3 = 0;
  if (0 < param_4[1]) {
    lVar2 = 0;
    do {
      FUN_18004d200((undefined8 *)
                    (*(longlong *)(*(longlong *)(param_1 + 8) + (longlong)param_3 * 8 + lVar2) +
                    (longlong)param_2 * 4),*(undefined8 **)(*(longlong *)(param_4 + 2) + lVar2),
                    (longlong)(iVar1 << 2));
      iVar3 = iVar3 + 1;
      lVar2 = lVar2 + 8;
    } while (iVar3 < param_4[1]);
  }
  return;
}

int * FUN_180036270(int *param_1)

{
  int *piVar1;

  piVar1 = FUN_180035fc0(*param_1,param_1[1]);
  if (piVar1 == (int *)0x0) {
    return (int *)0x0;
  }
  FUN_18004d200((undefined8 *)**(undefined8 **)(piVar1 + 2),
                (undefined8 *)**(undefined8 **)(param_1 + 2),(longlong)(*param_1 * param_1[1]) << 2)
  ;
  return piVar1;
}

void FUN_1800362d0(uint *param_1,longlong param_2)

{
  int *piVar1;
  int *piVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  uint uVar10;
  int iVar11;
  ulonglong *_Memory;
  void *_Memory_00;
  void *pvVar12;
  longlong lVar13;
  longlong lVar14;
  longlong lVar15;
  int *piVar16;
  int iVar17;
  longlong lVar18;
  longlong lVar19;
  int iVar20;
  ulonglong *puVar21;
  size_t _Count;

  uVar3 = *param_1;
  _Count = (size_t)(int)uVar3;
  uVar4 = param_1[1];
  lVar15 = (longlong)(int)uVar4;
  iVar20 = 0;
  _Memory = malloc(lVar15 * 8);
  _Memory_00 = calloc(_Count,4);
  lVar19 = 0;
  do {
    pvVar12 = malloc(_Count * 4);
    lVar18 = lVar19 + 1;
    _Memory[lVar19] = (ulonglong)pvVar12;
    lVar19 = lVar18;
  } while (lVar18 < 3);
  if (-1 < lVar15) {
    lVar19 = -1;
    puVar21 = _Memory;
    do {
      if (-1 < lVar19 + -2) {
        pvVar12 = (void *)puVar21[-3];
        lVar18 = 0;
        if ((0 < (int)uVar3) && (7 < uVar3)) {
          iVar17 = 0;
          if (((void *)((longlong)pvVar12 + (longlong)(int)(uVar3 - 1) * 4) < _Memory_00) ||
             ((void *)((longlong)_Memory_00 + (longlong)(int)(uVar3 - 1) * 4) < pvVar12)) {
            uVar10 = uVar3 & 0x80000007;
            if ((int)uVar10 < 0) {
              uVar10 = (uVar10 - 1 | 0xfffffff8) + 1;
            }
            do {
              lVar13 = (longlong)iVar17;
              lVar18 = lVar18 + 8;
              piVar16 = (int *)((longlong)pvVar12 + lVar13 * 4);
              iVar11 = piVar16[1];
              iVar5 = piVar16[2];
              iVar6 = piVar16[3];
              piVar1 = (int *)((longlong)_Memory_00 + lVar13 * 4);
              iVar7 = piVar1[1];
              iVar8 = piVar1[2];
              iVar9 = piVar1[3];
              piVar2 = (int *)((longlong)_Memory_00 + lVar13 * 4);
              *piVar2 = *piVar1 - *piVar16;
              piVar2[1] = iVar7 - iVar11;
              piVar2[2] = iVar8 - iVar5;
              piVar2[3] = iVar9 - iVar6;
              iVar11 = iVar17 + 4;
              iVar17 = iVar17 + 8;
              lVar13 = (longlong)iVar11;
              piVar16 = (int *)((longlong)_Memory_00 + lVar13 * 4);
              iVar11 = piVar16[1];
              iVar5 = piVar16[2];
              iVar6 = piVar16[3];
              piVar1 = (int *)((longlong)pvVar12 + lVar13 * 4);
              iVar7 = piVar1[1];
              iVar8 = piVar1[2];
              iVar9 = piVar1[3];
              piVar2 = (int *)((longlong)_Memory_00 + lVar13 * 4);
              *piVar2 = *piVar16 - *piVar1;
              piVar2[1] = iVar11 - iVar7;
              piVar2[2] = iVar5 - iVar8;
              piVar2[3] = iVar6 - iVar9;
            } while (iVar17 < (int)(uVar3 - uVar10));
          }
        }
        if (lVar18 < (longlong)_Count) {
          lVar13 = _Count - lVar18;
          piVar16 = (int *)((longlong)_Memory_00 + lVar18 * 4);
          do {
            *piVar16 = *piVar16 -
                       *(int *)(((longlong)pvVar12 - (longlong)_Memory_00) + (longlong)piVar16);
            lVar13 = lVar13 + -1;
            piVar16 = piVar16 + 1;
          } while (lVar13 != 0);
        }
        iVar20 = iVar20 + -1;
      }
      lVar18 = lVar19 + 1;
      if (lVar18 < lVar15) {
        if (-1 < lVar19 + -2) {
          *puVar21 = puVar21[-3];
        }
        FUN_18004d200((undefined8 *)*puVar21,
                      *(undefined8 **)
                       ((*(longlong *)(param_1 + 2) - (longlong)_Memory) + (longlong)puVar21),
                      _Count * 4);
        pvVar12 = (void *)*puVar21;
        lVar13 = 0;
        if ((0 < (int)uVar3) && (7 < uVar3)) {
          iVar17 = 0;
          if (((void *)((longlong)pvVar12 + (longlong)(int)(uVar3 - 1) * 4) < _Memory_00) ||
             ((void *)((longlong)_Memory_00 + (longlong)(int)(uVar3 - 1) * 4) < pvVar12)) {
            uVar10 = uVar3 & 0x80000007;
            if ((int)uVar10 < 0) {
              uVar10 = (uVar10 - 1 | 0xfffffff8) + 1;
            }
            do {
              lVar14 = (longlong)iVar17;
              lVar13 = lVar13 + 8;
              piVar16 = (int *)((longlong)_Memory_00 + lVar14 * 4);
              iVar11 = piVar16[1];
              iVar5 = piVar16[2];
              iVar6 = piVar16[3];
              piVar1 = (int *)((longlong)pvVar12 + lVar14 * 4);
              iVar7 = piVar1[1];
              iVar8 = piVar1[2];
              iVar9 = piVar1[3];
              piVar2 = (int *)((longlong)_Memory_00 + lVar14 * 4);
              *piVar2 = *piVar1 + *piVar16;
              piVar2[1] = iVar7 + iVar11;
              piVar2[2] = iVar8 + iVar5;
              piVar2[3] = iVar9 + iVar6;
              iVar11 = iVar17 + 4;
              iVar17 = iVar17 + 8;
              lVar14 = (longlong)iVar11;
              piVar16 = (int *)((longlong)pvVar12 + lVar14 * 4);
              iVar11 = piVar16[1];
              iVar5 = piVar16[2];
              iVar6 = piVar16[3];
              piVar1 = (int *)((longlong)_Memory_00 + lVar14 * 4);
              iVar7 = piVar1[1];
              iVar8 = piVar1[2];
              iVar9 = piVar1[3];
              piVar2 = (int *)((longlong)_Memory_00 + lVar14 * 4);
              *piVar2 = *piVar16 + *piVar1;
              piVar2[1] = iVar11 + iVar7;
              piVar2[2] = iVar5 + iVar8;
              piVar2[3] = iVar6 + iVar9;
            } while (iVar17 < (int)(uVar3 - uVar10));
          }
        }
        if (lVar13 < (longlong)_Count) {
          lVar14 = _Count - lVar13;
          piVar16 = (int *)((longlong)_Memory_00 + lVar13 * 4);
          do {
            *piVar16 = *piVar16 +
                       *(int *)((longlong)piVar16 + ((longlong)pvVar12 - (longlong)_Memory_00));
            lVar14 = lVar14 + -1;
            piVar16 = piVar16 + 1;
          } while (lVar14 != 0);
        }
        iVar20 = iVar20 + 1;
      }
      if (-1 < lVar19) {
        lVar13 = -1;
        iVar11 = 0;
        iVar17 = 0;
        lVar19 = *(longlong *)
                  ((longlong)puVar21 + *(longlong *)(param_1 + 2) + (-8 - (longlong)_Memory));
        if (-1 < (longlong)_Count) {
          piVar16 = (int *)((longlong)_Memory_00 + -0xc);
          lVar14 = (*(longlong *)
                     ((longlong)puVar21 + *(longlong *)(param_2 + 8) + (-8 - (longlong)_Memory)) -
                   (longlong)_Memory_00) + 8;
          do {
            if (-1 < lVar13 + -2) {
              iVar11 = iVar11 - *piVar16;
              iVar17 = iVar17 - iVar20;
            }
            if (lVar13 + 1 < (longlong)_Count) {
              iVar11 = iVar11 + piVar16[3];
              iVar17 = iVar17 + iVar20;
            }
            if (-1 < lVar13) {
              if (iVar17 == 9) {
                *(int *)(lVar14 + (longlong)piVar16) =
                     ((*(int *)((lVar19 - (longlong)_Memory_00) + 8 + (longlong)piVar16) +
                      iVar11 * 4) * 2 + 0x25) / 0x4a;
              }
              else {
                *(int *)(lVar14 + (longlong)piVar16) = iVar11 / iVar17;
              }
            }
            lVar13 = lVar13 + 1;
            piVar16 = piVar16 + 1;
          } while (lVar13 < (longlong)_Count);
        }
      }
      puVar21 = puVar21 + 1;
      lVar19 = lVar18;
    } while (lVar18 < lVar15);
  }
  for (lVar19 = (longlong)(int)(uVar4 - 3); lVar19 < lVar15; lVar19 = lVar19 + 1) {
    free((void *)_Memory[lVar19]);
  }
  free(_Memory);
  free(_Memory_00);
  return;
}

void FUN_180036680(int *param_1,longlong param_2,longlong param_3,int param_4)

{
  longlong lVar1;
  longlong lVar2;
  longlong lVar3;
  int iVar4;
  longlong lVar5;
  longlong *plVar6;
  longlong lVar7;
  longlong lVar8;
  int iVar9;
  longlong *plVar10;
  longlong lVar11;
  longlong lVar12;
  int iVar13;

  lVar8 = 1;
  if (param_4 < 2) {
    iVar9 = 1 - param_4;
  }
  else {
    iVar9 = param_4 + -2;
  }
  iVar4 = param_1[1] + -2;
  if (param_4 < iVar4) {
    iVar4 = param_4 + 2;
  }
  else if (param_4 == iVar4) {
    iVar4 = param_1[1] + -1;
  }
  lVar1 = *(longlong *)(param_1 + 2);
  iVar13 = 0;
  if (0 < *param_1) {
    lVar5 = 0;
    plVar6 = (longlong *)((longlong)iVar4 * 8 + lVar1);
    plVar10 = (longlong *)((longlong)iVar9 * 8 + lVar1);
    lVar11 = (longlong)(*param_1 + -2);
    do {
      lVar7 = lVar8;
      if (1 < lVar5) {
        lVar7 = lVar5 + -2;
      }
      if (lVar5 < lVar11) {
        lVar12 = lVar5 + 2;
      }
      else {
        lVar12 = lVar11 + 1;
        if (lVar5 != lVar11) {
          lVar12 = lVar11;
        }
      }
      if (param_2 != 0) {
        lVar2 = *plVar6;
        lVar3 = *plVar10;
        iVar9 = (((*(int *)(lVar3 + lVar12 * 4) - *(int *)(lVar3 + lVar7 * 4)) -
                 *(int *)(lVar2 + lVar7 * 4)) + *(int *)(lVar2 + lVar12 * 4)) * 3;
        lVar2 = *(longlong *)(lVar1 + (longlong)param_4 * 8);
        iVar4 = (*(int *)(lVar2 + lVar12 * 4) - *(int *)(lVar2 + lVar7 * 4)) * 10;
        *(int *)(param_2 + lVar5 * 4) = (int)((uint)(0 < iVar4 + iVar9) + iVar4 + 0x1f + iVar9) >> 6
        ;
      }
      lVar2 = *plVar6;
      lVar3 = *plVar10;
      iVar9 = (((*(int *)(lVar2 + lVar7 * 4) - *(int *)(lVar3 + lVar7 * 4)) -
               *(int *)(lVar3 + lVar12 * 4)) + *(int *)(lVar2 + lVar12 * 4)) * 3;
      iVar4 = (*(int *)(lVar2 + lVar5 * 4) - *(int *)(lVar3 + lVar5 * 4)) * 10;
      iVar13 = iVar13 + 1;
      lVar5 = lVar5 + 1;
      lVar8 = lVar8 + -1;
      *(int *)(param_3 + -4 + lVar5 * 4) = (int)((0 < iVar4 + iVar9) + 0x1f + iVar4 + iVar9) >> 6;
    } while (iVar13 < *param_1);
  }
  return;
}

void FUN_180036820(int *param_1,longlong param_2,longlong param_3)

{
  longlong lVar1;
  longlong lVar2;
  longlong lVar3;
  longlong lVar4;
  ulonglong uVar5;
  int iVar6;
  int iVar7;
  ulonglong uVar8;
  ulonglong uVar9;
  int iVar10;
  longlong lVar11;
  int iVar12;
  longlong lVar13;
  ulonglong uVar14;
  ulonglong uVar15;
  longlong lVar16;
  uint uVar17;
  longlong local_res10;
  ulonglong uVar18;

  uVar8 = 0;
  if (param_2 == 0) {
    iVar7 = param_1[1];
    if (0 < iVar7) {
      iVar12 = *param_1;
      local_res10 = 1;
      uVar9 = uVar8;
      uVar5 = uVar8;
      do {
        lVar1 = *(longlong *)(*(longlong *)(param_3 + 8) + uVar9 * 8);
        lVar13 = local_res10;
        if (1 < (longlong)uVar9) {
          lVar13 = uVar9 - 2;
        }
        iVar6 = iVar7 + -2;
        iVar10 = (int)uVar5;
        if (iVar10 < iVar6) {
          iVar6 = iVar10 + 2;
        }
        else if (iVar10 == iVar6) {
          iVar6 = iVar7 + -1;
        }
        lVar2 = *(longlong *)(param_1 + 2);
        if (0 < iVar12) {
          uVar15 = (ulonglong)(iVar12 + -2);
          lVar11 = 1;
          uVar5 = uVar8;
          uVar18 = uVar8;
          do {
            lVar16 = lVar11;
            if (1 < (longlong)uVar5) {
              lVar16 = uVar5 - 2;
            }
            if ((longlong)uVar5 < (longlong)uVar15) {
              uVar14 = uVar5 + 2;
            }
            else {
              uVar14 = uVar15 + 1;
              if (uVar5 != uVar15) {
                uVar14 = uVar15;
              }
            }
            lVar3 = *(longlong *)(lVar2 + (longlong)iVar6 * 8);
            lVar4 = *(longlong *)(lVar2 + lVar13 * 8);
            iVar7 = (((*(int *)(lVar3 + lVar16 * 4) - *(int *)(lVar4 + lVar16 * 4)) -
                     *(int *)(lVar4 + uVar14 * 4)) + *(int *)(lVar3 + uVar14 * 4)) * 3;
            iVar12 = (*(int *)(lVar3 + uVar5 * 4) - *(int *)(lVar4 + uVar5 * 4)) * 10;
            uVar17 = (int)uVar18 + 1;
            uVar18 = (ulonglong)uVar17;
            uVar5 = uVar5 + 1;
            lVar11 = lVar11 + -1;
            *(int *)(lVar1 + -4 + uVar5 * 4) =
                 (int)((0 < iVar12 + iVar7) + 0x1f + iVar12 + iVar7) >> 6;
            iVar12 = *param_1;
          } while ((int)uVar17 < iVar12);
        }
        iVar7 = param_1[1];
        local_res10 = local_res10 + -1;
        uVar5 = (ulonglong)(iVar10 + 1U);
        uVar9 = uVar9 + 1;
      } while ((int)(iVar10 + 1U) < iVar7);
      return;
    }
  }
  else {
    uVar9 = uVar8;
    if (0 < param_1[1]) {
      do {
        FUN_180036680(param_1,*(longlong *)(*(longlong *)(param_2 + 8) + uVar9),
                      *(longlong *)(*(longlong *)(param_3 + 8) + uVar9),(int)uVar8);
        uVar17 = (int)uVar8 + 1;
        uVar8 = (ulonglong)uVar17;
        uVar9 = uVar9 + 8;
      } while ((int)uVar17 < param_1[1]);
    }
  }
  return;
}

void FUN_1800369e0(uint *param_1,longlong param_2,longlong param_3)

{
  uint uVar1;
  uint uVar2;
  longlong lVar3;
  longlong lVar4;
  int iVar5;
  longlong lVar6;
  longlong lVar7;
  int iVar8;
  longlong lVar9;
  int iVar10;
  ulonglong uVar11;
  longlong lVar12;
  ulonglong uVar13;
  longlong lVar14;
  int local_res20;

  uVar1 = param_1[1];
  uVar2 = *param_1;
  uVar11 = (ulonglong)(int)uVar2;
  lVar9 = 0;
  local_res20 = 0;
  if (0 < (int)uVar1) {
    uVar13 = uVar11;
    do {
      if (lVar9 == 0) {
        lVar6 = 0;
      }
      else {
        lVar6 = lVar9 + -1;
      }
      lVar7 = *(longlong *)(param_1 + 2);
      lVar3 = *(longlong *)(lVar7 + lVar6 * 8);
      lVar4 = *(longlong *)(lVar7 + lVar9 * 8);
      lVar6 = (longlong)(int)uVar1 + -1;
      if (local_res20 != uVar1 - 1) {
        lVar6 = lVar9 + 1;
      }
      lVar6 = *(longlong *)(lVar7 + lVar6 * 8);
      iVar10 = 0;
      lVar7 = 0;
      if (0 < (longlong)uVar11) {
        do {
          if (lVar7 == 0) {
            lVar14 = 0;
          }
          else {
            lVar14 = lVar7 + -1;
          }
          lVar12 = uVar11 - 1;
          if (iVar10 != (int)uVar13 + -1) {
            lVar12 = lVar7 + 1;
          }
          if (param_2 != 0) {
            iVar8 = (((*(int *)(lVar4 + lVar12 * 4) - *(int *)(lVar4 + lVar14 * 4)) * 2 -
                     *(int *)(lVar6 + lVar14 * 4)) - *(int *)(lVar3 + lVar14 * 4)) +
                    *(int *)(lVar6 + lVar12 * 4) + *(int *)(lVar3 + lVar12 * 4);
            iVar5 = -4;
            if (0 < iVar8) {
              iVar5 = 4;
            }
            iVar5 = iVar5 + iVar8;
            *(int *)(*(longlong *)(*(longlong *)(param_2 + 8) + lVar9 * 8) + lVar7 * 4) =
                 (int)(iVar5 + (iVar5 >> 0x1f & 7U)) >> 3;
          }
          if (param_3 != 0) {
            iVar8 = (((*(int *)(lVar6 + lVar7 * 4) - *(int *)(lVar3 + lVar7 * 4)) * 2 -
                     *(int *)(lVar3 + lVar14 * 4)) - *(int *)(lVar3 + lVar12 * 4)) +
                    *(int *)(lVar6 + lVar14 * 4) + *(int *)(lVar6 + lVar12 * 4);
            iVar5 = -4;
            if (0 < iVar8) {
              iVar5 = 4;
            }
            iVar5 = iVar5 + iVar8;
            *(int *)(*(longlong *)(*(longlong *)(param_3 + 8) + lVar9 * 8) + lVar7 * 4) =
                 (int)(iVar5 + (iVar5 >> 0x1f & 7U)) >> 3;
          }
          lVar7 = lVar7 + 1;
          iVar10 = iVar10 + 1;
        } while (lVar7 < (longlong)uVar11);
        uVar13 = (ulonglong)uVar2;
      }
      local_res20 = local_res20 + 1;
      lVar9 = lVar9 + 1;
    } while (lVar9 < (int)uVar1);
  }
  return;
}

void FUN_180038480(undefined8 *param_1,int param_2,int param_3)

{
  uint uVar1;
  int iVar2;
  byte *pbVar3;
  longlong lVar4;
  longlong lVar5;
  uint uVar6;
  uint uVar7;

  uVar7 = 0;
  uVar6 = 0xff;
  lVar5 = (longlong)(param_2 * param_3);
  pbVar3 = (byte *)*param_1;
  lVar4 = lVar5;
  if (0 < param_2 * param_3) {
    do {
      uVar1 = (uint)*pbVar3;
      if (uVar7 < *pbVar3) {
        uVar7 = uVar1;
      }
      if (uVar1 < uVar6) {
        uVar6 = uVar1;
      }
      lVar4 = lVar4 + -1;
      pbVar3 = pbVar3 + 1;
    } while (lVar4 != 0);
    if (uVar7 == uVar6) {
      return;
    }
  }
  pbVar3 = (byte *)*param_1;
  if (0 < lVar5) {
    do {
      iVar2 = (*pbVar3 - uVar6) * (int)(0x3fc00 / (longlong)(int)(uVar7 - uVar6)) + 0x200;
      *pbVar3 = (byte)((int)(iVar2 + (iVar2 >> 0x1f & 0x3ffU)) >> 10);
      lVar5 = lVar5 + -1;
      pbVar3 = pbVar3 + 1;
    } while (lVar5 != 0);
  }
  return;
}

longlong * FUN_180038500(longlong param_1,int param_2,int param_3,uint param_4,uint param_5)

{
  longlong lVar1;
  int iVar2;
  undefined1 uVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  longlong *plVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  uint uVar11;
  longlong lVar12;
  ulonglong uVar13;
  ulonglong uVar14;
  int iVar15;
  ulonglong uVar16;
  ulonglong uVar17;
  byte *pbVar18;
  longlong lVar19;
  longlong *plVar20;

  uVar17 = (ulonglong)param_5;
  plVar7 = FUN_18002e460(param_4,param_5);
  iVar8 = (param_3 << 10) / (int)param_5;
  iVar2 = (param_2 << 10) / (int)param_4;
  if (plVar7 == (longlong *)0x0) {
    return (longlong *)0x0;
  }
  if (0 < (int)param_5) {
    iVar9 = iVar8 * -0x200;
    plVar20 = plVar7;
    do {
      uVar14 = (ulonglong)param_4;
      iVar4 = iVar9 + (iVar9 >> 0x1f & 0x3ffU);
      uVar11 = iVar4 >> 0x1f & 0x3ff;
      uVar5 = (iVar4 >> 10) + uVar11;
      iVar4 = (int)uVar5 >> 10;
      uVar16 = (ulonglong)((uVar5 & 0x3ff) - uVar11);
      if (iVar4 == param_3 + -1) {
        uVar16 = 0;
      }
      if (0 < (int)param_4) {
        lVar19 = (longlong)iVar4;
        iVar4 = iVar2 * -0x200;
        uVar13 = 0;
        do {
          iVar6 = iVar4 + (iVar4 >> 0x1f & 0x3ffU);
          uVar11 = iVar6 >> 0x1f & 0x3ff;
          uVar5 = (iVar6 >> 10) + uVar11;
          iVar10 = (int)uVar5 >> 10;
          iVar6 = (uVar5 & 0x3ff) - uVar11;
          if (iVar10 == param_2 + -1) {
            iVar6 = 0;
          }
          iVar15 = (int)uVar16;
          if (iVar15 < 1) {
            if (iVar6 < 1) {
              uVar3 = *(undefined1 *)((longlong)iVar10 + *(longlong *)(param_1 + lVar19 * 8));
            }
            else {
LAB_180038695:
              lVar12 = (longlong)iVar10;
              if (iVar15 < 1) {
                lVar1 = *(longlong *)(param_1 + lVar19 * 8);
                iVar6 = ((uint)*(byte *)(lVar12 + lVar1) * (0x400 - iVar6) + 0x200 +
                        (uint)*(byte *)(lVar12 + 1 + lVar1) * iVar6) * 0x400;
                iVar6 = iVar6 + (iVar6 >> 0x1f & 0xfffffU);
              }
              else {
                lVar1 = *(longlong *)(param_1 + 8 + lVar19 * 8);
                pbVar18 = (byte *)(*(longlong *)(param_1 + lVar19 * 8) + lVar12);
                iVar6 = ((uint)*(byte *)(lVar1 + lVar12) * iVar15 +
                        (uint)*pbVar18 * (0x400 - iVar15)) * (0x400 - iVar6) + 0x80000 +
                        ((uint)*(byte *)(lVar1 + 1 + lVar12) * iVar15 +
                        (uint)pbVar18[1] * (0x400 - iVar15)) * iVar6;
                iVar6 = iVar6 + (iVar6 >> 0x1f & 0xfffffU);
              }
              uVar3 = (undefined1)(iVar6 >> 0x14);
            }
          }
          else {
            if (0 < iVar6) goto LAB_180038695;
            iVar6 = ((uint)*(byte *)(*(longlong *)(param_1 + 8 + lVar19 * 8) + (longlong)iVar10) *
                     iVar15 + (uint)*(byte *)((longlong)iVar10 + *(longlong *)(param_1 + lVar19 * 8)
                                             ) * (0x400 - iVar15) + 0x200) * 0x400;
            uVar3 = (undefined1)((int)(iVar6 + (iVar6 >> 0x1f & 0xfffffU)) >> 0x14);
          }
          iVar4 = iVar4 + iVar2 * 0x400;
          *(undefined1 *)(uVar13 + *plVar20) = uVar3;
          uVar14 = uVar14 - 1;
          uVar13 = uVar13 + 1;
        } while (uVar14 != 0);
      }
      plVar20 = plVar20 + 1;
      iVar9 = iVar9 + iVar8 * 0x400;
      uVar17 = uVar17 - 1;
    } while (uVar17 != 0);
  }
  return plVar7;
}

undefined8 FUN_1800387c0(undefined8 *param_1,int param_2,int param_3)

{
  undefined8 *_Memory;

  _Memory = FUN_18002e460(param_2,param_3);
  FUN_18004d200((undefined8 *)*_Memory,(undefined8 *)*param_1,(longlong)(param_2 * param_3));
  FUN_18002e4f0(param_2,param_3,(longlong)_Memory,5);
  FUN_18002e7a0(param_2,param_3,(longlong)param_1,(longlong)_Memory);
  FUN_18004d200((undefined8 *)*param_1,(undefined8 *)*_Memory,(longlong)(param_2 * param_3));
  free(_Memory);
  return 0;
}

/* ===== batch 4 ===== */

/* ---- Ghidra SIMD/intrinsic support (packed 4x32 lanes over 16 bytes) ---- */
typedef union { uint8_t b[16]; uint32_t u[4]; int32_t i[4]; uint64_t q[2]; } v128;
static inline v128 v_zero(void){ v128 r; r.q[0]=0; r.q[1]=0; return r; }
static inline v128 v_load(const void *p){ v128 r; memcpy(&r,p,16); return r; }
static inline void v_store(void *p, v128 v){ memcpy(p,&v,16); }
static inline v128 ZEXT816(uint64_t x){ v128 r; r.q[0]=x; r.q[1]=0; return r; }
static inline v128 ZEXT416(uint32_t x){ v128 r; r.u[0]=x; r.u[1]=0; r.q[1]=0; return r; }
/* pmovzxbd: zero-extend low 4 bytes of src into 4 dword lanes; dst arg ignored */
static inline v128 pmovzxbd(v128 a, v128 b){ v128 r; (void)a;
  r.u[0]=b.b[0]; r.u[1]=b.b[1]; r.u[2]=b.b[2]; r.u[3]=b.b[3]; return r; }
/* pmulld: packed 32-bit signed multiply, low result */
static inline v128 pmulld(v128 a, v128 b){ v128 r;
  r.i[0]=(int32_t)((uint32_t)a.i[0]*(uint32_t)b.i[0]);
  r.i[1]=(int32_t)((uint32_t)a.i[1]*(uint32_t)b.i[1]);
  r.i[2]=(int32_t)((uint32_t)a.i[2]*(uint32_t)b.i[2]);
  r.i[3]=(int32_t)((uint32_t)a.i[3]*(uint32_t)b.i[3]); return r; }

/* ================ FUN_180038860 ================ */
ulonglong FUN_180038860(longlong param_1,uint param_2,int param_3)
{
  byte *pbVar1;
  longlong lVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  longlong lVar6;
  longlong lVar7;
  longlong lVar8;
  longlong lVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  v128 in_XMM1;
  v128 auVar13;
  v128 auVar14;
  v128 auVar15;
  v128 auVar16;
  v128 auVar17;

  in_XMM1 = v_zero();
  auVar13 = v_zero();
  iVar12 = 0;
  lVar8 = 0;
  lVar9 = (longlong)(int)param_2;
  if (0 < param_3) {
    do {
      lVar2 = *(longlong *)(param_1 + lVar8 * 8);
      iVar11 = 0;
      iVar10 = 0;
      lVar6 = 0;
      if (1 < lVar9) {
        do {
          pbVar1 = (byte *)(lVar2 + lVar6);
          lVar6 = lVar6 + 2;
          iVar11 = iVar11 + (uint)*pbVar1;
          iVar10 = iVar10 + (uint)*(byte *)(lVar2 + -1 + lVar6);
        } while (lVar6 < lVar9 + -1);
      }
      if (lVar6 < lVar9) {
        iVar12 = iVar12 + (uint)*(byte *)(lVar6 + lVar2);
      }
      lVar8 = lVar8 + 1;
      iVar12 = iVar12 + iVar10 + iVar11;
    } while (lVar8 < param_3);
  }
  iVar10 = 0;
  iVar12 = iVar12 / (int)(param_2 * param_3);
  if (0 < param_3) {
    auVar17 = ZEXT816(0);
    lVar8 = 0;
    auVar13 = auVar17;
    do {
      lVar2 = *(longlong *)(param_1 + lVar8 * 8);
      lVar6 = 0;
      if (((0 < (int)param_2) && (7 < param_2)) && (1 < DAT_1800920bc)) {
        iVar11 = 0;
        uVar3 = param_2 & 0x80000007;
        if ((int)uVar3 < 0) {
          uVar3 = (uVar3 - 1 | 0xfffffff8) + 1;
        }
        auVar15 = auVar13;
        do {
          lVar7 = (longlong)iVar11;
          lVar6 = lVar6 + 8;
          iVar4 = iVar11 + 4;
          iVar11 = iVar11 + 8;
          auVar13 = pmovzxbd(in_XMM1,ZEXT416(*(uint *)(lVar7 + lVar2)));
          auVar14.i[0] = auVar13.i[0] - iVar12;
          auVar14.i[1] = auVar13.i[1] - iVar12;
          auVar14.i[2] = auVar13.i[2] - iVar12;
          auVar14.i[3] = auVar13.i[3] - iVar12;
          auVar14 = pmulld(auVar14,auVar14);
          auVar13.i[0] = auVar14.i[0] + auVar15.i[0];
          auVar13.i[1] = auVar14.i[1] + auVar15.i[1];
          auVar13.i[2] = auVar14.i[2] + auVar15.i[2];
          auVar13.i[3] = auVar14.i[3] + auVar15.i[3];
          auVar15 = pmovzxbd(auVar13,ZEXT416(*(uint *)(iVar4 + lVar2)));
          auVar16.i[0] = auVar15.i[0] - iVar12;
          auVar16.i[1] = auVar15.i[1] - iVar12;
          auVar16.i[2] = auVar15.i[2] - iVar12;
          auVar16.i[3] = auVar15.i[3] - iVar12;
          auVar15 = pmulld(auVar16,auVar16);
          in_XMM1.i[0] = auVar15.i[0] + auVar17.i[0];
          in_XMM1.i[1] = auVar15.i[1] + auVar17.i[1];
          in_XMM1.i[2] = auVar15.i[2] + auVar17.i[2];
          in_XMM1.i[3] = auVar15.i[3] + auVar17.i[3];
          auVar17 = in_XMM1;
          auVar15 = auVar13;
        } while (iVar11 < (int)(param_2 - uVar3));
      }
      iVar11 = 0;
      iVar4 = 0;
      if (lVar6 < lVar9) {
        if (1 < lVar9 - lVar6) {
          do {
            pbVar1 = (byte *)(lVar2 + lVar6);
            lVar6 = lVar6 + 2;
            iVar5 = (uint)*pbVar1 - iVar12;
            iVar11 = iVar11 + iVar5 * iVar5;
            iVar5 = (uint)*(byte *)(lVar2 + -1 + lVar6) - iVar12;
            iVar4 = iVar4 + iVar5 * iVar5;
          } while (lVar6 < lVar9 + -1);
        }
        if (lVar6 < lVar9) {
          iVar5 = (uint)*(byte *)(lVar6 + lVar2) - iVar12;
          iVar10 = iVar10 + iVar5 * iVar5;
        }
        iVar10 = iVar10 + iVar4 + iVar11;
      }
      lVar8 = lVar8 + 1;
    } while (lVar8 < param_3);
    iVar10 = iVar10 + auVar17.i[0] + auVar13.i[0] + auVar17.i[2] + auVar13.i[2] +
                      auVar17.i[1] + auVar13.i[1] + auVar17.i[3] + auVar13.i[3];
  }
  return (longlong)iVar10 / (longlong)(int)(param_2 * param_3) & 0xffffffff;
}

/* ================ FUN_180038a50 ================ */
int FUN_180038a50(longlong param_1,int param_2,int param_3,undefined1 *param_4,int param_5)
{
  byte *pbVar1;
  byte *pbVar2;
  longlong lVar3;
  longlong lVar4;
  byte *pbVar5;
  byte *pbVar6;
  byte *pbVar7;
  byte *pbVar8;
  longlong lVar9;
  longlong lVar10;
  longlong *plVar11;
  longlong lVar12;
  int iVar13;
  longlong lVar14;
  int iVar15;
  int iVar16;
  uint uVar17;
  uint uVar18;
  uint uVar19;
  int iVar20;
  uint uVar21;
  longlong lVar22;
  int iVar23;
  int iVar24;
  int local_res18;

  iVar16 = 0;
  lVar22 = (longlong)param_3;
  local_res18 = 0;
  iVar24 = 0;
  iVar23 = 0;
  iVar13 = 0;
  if (lVar22 < param_3 + 0xc) {
    lVar14 = (param_3 + 0xc) - lVar22;
    iVar24 = 0;
    plVar11 = (longlong *)(param_1 + lVar22 * 8);
    do {
      if ((longlong)param_2 < (longlong)(param_2 + 0xc)) {
        lVar22 = *plVar11;
        lVar9 = plVar11[-1];
        lVar10 = plVar11[1];
        lVar12 = (longlong)param_2;
        do {
          pbVar5 = (byte *)(lVar10 + -1 + lVar12);
          pbVar6 = (byte *)(lVar9 + -1 + lVar12);
          pbVar7 = (byte *)(lVar10 + 1 + lVar12);
          pbVar8 = (byte *)(lVar9 + 1 + lVar12);
          lVar3 = lVar12 + 1;
          pbVar1 = (byte *)(lVar10 + lVar12);
          iVar23 = iVar23 + (uint)*(byte *)(lVar12 + lVar22);
          lVar4 = lVar12 + -1;
          pbVar2 = (byte *)(lVar9 + lVar12);
          lVar12 = lVar12 + 1;
          uVar19 = (uint)*pbVar5;
          uVar17 = (uint)*pbVar6;
          uVar18 = (uint)*pbVar8;
          uVar21 = (uint)*pbVar7;
          iVar15 = ((((uint)*(byte *)(lVar3 + lVar22) - (uint)*(byte *)(lVar4 + lVar22)) * 4 -
                    uVar19) - uVar17) + uVar21 + uVar18;
          iVar13 = ((((uint)*pbVar1 - (uint)*pbVar2) * 4 - uVar17) - uVar18) + uVar19 + uVar21;
          iVar16 = iVar16 + iVar15 * iVar15;
          iVar24 = iVar24 + iVar13 * iVar13;
          local_res18 = local_res18 + iVar13 * iVar15;
        } while (lVar12 < param_2 + 0xc);
      }
      lVar14 = lVar14 + -1;
      iVar13 = local_res18;
      plVar11 = plVar11 + 1;
    } while (lVar14 != 0);
  }
  iVar15 = (iVar16 - iVar24 >> 0x1f & 0x7fU) + (iVar16 - iVar24);
  iVar13 = (iVar13 * 2 >> 0x1f & 0x7fU) + iVar13 * 2;
  iVar20 = ((int)((iVar24 >> 0x1f & 0x7fU) + iVar24) >> 7) +
           ((int)(iVar16 + (iVar16 >> 0x1f & 0x7fU)) >> 7);
  iVar13 = (int)((iVar13 >> 0x1f & 7U) + (iVar13 >> 7)) >> 3;
  iVar24 = (int)((iVar15 >> 0x1f & 7U) + (iVar15 >> 7)) >> 3;
  iVar15 = iVar24 * iVar24 + iVar13 * iVar13;
  iVar13 = (int)((iVar20 >> 0x1f & 7U) + iVar20) >> 3;
  iVar13 = (iVar13 * iVar13) / 0xfe01;
  iVar16 = param_5 * 0x42 + 0xa39;
  iVar24 = 0x1324;
  if ((0x1323 < iVar16) && (iVar24 = iVar16, 0x1b90 < iVar16)) {
    iVar24 = 0x1b90;
  }
  if ((iVar13 < 1) || (iVar20 <= iVar24)) {
    *param_4 = 0;
  }
  else if (iVar15 < iVar13 * 0x6400) {
    *param_4 = iVar13 * 0x1900 <= iVar15;
  }
  else {
    *param_4 = 2;
  }
  return iVar23 / 0x90;
}

/* ================ FUN_180038cb0 ================ */
ulonglong FUN_180038cb0(undefined8 *param_1,uint param_2,int param_3,longlong *param_4)
{
  uint uVar1;
  int iVar2;
  int iVar3;
  undefined8 *_Memory;
  ulonglong uVar4;
  longlong lVar5;
  int iVar6;
  longlong lVar7;
  int iVar8;
  int iVar9;
  longlong lVar10;
  longlong lVar11;
  undefined1 auStackY_b8 [32];
  byte local_88 [4];
  int local_84;
  int local_80;
  uint local_7c;
  int local_78;
  int local_74;
  int local_70 [4];
  int local_60 [4];
  ulonglong local_50;

  local_50 = DAT_180092040 ^ (ulonglong)auStackY_b8;
  iVar8 = 0;
  local_60[0] = 0;
  local_60[1] = 0;
  local_60[2] = 0;
  local_70[0] = 0;
  local_70[1] = 0;
  iVar2 = param_3 * param_2;
  local_70[2] = 0;
  local_84 = 0;
  local_80 = iVar2;
  local_7c = param_2;
  local_78 = param_3;
  local_74 = iVar2;
  _Memory = FUN_18002e460(param_2,param_3);
  uVar4 = FUN_180038860((longlong)param_1,param_2,param_3);
  FUN_18004d200((undefined8 *)*_Memory,(undefined8 *)*param_1,(longlong)iVar2);
  FUN_18004d980((undefined1 (*) [16])*param_4,0,(longlong)iVar2);
  uVar1 = local_7c;
  iVar9 = 1;
  if (param_3 + -0xc < 2) {
  }
  else {
    iVar2 = local_7c - 0xc;
    iVar8 = local_84;
    do {
      iVar6 = 1;
      if (1 < iVar2) {
        iVar8 = iVar8 + (uVar1 - 0xe) / 0xc + 1;
        do {
          iVar3 = FUN_180038a50((longlong)_Memory,iVar6,iVar9,local_88,(int)uVar4);
          iVar6 = iVar6 + 0xc;
          local_60[local_88[0]] = local_60[local_88[0]] + iVar3;
          local_70[local_88[0]] = local_70[local_88[0]] + 1;
        } while (iVar6 < iVar2);
      }
      iVar9 = iVar9 + 0xc;
    } while (iVar9 < param_3 + -0xc);
    iVar2 = local_74;
    iVar9 = local_70[1];
    local_84 = iVar8;
    if (0 < local_70[2]) {
      iVar8 = local_60[2] / local_70[2];
      goto LAB_180038df9;
    }
    if (0 < local_70[1]) {
      iVar8 = local_60[1] / local_70[1];
      goto LAB_180038df9;
    }
    iVar8 = local_60[0];
  }
  iVar8 = iVar8 / local_70[0];
  iVar9 = local_70[1];
LAB_180038df9:
  iVar3 = local_70[2];
  iVar6 = local_78;
  uVar1 = local_7c;
  lVar11 = (longlong)local_78;
  lVar7 = (longlong)(int)local_7c;
  FUN_18002e4f0(local_7c,local_78,(longlong)_Memory,0xc);
  if (0 < iVar6) {
    lVar10 = (longlong)_Memory - (longlong)param_4;
    do {
      lVar5 = 0;
      if (0 < (int)uVar1) {
        do {
          if (((0xff - iVar8) / 2 - (0xff - iVar8)) + 0xff <=
              (int)(uint)*(byte *)(*(longlong *)(lVar10 + (longlong)param_4) + lVar5)) {
            iVar2 = iVar2 + -1;
            *(undefined1 *)(lVar5 + *param_4) = 0xff;
          }
          lVar5 = lVar5 + 1;
        } while (lVar5 < lVar7);
      }
      param_4 = param_4 + 1;
      lVar11 = lVar11 + -1;
    } while (lVar11 != 0);
  }
  free(_Memory);
  if (((iVar3 == 0) && (iVar9 == 0)) && (local_70[0] == local_84)) {
    uVar4 = 0;
  }
  else {
    uVar4 = (longlong)(iVar2 << 7) / (longlong)local_80 & 0xffffffff;
  }
  return uVar4;
}

/* ================ FUN_180038ec0 ================ */
ulonglong FUN_180038ec0(longlong *param_1,uint param_2,uint param_3,longlong *param_4)
{
  int iVar1;
  byte *pbVar2;
  byte *pbVar3;
  byte *pbVar4;
  byte *pbVar5;
  byte *pbVar6;
  byte bVar7;
  int iVar8;
  int iVar9;
  void *_Memory;
  int *_Memory_00;
  int iVar10;
  int iVar11;
  int iVar12;
  byte *pbVar13;
  size_t _Count;
  ulonglong uVar14;
  longlong lVar15;
  int iVar16;
  int iVar17;
  longlong lVar18;
  byte *pbVar19;
  ulonglong uVar20;
  longlong lVar21;
  int iVar22;
  int iVar23;
  int *piVar24;
  longlong lVar25;
  longlong *plVar26;
  int iStack0000000000000028;
  int iStack0000000000000038;

  iVar10 = param_2 * param_3;
  iVar16 = 0;
  iStack0000000000000038 = 0;
  iStack0000000000000028 = iVar10;
  if (param_4 != (longlong *)0x0) {
    FUN_18004d980((undefined1 (*) [16])*param_4,0,(longlong)iVar10);
  }
  _Count = (size_t)(((int)param_2 / 6) *
                   ((int)param_3 / 6 + ((int)param_3 >> 0x1f) +
                   (int)(((longlong)(int)param_3 / 6 + ((longlong)(int)param_3 >> 0x3f) &
                         0xffffffffU) >> 0x1f)));
  _Memory = calloc(_Count,4);
  _Memory_00 = malloc(_Count * 4);
  iVar1 = param_3 - 6;
  iVar22 = 0;
  if (0 < iVar1) {
    lVar18 = 0;
    lVar25 = ((longlong)iVar1 - 1U) / 6 + 1;
    plVar26 = param_1;
    do {
      lVar15 = 0;
      if (0 < (int)(param_2 - 6)) {
        piVar24 = (int *)(lVar18 * 4 + (longlong)_Memory);
        uVar14 = (ulonglong)((param_2 - 7) / 6 + 1);
        lVar18 = lVar18 + uVar14;
        do {
          iVar22 = 0;
          lVar21 = 6;
          pbVar19 = (byte *)(*plVar26 + lVar15);
          pbVar13 = pbVar19;
          do {
            pbVar2 = pbVar13 + 4;
            pbVar3 = pbVar13 + 5;
            pbVar4 = pbVar13 + 3;
            pbVar5 = pbVar13 + 2;
            pbVar6 = pbVar13 + 1;
            bVar7 = *pbVar13;
            pbVar13 = pbVar13 + (int)param_2;
            iVar22 = iVar22 + (uint)bVar7 +
                     (uint)*pbVar3 + (uint)*pbVar2 + (uint)*pbVar4 + (uint)*pbVar5 + (uint)*pbVar6;
            lVar21 = lVar21 + -1;
          } while (lVar21 != 0);
          if (piVar24 != (int *)0x0) {
            iVar11 = 0;
            lVar21 = 6;
            iVar8 = iVar22 / 0x24;
            do {
              bVar7 = *pbVar19;
              pbVar13 = pbVar19 + 1;
              pbVar2 = pbVar19 + 2;
              pbVar3 = pbVar19 + 3;
              pbVar4 = pbVar19 + 4;
              pbVar5 = pbVar19 + 5;
              pbVar19 = pbVar19 + (int)param_2;
              iVar11 = iVar11 + ((uint)bVar7 - iVar8) * ((uint)bVar7 - iVar8) +
                       ((uint)*pbVar13 - iVar8) * ((uint)*pbVar13 - iVar8) +
                       ((uint)*pbVar2 - iVar8) * ((uint)*pbVar2 - iVar8) +
                       ((uint)*pbVar3 - iVar8) * ((uint)*pbVar3 - iVar8) +
                       ((uint)*pbVar4 - iVar8) * ((uint)*pbVar4 - iVar8) +
                       ((uint)*pbVar5 - iVar8) * ((uint)*pbVar5 - iVar8);
              lVar21 = lVar21 + -1;
            } while (lVar21 != 0);
            *piVar24 = iVar11 / 0x24;
          }
          *(int *)(((longlong)_Memory_00 - (longlong)_Memory) + (longlong)piVar24) = iVar22;
          if (iVar16 < *piVar24) {
            iVar16 = *piVar24;
          }
          piVar24 = piVar24 + 1;
          lVar15 = lVar15 + 6;
          uVar14 = uVar14 - 1;
        } while (uVar14 != 0);
      }
      plVar26 = plVar26 + 6;
      lVar25 = lVar25 + -1;
      iVar22 = iStack0000000000000038;
    } while (lVar25 != 0);
  }
  uVar20 = (ulonglong)(int)param_2;
  uVar14 = uVar20;
  if (0 < (int)param_3) {
    uVar14 = (ulonglong)param_3;
    do {
      lVar18 = 0;
      if (1 < (longlong)uVar20) {
        do {
          lVar18 = lVar18 + 2;
        } while (lVar18 < (longlong)(uVar20 - 1));
      }
      uVar14 = uVar14 - 1;
    } while (uVar14 != 0);
    uVar14 = (ulonglong)param_2;
  }
  iVar8 = (int)(iVar16 * 0xf + (iVar16 * 0xf >> 0x1f & 0x7fU)) >> 7;
  iVar16 = 0x14;
  if (0x14 < iVar8) {
    iVar16 = iVar8;
  }
  iVar8 = iVar22;
  if (0 < (longlong)_Count) {
    piVar24 = _Memory_00;
    do {
      if (iVar16 < *(int *)(((longlong)_Memory - (longlong)_Memory_00) + (longlong)piVar24)) {
        iVar8 = iVar8 + *piVar24;
        iVar22 = iVar22 + 1;
      }
      piVar24 = piVar24 + 1;
      _Count = _Count - 1;
    } while (_Count != 0);
    if (0 < iVar22) {
      iVar22 = (iVar8 / iVar22) * 9;
      iVar8 = (int)((iVar22 >> 0x1f & 7U) + iVar22) >> 3;
    }
  }
  iVar22 = iVar8 * 0x80 + 0x40;
  if (0 < (int)param_3) {
    lVar25 = -2;
    iVar8 = -2;
    lVar18 = (longlong)(int)param_3;
    plVar26 = param_4;
    do {
      lVar15 = 0;
      if (0 < (longlong)uVar20) {
        iVar11 = (int)uVar14 + -6;
        iVar17 = -2;
        do {
          iVar9 = iVar17;
          if (iVar17 < 0) {
            iVar9 = 0;
          }
          lVar21 = lVar25;
          iVar23 = iVar8;
          if (lVar25 < 0) {
            lVar21 = 0;
            iVar23 = 0;
          }
          if (iVar11 < iVar9) {
            iVar9 = iVar11;
          }
          if (iVar1 < iVar23) {
            lVar21 = (longlong)(int)param_3 + -6;
          }
          iVar23 = 0;
          pbVar19 = (byte *)((longlong)iVar9 + param_1[lVar21]);
          lVar21 = 6;
          pbVar13 = pbVar19;
          do {
            pbVar2 = pbVar13 + 4;
            pbVar3 = pbVar13 + 5;
            pbVar4 = pbVar13 + 3;
            pbVar5 = pbVar13 + 2;
            pbVar6 = pbVar13 + 1;
            bVar7 = *pbVar13;
            pbVar13 = pbVar13 + uVar20;
            iVar23 = iVar23 + (uint)bVar7 +
                     (uint)*pbVar3 + (uint)*pbVar2 + (uint)*pbVar4 + (uint)*pbVar5 + (uint)*pbVar6;
            lVar21 = lVar21 + -1;
          } while (lVar21 != 0);
          iVar12 = 0;
          lVar21 = 6;
          iVar9 = iVar23 / 0x24;
          do {
            bVar7 = *pbVar19;
            pbVar13 = pbVar19 + 1;
            pbVar2 = pbVar19 + 2;
            pbVar3 = pbVar19 + 3;
            pbVar4 = pbVar19 + 4;
            pbVar5 = pbVar19 + 5;
            pbVar19 = pbVar19 + uVar20;
            iVar12 = iVar12 + ((uint)bVar7 - iVar9) * ((uint)bVar7 - iVar9) +
                     ((uint)*pbVar13 - iVar9) * ((uint)*pbVar13 - iVar9) +
                     ((uint)*pbVar2 - iVar9) * ((uint)*pbVar2 - iVar9) +
                     ((uint)*pbVar3 - iVar9) * ((uint)*pbVar3 - iVar9) +
                     ((uint)*pbVar4 - iVar9) * ((uint)*pbVar4 - iVar9) +
                     ((uint)*pbVar5 - iVar9) * ((uint)*pbVar5 - iVar9);
            lVar21 = lVar21 + -1;
          } while (lVar21 != 0);
          if (((iVar12 / 0x24 < 0xf) ||
              ((int)(iVar22 + (iVar22 >> 0x1f & 0x7fU)) >> 7 <= iVar23 && iVar12 / 0x24 <= iVar16))
             && (iStack0000000000000028 = iStack0000000000000028 + -1, param_4 != (longlong *)0x0))
          {
            *(undefined1 *)(lVar15 + *plVar26) = 0xff;
          }
          lVar15 = lVar15 + 1;
          iVar17 = iVar17 + 1;
        } while (lVar15 < (longlong)uVar20);
      }
      uVar14 = (ulonglong)param_2;
      iVar8 = iVar8 + 1;
      lVar25 = lVar25 + 1;
      plVar26 = plVar26 + 1;
      lVar18 = lVar18 + -1;
    } while (lVar18 != 0);
  }
  free(_Memory);
  free(_Memory_00);
  return (longlong)(iStack0000000000000028 << 7) / (longlong)iVar10 & 0xffffffff;
}

/* ================ FUN_180039420 ================ */
void FUN_180039420(longlong *param_1,int param_2,uint param_3)
{
  int iVar1;
  undefined8 *_Memory;
  longlong lVar2;
  ulonglong uVar3;
  longlong lVar4;

  _Memory = FUN_18002e460(param_2,param_3);
  FUN_18004d200((undefined8 *)*_Memory,(undefined8 *)*param_1,(longlong)(int)(param_2 * param_3));
  FUN_18002e4f0(param_2,param_3,(longlong)_Memory,3);
  if (0 < (int)param_3) {
    uVar3 = (ulonglong)param_3;
    lVar4 = (longlong)_Memory - (longlong)param_1;
    do {
      lVar2 = 0;
      if (0 < (longlong)param_2) {
        do {
          iVar1 = ((uint)*(byte *)(*param_1 + lVar2) -
                  (uint)*(byte *)(lVar2 + *(longlong *)(lVar4 + (longlong)param_1))) + 0x80;
          if (iVar1 < 0) {
            iVar1 = 0;
          }
          else if (0xff < iVar1) {
            iVar1 = 0xff;
          }
          *(char *)(*param_1 + lVar2) = (char)iVar1;
          lVar2 = lVar2 + 1;
        } while (lVar2 < param_2);
      }
      param_1 = param_1 + 1;
      uVar3 = uVar3 - 1;
    } while (uVar3 != 0);
  }
  free(_Memory);
  return;
}

/* ================ FUN_1800394f0 ================ */
void FUN_1800394f0(longlong *param_1,int param_2,int param_3)
{
  longlong lVar1;
  char *pcVar2;
  longlong lVar3;
  longlong *plVar4;
  int iVar5;
  char *pcVar6;
  ulonglong uVar7;
  longlong lVar8;
  longlong lVar9;
  longlong lVar10;

  pcVar6 = (char *)*param_1;
  lVar9 = 1;
  iVar5 = param_3 + -1;
  lVar10 = (longlong)param_2;
  if (1 < iVar5) {
    lVar8 = 1;
    uVar7 = (ulonglong)(param_3 - 2);
    do {
      lVar3 = 1;
      if (1 < (longlong)(param_2 + -1)) {
        do {
          if (*(char *)(lVar3 + param_1[lVar8]) == -1) {
            if (*(char *)(param_1[lVar8 + -1] + -1 + lVar3) != -1) {
              *(undefined1 *)(param_1[lVar8 + -1] + -1 + lVar3) = 0x80;
            }
            if (*(char *)(param_1[lVar8 + -1] + lVar3) != -1) {
              *(undefined1 *)(param_1[lVar8 + -1] + lVar3) = 0x80;
            }
            if (*(char *)(param_1[lVar8 + -1] + 1 + lVar3) != -1) {
              *(undefined1 *)(param_1[lVar8 + -1] + 1 + lVar3) = 0x80;
            }
            if (*(char *)(lVar3 + 1 + param_1[lVar8]) != -1) {
              *(undefined1 *)(lVar3 + 1 + param_1[lVar8]) = 0x80;
            }
            if (*(char *)(param_1[lVar8 + 1] + 1 + lVar3) != -1) {
              *(undefined1 *)(param_1[lVar8 + 1] + 1 + lVar3) = 0x80;
            }
            if (*(char *)(param_1[lVar8 + 1] + lVar3) != -1) {
              *(undefined1 *)(param_1[lVar8 + 1] + lVar3) = 0x80;
            }
            if (*(char *)(param_1[lVar8 + 1] + -1 + lVar3) != -1) {
              *(undefined1 *)(param_1[lVar8 + 1] + -1 + lVar3) = 0x80;
            }
            if (*(char *)(lVar3 + -1 + param_1[lVar8]) != -1) {
              *(undefined1 *)(lVar3 + -1 + param_1[lVar8]) = 0x80;
            }
          }
          lVar3 = lVar3 + 1;
        } while (lVar3 < param_2 + -1);
      }
      lVar8 = lVar8 + 1;
      uVar7 = uVar7 - 1;
    } while (uVar7 != 0);
  }
  lVar8 = 1;
  lVar3 = (longlong)(param_2 + -1);
  if (1 < lVar3) {
    do {
      lVar1 = *param_1;
      if (*(char *)(lVar8 + lVar1) == -1) {
        if (*(char *)(lVar8 + 1 + lVar1) != -1) {
          *(undefined1 *)(lVar8 + 1 + lVar1) = 0x80;
        }
        if (*(char *)(lVar8 + 1 + param_1[1]) != -1) {
          *(undefined1 *)(lVar8 + 1 + param_1[1]) = 0x80;
        }
        if (*(char *)(lVar8 + param_1[1]) != -1) {
          *(undefined1 *)(lVar8 + param_1[1]) = 0x80;
        }
        if (*(char *)(lVar8 + -1 + param_1[1]) != -1) {
          *(undefined1 *)(lVar8 + -1 + param_1[1]) = 0x80;
        }
        if (*(char *)(lVar8 + -1 + *param_1) != -1) {
          *(undefined1 *)(lVar8 + -1 + *param_1) = 0x80;
        }
      }
      lVar8 = lVar8 + 1;
    } while (lVar8 < lVar3);
    if (1 < lVar3) {
      do {
        if (*(char *)(lVar9 + param_1[(longlong)param_3 + -1]) == -1) {
          if (*(char *)(lVar9 + -1 + param_1[(longlong)param_3 + -2]) != -1) {
            *(undefined1 *)(lVar9 + -1 + param_1[(longlong)param_3 + -2]) = 0x80;
          }
          if (*(char *)(lVar9 + param_1[(longlong)param_3 + -2]) != -1) {
            *(undefined1 *)(lVar9 + param_1[(longlong)param_3 + -2]) = 0x80;
          }
          if (*(char *)(lVar9 + 1 + param_1[(longlong)param_3 + -2]) != -1) {
            *(undefined1 *)(lVar9 + 1 + param_1[(longlong)param_3 + -2]) = 0x80;
          }
          if (*(char *)(lVar9 + 1 + param_1[(longlong)param_3 + -1]) != -1) {
            *(undefined1 *)(lVar9 + 1 + param_1[(longlong)param_3 + -1]) = 0x80;
          }
          if (*(char *)(lVar9 + -1 + param_1[(longlong)param_3 + -1]) != -1) {
            *(undefined1 *)(lVar9 + -1 + param_1[(longlong)param_3 + -1]) = 0x80;
          }
        }
        lVar9 = lVar9 + 1;
      } while (lVar9 < lVar3);
    }
  }
  if (1 < iVar5) {
    plVar4 = param_1 + 2;
    uVar7 = (ulonglong)(param_3 - 2);
    do {
      if (*(char *)plVar4[-1] == -1) {
        if (*(char *)plVar4[-2] != -1) {
          *(char *)plVar4[-2] = -0x80;
        }
        if (*(char *)(plVar4[-2] + 1) != -1) {
          *(undefined1 *)(plVar4[-2] + 1) = 0x80;
        }
        if (*(char *)(plVar4[-1] + 1) != -1) {
          *(undefined1 *)(plVar4[-1] + 1) = 0x80;
        }
        if (*(char *)(*plVar4 + 1) != -1) {
          *(undefined1 *)(*plVar4 + 1) = 0x80;
        }
        if (*(char *)*plVar4 != -1) {
          *(char *)*plVar4 = -0x80;
        }
      }
      plVar4 = plVar4 + 1;
      uVar7 = uVar7 - 1;
    } while (uVar7 != 0);
    if (1 < iVar5) {
      plVar4 = param_1 + 2;
      uVar7 = (ulonglong)(param_3 - 2);
      do {
        if (*(char *)(plVar4[-1] + -1 + lVar10) == -1) {
          if (*(char *)(plVar4[-2] + -2 + lVar10) != -1) {
            *(undefined1 *)(plVar4[-2] + -2 + lVar10) = 0x80;
          }
          if (*(char *)(plVar4[-2] + -1 + lVar10) != -1) {
            *(undefined1 *)(plVar4[-2] + -1 + lVar10) = 0x80;
          }
          if (*(char *)(lVar10 + -1 + *plVar4) != -1) {
            *(undefined1 *)(lVar10 + -1 + *plVar4) = 0x80;
          }
          if (*(char *)(lVar10 + -2 + *plVar4) != -1) {
            *(undefined1 *)(lVar10 + -2 + *plVar4) = 0x80;
          }
          if (*(char *)(plVar4[-1] + -2 + lVar10) != -1) {
            *(undefined1 *)(plVar4[-1] + -2 + lVar10) = 0x80;
          }
        }
        plVar4 = plVar4 + 1;
        uVar7 = uVar7 - 1;
      } while (uVar7 != 0);
    }
  }
  pcVar2 = (char *)*param_1;
  if (*pcVar2 == -1) {
    if (pcVar2[1] != -1) {
      pcVar2[1] = -0x80;
    }
    if (*(char *)(param_1[1] + 1) != -1) {
      *(undefined1 *)(param_1[1] + 1) = 0x80;
    }
    if (*(char *)param_1[1] != -1) {
      *(char *)param_1[1] = -0x80;
    }
  }
  if (*(char *)(lVar10 + -1 + *param_1) == -1) {
    if (*(char *)(lVar10 + -1 + param_1[1]) != -1) {
      *(undefined1 *)(lVar10 + -1 + param_1[1]) = 0x80;
    }
    if (*(char *)(lVar10 + -2 + param_1[1]) != -1) {
      *(undefined1 *)(lVar10 + -2 + param_1[1]) = 0x80;
    }
    if (*(char *)(lVar10 + -2 + *param_1) != -1) {
      *(undefined1 *)(lVar10 + -2 + *param_1) = 0x80;
    }
  }
  if (*(char *)(lVar10 + -1 + param_1[(longlong)param_3 + -1]) == -1) {
    if (*(char *)(lVar10 + -2 + param_1[(longlong)param_3 + -2]) != -1) {
      *(undefined1 *)(lVar10 + -2 + param_1[(longlong)param_3 + -2]) = 0x80;
    }
    if (*(char *)(lVar10 + -1 + param_1[(longlong)param_3 + -2]) != -1) {
      *(undefined1 *)(lVar10 + -1 + param_1[(longlong)param_3 + -2]) = 0x80;
    }
    if (*(char *)(lVar10 + -2 + param_1[(longlong)param_3 + -1]) != -1) {
      *(undefined1 *)(lVar10 + -2 + param_1[(longlong)param_3 + -1]) = 0x80;
    }
  }
  if (*(char *)param_1[(longlong)param_3 + -1] == -1) {
    if (*(char *)param_1[(longlong)param_3 + -2] != -1) {
      *(char *)param_1[(longlong)param_3 + -2] = -0x80;
    }
    if (*(char *)(param_1[(longlong)param_3 + -2] + 1) != -1) {
      *(undefined1 *)(param_1[(longlong)param_3 + -2] + 1) = 0x80;
    }
    if (*(char *)(param_1[(longlong)param_3 + -1] + 1) != -1) {
      *(undefined1 *)(param_1[(longlong)param_3 + -1] + 1) = 0x80;
    }
  }
  lVar9 = (longlong)(param_2 * param_3);
  if (0 < lVar9) {
    do {
      if (*pcVar6 == -0x80) {
        *pcVar6 = -1;
      }
      pcVar6 = pcVar6 + 1;
      lVar9 = lVar9 + -1;
    } while (lVar9 != 0);
  }
  return;
}

/* ================ FUN_1800398e0 ================ */
void FUN_1800398e0(int param_1,int param_2,longlong param_3,int param_4)
{
  longlong lVar1;
  longlong lVar2;
  int iVar3;
  longlong lVar4;
  void *_Memory;
  int *_Memory_00;
  void *pvVar5;
  int *piVar6;
  longlong lVar7;
  byte *pbVar8;
  size_t _Count;
  longlong lVar9;
  undefined8 *puVar10;
  int iVar11;
  size_t sVar12;
  int iVar13;
  int iVar14;

  lVar4 = (longlong)param_2;
  _Count = (size_t)param_1;
  iVar3 = param_2;
  if (5 < param_2) {
    iVar3 = 5;
  }
  iVar14 = 0;
  _Memory = malloc(lVar4 * 8);
  _Memory_00 = calloc(_Count,4);
  lVar9 = 0;
  if (0 < (longlong)iVar3) {
    do {
      pvVar5 = malloc(_Count);
      lVar9 = lVar9 + 1;
      *(void **)((longlong)_Memory + lVar9 * 8 + -8) = pvVar5;
    } while (lVar9 < iVar3);
  }
  if (-2 < lVar4) {
    puVar10 = (undefined8 *)((longlong)_Memory + -0x28);
    lVar9 = -2;
    do {
      if (-1 < lVar9 + -3) {
        pbVar8 = (byte *)*puVar10;
        piVar6 = _Memory_00;
        sVar12 = _Count;
        if (0 < (longlong)_Count) {
          do {
            if ((int)(uint)*pbVar8 < param_4) {
              *piVar6 = *piVar6 + ((uint)*pbVar8 - param_4);
            }
            pbVar8 = pbVar8 + 1;
            sVar12 = sVar12 - 1;
            piVar6 = piVar6 + 1;
          } while (sVar12 != 0);
        }
        iVar14 = iVar14 + -1;
      }
      if (lVar9 + 2 < lVar4) {
        if (-1 < lVar9 + -3) {
          puVar10[5] = *puVar10;
        }
        FUN_18004d200((undefined8 *)puVar10[5],
                      *(undefined8 **)((param_3 - (longlong)_Memory) + 0x28 + (longlong)puVar10),
                      _Count);
        pbVar8 = (byte *)puVar10[5];
        piVar6 = _Memory_00;
        sVar12 = _Count;
        if (0 < (longlong)_Count) {
          do {
            if ((int)(uint)*pbVar8 < param_4) {
              *piVar6 = *piVar6 + (param_4 - (uint)*pbVar8);
            }
            pbVar8 = pbVar8 + 1;
            sVar12 = sVar12 - 1;
            piVar6 = piVar6 + 1;
          } while (sVar12 != 0);
        }
        iVar14 = iVar14 + 1;
      }
      if (-1 < lVar9) {
        lVar2 = *(longlong *)((param_3 - (longlong)_Memory) + 0x18 + (longlong)puVar10);
        iVar11 = 0;
        iVar13 = 0;
        if (-2 < (longlong)_Count) {
          lVar7 = 0;
          piVar6 = _Memory_00;
          do {
            if (-1 < lVar7 + -5) {
              iVar11 = iVar11 - piVar6[-5];
              iVar13 = iVar13 - iVar14;
            }
            if (lVar7 < (longlong)_Count) {
              iVar11 = iVar11 + *piVar6;
              iVar13 = iVar13 + iVar14;
            }
            if (-1 < lVar7 + -2) {
              *(char *)(lVar2 + -2 + lVar7) = (char)(iVar11 / iVar13);
            }
            piVar6 = piVar6 + 1;
            lVar1 = lVar7 + -1;
            lVar7 = lVar7 + 1;
          } while (lVar1 < (longlong)_Count);
        }
      }
      lVar9 = lVar9 + 1;
      puVar10 = puVar10 + 1;
    } while (lVar9 < lVar4);
  }
  for (lVar9 = (longlong)(param_2 - iVar3); lVar9 < lVar4; lVar9 = lVar9 + 1) {
    free(*(void **)((longlong)_Memory + lVar9 * 8));
  }
  free(_Memory);
  free(_Memory_00);
  return;
}

/* ================ FUN_180039af0 ================ */
void FUN_180039af0(longlong *param_1,int param_2,int param_3)
{
  byte *pbVar1;
  byte bVar2;
  longlong lVar3;
  undefined8 *_Memory;
  int iVar4;
  longlong lVar5;
  undefined1 uVar6;
  int iVar7;
  uint uVar8;
  int iVar9;
  int iVar10;
  longlong lVar11;
  longlong lVar12;
  longlong lVar13;
  undefined1 auStack_488 [48];
  int local_458;
  undefined8 *local_450;
  int local_448 [256];
  ulonglong local_48;

  local_48 = DAT_180092040 ^ (ulonglong)auStack_488;
  lVar12 = (longlong)param_2;
  lVar13 = (longlong)param_3;
  uVar8 = 0;
  local_458 = param_3 * param_2;
  iVar10 = (local_458 >> 0x1f & 7U) + local_458;
  local_450 = FUN_18002e460(param_2,param_3);
  FUN_18004d980((undefined1 (*) [16])local_448,0,0x400);
  _Memory = local_450;
  lVar11 = 0;
  if (0 < param_3) {
    do {
      lVar5 = 0;
      if (0 < param_2) {
        lVar3 = param_1[lVar11];
        do {
          pbVar1 = (byte *)(lVar3 + lVar5);
          lVar5 = lVar5 + 1;
          local_448[*pbVar1] = local_448[*pbVar1] + 1;
        } while (lVar5 < lVar12);
      }
      lVar11 = lVar11 + 1;
    } while (lVar11 < lVar13);
  }
  iVar4 = 0;
  iVar7 = 0xff;
  lVar11 = 0xff;
  do {
    iVar4 = iVar4 + local_448[lVar11];
    iVar9 = iVar7;
    if (iVar10 >> 3 < iVar4) break;
    iVar7 = iVar7 + -1;
    lVar11 = lVar11 + -1;
    iVar9 = 0xff;
  } while (-1 < lVar11);
  FUN_18004d200((undefined8 *)*local_450,(undefined8 *)*param_1,(longlong)local_458);
  FUN_1800398e0(param_2,param_3,(longlong)_Memory,iVar9);
  lVar11 = 0;
  if (0 < param_3) {
    do {
      lVar5 = 0;
      if (0 < param_2) {
        do {
          if (uVar8 < *(byte *)(_Memory[lVar11] + lVar5)) {
            uVar8 = (uint)*(byte *)(_Memory[lVar11] + lVar5);
          }
          lVar5 = lVar5 + 1;
        } while (lVar5 < lVar12);
      }
      lVar11 = lVar11 + 1;
    } while (lVar11 < lVar13);
    if (0 < param_3) {
      lVar11 = (longlong)_Memory - (longlong)param_1;
      do {
        lVar5 = 0;
        if (0 < param_2) {
          do {
            bVar2 = *(byte *)(lVar5 + *(longlong *)(lVar11 + (longlong)param_1));
            if (bVar2 == 0) {
              *(char *)(lVar5 + *param_1) = (char)iVar9;
            }
            else {
              lVar3 = *param_1;
              if ((int)(uint)*(byte *)(lVar3 + lVar5) < iVar9) {
                iVar10 = iVar9 - (int)((iVar9 - (uint)*(byte *)(lVar3 + lVar5)) * uVar8) /
                                 (int)(uint)bVar2;
                uVar6 = 0;
                if (0 < iVar10) {
                  uVar6 = (char)iVar10;
                }
                *(undefined1 *)(lVar3 + lVar5) = uVar6;
              }
              else {
                *(char *)(lVar3 + lVar5) = (char)iVar9;
              }
            }
            lVar5 = lVar5 + 1;
          } while (lVar5 < lVar12);
        }
        param_1 = param_1 + 1;
        lVar13 = lVar13 + -1;
      } while (lVar13 != 0);
    }
  }
  free(_Memory);
  return;
}

/* ================ FUN_180039cc0 ================ */
int FUN_180039cc0(int param_1)
{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;

  puVar1 = DAT_18009d018;
  iVar2 = 0;
  iVar3 = 0;
  *(int*)DAT_18009d018 = 0x10;
  *(undefined8 *)(puVar1 + 1) = 1000;
  puVar1[3] = 3;
  puVar1[4] = 9;
  puVar1[5] = 0x700;
  puVar1[6] = 1;
  puVar1[7] = 2;
  *(undefined8 *)(puVar1 + 8) = 2;
  *(undefined8 *)(puVar1 + 10) = 1;
  *(undefined8 *)(puVar1 + 0xc) = 0;
  puVar1[0xe] = 0;
  puVar1[0xf] = 0x288;
  puVar1[0x11] = 0x400;
  puVar1[0x12] = 0x73;
  puVar1[0x13] = 7;
  puVar1[0x14] = 8;
  puVar1[0x16] = 5;
  puVar1[0x17] = 0xffffffff;
  *(undefined8 *)(puVar1 + 0x18) = 0x18;
  *(undefined8 *)(puVar1 + 0x24) = 0;
  puVar1[0x1b] = 10;
  puVar1[0x1c] = 0x14;
  puVar1[0x1d] = 5;
  puVar1[0x1e] = 0xf0;
  puVar1[0x1f] = 1;
  puVar1[0x20] = 0x14;
  *(undefined8 *)(puVar1 + 0x22) = 170000;
  puVar1[0x1a] = 100;
  puVar1[0x26] = 2;
  switch(param_1) {
  case 5:
    puVar1[0x10] = 700;
    puVar1[0x15] = 0x294;
    puVar1[0x1a] = 0x55;
    puVar1[0x21] = 0x2df;
    break;
  default:
    iVar3 = -0x3ea;
    break;
  case 7:
  case 0x87:
    puVar1[1] = 0x4b0;
    puVar1[0x10] = 0x2d0;
    puVar1[0x15] = 0x26c;
    puVar1[0x17] = 8;
    puVar1[0x18] = 0x1c;
    puVar1[0x21] = 0x366;
    break;
  case 8:
  case 0x88:
    puVar1[0x10] = 900;
    puVar1[0x15] = 0x294;
    puVar1[0x16] = 7;
    puVar1[0x17] = 6;
    puVar1[0x18] = 0x24;
    puVar1[0x19] = 1;
    puVar1[0x21] = 0x44c;
    puVar1[0xb] = 1;
    puVar1[0x25] = 0x20;
    break;
  case 9:
  case 0x89:
    puVar1[0x10] = 0x2d0;
    puVar1[0x15] = 0x294;
    puVar1[0x17] = 7;
    puVar1[0x18] = 0x1a;
    puVar1[0x21] = 0x3c5;
    puVar1[0x25] = 0xc;
    puVar1[0xf] = 0x25c;
    iVar3 = iVar2;
    break;
  case 0xb:
    puVar1[5] = 0x400;
    goto LAB_180039f19;
  case 0xe:
    puVar1[1] = 0x4b0;
    puVar1[0x10] = 0x2d0;
    puVar1[0x15] = 0x26c;
    puVar1[0x17] = 8;
    puVar1[0x18] = 0x1c;
    puVar1[0x21] = 0x2df;
    break;
  case 0x10:
    puVar1[1] = 0x4b5;
    puVar1[2] = 1;
    puVar1[5] = 0x467;
    puVar1[0x10] = 0x3aa;
    puVar1[0x12] = 0x7f;
    puVar1[0x13] = 8;
    puVar1[0x14] = 7;
    puVar1[0x15] = 0x294;
    puVar1[0x21] = 0x2df;
    puVar1[0xc] = 1;
    break;
  case 0x11:
    puVar1[5] = 0x500;
LAB_180039f19:
    puVar1[0x10] = 700;
    goto LAB_180039f20;
  case 0x12:
    puVar1[1] = 0x4e7;
    puVar1[5] = 0x500;
    puVar1[0x10] = 0x368;
    puVar1[0x12] = 0x7b;
    puVar1[0x13] = 9;
    puVar1[0x14] = 6;
    puVar1[0x17] = 7;
    puVar1[0x18] = 0x20;
    goto LAB_180039f20;
  case 0x13:
  case 0x93:
    *puVar1 = 0xf;
    puVar1[5] = 0x780;
    puVar1[0x10] = 0x2d0;
    puVar1[0x15] = 0x26c;
    puVar1[0x17] = 7;
    puVar1[0x18] = 0x1a;
    puVar1[0x21] = 0x29e;
    iVar3 = iVar2;
    break;
  case 0x14:
    puVar1[1] = 0x4b5;
    puVar1[5] = 0x467;
    puVar1[0x10] = 0x3aa;
    puVar1[0x12] = 0x7f;
    puVar1[0x13] = 8;
    puVar1[0x14] = 7;
LAB_180039f20:
    puVar1[0x15] = 0x294;
    puVar1[0x21] = 0x2df;
    break;
  case 0x15:
  case 0x95:
    puVar1[1] = 0;
    puVar1[2] = 1;
    puVar1[0x10] = 800;
    puVar1[0x12] = 0x7d;
    puVar1[0x15] = 0x226;
    puVar1[0x16] = 7;
    puVar1[0x17] = 7;
    puVar1[0x18] = 0x18;
    puVar1[0x19] = 1;
    puVar1[0x21] = 0x307;
    *(undefined8 *)(puVar1 + 0x25) = 0x14;
    iVar3 = iVar2;
    break;
  case 0x16:
    puVar1[5] = 0x800;
    puVar1[0x10] = 0x2d0;
    puVar1[0x12] = 0x6c;
    puVar1[0x15] = 500;
    puVar1[0x17] = 7;
    puVar1[0x18] = 0x1e;
    puVar1[0x1b] = 0;
    puVar1[0x21] = 0x226;
    puVar1[0xd] = 400;
    puVar1[0xe] = 1000;
    break;
  case 0x17:
    puVar1[5] = 0x938;
    puVar1[0xd] = 400;
    puVar1[0x10] = 700;
    puVar1[0x12] = 0x75;
    puVar1[0x15] = 0x1c2;
    puVar1[0x16] = 6;
    puVar1[0x17] = 8;
    puVar1[0x18] = 0x18;
    puVar1[0x19] = 1;
    puVar1[0x1a] = 0x55;
    puVar1[0x21] = 500;
  }
  if ((((param_1 == 0x87) || (param_1 == 0x88)) || (param_1 == 0x89)) ||
     ((param_1 == 0x93 || (param_1 == 0x95)))) {
    puVar1[0x25] = puVar1[0x25] + 8;
    puVar1[0x21] = puVar1[0x15];
  }
  else if (iVar3 != 0) {
    return -0x3ea;
  }
  return iVar3;
}

/* ================ FUN_18003a200 ================ */
void FUN_18003a200(undefined8 *param_1)
{
  if ((void *)*param_1 != (void *)0x0) {
    free((void *)*param_1);
  }
  if ((void *)param_1[1] != (void *)0x0) {
    free((void *)param_1[1]);
  }
  if ((void *)param_1[5] != (void *)0x0) {
    free((void *)param_1[5]);
  }
  if ((void *)param_1[6] != (void *)0x0) {
    free((void *)param_1[6]);
  }
  if ((void *)param_1[2] != (void *)0x0) {
    free((void *)param_1[2]);
  }
  if ((void *)param_1[3] != (void *)0x0) {
    free((void *)param_1[3]);
  }
  if ((void *)param_1[4] != (void *)0x0) {
    free((void *)param_1[4]);
  }
  if ((void *)param_1[7] != (void *)0x0) {
    free((void *)param_1[7]);
  }
  free(param_1);
  return;
}

/* ================ FUN_18003a290 ================ */
void FUN_18003a290(ulonglong param_1,ulonglong param_2,uint param_3)
{
  byte *pbVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  v128 auVar6;
  uint uVar7;
  longlong lVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  v128 in_XMM1;
  v128 auVar12;
  v128 auVar13;

  in_XMM1 = v_zero();
  auVar6 = v_load((const void *)_DAT_1800898b0);
  iVar5 = _UNK_1800898ac;
  iVar4 = _UNK_1800898a8;
  iVar3 = _UNK_1800898a4;
  iVar2 = _DAT_1800898a0;
  iVar10 = 0;
  iVar9 = 0;
  iVar11 = iVar10;
  if ((((0 < (int)param_3) && (iVar11 = iVar9, 7 < param_3)) && (1 < DAT_1800920bc)) &&
     (((longlong)(int)(param_3 - 1) + param_2 < param_1 ||
      (param_1 + (longlong)(int)(param_3 - 1) * 4 < param_2)))) {
    uVar7 = param_3 & 0x80000007;
    if ((int)uVar7 < 0) {
      uVar7 = (uVar7 - 1 | 0xfffffff8) + 1;
    }
    do {
      auVar12 = pmovzxbd(in_XMM1,ZEXT416(*(uint *)((longlong)iVar10 + param_2)));
      auVar12 = pmulld(auVar12,auVar6);
      auVar13.i[0] = auVar12.i[0] + iVar2 >> 0x10;
      auVar13.i[1] = auVar12.i[1] + iVar3 >> 0x10;
      auVar13.i[2] = auVar12.i[2] + iVar4 >> 0x10;
      auVar13.i[3] = auVar12.i[3] + iVar5 >> 0x10;
      v_store((void *)(param_1 + (longlong)iVar10 * 4), auVar13);
      iVar11 = iVar10 + 4;
      iVar10 = iVar10 + 8;
      auVar12 = pmovzxbd(auVar13,ZEXT416(*(uint *)((longlong)iVar11 + param_2)));
      auVar12 = pmulld(auVar12,auVar6);
      in_XMM1.i[0] = auVar12.i[0] + iVar2 >> 0x10;
      in_XMM1.i[1] = auVar12.i[1] + iVar3 >> 0x10;
      in_XMM1.i[2] = auVar12.i[2] + iVar4 >> 0x10;
      in_XMM1.i[3] = auVar12.i[3] + iVar5 >> 0x10;
      v_store((void *)(param_1 + (longlong)iVar11 * 4), in_XMM1);
      iVar11 = iVar10;
    } while (iVar10 < (int)(param_3 - uVar7));
  }
  lVar8 = (longlong)iVar11;
  while (lVar8 < (int)param_3) {
    pbVar1 = (byte *)(lVar8 + param_2);
    lVar8 = lVar8 + 1;
    *(int *)((param_1 - 4) + lVar8 * 4) = (int)((uint)*pbVar1 * 0x202020 + 0x8000) >> 0x10;
  }
  return;
}

/* ================ FUN_18003a3a0 ================ */
int * FUN_18003a3a0(int *param_1,int param_2,int param_3,int param_4)
{
  int *piVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int *piVar6;
  uint *puVar7;
  int *piVar8;
  int *piVar9;
  ulonglong uVar10;
  ulonglong uVar11;
  uint uVar12;
  longlong lVar13;
  ulonglong uVar14;
  ulonglong uVar15;
  ulonglong uVar16;
  uint uVar17;
  uint uVar18;
  longlong lVar19;
  uint uVar20;
  longlong lVar21;
  undefined1 auStack_188 [32];
  int local_168;
  int local_164;
  int *local_160;
  int *local_158;
  uint local_150;
  longlong local_148;
  /* param_4==4 packed stack arrays (Ghidra split these into overlapping scalars) */
  int W[8];      /* local_100 */
  int H[8];      /* local_140 */
  int srcx[8];   /* local_c0 */
  int srcy[8];   /* local_a0 (+ local_98/uStack_90/local_88/local_84) */
  int dstx[8];   /* local_120 */
  int dsty[8];   /* local_e0 */
  int flipx[8];  /* local_80 (+ uStack_78/local_70/local_68) */
  int flipy[8];  /* local_60 (+ uStack_58/local_50/uStack_48) */
  ulonglong local_40;

  local_40 = DAT_180092040 ^ (ulonglong)auStack_188;
  lVar19 = (longlong)param_2;
  lVar21 = (longlong)param_3;
  local_168 = param_2;
  local_164 = param_3;
  local_158 = param_1;
  piVar6 = FUN_180035fc0(*param_1 + param_2 * 2,param_1[1] + param_3 * 2);
  local_160 = piVar6;
  if (piVar6 != (int *)0x0) {
    if (param_4 == 0) {
      FUN_1800361d0((longlong)piVar6,param_2,param_3,param_1);
      uVar14 = 0;
      local_148 = lVar21;
      if (0 < lVar21) {
        lVar13 = (longlong)(piVar6[1] - param_3) * 8;
        uVar11 = uVar14;
        do {
          FUN_18004d200((undefined8 *)
                        (*(longlong *)(*(longlong *)(piVar6 + 2) + uVar11 * 8) + lVar19 * 4),
                        (undefined8 *)**(undefined8 **)(param_1 + 2),(longlong)*param_1 << 2);
          FUN_18004d200((undefined8 *)
                        (*(longlong *)(lVar13 + *(longlong *)(piVar6 + 2)) + lVar19 * 4),
                        *(undefined8 **)(*(longlong *)(param_1 + 2) + -8 + (longlong)param_1[1] * 8)
                        ,(longlong)*param_1 << 2);
          uVar11 = uVar11 + 1;
          lVar13 = lVar13 + 8;
          param_2 = local_168;
        } while ((longlong)uVar11 < lVar21);
      }
      puVar7 = (uint *)FUN_180035fc0(1,param_1[1]);
      if (puVar7 != (uint *)0x0) {
        FUN_180036060((longlong)param_1,0,0,puVar7);
        uVar11 = uVar14;
        if (0 < param_2) {
          do {
            FUN_1800361d0((longlong)piVar6,(int)uVar11,param_3,(int *)puVar7);
            uVar12 = (int)uVar11 + 1;
            uVar11 = (ulonglong)uVar12;
            param_1 = local_158;
          } while ((int)uVar12 < param_2);
        }
        piVar8 = local_158;
        uVar12 = *puVar7;
        iVar3 = *param_1;
        if (((uVar12 & 0x3fffffff) != 0) && (param_2 = local_168, 0 < (int)puVar7[1])) {
          uVar11 = uVar14;
          uVar16 = uVar14;
          do {
            FUN_18004d200(*(undefined8 **)(uVar11 + *(longlong *)(puVar7 + 2)),
                          (undefined8 *)
                          (*(longlong *)(uVar11 + *(longlong *)(piVar8 + 2)) +
                          (longlong)(iVar3 + -1) * 4),(longlong)(int)(uVar12 << 2));
            uVar17 = (int)uVar16 + 1;
            uVar16 = (ulonglong)uVar17;
            uVar11 = uVar11 + 8;
            piVar6 = local_160;
            param_2 = local_168;
          } while ((int)uVar17 < (int)puVar7[1]);
        }
        local_150 = 0;
        if (0 < param_2) {
          uVar12 = puVar7[1];
          do {
            lVar21 = local_148;
            piVar8 = local_160;
            uVar11 = 0;
            iVar3 = *piVar6;
            uVar17 = *puVar7;
            uVar20 = (uint)uVar14;
            if (0 < (int)uVar12) {
              uVar16 = uVar11;
              do {
                FUN_18004d200((undefined8 *)
                              (*(longlong *)(*(longlong *)(piVar8 + 2) + uVar11 + lVar21 * 8) +
                              (longlong)(int)((iVar3 - (uint)uVar14) + -1) * 4),
                              *(undefined8 **)(uVar11 + *(longlong *)(puVar7 + 2)),
                              (longlong)(int)(uVar17 << 2));
                uVar12 = puVar7[1];
                uVar18 = (int)uVar16 + 1;
                uVar16 = (ulonglong)uVar18;
                uVar11 = uVar11 + 8;
                piVar6 = local_160;
                uVar20 = local_150;
              } while ((int)uVar18 < (int)uVar12);
            }
            local_150 = uVar20 + 1;
            uVar14 = (ulonglong)local_150;
          } while ((int)local_150 < local_168);
        }
        free(puVar7);
        param_1 = local_158;
        lVar21 = local_148;
      }
      uVar14 = 0;
      iVar3 = local_164 * 8;
      piVar8 = calloc((longlong)iVar3 + ((longlong)(local_164 * local_168) + 4) * 4,1);
      if (piVar8 != (int *)0x0) {
        piVar1 = piVar8 + 1;
        *piVar8 = local_168;
        piVar9 = piVar8 + 4;
        *piVar1 = local_164;
        *(int **)(piVar8 + 2) = piVar9;
        lVar19 = (longlong)piVar9 + (longlong)iVar3;
        if (0 < lVar21) {
          uVar11 = uVar14;
          do {
            *(longlong *)(piVar9 + uVar11 * 2) = lVar19;
            uVar11 = uVar11 + 1;
            lVar19 = lVar19 + (longlong)local_168 * 4;
          } while ((longlong)uVar11 < lVar21);
        }
        uVar2 = *(undefined4 *)**(undefined8 **)(param_1 + 2);
        if (0 < *piVar1) {
          iVar3 = *piVar8;
          uVar11 = uVar14;
          uVar16 = uVar14;
          do {
            uVar10 = uVar14;
            uVar15 = uVar14;
            if (0 < iVar3) {
              do {
                uVar12 = (int)uVar15 + 1;
                *(undefined4 *)(uVar10 + *(longlong *)(*(longlong *)(piVar8 + 2) + uVar11)) = uVar2;
                iVar3 = *piVar8;
                uVar10 = uVar10 + 4;
                uVar15 = (ulonglong)uVar12;
              } while ((int)uVar12 < iVar3);
            }
            uVar12 = (int)uVar16 + 1;
            uVar16 = (ulonglong)uVar12;
            uVar11 = uVar11 + 8;
          } while ((int)uVar12 < *piVar1);
        }
        iVar3 = *piVar8;
        if (0 < *piVar1) {
          uVar11 = uVar14;
          uVar16 = uVar14;
          do {
            FUN_18004d200(*(undefined8 **)(*(longlong *)(piVar6 + 2) + uVar11),
                          *(undefined8 **)(*(longlong *)(piVar8 + 2) + uVar11),
                          (longlong)(iVar3 << 2));
            uVar12 = (int)uVar16 + 1;
            uVar16 = (ulonglong)uVar12;
            uVar11 = uVar11 + 8;
            param_1 = local_158;
          } while ((int)uVar12 < *piVar1);
        }
        piVar9 = local_160;
        uVar2 = *(undefined4 *)(**(longlong **)(param_1 + 2) + -4 + (longlong)*param_1 * 4);
        if (0 < *piVar1) {
          iVar3 = *piVar8;
          uVar11 = uVar14;
          uVar16 = uVar14;
          do {
            uVar10 = uVar14;
            uVar15 = uVar14;
            if (0 < iVar3) {
              do {
                uVar12 = (int)uVar15 + 1;
                *(undefined4 *)(uVar10 + *(longlong *)(*(longlong *)(piVar8 + 2) + uVar11)) = uVar2;
                iVar3 = *piVar8;
                uVar10 = uVar10 + 4;
                uVar15 = (ulonglong)uVar12;
              } while ((int)uVar12 < iVar3);
            }
            uVar12 = (int)uVar16 + 1;
            uVar16 = (ulonglong)uVar12;
            uVar11 = uVar11 + 8;
          } while ((int)uVar12 < *piVar1);
        }
        iVar3 = *piVar8;
        iVar4 = *piVar6 - local_168;
        if (0 < *piVar1) {
          uVar11 = uVar14;
          do {
            FUN_18004d200((undefined8 *)
                          (*(longlong *)(*(longlong *)(piVar9 + 2) + uVar14) + (longlong)iVar4 * 4),
                          *(undefined8 **)(*(longlong *)(piVar8 + 2) + uVar14),
                          (longlong)(iVar3 << 2));
            uVar12 = (int)uVar11 + 1;
            uVar11 = (ulonglong)uVar12;
            uVar14 = uVar14 + 8;
            param_1 = local_158;
            piVar6 = local_160;
          } while ((int)uVar12 < *piVar1);
        }
        piVar9 = local_160;
        uVar14 = 0;
        uVar2 = **(undefined4 **)(*(longlong *)(param_1 + 2) + -8 + (longlong)param_1[1] * 8);
        if (0 < *piVar1) {
          iVar3 = *piVar8;
          uVar11 = uVar14;
          uVar16 = uVar14;
          do {
            uVar10 = uVar14;
            uVar15 = uVar14;
            if (0 < iVar3) {
              do {
                uVar12 = (int)uVar15 + 1;
                *(undefined4 *)(uVar10 + *(longlong *)(*(longlong *)(piVar8 + 2) + uVar11)) = uVar2;
                iVar3 = *piVar8;
                uVar10 = uVar10 + 4;
                uVar15 = (ulonglong)uVar12;
              } while ((int)uVar12 < iVar3);
            }
            uVar12 = (int)uVar16 + 1;
            uVar16 = (ulonglong)uVar12;
            uVar11 = uVar11 + 8;
          } while ((int)uVar12 < *piVar1);
        }
        iVar3 = *piVar8;
        if (0 < *piVar1) {
          iVar4 = piVar6[1] - local_164;
          uVar11 = uVar14;
          do {
            FUN_18004d200(*(undefined8 **)(*(longlong *)(piVar9 + 2) + (longlong)iVar4 * 8 + uVar14)
                          ,*(undefined8 **)(*(longlong *)(piVar8 + 2) + uVar14),
                          (longlong)(iVar3 << 2));
            uVar12 = (int)uVar11 + 1;
            uVar11 = (ulonglong)uVar12;
            uVar14 = uVar14 + 8;
            param_1 = local_158;
          } while ((int)uVar12 < *piVar1);
        }
        uVar14 = 0;
        uVar2 = *(undefined4 *)
                 (*(longlong *)(*(longlong *)(param_1 + 2) + -8 + (longlong)param_1[1] * 8) + -4 +
                 (longlong)*param_1 * 4);
        if (0 < *piVar1) {
          iVar3 = *piVar8;
          uVar11 = uVar14;
          uVar16 = uVar14;
          do {
            uVar10 = uVar14;
            uVar15 = uVar14;
            if (0 < iVar3) {
              do {
                uVar12 = (int)uVar15 + 1;
                *(undefined4 *)(uVar10 + *(longlong *)(*(longlong *)(piVar8 + 2) + uVar11)) = uVar2;
                iVar3 = *piVar8;
                uVar10 = uVar10 + 4;
                uVar15 = (ulonglong)uVar12;
              } while ((int)uVar12 < iVar3);
            }
            uVar12 = (int)uVar16 + 1;
            uVar16 = (ulonglong)uVar12;
            uVar11 = uVar11 + 8;
          } while ((int)uVar12 < *piVar1);
        }
        iVar3 = *piVar8;
        iVar4 = *local_160 - local_168;
        if (0 < *piVar1) {
          iVar5 = local_160[1] - local_164;
          uVar11 = uVar14;
          do {
            FUN_18004d200((undefined8 *)
                          (*(longlong *)
                            (*(longlong *)(local_160 + 2) + (longlong)iVar5 * 8 + uVar11) +
                          (longlong)iVar4 * 4),*(undefined8 **)(*(longlong *)(piVar8 + 2) + uVar11),
                          (longlong)(iVar3 << 2));
            uVar12 = (int)uVar14 + 1;
            uVar14 = (ulonglong)uVar12;
            uVar11 = uVar11 + 8;
          } while ((int)uVar12 < *piVar1);
        }
        free(piVar8);
      }
    }
    else if (param_4 == 4) {
      W[0] = *param_1;
      H[2] = param_1[1];
      uVar11 = 0;
      *(undefined8 *)&srcy[2] = _DAT_180089870;
      *(undefined8 *)&srcy[4] = _UNK_180089878;
      *(undefined8 *)&flipy[4] = _DAT_180089880;
      *(undefined8 *)&flipy[6] = _UNK_180089888;
      W[1] = W[0];
      *(undefined8 *)&flipx[0] = _DAT_180089860;
      *(undefined8 *)&flipx[2] = _UNK_180089868;
      srcx[3] = (W[0] - param_2) + -1;
      H[3] = H[2];
      *(undefined8 *)&flipy[0] = _DAT_180089870;
      *(undefined8 *)&flipy[2] = _UNK_180089878;
      srcx[5] = srcx[3];
      srcx[7] = srcx[3];
      srcy[1] = (H[2] - param_3) + -1;
      W[2] = param_2;
      W[3] = param_2;
      srcy[6] = srcy[1];
      srcy[7] = srcy[1];
      W[4] = param_2;
      W[5] = param_2;
      *(undefined8 *)&flipx[4] = 0x100000001;
      *(undefined8 *)&flipx[6] = 0x100000001;
      dstx[3] = *piVar6 - param_2;
      W[6] = param_2;
      W[7] = param_2;
      dstx[5] = dstx[3];
      dstx[7] = dstx[3];
      H[0] = param_3;
      H[1] = param_3;
      dsty[1] = piVar6[1] - param_3;
      H[4] = param_3;
      H[5] = param_3;
      H[6] = param_3;
      H[7] = param_3;
      srcx[0] = 0;
      srcx[1] = 0;
      dsty[6] = dsty[1];
      dsty[7] = dsty[1];
      srcx[2] = 1;
      srcx[4] = 1;
      srcx[6] = 1;
      srcy[0] = 1;
      dstx[0] = param_2;
      dstx[1] = param_2;
      dstx[2] = 0;
      dstx[4] = 0;
      dstx[6] = 0;
      dsty[0] = 0;
      dsty[2] = param_3;
      dsty[3] = param_3;
      dsty[4] = 0;
      dsty[5] = 0;
      uVar14 = uVar11;
      do {
        puVar7 = (uint *)FUN_180035fc0(*(int *)((longlong)W + uVar14),
                                       *(int *)((longlong)H + uVar14));
        if (puVar7 != (uint *)0x0) {
          FUN_180036060((longlong)param_1,*(int *)((longlong)srcx + uVar14),
                        *(int *)((longlong)srcy + uVar14),puVar7);
          if (*(int *)((longlong)flipx + uVar14) != 0) {
            FUN_180036100((int *)puVar7);
          }
          if ((*(int *)((longlong)flipy + uVar14) != 0) &&
             (uVar16 = uVar11, uVar10 = uVar11, 0 < (int)puVar7[1])) {
            do {
              lVar21 = (longlong)(int)(*puVar7 - 1);
              uVar15 = uVar11;
              if (0 < lVar21) {
                do {
                  lVar21 = lVar21 + -1;
                  uVar15 = uVar15 + 1;
                  lVar19 = *(longlong *)(uVar16 + *(longlong *)(puVar7 + 2));
                  uVar2 = *(undefined4 *)(lVar19 + -4 + uVar15 * 4);
                  *(undefined4 *)(lVar19 + -4 + uVar15 * 4) =
                       *(undefined4 *)(lVar19 + 4 + lVar21 * 4);
                  *(undefined4 *)
                   (*(longlong *)(*(longlong *)(puVar7 + 2) + uVar16) + 4 + lVar21 * 4) = uVar2;
                } while ((longlong)uVar15 < lVar21);
              }
              uVar12 = (int)uVar10 + 1;
              uVar16 = uVar16 + 8;
              uVar10 = (ulonglong)uVar12;
            } while ((int)uVar12 < (int)puVar7[1]);
          }
          FUN_1800361d0((longlong)piVar6,*(int *)((longlong)dstx + uVar14),
                        *(int *)((longlong)dsty + uVar14),(int *)puVar7);
          free(puVar7);
        }
        uVar14 = uVar14 + 4;
      } while ((longlong)uVar14 < 0x20);
      FUN_1800361d0((longlong)piVar6,param_2,param_3,param_1);
    }
  }
  return local_160;
}

/* ================ FUN_18003ab90 ================ */
void FUN_18003ab90(int *param_1,int *param_2,longlong param_3,int param_4)
{
  int *piVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int *_Memory;
  int *piVar5;
  int iVar6;
  longlong lVar7;
  int iVar8;
  longlong lVar9;
  longlong lVar10;
  int iVar11;
  longlong lVar12;

  iVar2 = param_2[1] / 2;
  iVar3 = *param_2 / 2;
  _Memory = FUN_18003a3a0(param_1,iVar3,iVar2,param_4);
  if (_Memory != (int *)0x0) {
    if (iVar2 < _Memory[1] - iVar2) {
      iVar6 = *_Memory - iVar3;
      lVar12 = 0;
      iVar11 = iVar2;
      do {
        if (iVar3 < iVar6) {
          lVar9 = 0;
          iVar8 = iVar3;
          do {
            iVar6 = 0;
            lVar10 = 0;
            if (0 < (longlong)param_2[1]) {
              do {
                if (0 < (longlong)*param_2) {
                  piVar1 = *(int **)(*(longlong *)(param_2 + 2) + lVar10 * 8);
                  piVar5 = piVar1;
                  lVar7 = (longlong)*param_2;
                  do {
                    iVar6 = iVar6 + *(int *)((*(longlong *)
                                               (*(longlong *)(_Memory + 2) + lVar12 + lVar10 * 8) -
                                             (longlong)piVar1) + lVar9 + (longlong)piVar5) * *piVar5
                    ;
                    lVar7 = lVar7 + -1;
                    piVar5 = piVar5 + 1;
                  } while (lVar7 != 0);
                }
                lVar10 = lVar10 + 1;
              } while (lVar10 < param_2[1]);
            }
            iVar4 = -0x2000;
            if (0 < iVar6) {
              iVar4 = 0x2000;
            }
            iVar8 = iVar8 + 1;
            *(int *)(lVar9 + *(longlong *)(lVar12 + *(longlong *)(param_3 + 8))) =
                 (int)(iVar4 + iVar6 + (iVar4 + iVar6 >> 0x1f & 0x3fffU)) >> 0xe;
            iVar6 = *_Memory - iVar3;
            lVar9 = lVar9 + 4;
          } while (iVar8 < iVar6);
        }
        iVar11 = iVar11 + 1;
        lVar12 = lVar12 + 8;
      } while (iVar11 < _Memory[1] - iVar2);
    }
    free(_Memory);
  }
  return;
}

/* ================ FUN_18003ad10 ================ */
undefined8 FUN_18003ad10(int *param_1,longlong param_2,int param_3)
{
  int iVar1;
  int *_Memory;
  undefined8 uVar2;
  undefined1 auStack_68 [32];
  ushort local_48 [12];
  ulonglong local_30;

  local_30 = DAT_180092040 ^ (ulonglong)auStack_68;
  local_48[10] = 0x24a;
  iVar1 = param_3 + -10;
  local_48[0] = 0;
  local_48[1] = 0x19;
  local_48[2] = 0x32;
  local_48[3] = 0x4b;
  local_48[4] = 0x7c;
  local_48[5] = 0xad;
  local_48[6] = 0xde;
  local_48[7] = 0x12f;
  local_48[8] = 0x180;
  local_48[9] = 0x1d1;
  _Memory = FUN_180035fc0((uint)(byte)(&DAT_18007d538)[iVar1],(uint)(byte)(&DAT_18007d538)[iVar1]);
  if (_Memory == (int *)0x0) {
    uVar2 = 0xfffffc18;
  }
  else {
    FUN_18004d200((undefined8 *)**(undefined8 **)(_Memory + 2),
                  (undefined8 *)(&DAT_1800849b0 + (ulonglong)local_48[iVar1] * 4),
                  (longlong)
                  (int)((uint)(byte)(&DAT_18007d538)[iVar1] * (uint)(byte)(&DAT_18007d538)[iVar1])
                  << 2);
    FUN_18003ab90(param_1,_Memory,param_2,0);
    free(_Memory);
    uVar2 = 0;
  }
  return uVar2;
}

/* ================ FUN_18003ae00 ================ */
void * FUN_18003ae00(int param_1,int param_2)
{
  int iVar1;
  void *pvVar2;
  int *piVar3;
  int iVar4;

  iVar1 = param_2 / 2;
  pvVar2 = malloc((longlong)(param_1 + iVar1 * 2) << 2);
  if (pvVar2 == (void *)0x0) {
    return (void *)0x0;
  }
  iVar4 = -iVar1;
  if (iVar4 < iVar1 + param_1) {
    piVar3 = (int *)((longlong)pvVar2 + ((longlong)iVar1 + (longlong)iVar4) * 4);
    do {
      if (iVar4 < 0) {
        *piVar3 = 0;
      }
      else if (iVar4 < param_1) {
        *piVar3 = iVar4;
      }
      else {
        *piVar3 = param_1 + -1;
      }
      iVar4 = iVar4 + 1;
      piVar3 = piVar3 + 1;
    } while (iVar4 < iVar1 + param_1);
  }
  return pvVar2;
}

/* ================ FUN_18003ae80 ================ */
undefined8 FUN_18003ae80(int *param_1,int *param_2,uint param_3)
{
  undefined4 uVar1;
  undefined4 uVar2;
  longlong lVar3;
  longlong *plVar4;
  longlong lVar5;
  uint uVar6;
  int iVar7;
  int *_Memory;
  void *pvVar8;
  longlong lVar9;
  int *piVar10;
  longlong lVar11;
  int *piVar12;
  longlong lVar13;
  longlong *plVar14;
  int iVar15;
  int iVar16;
  longlong lVar17;
  longlong lVar18;
  longlong lVar19;
  longlong lVar20;
  longlong lVar21;
  int iVar22;
  int iVar23;
  int iVar28;
  int iVar29;
  int iVar30;
  int iVar31;
  v128 auVar24;
  v128 auVar25;
  int iVar32;
  v128 auVar27;
  int iVar33;
  v128 auVar26;

  iVar16 = param_1[1];
  lVar21 = (longlong)iVar16;
  iVar23 = *param_1;
  lVar13 = (longlong)iVar23;
  lVar19 = (longlong)(int)param_3;
  _Memory = FUN_180035fc0(iVar23,iVar16);
  lVar3 = *(longlong *)(param_1 + 2);
  plVar4 = *(longlong **)(_Memory + 2);
  pvVar8 = FUN_18003ae00(iVar23,param_3);
  if (0 < lVar21) {
    lVar17 = lVar3 - (longlong)plVar4;
    lVar11 = lVar21;
    plVar14 = plVar4;
    iVar23 = DAT_1800920bc;
    do {
      lVar20 = 0;
      if (0 < lVar13) {
        piVar10 = (int *)((longlong)pvVar8 + 8);
        do {
          iVar15 = 0;
          lVar18 = 0;
          if (((0 < (int)param_3) && (7 < param_3)) && (1 < iVar23)) {
            lVar5 = *(longlong *)(lVar17 + (longlong)plVar14);
            uVar6 = param_3 & 0x80000007;
            if ((int)uVar6 < 0) {
              uVar6 = (uVar6 - 1 | 0xfffffff8) + 1;
            }
            iVar22 = 0;
            iVar28 = 0;
            iVar30 = 0;
            iVar32 = 0;
            piVar12 = piVar10;
            iVar23 = iVar22;
            iVar29 = iVar28;
            iVar31 = iVar30;
            iVar33 = iVar32;
            do {
              uVar1 = *(undefined4 *)(lVar5 + (longlong)*piVar12 * 4);
              uVar2 = *(undefined4 *)(lVar5 + (longlong)piVar12[-2] * 4);
              lVar9 = (longlong)iVar15;
              auVar24.i[1] = uVar1;
              auVar24.i[0] = uVar2;
              lVar18 = lVar18 + 8;
              iVar7 = iVar15 + 4;
              iVar15 = iVar15 + 8;
              auVar24.i[2] = uVar1;
              auVar24.i[3] = *(undefined4 *)(lVar5 + (longlong)piVar12[1] * 4);
              auVar25.q[1] = auVar24.q[1];
              auVar25.i[1] = *(undefined4 *)(lVar5 + (longlong)piVar12[-1] * 4);
              auVar25.i[0] = uVar2;
              auVar25 = pmulld(auVar25,v_load((const void *)(param_2 + lVar9)));
              iVar22 = auVar25.i[0] + iVar22;
              iVar28 = auVar25.i[1] + iVar28;
              iVar30 = auVar25.i[2] + iVar30;
              iVar32 = auVar25.i[3] + iVar32;
              uVar1 = *(undefined4 *)(lVar5 + (longlong)piVar12[2] * 4);
              uVar2 = *(undefined4 *)(lVar5 + (longlong)piVar12[4] * 4);
              auVar27.i[1] = uVar2;
              auVar27.i[0] = uVar1;
              auVar27.i[2] = uVar2;
              auVar27.i[3] = *(undefined4 *)(lVar5 + (longlong)piVar12[5] * 4);
              auVar26.q[1] = auVar27.q[1];
              auVar26.i[1] = *(undefined4 *)(lVar5 + (longlong)piVar12[3] * 4);
              auVar26.i[0] = uVar1;
              auVar25 = pmulld(auVar26,v_load((const void *)(param_2 + iVar7)));
              iVar23 = auVar25.i[0] + iVar23;
              iVar29 = auVar25.i[1] + iVar29;
              iVar31 = auVar25.i[2] + iVar31;
              iVar33 = auVar25.i[3] + iVar33;
              piVar12 = piVar12 + 8;
            } while (iVar15 < (int)(param_3 - uVar6));
            iVar15 = iVar23 + iVar22 + iVar31 + iVar30 + iVar29 + iVar28 + iVar33 + iVar32;
          }
          iVar23 = 0;
          iVar29 = 0;
          if (lVar18 < lVar19) {
            if (1 < lVar19 - lVar18) {
              lVar5 = lVar18 + 1;
              lVar9 = ((lVar19 - lVar18) - 2U >> 1) + 1;
              lVar18 = lVar18 + lVar9 * 2;
              piVar12 = param_2 + lVar5;
              do {
                iVar23 = iVar23 + *(int *)(*(longlong *)(lVar17 + (longlong)plVar14) +
                                          (longlong)
                                          *(int *)((longlong)piVar12 +
                                                  (-0xc - (longlong)param_2) + (longlong)piVar10) *
                                          4) * piVar12[-1];
                iVar29 = iVar29 + *(int *)(*(longlong *)(lVar17 + (longlong)plVar14) +
                                          (longlong)
                                          *(int *)((longlong)piVar10 + (-0x10 - (longlong)param_2) +
                                                  (longlong)(piVar12 + 2)) * 4) * *piVar12;
                lVar9 = lVar9 + -1;
                piVar12 = piVar12 + 2;
              } while (lVar9 != 0);
            }
            if (lVar18 < lVar19) {
              iVar15 = iVar15 + *(int *)(*(longlong *)(lVar17 + (longlong)plVar14) +
                                        (longlong)piVar10[lVar18 + -2] * 4) * param_2[lVar18];
            }
            iVar15 = iVar15 + iVar29 + iVar23;
          }
          lVar20 = lVar20 + 1;
          piVar10 = piVar10 + 1;
          *(int *)(*plVar14 + -4 + lVar20 * 4) = iVar15 + 0x4000 >> 0xf;
          iVar23 = DAT_1800920bc;
        } while (lVar20 < lVar13);
      }
      plVar14 = plVar14 + 1;
      lVar11 = lVar11 + -1;
    } while (lVar11 != 0);
  }
  free(pvVar8);
  FUN_18004d200((undefined8 *)**(undefined8 **)(param_1 + 2),
                (undefined8 *)**(undefined8 **)(_Memory + 2),(longlong)(*_Memory * _Memory[1]) << 2)
  ;
  iVar23 = (int)param_3 / 2;
  pvVar8 = malloc((longlong)(iVar16 + iVar23 * 2) << 2);
  if (pvVar8 != (void *)0x0) {
    iVar15 = -iVar23;
    if (iVar15 < iVar23 + iVar16) {
      lVar11 = (longlong)iVar15;
      piVar10 = (int *)((longlong)pvVar8 + (iVar23 + lVar11) * 4);
      do {
        if (iVar15 < 0) {
          *piVar10 = 0;
        }
        else if (lVar11 < lVar21) {
          *piVar10 = iVar15;
        }
        else {
          *piVar10 = iVar16 + -1;
        }
        iVar15 = iVar15 + 1;
        lVar11 = lVar11 + 1;
        piVar10 = piVar10 + 1;
      } while (iVar15 < iVar23 + iVar16);
    }
  }
  lVar11 = 0;
  if (0 < lVar13) {
    do {
      lVar17 = 0;
      if (0 < lVar21) {
        lVar20 = (longlong)pvVar8 - (longlong)param_2;
        do {
          iVar16 = 0;
          piVar10 = param_2;
          lVar18 = lVar19;
          if (0 < lVar19) {
            do {
              iVar16 = iVar16 + *(int *)(*(longlong *)
                                          (lVar3 + (longlong)*(int *)(lVar20 + (longlong)piVar10) *
                                                   8) + lVar11 * 4) * *piVar10;
              lVar18 = lVar18 + -1;
              piVar10 = piVar10 + 1;
            } while (lVar18 != 0);
          }
          plVar14 = plVar4 + lVar17;
          lVar17 = lVar17 + 1;
          lVar20 = lVar20 + 4;
          *(int *)(*plVar14 + lVar11 * 4) = iVar16 + 0x4000 >> 0xf;
        } while (lVar17 < lVar21);
      }
      lVar11 = lVar11 + 1;
    } while (lVar11 < lVar13);
  }
  free(pvVar8);
  FUN_18004d200((undefined8 *)**(undefined8 **)(param_1 + 2),
                (undefined8 *)**(undefined8 **)(_Memory + 2),(longlong)(*_Memory * _Memory[1]) << 2)
  ;
  free(_Memory);
  return 0;
}

/* ===== batch 5 ===== */

/* ---- Ghidra CONCAT intrinsics ---- */




undefined8 FUN_18003b300(longlong param_1,int *param_2)

{
  int iVar1;
  undefined8 uVar2;
  undefined1 auStack_48 [32];
  byte local_28 [16];
  ulonglong local_18;

  local_18 = DAT_180092040 ^ (ulonglong)auStack_48;
  iVar1 = param_2[8];
  if (iVar1 == 1) {
    uVar2 = FUN_18003ad10(*(int **)(param_1 + 0x28),(longlong)*(int **)(param_1 + 0x28),*param_2);
  }
  else {
    if (iVar1 != 2) {
      if (iVar1 != 3) {
        return 0;
      }
      FUN_1800362d0(*(uint **)(param_1 + 0x28),(longlong)*(uint **)(param_1 + 0x28));
      return 0;
    }
    local_28[0] = 0;
    local_28[1] = 5;
    local_28[2] = 10;
    local_28[3] = 0xf;
    local_28[4] = 0x16;
    local_28[5] = 0x1d;
    local_28[6] = 0x24;
    local_28[7] = 0x2d;
    local_28[8] = 0x36;
    local_28[9] = 0x3f;
    local_28[10] = 0x4a;
    iVar1 = *param_2 + -10;
    if (10 < *param_2 + -10) {
      iVar1 = 10;
    }
    uVar2 = FUN_18003ae80(*(int **)(param_1 + 0x28),
                          (int *)(&UNK_180085380 + (ulonglong)local_28[iVar1] * 4),
                          (uint)(byte)(&DAT_18007d538)[iVar1]);
  }
  if ((int)uVar2 == 0) {
    return 0;
  }
  return uVar2;
}

void FUN_18003b3d0(int *param_1,int *param_2,int param_3,int param_4,int param_5)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  int iVar5;

  iVar1 = param_5 * 2 + 1;
  iVar4 = (param_5 / 2 + 0x600) / param_5;
  if (param_5 == 1) {
    param_1[0] = 0x600;
    param_1[1] = 0x1400;
    param_1[2] = 0x600;
    param_2[0] = -0x4000;
    param_2[1] = 0;
    param_2[2] = 0x4000;
  }
  else {
    iVar5 = 0;
    do {
      iVar2 = param_4;
      piVar3 = param_2;
      if (iVar5 == 0) {
        iVar2 = param_3;
        piVar3 = param_1;
      }
      piVar3[0] = 0;
      piVar3[1] = 0;
      piVar3[2] = 0;
      piVar3[3] = 0;
      piVar3[4] = 0;
      piVar3[5] = 0;
      piVar3[6] = 0;
      piVar3[7] = 0;
      piVar3[8] = 0;
      piVar3[9] = 0;
      piVar3[10] = 0;
      piVar3[0xb] = 0;
      piVar3[0xc] = 0;
      piVar3[0xd] = 0;
      piVar3[0xe] = 0;
      piVar3[0xf] = 0;
      if (iVar2 == 0) {
        *piVar3 = iVar4;
        piVar3[iVar1 / 2] = (param_5 / 2 + 0x1400) / param_5;
        piVar3[(longlong)iVar1 + -1] = iVar4;
      }
      else if (iVar2 == 1) {
        *piVar3 = -0x4000;
        piVar3[iVar1 / 2] = 0;
        piVar3[(longlong)iVar1 + -1] = 0x4000;
      }
      iVar5 = iVar5 + 1;
    } while (iVar5 < 2);
  }
  return;
}

undefined8 FUN_18003b510(int *param_1,int *param_2,int param_3,int param_4,int param_5)

{
  int iVar1;
  int *_Memory;
  int *_Memory_00;
  undefined8 uVar2;
  undefined1 auStackY_108 [32];
  int local_c8 [16];
  int local_88 [16];
  ulonglong local_48;

  local_48 = DAT_180092040 ^ (ulonglong)auStackY_108;
  iVar1 = param_5 * 2 + 1;
  uVar2 = 0xfffffc18;
  _Memory = FUN_180035fc0(iVar1,1);
  _Memory_00 = FUN_180035fc0(1,iVar1);
  if (_Memory != (int *)0x0) {
    if (_Memory_00 != (int *)0x0) {
      FUN_18003b3d0(local_c8,local_88,param_3,param_4,param_5);
      FUN_18004d200((undefined8 *)**(undefined8 **)(_Memory + 2),(undefined8 *)local_c8,
                    (longlong)iVar1 << 2);
      FUN_18004d200((undefined8 *)**(undefined8 **)(_Memory_00 + 2),(undefined8 *)local_88,
                    (longlong)iVar1 << 2);
      FUN_18003ab90(param_1,_Memory,(longlong)param_2,4);
      FUN_18003ab90(param_2,_Memory_00,(longlong)param_2,4);
      uVar2 = 0;
    }
    free(_Memory);
  }
  if (_Memory_00 != (int *)0x0) {
    free(_Memory_00);
  }
  return uVar2;
}

void FUN_18003b650(longlong *param_1,int *param_2)

{
  int iVar1;

  iVar1 = param_2[10];
  if (iVar1 == 0) {
    iVar1 = (*param_2 * 0xf) / 100;
    FUN_18003b510((int *)param_1[5],(int *)*param_1,1,0,iVar1);
    FUN_18003b510((int *)param_1[5],(int *)param_1[1],0,1,iVar1);
    FUN_18003b510((int *)*param_1,(int *)param_1[2],1,0,iVar1);
    FUN_18003b510((int *)param_1[1],(int *)param_1[4],0,1,iVar1);
    FUN_18003b510((int *)*param_1,(int *)param_1[3],0,1,iVar1);
    return;
  }
  if (iVar1 == 1) {
    FUN_180036820((int *)param_1[5],*param_1,param_1[1]);
    FUN_180036820((int *)*param_1,param_1[2],param_1[3]);
    FUN_180036820((int *)param_1[1],0,param_1[4]);
    return;
  }
  if (iVar1 == 2) {
    FUN_1800369e0((uint *)param_1[5],*param_1,param_1[1]);
    FUN_1800369e0((uint *)*param_1,param_1[2],param_1[3]);
    FUN_1800369e0((uint *)param_1[1],0,param_1[4]);
    return;
  }
  return;
}

undefined8 FUN_18003b780(longlong *param_1,int *param_2)

{
  int *piVar1;
  undefined8 uVar2;
  longlong lVar3;
  longlong lVar4;
  char *pcVar5;
  int iVar6;
  int iVar7;
  longlong lVar8;
  longlong lVar9;

  iVar7 = *(int *)param_1[5];
  iVar6 = ((int *)param_1[5])[1];
  pcVar5 = *(char **)param_1[7];
  piVar1 = FUN_180035fc0(iVar7,iVar6);
  *param_1 = (longlong)piVar1;
  piVar1 = FUN_180035fc0(iVar7,iVar6);
  param_1[1] = (longlong)piVar1;
  piVar1 = FUN_180035fc0(iVar7,iVar6);
  param_1[2] = (longlong)piVar1;
  piVar1 = FUN_180035fc0(iVar7,iVar6);
  param_1[3] = (longlong)piVar1;
  piVar1 = FUN_180035fc0(iVar7,iVar6);
  param_1[4] = (longlong)piVar1;
  if ((((*param_1 == 0) || (param_1[1] == 0)) || (param_1[2] == 0)) ||
     ((param_1[3] == 0 || (piVar1 == (int *)0x0)))) {
    uVar2 = 0xfffffc18;
  }
  else {
    FUN_18003b650(param_1,param_2);
    lVar8 = (longlong)(iVar6 * iVar7);
    piVar1 = (int *)**(undefined8 **)(param_1[4] + 8);
    if (0 < iVar6 * iVar7) {
      lVar9 = **(longlong **)(param_1[3] + 8) - (longlong)piVar1;
      lVar4 = **(longlong **)(param_1[3] + 8) - (longlong)piVar1;
      lVar3 = **(longlong **)(param_1[2] + 8) - (longlong)piVar1;
      do {
        iVar7 = *(int *)(lVar4 + (longlong)piVar1);
        iVar6 = *piVar1 * *(int *)((longlong)piVar1 + lVar3);
        iVar7 = iVar7 * iVar7;
        *(int *)(lVar9 + (longlong)piVar1) = (int)((((0 < iVar6 - iVar7) + 7) - iVar7) + iVar6) >> 4
        ;
        if (*pcVar5 != -1) {
          *pcVar5 = *(int *)((longlong)piVar1 + lVar3) < 0;
        }
        piVar1 = piVar1 + 1;
        pcVar5 = pcVar5 + 1;
        lVar8 = lVar8 + -1;
      } while (lVar8 != 0);
    }
    if ((void *)param_1[4] != (void *)0x0) {
      free((void *)param_1[4]);
    }
    if ((void *)param_1[2] != (void *)0x0) {
      free((void *)param_1[2]);
    }
    param_1[4] = 0;
    param_1[2] = 0;
    param_1[6] = param_1[3];
    uVar2 = 0;
    param_1[3] = 0;
  }
  return uVar2;
}

int * FUN_18003b910(longlong param_1,longlong param_2)

{
  uint *puVar1;
  longlong lVar2;
  undefined8 *puVar3;
  undefined8 uVar4;
  int iVar5;
  int *_Memory;
  int *piVar6;
  int *piVar7;
  int *piVar8;
  uint uVar9;
  longlong lVar10;
  int *piVar11;
  int iVar12;
  uint uVar13;
  int iVar14;
  int iVar15;
  int iVar16;
  longlong lVar17;
  uint *puVar18;
  uint uVar19;
  longlong lVar20;
  int *piVar21;
  longlong lVar22;
  uint uVar23;
  int *piVar24;
  longlong local_res20;

  iVar15 = *(int *)(param_2 + 0x10);
  lVar10 = *(longlong *)(param_1 + 0x38);
  piVar24 = (int *)0x0;
  iVar14 = 0;
  uVar23 = 0;
  iVar5 = (*(int *)(param_1 + 0x44) * *(int *)(param_1 + 0x40)) / (iVar15 * iVar15);
  _Memory = calloc(0x10,(longlong)iVar5);
  if (_Memory == (int *)0x0) {
    return (int *)0x0;
  }
  iVar12 = 0xb;
  if (0xb < *(int *)(*(longlong *)(param_1 + 0x30) + 4) + -0xb) {
    lVar10 = lVar10 + -8;
    local_res20 = 0x60;
    piVar11 = piVar24;
    do {
      iVar16 = 0xb;
      lVar20 = *(longlong *)(*(int **)(param_1 + 0x30) + 2);
      lVar22 = 0xb;
      lVar2 = *(longlong *)(lVar20 + -8 + local_res20);
      if (0xb < **(int **)(param_1 + 0x30) + -0xb) {
        lVar17 = *(longlong *)(lVar20 + -0x10 + local_res20) - lVar2;
        lVar20 = *(longlong *)(lVar20 + local_res20) - lVar2;
        piVar6 = (int *)(lVar2 + 0x30);
        do {
          if ((((((*(char *)(lVar22 + *(longlong *)(lVar10 + local_res20)) != -1) &&
                 (uVar19 = piVar6[-1], *(int *)(param_2 + 4) < (int)uVar19)) &&
                (piVar6[-2] < (int)uVar19)) &&
               ((*piVar6 < (int)uVar19 && (*(int *)(lVar17 + -8 + (longlong)piVar6) < (int)uVar19)))
               ) && ((*(int *)((longlong)piVar6 + lVar17 + -4) < (int)uVar19 &&
                     ((*(int *)(lVar17 + (longlong)piVar6) < (int)uVar19 &&
                      (*(int *)(lVar20 + -8 + (longlong)piVar6) < (int)uVar19)))))) &&
             ((*(int *)((longlong)piVar6 + lVar20 + -4) < (int)uVar19 &&
              (*(int *)(lVar20 + (longlong)piVar6) < (int)uVar19)))) {
            uVar13 = (uint)piVar11;
            uVar9 = (uVar19 ^ (int)uVar19 >> 0x1f) - ((int)uVar19 >> 0x1f);
            piVar7 = piVar24;
            piVar8 = _Memory;
            piVar21 = piVar24;
            uVar19 = uVar23;
            if (0 < (int)uVar13) {
              do {
                uVar19 = (uint)piVar21;
                if ((iVar12 - piVar8[1]) * (iVar12 - piVar8[1]) +
                    (iVar16 - *piVar8) * (iVar16 - *piVar8) < iVar15 - (iVar15 >> 2)) {
                  if (uVar9 <= (uint)_Memory[(longlong)piVar7 * 4 + 2]) goto LAB_18003bb76;
                  break;
                }
                piVar7 = (int *)((longlong)piVar7 + 1);
                uVar19 = uVar19 + 1;
                piVar8 = piVar8 + 4;
                piVar21 = (int *)(ulonglong)uVar19;
              } while ((longlong)piVar7 < (longlong)(int)uVar13);
              if ((int)uVar19 < 0) goto LAB_18003bb76;
            }
            piVar7 = _Memory + (longlong)piVar7 * 4;
            if (uVar19 == uVar13) {
              piVar11 = (int *)(ulonglong)(uVar13 + 1);
            }
            piVar7[2] = uVar9;
            *piVar7 = iVar16;
            piVar7[1] = iVar12;
            *(undefined1 *)(piVar7 + 3) =
                 *(undefined1 *)(lVar22 + *(longlong *)(lVar10 + local_res20));
            iVar14 = (int)piVar11;
            if (iVar5 <= iVar14) goto LAB_18003bb18;
          }
LAB_18003bb76:
          iVar16 = iVar16 + 1;
          lVar22 = lVar22 + 1;
          piVar6 = piVar6 + 1;
        } while (iVar16 < **(int **)(param_1 + 0x30) + -0xb);
      }
      iVar14 = (int)piVar11;
      if (iVar5 <= iVar14) break;
      local_res20 = local_res20 + 8;
      iVar12 = iVar12 + 1;
    } while (iVar12 < *(int *)(*(longlong *)(param_1 + 0x30) + 4) + -0xb);
  }
LAB_18003bb18:
  free(*(void **)(param_1 + 0x38));
  *(undefined8 *)(param_1 + 0x38) = 0;
  if (*(int *)(param_2 + 0x38) != 0) {
    uVar23 = iVar14 * 2;
  }
  iVar15 = uVar23 + iVar14;
  piVar6 = calloc(1,(longlong)(iVar15 * 0x58 + 0x20));
  piVar11 = piVar24;
  if (piVar6 != (int *)0x0) {
    piVar7 = piVar6 + 8 + (longlong)iVar15 * 2;
    *(int **)(piVar6 + 6) = piVar6 + 8;
    piVar8 = piVar24;
    piVar11 = piVar6;
    if (0 < iVar15) {
      do {
        piVar8 = (int *)((longlong)piVar8 + 1);
        *(int **)(*(longlong *)(piVar6 + 6) + -8 + (longlong)piVar8 * 8) = piVar7;
        piVar7 = piVar7 + 0x14;
      } while ((longlong)piVar8 < (longlong)iVar15);
    }
  }
  *piVar11 = iVar14;
  if (0 < iVar14) {
    puVar18 = (uint *)(_Memory + 2);
    do {
      if (0x1fff < *puVar18) {
        *puVar18 = 0x1fff;
      }
      puVar1 = puVar18 + -2;
      uVar4 = *(undefined8 *)puVar18;
      piVar24 = (int *)((longlong)piVar24 + 1);
      puVar3 = *(undefined8 **)(*(longlong *)(piVar11 + 6) + -8 + (longlong)piVar24 * 8);
      puVar18 = puVar18 + 4;
      *puVar3 = *(undefined8 *)puVar1;
      puVar3[1] = uVar4;
    } while ((longlong)piVar24 < (longlong)iVar14);
  }
  free(_Memory);
  return piVar11;
}

undefined8 FUN_18003bc60(longlong param_1,int param_2,int param_3,int *param_4,int *param_5)

{
  longlong lVar1;
  longlong lVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  longlong lVar9;
  int iVar10;
  uint uVar11;
  longlong lVar12;

  lVar9 = (longlong)param_3;
  lVar12 = (longlong)param_2;
  lVar1 = *(longlong *)(param_1 + lVar12 * 8);
  lVar2 = *(longlong *)(param_1 + -8 + lVar12 * 8);
  lVar12 = *(longlong *)(param_1 + 8 + lVar12 * 8);
  iVar3 = *(int *)(lVar1 + -4 + lVar9 * 4);
  iVar4 = *(int *)(lVar2 + lVar9 * 4);
  iVar5 = *(int *)(lVar1 + lVar9 * 4);
  iVar6 = iVar3 + iVar5 * -2 + *(int *)(lVar1 + 4 + lVar9 * 4);
  iVar3 = ((*(int *)(lVar1 + 4 + lVar9 * 4) - iVar3) + 1) / 2;
  iVar10 = iVar4 + iVar5 * -2 + *(int *)(lVar12 + lVar9 * 4);
  iVar4 = ((*(int *)(lVar12 + lVar9 * 4) - iVar4) + 1) / 2;
  iVar5 = *(int *)(lVar2 + -4 + lVar9 * 4) + 2 +
          ((*(int *)(lVar12 + 4 + lVar9 * 4) - *(int *)(lVar2 + 4 + lVar9 * 4)) -
          *(int *)(lVar12 + -4 + lVar9 * 4));
  iVar5 = (int)(iVar5 + (iVar5 >> 0x1f & 3U)) >> 2;
  uVar8 = iVar5 * iVar5 - iVar10 * iVar6;
  if (uVar8 != 0) {
    uVar11 = iVar10 * iVar3 - iVar5 * iVar4;
    uVar7 = iVar6 * iVar4 - iVar5 * iVar3;
    iVar3 = (uVar8 ^ (int)uVar8 >> 0x1f) - ((int)uVar8 >> 0x1f);
    if (((int)((uVar11 ^ (int)uVar11 >> 0x1f) - ((int)uVar11 >> 0x1f)) <= iVar3) &&
       ((int)((uVar7 ^ (int)uVar7 >> 0x1f) - ((int)uVar7 >> 0x1f)) <= iVar3)) {
      iVar3 = (int)uVar8 >> 0x10;
      if (iVar3 != 0) {
        *param_4 = (int)(iVar3 / 2 + uVar11) / iVar3;
        *param_5 = (int)(iVar3 / 2 + uVar7) / iVar3;
        return 1;
      }
      *param_5 = 0;
      *param_4 = 0;
      return 1;
    }
  }
  return 0;
}

void FUN_18003bda0(int *param_1,longlong param_2)

{
  int iVar1;
  void *pvVar2;
  undefined8 uVar3;
  int iVar4;
  ulonglong uVar5;
  uint uVar6;
  ulonglong uVar7;
  ulonglong uVar8;
  ulonglong uVar9;
  ulonglong uVar10;
  int local_res8 [2];
  int local_res10 [2];
  longlong local_res18;
  longlong local_res20;

  local_res20 = *(longlong *)(param_2 + 0x30);
  uVar5 = 0;
  iVar4 = 0;
  if (*param_1 != 0) {
    pvVar2 = malloc((longlong)*param_1 << 2);
    *(void **)(param_2 + 0x48) = pvVar2;
    pvVar2 = malloc((longlong)*param_1 << 2);
    *(void **)(param_2 + 0x50) = pvVar2;
    if (0 < *param_1) {
      local_res18 = 0;
      uVar7 = uVar5;
      uVar8 = uVar5;
      uVar10 = uVar5;
      do {
        iVar4 = **(int **)(uVar7 + *(longlong *)(param_1 + 6));
        iVar1 = (*(int **)(uVar7 + *(longlong *)(param_1 + 6)))[1];
        uVar3 = FUN_18003bc60(*(longlong *)(local_res20 + 8),iVar1,iVar4,local_res8,local_res10);
        uVar9 = uVar8;
        if ((int)uVar3 != 0) {
          *(int *)(local_res18 + *(longlong *)(param_2 + 0x48)) = iVar4 * 0x10000 + local_res8[0];
          *(int *)(local_res18 + *(longlong *)(param_2 + 0x50)) = iVar1 * 0x10000 + local_res10[0];
          uVar10 = (ulonglong)((int)uVar10 + 1);
          *(undefined2 *)(*(longlong *)(uVar8 + *(longlong *)(param_1 + 6)) + 0xe) = 0;
          uVar9 = uVar8 + 8;
          *(undefined4 *)(*(longlong *)(uVar8 + *(longlong *)(param_1 + 6)) + 8) =
               *(undefined4 *)(*(longlong *)(uVar7 + *(longlong *)(param_1 + 6)) + 8);
          *(undefined1 *)(*(longlong *)(uVar8 + *(longlong *)(param_1 + 6)) + 0xc) =
               *(undefined1 *)(*(longlong *)(uVar7 + *(longlong *)(param_1 + 6)) + 0xc);
          local_res18 = local_res18 + 4;
        }
        iVar4 = (int)uVar10;
        uVar6 = (int)uVar5 + 1;
        uVar5 = (ulonglong)uVar6;
        uVar7 = uVar7 + 8;
        uVar8 = uVar9;
      } while ((int)uVar6 < *param_1);
    }
    *param_1 = iVar4;
  }
  return;
}

int FUN_18003bf10(uint param_1,uint param_2)

{
  int iVar1;
  uint uVar2;
  uint uVar3;

  uVar3 = (param_2 ^ (int)param_2 >> 0x1f) - ((int)param_2 >> 0x1f);
  uVar2 = (param_1 ^ (int)param_1 >> 0x1f) - ((int)param_1 >> 0x1f);
  if (uVar3 == 0) {
    iVar1 = 0x10e;
    if (0 < (int)param_1) {
      iVar1 = 0x5a;
    }
    return iVar1;
  }
  if (uVar2 == 0) {
    iVar1 = 0xb4;
    if (0 < (int)param_2) {
      iVar1 = 0;
    }
    return iVar1;
  }
  if (uVar3 < uVar2) {
    uVar2 = 0x2d0 - *(ushort *)
                     (&DAT_1800856f0 +
                     ((ulonglong)((uVar2 >> 1) + uVar3 * 0x80) / (ulonglong)uVar2) * 2);
  }
  else {
    uVar2 = (uint)*(ushort *)
                   (&DAT_1800856f0 +
                   ((ulonglong)((uVar3 >> 1) + uVar2 * 0x80) / (ulonglong)uVar3) * 2);
  }
  if ((int)param_2 < 1) {
    iVar1 = 0x5a0;
    if ((int)param_1 < 0) {
      return (int)(uVar2 + 0x5a4) >> 3;
    }
  }
  else {
    if (-1 < (int)param_1) goto LAB_18003bfc0;
    if ((int)uVar2 < 5) {
      return 0;
    }
    iVar1 = 0xb40;
  }
  uVar2 = iVar1 - uVar2;
LAB_18003bfc0:
  return (int)(uVar2 + 4) >> 3;
}

int FUN_18003bfe0(short *param_1,longlong *param_2,int param_3,longlong param_4,uint param_5,
                 longlong param_6)

{
  undefined8 *puVar1;
  int *piVar2;
  longlong lVar3;
  longlong lVar4;
  short sVar5;
  ushort uVar6;
  uint uVar7;
  undefined8 *_Memory;
  uint uVar8;
  int iVar9;
  longlong lVar10;
  uint uVar11;
  int iVar12;
  short sVar13;
  uint uVar14;
  int iVar15;
  short sVar16;
  longlong lVar17;
  uint uVar18;
  uint uVar19;
  short sVar20;
  short sVar21;
  longlong lVar22;
  uint auStack_20428 [124];
  uint auStack_20238 [32620];
  undefined1 auStack_488 [36];
  int local_464;
  uint local_460;
  uint local_45c;
  uint local_458;
  longlong local_450;
  short *local_448;
  longlong local_440;
  longlong local_438;
  uint auStack_428 [124];
  uint auStack_238 [124];
  ulonglong local_48;

  (void)auStack_20428;
  (void)auStack_20238;
  local_48 = DAT_180092040 ^ (ulonglong)auStack_488;
  sVar13 = (short)param_3;
  sVar20 = *param_1;
  lVar17 = 0;
  lVar22 = *(longlong *)(*param_2 + 8);
  lVar10 = *(longlong *)(param_2[1] + 8);
  iVar12 = param_3 * 5 + (int)param_1[2];
  local_45c = 0;
  local_458 = *(int *)(param_6 + 0x34) * *(int *)(param_6 + 0x34);
  local_464 = -1;
  uVar14 = param_5 & 0xff;
  sVar21 = 0;
  sVar16 = param_1[2] + sVar13 * -5;
  local_450 = lVar10;
  local_448 = param_1;
  local_440 = param_4;
  local_438 = lVar22;
  if (sVar16 <= iVar12) {
    local_460 = uVar14;
    do {
      lVar3 = *(longlong *)(lVar10 + (longlong)sVar16 * 8);
      lVar4 = *(longlong *)(lVar22 + (longlong)sVar16 * 8);
      uVar7 = (uint)(short)(sVar20 + sVar13 * -5);
      uVar14 = local_460;
      uVar8 = uVar7;
      while (local_460 = uVar14, (int)uVar8 <= (int)sVar20 + param_3 * 5) {
        sVar5 = (short)uVar7;
        uVar6 = sVar5 + sVar13;
        uVar7 = (uint)uVar6;
        lVar22 = (longlong)sVar5 * 4;
        lVar10 = (longlong)sVar21;
        sVar21 = sVar21 + 1;
        auStack_238[lVar10] = *(int *)(lVar22 + lVar4) * *(int *)(&DAT_180085500 + lVar10 * 4);
        auStack_428[lVar10] = *(int *)(lVar22 + lVar3) * *(int *)(&DAT_180085500 + lVar10 * 4);
        lVar10 = local_450;
        lVar22 = local_438;
        uVar14 = local_460;
        uVar8 = (uint)(short)uVar6;
      }
      sVar16 = sVar16 + sVar13;
    } while (sVar16 <= iVar12);
  }
  param_5 = (param_5 & 0xffffff00u) | (uint8_t)(param_5 >> 0x10);
  local_460 = 0x168 >> ((byte)param_5 & 0x1f);
  iVar15 = (int)uVar14 >> ((byte)param_5 & 0x1f);
  iVar12 = iVar15 + local_460;
  _Memory = calloc((longlong)(iVar12 * 2),4);
  lVar22 = 0x79;
  puVar1 = (undefined8 *)((longlong)_Memory + (longlong)iVar12 * 4);
  do {
    uVar14 = *(uint *)((longlong)auStack_238 + lVar17);
    if ((uVar14 != 0) || (*(int *)((longlong)auStack_428 + lVar17) != 0)) {
      uVar7 = *(uint *)((longlong)auStack_428 + lVar17);
      iVar12 = FUN_18003bf10(uVar7,uVar14);
      lVar10 = (longlong)(short)(iVar12 >> ((byte)param_5 & 0x1f));
      piVar2 = (int *)((longlong)_Memory + lVar10 * 4);
      *piVar2 = *piVar2 + uVar14;
      piVar2 = (int *)((longlong)puVar1 + lVar10 * 4);
      *piVar2 = *piVar2 + uVar7;
    }
    uVar14 = local_460;
    lVar17 = lVar17 + 4;
    lVar22 = lVar22 + -1;
  } while (lVar22 != 0);
  lVar22 = (longlong)(int)local_460;
  FUN_18004d200((undefined8 *)(lVar22 * 4 + (longlong)_Memory),_Memory,(longlong)iVar15 << 2);
  FUN_18004d200((undefined8 *)(lVar22 * 4 + (longlong)puVar1),puVar1,(longlong)iVar15 << 2);
  uVar7 = 0;
  uVar18 = 0;
  uVar19 = 0;
  sVar20 = 1 - (short)iVar15;
  uVar8 = 0;
  if (0 < (int)(uVar14 + iVar15)) {
    sVar13 = (short)iVar15 + -1 + sVar20;
    uVar8 = 0;
    do {
      uVar19 = uVar19 + *(int *)((longlong)_Memory + (longlong)sVar13 * 4);
      uVar18 = uVar18 + *(int *)((longlong)puVar1 + (longlong)sVar13 * 4);
      if (-1 < sVar20) {
        iVar12 = -0x1000;
        if (-1 < (int)uVar19) {
          iVar12 = 0x1000;
        }
        iVar9 = (int)(((int)(iVar12 + uVar19) >> 0x1f & 0x1fffU) + iVar12 + uVar19) >> 0xd;
        iVar12 = -0x1000;
        if (-1 < (int)uVar18) {
          iVar12 = 0x1000;
        }
        iVar12 = (int)(((int)(iVar12 + uVar18) >> 0x1f & 0x1fffU) + iVar12 + uVar18) >> 0xd;
        uVar11 = iVar12 * iVar12 + iVar9 * iVar9;
        if (local_440 != 0) {
          lVar22 = (longlong)sVar20;
          *(uint *)(local_440 + lVar22 * 0xc) = uVar19;
          *(uint *)(local_440 + 4 + lVar22 * 0xc) = uVar18;
          *(uint *)(local_440 + 8 + lVar22 * 0xc) = uVar11;
        }
        if (uVar8 < uVar11) {
          local_464 = (int)sVar20;
          uVar7 = uVar19;
          uVar8 = uVar11;
          local_45c = uVar18;
        }
        uVar19 = uVar19 - *(int *)((longlong)_Memory + (longlong)sVar20 * 4);
        uVar18 = uVar18 - *(int *)((longlong)puVar1 + (longlong)sVar20 * 4);
      }
      sVar13 = sVar13 + 1;
      sVar20 = sVar20 + 1;
    } while ((int)sVar13 < (int)(uVar14 + iVar15));
  }
  free(_Memory);
  if ((((int)local_458 < 1) || (local_458 <= uVar8)) && ((uVar7 != 0 || (local_45c != 0)))) {
    iVar12 = FUN_18003bf10(local_45c,uVar7);
    sVar20 = (short)iVar12;
  }
  else {
    sVar20 = -1;
  }
  local_448[7] = sVar20;
  return local_464;
}

undefined8
FUN_18003c320(longlong param_1,int param_2,int param_3,int param_4,int param_5,int param_6,
             undefined8 *param_7,longlong param_8)

{
  longlong lVar1;
  int iVar2;
  int *piVar3;
  longlong lVar4;
  longlong lVar5;
  int iVar6;
  ulonglong uVar7;
  longlong lVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  int iVar15;
  int iVar16;
  ulonglong uVar17;
  int iVar18;
  int iVar19;
  longlong lVar20;
  int iVar21;
  int iVar22;
  int iVar23;
  int iVar24;
  int iVar25;
  int iVar26;
  int iVar27;
  int *local_68;
  longlong local_60;

  iVar2 = *(int *)(param_8 + 0x10);
  lVar20 = (longlong)*(int *)(param_8 + 0xc);
  piVar3 = (int *)*param_7;
  iVar21 = -iVar2;
  if (iVar21 < iVar2) {
    lVar4 = param_7[1];
    lVar5 = param_7[2];
    lVar8 = 0;
    iVar14 = iVar21 * param_6;
    iVar12 = iVar21 * param_5;
    local_60 = 0;
    iVar24 = iVar21;
    do {
      if (iVar21 < iVar2) {
        local_68 = (int *)(param_1 + 8 + lVar8 * 4);
        iVar9 = iVar21 * param_5 + iVar14 + param_4;
        iVar18 = (iVar12 - iVar21 * param_6) + param_3;
        iVar16 = iVar21;
        iVar22 = iVar21;
        do {
          iVar22 = iVar22 + param_2;
          iVar26 = 0;
          iVar13 = 0;
          iVar27 = 0;
          iVar15 = 0;
          if (iVar24 + param_2 <= iVar24) {
            return 0xffffffff;
          }
          uVar7 = (ulonglong)(uint)((iVar24 + param_2) - iVar24);
          iVar11 = iVar9;
          iVar19 = iVar18;
          do {
            if (iVar16 < iVar22) {
              uVar17 = (ulonglong)(uint)(iVar22 - iVar16);
              iVar23 = iVar11;
              iVar25 = iVar19;
              do {
                iVar6 = (int)((0 < iVar23) + 0x7fff + iVar23) >> 0x10;
                iVar10 = (int)((0 < iVar25) + 0x7fff + iVar25) >> 0x10;
                if ((((-1 < iVar10) && (-1 < iVar6)) && (iVar10 < *piVar3)) && (iVar6 < piVar3[1]))
                {
                  lVar8 = (longlong)iVar10 * 4;
                  lVar1 = (longlong)iVar6 * 8;
                  iVar26 = iVar26 + *(int *)(*(longlong *)
                                              (*(longlong *)(piVar3 + 2) + (longlong)iVar6 * 8) +
                                            lVar8);
                  if (1 < lVar20) {
                    iVar6 = *(int *)(lVar8 + *(longlong *)(*(longlong *)(lVar4 + 8) + lVar1));
                    iVar10 = *(int *)(lVar8 + *(longlong *)(*(longlong *)(lVar5 + 8) + lVar1));
                    if (lVar20 == 2) {
                      iVar13 = iVar13 + iVar6 * iVar6 + iVar10 * iVar10;
                    }
                    else {
                      iVar13 = iVar13 + iVar6;
                      iVar27 = iVar27 + iVar10;
                    }
                  }
                  iVar15 = iVar15 + 1;
                }
                iVar23 = iVar23 + param_5;
                iVar25 = iVar25 - param_6;
                uVar17 = uVar17 - 1;
              } while (uVar17 != 0);
            }
            iVar11 = iVar11 + param_6;
            iVar19 = iVar19 + param_5;
            uVar7 = uVar7 - 1;
          } while (uVar7 != 0);
          if (iVar15 < 0xc) {
            return 0xffffffff;
          }
          if (lVar20 == 3) {
            iVar19 = (int)((0 < iVar13) + 7 + iVar13) >> 4;
            iVar11 = (int)((0 < iVar27) + 7 + iVar27) >> 4;
            iVar27 = iVar11 * param_6;
            iVar13 = iVar19 * param_5;
            iVar19 = iVar19 * param_6;
            iVar11 = iVar11 * param_5;
            iVar13 = (int)((0 < iVar13 + iVar27) + 0x7ff + iVar13 + iVar27) >> 0xc;
            iVar27 = (int)((((0 < iVar11 - iVar19) + 0x7ff) - iVar19) + iVar11) >> 0xc;
          }
          local_68[-2] = iVar26 / iVar15;
          if (1 < lVar20) {
            local_68[-1] = iVar13 / iVar15;
          }
          if (2 < lVar20) {
            *local_68 = iVar27 / iVar15;
          }
          iVar9 = iVar9 + param_2 * param_5;
          iVar18 = iVar18 - param_2 * param_6;
          iVar16 = iVar16 + param_2;
          local_68 = local_68 + lVar20;
          lVar8 = local_60 + lVar20;
          local_60 = lVar8;
        } while (iVar16 < iVar2);
      }
      iVar24 = iVar24 + param_2;
      iVar14 = iVar14 + param_2 * param_6;
      iVar12 = iVar12 + param_2 * param_5;
    } while (iVar24 < iVar2);
  }
  return 1;
}

void FUN_18003c720(longlong param_1,undefined8 *param_2,int param_3,int param_4,longlong param_5)

{
  byte *pbVar1;
  int iVar2;
  int iVar3;
  byte bVar4;
  undefined8 uVar5;
  longlong lVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  longlong lVar10;
  int *piVar11;
  longlong lVar12;
  int *piVar13;
  longlong lVar14;
  longlong lVar15;
  longlong lVar16;
  longlong lVar17;
  int *piVar18;
  undefined1 auStackY_1a8 [32];
  int local_168;
  longlong local_150;
  int local_128 [4];
  int local_118 [48];
  ulonglong local_58;

  local_58 = DAT_180092040 ^ (ulonglong)auStackY_1a8;
  iVar9 = 0;
  iVar2 = *(int *)(&DAT_180084410 + (longlong)*(short *)(param_1 + 0xe) * 4);
  iVar3 = *(int *)(&DAT_180083e70 + (longlong)*(short *)(param_1 + 0xe) * 4);
  local_128[0] = 0x4000;
  local_128[1] = 0x2aab;
  local_128[2] = 0x2000;
  local_150 = 0;
  local_168 = 2;
  do {
    iVar7 = local_128[local_150] * *(int *)(param_5 + 0x10);
    iVar8 = iVar7 >> 0xe;
    if (iVar8 << 0xe < iVar7) {
      iVar8 = iVar8 + 1;
    }
    uVar5 = FUN_18003c320((longlong)local_118,iVar8,param_3,param_4,iVar2,iVar3,param_2,param_5);
    if ((int)uVar5 < 1) {
      *(undefined2 *)(param_1 + 0xe) = 0xffff;
      return;
    }
    lVar16 = (longlong)*(int *)(param_5 + 0xc);
    lVar10 = (longlong)(local_168 * local_168);
    if (0 < lVar16) {
      piVar18 = local_118;
      lVar15 = lVar16;
      do {
        if (0 < lVar10) {
          lVar14 = 1;
          piVar11 = piVar18;
          lVar17 = lVar10;
          do {
            iVar7 = *piVar11;
            if (lVar14 < lVar10) {
              lVar6 = lVar10 - lVar14;
              piVar13 = piVar11;
              do {
                piVar13 = piVar13 + lVar16;
                lVar12 = (longlong)iVar9;
                bVar4 = (byte)iVar9;
                iVar9 = iVar9 + 1;
                pbVar1 = (byte *)((lVar12 >> 3) + 0x10 + param_1);
                *pbVar1 = *pbVar1 | (*piVar13 < iVar7) << (bVar4 & 7);
                lVar6 = lVar6 + -1;
              } while (lVar6 != 0);
            }
            piVar11 = piVar11 + lVar16;
            lVar14 = lVar14 + 1;
            lVar17 = lVar17 + -1;
          } while (lVar17 != 0);
        }
        piVar18 = piVar18 + 1;
        lVar15 = lVar15 + -1;
      } while (lVar15 != 0);
    }
    local_150 = local_150 + 1;
    local_168 = local_168 + 1;
  } while (local_150 < 3);
  return;
}

void FUN_18003c920(int *param_1,longlong *param_2,longlong param_3)

{
  int iVar1;
  undefined8 uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  void *pvVar6;
  longlong lVar7;
  uint *puVar8;
  undefined8 *puVar9;
  uint uVar10;
  void *pvVar11;
  undefined8 *puVar12;
  uint uVar13;
  int *piVar15;
  longlong lVar16;
  longlong lVar17;
  undefined4 *puVar18;
  int iVar19;
  uint uVar20;
  short sVar21;
  int iVar22;
  void *pvVar24;
  int iVar25;
  int local_res8;
  int local_res10;
  int local_res20;
  int local_94;
  longlong local_88;
  longlong local_arr3[3];
  void *pvVar14;
  void *pvVar23;

  pvVar14 = (void *)0x0;
  iVar25 = 0;
  piVar15 = (int *)**(undefined8 **)(param_1 + 6);
  local_arr3[0] = param_2[5];
  local_arr3[1] = *param_2;
  local_arr3[2] = param_2[1];
  iVar22 = 2;
  if (*(int *)(param_3 + 0x10) < 7) {
    iVar22 = 1;
  }
  uVar10 = 0;
  uVar13 = *(int *)(param_3 + 0x3c) / 10;
  iVar19 = *(int *)(param_3 + 0x3c) % 10;
  if (uVar13 == 0) {
    uVar13 = 0x40;
  }
  pvVar6 = pvVar14;
  for (iVar3 = iVar19; 1 < iVar3; iVar3 = iVar3 >> 1) {
    uVar10 = (int)pvVar6 + 1;
    pvVar6 = (void *)(ulonglong)uVar10;
  }
  if (*(int *)(param_3 + 0x38) == 0) {
    local_res10 = (int)param_2;
    local_res20 = local_res10;
    pvVar6 = pvVar14;
    iVar3 = local_res10;
  }
  else {
    local_res20 = (int)(uVar13 + 0x168) / iVar19;
    pvVar6 = malloc((longlong)local_res20 * 0xc);
    local_res10 = *param_1;
    iVar3 = local_res10 * 3;
  }
  local_94 = 0;
  if (0 < *param_1) {
    local_88 = (longlong)local_res10;
    pvVar24 = pvVar14;
    local_res8 = local_res10;
    do {
      *piVar15 = (int)((uint)(0 < *(int *)((longlong)pvVar24 + param_2[9])) +
                      *(int *)((longlong)pvVar24 + param_2[9]) + 0x7fff) >> 0x10;
      piVar15[1] = (int)((uint)(0 < *(int *)((longlong)pvVar24 + param_2[10])) +
                        *(int *)((longlong)pvVar24 + param_2[10]) + 0x7fff) >> 0x10;
      iVar4 = FUN_18003bfe0((short *)piVar15,param_2,iVar22,(longlong)pvVar6,
                            iVar19 * 0x100 | uVar13 | uVar10 << 0x10,param_3);
      lVar16 = (longlong)iVar4;
      if (-1 < *(short *)((longlong)piVar15 + 0xe)) {
        FUN_18003c720((longlong)piVar15,local_arr3,*(int *)((longlong)pvVar24 + param_2[9]),
                      *(int *)((longlong)pvVar24 + param_2[10]),param_3);
        iVar4 = *(int *)(param_3 + 0x38);
        if ((iVar4 != 0) && (local_88 < iVar3)) {
          lVar17 = (longlong)local_res20;
          iVar1 = *(int *)((longlong)pvVar6 + lVar16 * 0xc + 8);
          do {
            pvVar23 = (void *)0xffffffff;
            sVar21 = -1;
            lVar7 = 0x41;
            lVar16 = (longlong)((int)(short)lVar16 - (int)(0x20 / (longlong)iVar19));
            puVar18 = (undefined4 *)((longlong)pvVar6 + lVar16 * 0xc + 8);
            do {
              if (lVar16 < 0) {
                lVar16 = lVar16 + lVar17;
                puVar18 = puVar18 + lVar17 * 3;
              }
              if (lVar17 <= lVar16) {
                lVar16 = lVar16 - lVar17;
                puVar18 = puVar18 + lVar17 * -3;
              }
              *puVar18 = 0;
              puVar18 = puVar18 + 3;
              lVar16 = lVar16 + 1;
              lVar7 = lVar7 + -1;
            } while (lVar7 != 0);
            if (0 < lVar17) {
              puVar8 = (uint *)((longlong)pvVar6 + 8);
              pvVar11 = pvVar14;
              lVar16 = lVar17;
              uVar20 = (uint)(iVar1 * iVar4) >> 10;
              do {
                if (uVar20 <= *puVar8) {
                  pvVar23 = pvVar11;
                  uVar20 = *puVar8;
                }
                sVar21 = (short)pvVar23;
                pvVar11 = (void *)(ulonglong)((int)pvVar11 + 1);
                puVar8 = puVar8 + 3;
                lVar16 = lVar16 + -1;
              } while (lVar16 != 0);
            }
            lVar16 = (longlong)sVar21;
            if (sVar21 < 0) break;
            local_res8 = local_res8 + 1;
            puVar9 = *(undefined8 **)(*(longlong *)(param_1 + 6) + local_88 * 8);
            local_88 = local_88 + 1;
            uVar2 = *(undefined8 *)(piVar15 + 2);
            puVar8 = (uint *)((longlong)pvVar6 + lVar16 * 0xc);
            *puVar9 = *(undefined8 *)piVar15;
            puVar9[1] = uVar2;
            iVar5 = FUN_18003bf10(puVar8[1],*puVar8);
            *(short *)((longlong)puVar9 + 0xe) = (short)iVar5;
            FUN_18003c720((longlong)puVar9,local_arr3,*(int *)((longlong)pvVar24 + param_2[9]),
                          *(int *)((longlong)pvVar24 + param_2[10]),param_3);
          } while (local_88 != iVar3);
        }
      }
      local_94 = local_94 + 1;
      piVar15 = piVar15 + 0x14;
      pvVar24 = (void *)((longlong)pvVar24 + 4);
      local_res10 = local_res8;
    } while (local_94 < *param_1);
  }
  if (pvVar6 != (void *)0x0) {
    *param_1 = local_res10;
    free(pvVar6);
  }
  puVar9 = (undefined8 *)**(undefined8 **)(param_1 + 6);
  puVar12 = puVar9;
  pvVar6 = pvVar14;
  if (0 < *param_1) {
    do {
      if (*(short *)((longlong)puVar9 + 0xe) < 0) {
        pvVar6 = (void *)(ulonglong)((int)pvVar6 + 1);
      }
      else {
        if (puVar12 != puVar9) {
          uVar2 = puVar9[1];
          *puVar12 = *puVar9;
          puVar12[1] = uVar2;
          uVar2 = puVar9[3];
          puVar12[2] = puVar9[2];
          puVar12[3] = uVar2;
          uVar2 = puVar9[5];
          puVar12[4] = puVar9[4];
          puVar12[5] = uVar2;
          uVar2 = puVar9[7];
          puVar12[6] = puVar9[6];
          puVar12[7] = uVar2;
          uVar2 = puVar9[9];
          puVar12[8] = puVar9[8];
          puVar12[9] = uVar2;
        }
        puVar12 = puVar12 + 10;
      }
      iVar25 = (int)pvVar6;
      uVar13 = (int)pvVar14 + 1;
      pvVar14 = (void *)(ulonglong)uVar13;
      puVar9 = puVar9 + 10;
    } while ((int)uVar13 < *param_1);
  }
  *param_1 = *param_1 - iVar25;
  return;
}

int * FUN_18003cd60(longlong *param_1,int *param_2)

{
  undefined8 uVar1;
  int *piVar2;

  uVar1 = FUN_18003b780(param_1,param_2);
  if ((int)uVar1 != 0) {
    return (int *)0x0;
  }
  piVar2 = FUN_18003b910((longlong)param_1,(longlong)param_2);
  if (piVar2 == (int *)0x0) {
    return (int *)0x0;
  }
  FUN_18003bda0(piVar2,(longlong)param_1);
  if ((void *)param_1[6] != (void *)0x0) {
    free((void *)param_1[6]);
  }
  param_1[6] = 0;
  FUN_18003c920(piVar2,param_1,(longlong)param_2);
  if ((void *)param_1[9] != (void *)0x0) {
    free((void *)param_1[9]);
  }
  if ((void *)param_1[10] != (void *)0x0) {
    free((void *)param_1[10]);
  }
  return piVar2;
}

undefined8
FUN_18003ce10(longlong *param_1,int param_2,uint param_3,undefined8 *param_4,int *param_5)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  ulonglong *_Memory;
  longlong *plVar4;
  ulonglong uVar5;
  longlong *plVar6;
  int *piVar7;
  undefined1 *puVar8;
  int *piVar9;
  undefined8 uVar10;
  uint uVar11;
  uint uVar12;
  undefined1 uVar13;

  uVar13 = 0x80;
  if (param_5[0xb] == 1) {
    FUN_180039af0(param_1,param_2,param_3);
  }
  else if (param_5[0xb] == 2) {
    FUN_180039420(param_1,param_2,param_3);
  }
  iVar3 = param_5[5] * param_2;
  uVar12 = (int)((uint)(0 < iVar3) + iVar3 + 0x1ff) >> 10;
  iVar3 = param_5[5] * param_3;
  uVar11 = (int)((uint)(0 < iVar3) + iVar3 + 0x1ff) >> 10;
  _Memory = (ulonglong *)FUN_180038500((longlong)param_1,param_2,param_3,uVar12,uVar11);
  if (param_5[0x27] != 0) {
    free(param_1);
  }
  plVar4 = FUN_18002e460(uVar12,uVar11);
  iVar3 = param_5[6];
  if (iVar3 < 1) {
    FUN_18004d980((undefined1 (*) [16])*plVar4,0,(longlong)(int)(uVar11 * uVar12));
  }
  else if (iVar3 == 1) {
    uVar5 = FUN_180038cb0(_Memory,uVar12,uVar11,plVar4);
    uVar13 = (undefined1)uVar5;
  }
  else if (iVar3 == 2) {
    uVar5 = FUN_180038ec0((longlong *)_Memory,uVar12,uVar11,plVar4);
    uVar13 = (undefined1)uVar5;
  }
  if (param_5[7] == 1) {
    FUN_180038480(_Memory,uVar12,uVar11);
  }
  else if (param_5[7] == 2) {
    FUN_1800387c0(_Memory,uVar12,uVar11);
  }
  plVar6 = calloc(1,0x58);
  *(uint *)(plVar6 + 8) = uVar12;
  *(uint *)((longlong)plVar6 + 0x44) = uVar11;
  plVar6[7] = (longlong)plVar4;
  piVar7 = FUN_180035fc0(uVar12,uVar11);
  FUN_18003a290(**(ulonglong **)(piVar7 + 2),*_Memory,uVar11 * uVar12);
  plVar6[5] = (longlong)piVar7;
  free(_Memory);
  FUN_18003b300((longlong)plVar6,param_5);
  piVar7 = FUN_18003cd60(plVar6,param_5);
  if (param_4 != (undefined8 *)0x0) {
    *param_4 = (undefined8)piVar7;
  }
  if (piVar7 != (int *)0x0) {
    *(short *)(piVar7 + 1) = (short)uVar12;
    *(short *)((longlong)piVar7 + 6) = (short)uVar11;
    iVar3 = param_5[3];
    *(undefined1 *)((longlong)piVar7 + 9) = uVar13;
    *(undefined1 *)((longlong)piVar7 + 10) = 0;
    *(char *)(piVar7 + 2) = (char)iVar3;
    if (param_5[2] != 0) {
      puVar8 = malloc((longlong)(int)(uVar11 * uVar12));
      iVar3 = 0;
      *(undefined1 **)(piVar7 + 4) = puVar8;
      piVar2 = (int *)plVar6[5];
      piVar9 = (int *)**(undefined8 **)(piVar2 + 2);
      if (0 < piVar2[1] * *piVar2) {
        do {
          iVar1 = *piVar9;
          iVar3 = iVar3 + 1;
          piVar9 = piVar9 + 1;
          *puVar8 = (char)((uint)(iVar1 * 0x7f8 + 0x8000) >> 0x10);
          puVar8 = puVar8 + 1;
        } while (iVar3 < piVar2[1] * *piVar2);
      }
    }
    if (*(longlong *)(param_5 + 0x28) != 0) {
      piVar9 = FUN_180036270((int *)plVar6[5]);
      **(undefined8 **)(param_5 + 0x28) = (undefined8)piVar9;
      piVar9 = FUN_180036270((int *)*plVar6);
      *(int **)(*(longlong *)(param_5 + 0x28) + 8) = piVar9;
      piVar9 = FUN_180036270((int *)plVar6[1]);
      *(int **)(*(longlong *)(param_5 + 0x28) + 0x10) = piVar9;
    }
  }
  FUN_18003a200(plVar6);
  uVar10 = 0xfffffc18;
  if (piVar7 != (int *)0x0) {
    uVar10 = 0;
  }
  return uVar10;
}

undefined8
FUN_18002ba30(undefined8 *param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4)

{
  longlong *plVar1;
  undefined1 *puVar2;
  longlong lVar3;
  undefined8 uVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  undefined1 auStackY_358 [32];
  undefined8 in_stack_fffffffffffffcc8;
  longlong local_328 [60];
  /* Ghidra's contiguous 0x40 header buffer (local_148..uStack_110); the named
     fields are accessed by their frame offsets. local_108 is a separate buffer. */
  uint8_t hb [0x40];
  undefined1 local_108 [240];
  ulonglong local_18;

  local_18 = DAT_180092040 ^ (ulonglong)auStackY_358;
  in_stack_fffffffffffffcc8 = 0;
  FUN_18002b720(3,"990201==>",param_3,param_4);
  *(undefined8 *)(hb + 0) = *param_1;
  uVar5 = param_1[1];
  uVar4 = 0;
  *(undefined2 *)(hb + 8) = (undefined2)uVar5;
  *(short *)(hb + 0xa) = (short)((ulonglong)uVar5 >> 0x10);
  *(undefined4 *)(hb + 0xc) = (undefined4)((ulonglong)uVar5 >> 0x20);
  *(undefined8 *)(hb + 0x20) = param_1[4];
  *(undefined8 *)(hb + 0x28) = param_1[5];
  *(undefined4 *)(hb + 0x10) = (undefined4)param_1[2];
  *(undefined4 *)(hb + 0x14) = (undefined4)((ulonglong)param_1[2] >> 0x20);
  *(undefined4 *)(hb + 0x18) = (undefined4)param_1[3];
  *(undefined4 *)(hb + 0x1c) = (undefined4)((ulonglong)param_1[3] >> 0x20);
  *(undefined8 *)(hb + 0x30) = param_1[6];
  *(undefined8 *)(hb + 0x38) = param_1[7];
  FUN_18004d980((undefined1 (*) [16])local_108,0,0xf0);
  plVar1 = local_328;
  puVar2 = local_108;
  lVar3 = 0x34;
  do {
    *plVar1 = (longlong)puVar2;
    puVar2 = puVar2 + 4;
    plVar1 = plVar1 + 1;
    lVar3 = lVar3 + -1;
  } while (lVar3 != 0);
  uVar6 = 1;
  FUN_18002fc80(0x18007a530,local_328,(uint)CONCAT71(((ulonglong)uVar4 >> 8),0x18),1);
  uVar5 = 0x40;
  FUN_1800301f0((byte *)hb,local_328,0x40,uVar6,in_stack_fffffffffffffcc8,0xc);
  if ((*(uint8_t *)(hb + 0) == 0x41) && (*(uint8_t *)(hb + 1) == 'E')) {
    _DAT_1800970c0 = CONCAT44(*(uint32_t *)(hb + 0x10),*(uint32_t *)(hb + 0xc));
    uRam00000001800970c8 = CONCAT44(*(uint32_t *)(hb + 0x18),*(uint32_t *)(hb + 0x14));
    DAT_1800a04f0 = *(short *)(hb + 2);
    DAT_1800a04f2 = *(short *)(hb + 4);
    _DAT_1800a04f4 = CONCAT22(*(uint16_t *)(hb + 8),*(uint16_t *)(hb + 6));
    DAT_1800a04f8 = *(short *)(hb + 0xa);
    DAT_1800970d0 = *(uint32_t *)(hb + 0x1c);
    FUN_18002b720(3,"990202==>good",uVar5,uVar6);
    if (DAT_1800a04f8 == 0) {
      DAT_1800a04f8 = *(short *)(DAT_18009d018 + 0x84);
    }
    if (DAT_1800a04f0 == 0) {
      DAT_1800a04f0 = *(short *)(DAT_18009d018 + 0x54);
    }
    FUN_18002b720(3,"9902FF==>",uVar5,uVar6);
    uVar5 = 0;
  }
  else {
    DAT_1800a04f0 = *(short *)(DAT_18009d018 + 0x54);
    _DAT_1800a04f4 = 0;
    DAT_1800a04f8 = *(short *)(DAT_18009d018 + 0x84);
    uVar5 = 0xfffffc0e;
  }
  return uVar5;
}

/* ===== batch 6 ===== */

static inline int POPCOUNT(unsigned long long x){return __builtin_popcountll(x);}









undefined8 FUN_18002bc30(undefined8 *param_1,uint *param_2,undefined8 param_3,undefined8 param_4)
{
  uint uVar1;
  undefined8 uVar2;
  undefined8 *_Memory;
  undefined *puVar3;
  ulonglong uVar4;
  undefined1 uVar5;
  ulonglong uVar6;

  uVar5 = (undefined1)param_4;
  uVar6 = (ulonglong)((ulonglong)param_4 >> 8);
  FUN_18002b720(3,"990301==>",param_3,param_4);
  if ((param_1 != (undefined8 *)0x0) && (param_2 != (uint *)0x0)) {
    uVar1 = *param_2;
    if ((int)uVar1 < 1) {
      uVar2 = 0xfffffc10;
    }
    else {
      _Memory = malloc((longlong)(int)(uVar1 + 5));
      if (_Memory == (undefined8 *)0x0) {
        FUN_18002b720(1,"990302==>%d Fail",(ulonglong)uVar1,CONCAT71(uVar6,uVar5));
        uVar2 = 0xfffffc0f;
      }
      else {
        FUN_18004d200((undefined8 *)((longlong)_Memory + 5),param_1,(longlong)(int)uVar1);
        *(undefined1 *)((longlong)_Memory + 4) = 0x45;
        *param_2 = *param_2 + 1;
        puVar3 = &DAT_18007a568;
        uVar5 = 0x20;
        if ((int)param_3 != 0) {
          puVar3 = &DAT_18007a548;
        }
        FUN_180030530((byte *)((longlong)_Memory + 4),(longlong)puVar3,*param_2,CONCAT71(uVar6,0x20));
        *(undefined1 *)_Memory = 0x44;
        *(undefined1 *)((longlong)_Memory + 1) = *(undefined1 *)((longlong)param_2 + 2);
        *(char *)((longlong)_Memory + 2) = (char)(*param_2 >> 8);
        *(char *)((longlong)_Memory + 3) = (char)*param_2;
        *param_2 = *param_2 + 4;
        FUN_18002b720(3,"990305==>%d",(ulonglong)*param_2,CONCAT71(uVar6,uVar5));
        uVar4 = (ulonglong)(int)*param_2;
        FUN_18004d200(param_1,_Memory,uVar4);
        free(_Memory);
        FUN_18002b720(3,"9903FF==>",uVar4,CONCAT71(uVar6,uVar5));
        uVar2 = 0;
      }
    }
    return uVar2;
  }
  return 0xfffffc11;
}

undefined8 FUN_18002bd80(char *param_1,int *param_2,int param_3,undefined8 param_4)
{
  undefined8 uVar1;
  uint uVar2;
  ulonglong uVar3;
  undefined1 uVar4;
  ulonglong uVar5;
  undefined *puVar6;

  uVar5 = (ulonglong)((ulonglong)param_4 >> 8);
  puVar6 = &DAT_18007a568;
  if (param_3 != 0) {
    puVar6 = &DAT_18007a548;
  }
  if (param_2 == (int *)0x0) {
    return 0xfffffc11;
  }
  if (0x3c004 < *param_2 - 1U) {
    return 0xfffffc10;
  }
  if ((*param_1 != 'D') && (*param_1 != 'E')) {
    return 0xfffffc0e;
  }
  uVar2 = (uint)CONCAT21(CONCAT11(param_1[1],param_1[2]),param_1[3]);
  if (uVar2 + 4 != (uint)*param_2) {
    return 0xfffffc0e;
  }
  uVar4 = 0x20;
  FUN_180030610((byte *)(param_1 + 4),(longlong)puVar6,uVar2,0x20);
  if (param_1[4] == 'E') {
    uVar3 = (ulonglong)(int)(uVar2 - 1);
    FUN_18004d200((undefined8 *)param_1,(undefined8 *)(param_1 + 5),uVar3);
    *param_2 = uVar2 - 1;
    FUN_18002b720(3,"9904FF==>",uVar3,CONCAT71(uVar5,uVar4));
    uVar1 = 0;
  }
  else {
    uVar1 = 0xfffffc0e;
  }
  return uVar1;
}

undefined8
FUN_18002bf50(undefined8 *param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4)
{
  char *pcVar1;
  undefined8 uVar2;

  uVar2 = param_3;
  FUN_18002b720(2,"990701==>non-neon",param_3,param_4);
  if (DAT_18009707c != 0x7fffffff) {
    return 0xfffffc02;
  }
  if (DAT_18009d018 == 0) {
    DAT_18009d018 = (longlong)FUN_1800350e0();
  }
  DAT_18009707c = 7000;
  FUN_18002b720(3,"990601==>",uVar2,param_4);
  DAT_1800a04fc = (int)param_3;
  FUN_180039cc0(DAT_1800a04fc);
  pcVar1 = "9906FF==>";
  FUN_18002b720(3,"9906FF==>",uVar2,param_4);
  FUN_18002ba30(param_1,pcVar1,uVar2,param_4);
  FUN_18002b720(3,"9907FF==>%d %d %d %d",(ulonglong)DAT_1800a04f6,(ulonglong)DAT_1800a04f4);
  return 0;
}

undefined2 *
FUN_18002c0d0(undefined8 param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4)
{
  undefined4 uVar1;
  longlong lVar2;
  undefined2 *puVar3;
  void *pvVar4;
  void *pvVar5;
  void *pvVar6;

  FUN_18002b720(3,"990901==>",param_3,param_4);
  lVar2 = DAT_18009d018;
  DAT_18009d014 = 0;
  if (DAT_18009707c != 7000) {
    return (undefined2 *)0x0;
  }
  *(undefined4 *)(DAT_18009d018 + 0x70) = 100;
  puVar3 = malloc(0x40);
  if (puVar3 != (undefined2 *)0x0) {
    pvVar4 = calloc(1,0x20);
    if (pvVar4 == (void *)0x0) {
      FUN_18002b720(1,"990903==>Fail",param_3,param_4);
      puVar3 = (undefined2 *)0x0;
    }
    else {
      pvVar5 = calloc(1,0x3c);
      if (pvVar5 == (void *)0x0) {
        FUN_18002b720(1,"990904==>Fail",param_3,param_4);
        puVar3 = (undefined2 *)0x0;
      }
      else {
        *puVar3 = 0x4452;
        uVar1 = *(undefined4 *)(lVar2 + 0x70);
        *(undefined8 *)(puVar3 + 4) = 0;
        *(undefined4 *)(puVar3 + 8) = 0;
        *(undefined4 *)(puVar3 + 0x10) = 100;
        *(undefined4 *)(puVar3 + 2) = uVar1;
        pvVar6 = FUN_180032c60();
        *(void **)(puVar3 + 0x1c) = pvVar5;
        *(void **)(puVar3 + 0x18) = pvVar4;
        *(void **)(puVar3 + 0xc) = pvVar6;
        *(undefined4 *)(puVar3 + 0x12) = 0xd;
        *(undefined4 *)(puVar3 + 0x14) = 0x9c;
        DAT_1800a04e0 = 0;
        *(undefined4 *)(*(longlong *)(puVar3 + 0x1c) + 0x18) = 0xffffffff;
        **(undefined1 **)(puVar3 + 0x1c) = 1;
        DAT_18009707c = 0x1b59;
        *(undefined1 *)(*(longlong *)(puVar3 + 0x1c) + 1) = 0;
        *(undefined4 *)(*(longlong *)(puVar3 + 0x1c) + 0x1c) = 0;
        *(undefined4 *)(*(longlong *)(puVar3 + 0x1c) + 0x20) = 0;
        *(undefined4 *)(*(longlong *)(puVar3 + 0x1c) + 0x24) = 0;
        *(undefined4 *)(*(longlong *)(puVar3 + 0x1c) + 0x28) = 0;
        *(undefined4 *)(*(longlong *)(puVar3 + 0x1c) + 0x2c) = 0;
        *(undefined4 *)(*(longlong *)(puVar3 + 0x1c) + 0x30) = 0;
        FUN_18002b720(3,"9909FF==>",param_3,param_4);
      }
    }
    return puVar3;
  }
  FUN_18002b720(1,"990902==>Fail",param_3,param_4);
  return (undefined2 *)0x0;
}

int FUN_18002d150(int *param_1,longlong *param_2,int *param_3)
{
  int iVar1;
  longlong lVar2;
  int iVar3;
  byte *pbVar4;
  ulonglong uVar5;
  ulonglong uVar6;
  uint uVar7;
  ulonglong uVar8;
  uint8_t keybuf[16];

  if (param_1 == (int *)0x0) {
    iVar3 = -0x3ea;
  }
  else {
    iVar3 = FUN_180035cf0(param_1,param_2,param_3,DAT_18009d018);
    if (iVar3 == 0) {
      iVar1 = *param_3;
      lVar2 = *param_2;
      uVar8 = 0;
      *(undefined8 *)keybuf = _DAT_18007d2d8;
      *(undefined8 *)(keybuf + 8) = _UNK_18007d2e0;
      uVar5 = uVar8;
      if (0 < iVar1 + -0x20) {
        do {
          uVar7 = (uint)uVar8;
          uVar6 = uVar5 + 1;
          uVar8 = (ulonglong)(uVar7 + 1);
          pbVar4 = keybuf + (ulonglong)(uVar7 & 0xf);
          *pbVar4 = *pbVar4 ^ *(byte *)(uVar5 + 0x20 + lVar2);
          uVar5 = uVar6;
        } while ((longlong)uVar6 < (longlong)(iVar1 + -0x20));
      }
      {
        ulonglong r = 0;
        int i;
        for (i = 0; i < 8; i++) {
          r |= (ulonglong)(byte)(keybuf[i] ^ keybuf[8 + i]) << (i * 8);
        }
        *(ulonglong *)(lVar2 + 4) = r;
      }
    }
  }
  return iVar3;
}

uint FUN_18002e020(undefined8 *param_1,undefined8 *param_2,ulonglong param_3,byte *param_4)
{
  ushort uVar1;
  uint uVar2;
  char *_Memory;
  ulonglong uVar3;
  void *pvVar4;
  ulonglong uVar5;
  size_t _Size;
  byte *pbVar6;
  int local_res18 [2];

  local_res18[0] = (int)param_3;
  _Size = (size_t)local_res18[0];
  uVar3 = param_3;
  pbVar6 = param_4;
  FUN_18002b720(3,"994101==>",param_3,param_4);
  FUN_18002b720(3,"994001==>",uVar3,pbVar6);
  _Memory = malloc(_Size);
  if (_Memory == (char *)0x0) {
    uVar2 = 0xfffffc0f;
    FUN_18002b720(1,"994102==>Fail %d",0xfffffc0f,pbVar6);
  }
  else {
    FUN_18002b720(3,"994002==>%d",param_3 & 0xffffffff,pbVar6);
    FUN_18004d200((undefined8 *)_Memory,param_2,_Size);
    FUN_18002b720(3,"9940FF==>",_Size,pbVar6);
    FUN_18002b720(3,"990401==>",_Size,pbVar6);
    uVar3 = FUN_18002bd80(_Memory,local_res18,1,pbVar6);
    uVar2 = (uint)uVar3;
    if (uVar2 == 0) {
      uVar1 = *(ushort *)(_Memory + 2);
      uVar3 = (ulonglong)uVar1;
      if (uVar1 - 1 < 0x28) {
        pvVar4 = FUN_180035450((uint)uVar1);
        *param_1 = (undefined8)pvVar4;
        uVar5 = FUN_18002d250((longlong)_Memory,local_res18[0]);
        uVar2 = (uint)uVar5;
        if (uVar2 == 0) {
          uVar3 = FUN_18002b840((longlong)_Memory,param_4,uVar3,pbVar6);
          uVar2 = (uint)uVar3;
          FUN_18002b720(3,"994106==>%d",(ulonglong)*param_4,pbVar6);
          if (uVar2 == 0) {
            uVar2 = FUN_180035e80((uint *)*param_1,_Memory);
            if (uVar2 != 0) {
              FUN_18002b720(1,"994108==>Fail %d",(ulonglong)uVar2,pbVar6);
            }
          }
          else {
            FUN_18002b720(1,"994107==>Fail %d",uVar3 & 0xffffffff,pbVar6);
          }
        }
        else {
          FUN_18002b720(1,"994105==>Fail %d",uVar5 & 0xffffffff,pbVar6);
        }
      }
      else {
        FUN_18002b720(1,"994104==>Fail %d",uVar3,pbVar6);
        uVar2 = 0xfffffc17;
      }
    }
    else {
      FUN_18002b720(1,"994103==>Fail %d",uVar3 & 0xffffffff,pbVar6);
    }
    free(_Memory);
  }
  FUN_18002b720(3,"9941FF==>%d",(ulonglong)uVar2,pbVar6);
  return uVar2;
}

ulonglong FUN_18002e350(int *param_1,longlong *param_2,uint *param_3,byte *param_4)
{
  uint uVar1;
  ulonglong uVar2;
  ulonglong uVar3;
  undefined8 uVar4;
  byte *pbVar5;

  pbVar5 = param_4;
  FUN_18002b720(3,"994301==>",param_3,param_4);
  uVar1 = FUN_18002d150(param_1,param_2,(int *)param_3);
  if (uVar1 == 0) {
    uVar1 = *param_3;
    if (uVar1 < 0x3c001) {
      uVar2 = FUN_18002b7b0((char *)*param_2,*param_4,(ulonglong)uVar1,pbVar5);
      uVar3 = uVar2 & 0xffffffff;
      if ((int)uVar2 == 0) {
        uVar4 = 1;
        uVar2 = FUN_18002bc30((undefined8 *)*param_2,param_3,1,pbVar5);
        uVar3 = uVar2 & 0xffffffff;
        if ((int)uVar2 == 0) {
          FUN_18002b720(3,"9943FF==>",uVar4,pbVar5);
          uVar3 = 0;
        }
        else {
          FUN_18002b720(1,"994305==>%d, Fail",uVar2 & 0xffffffff,pbVar5);
        }
      }
      else {
        FUN_18002b720(1,"994304==>%d, Fail",uVar2 & 0xffffffff,pbVar5);
      }
    }
    else {
      FUN_18002b720(1,"994303==>%d, Fail",(ulonglong)uVar1,pbVar5);
      uVar3 = 0xfffffc10;
    }
  }
  else {
    FUN_18002b720(1,"994302==>%d, Fail",(ulonglong)uVar1,pbVar5);
    uVar3 = (ulonglong)uVar1;
  }
  return uVar3;
}

void FUN_180030ff0(longlong *param_1,ulonglong param_2,int param_3,int param_4,longlong param_5)
{
  int *piVar1;

  if (*param_1 == 0) {
    piVar1 = FUN_180035fc0(param_3,param_4);
    *param_1 = (longlong)piVar1;
    piVar1 = FUN_180035fc0(param_3,param_4);
    param_1[1] = (longlong)piVar1;
    piVar1 = FUN_180035fc0(param_3,param_4);
    param_1[2] = (longlong)piVar1;
    FUN_18003a290(**(ulonglong **)(*param_1 + 8),param_2,param_3 * param_4);
    if (*(int *)(param_5 + 0x28) == 1) {
      FUN_180036820((int *)*param_1,param_1[1],param_1[2]);
    }
    else if (*(int *)(param_5 + 0x28) == 2) {
      FUN_1800369e0((uint *)*param_1,param_1[1],param_1[2]);
    }
  }
  return;
}

undefined8
FUN_1800312f0(int *param_1,uint *param_2,undefined8 *param_3,int *param_4,int param_5,int param_6,
             char *param_7,longlong param_8)
{
  char cVar1;
  ushort uVar2;
  int iVar3;
  int *piVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  char *_Memory;
  int *piVar8;
  int *piVar9;
  short sVar10;
  longlong *plVar11;
  ulonglong *puVar12;
  int iVar13;
  char *pcVar14;
  longlong lVar15;
  longlong lVar16;
  char cVar17;
  char local_78;
  int local_74;
  longlong local_68;
  int local_58;
  int local_54;
  int local_50;
  int local_4c;
  int local_48;
  char *local_40;

  uVar5 = param_2[1];
  iVar13 = *param_1;
  uVar2 = *(ushort *)((longlong)param_2 + 6);
  iVar6 = *(int *)(param_8 + 0x10) * 0x16a0a;
  piVar9 = (int *)0x0;
  local_4c = (int)((uint)(0 < iVar6) + iVar6 + 0x7fff) >> 0x10;
  local_48 = (int)((uint)(byte)param_2[2] * 0xa2 + 7) >> 3;
  _Memory = calloc((longlong)iVar13,1);
  cVar17 = '\0';
  iVar6 = 0;
  local_78 = '\0';
  local_40 = _Memory;
  if (0 < iVar13) {
    lVar15 = 0;
    pcVar14 = _Memory;
    do {
      if (*(short *)(*(longlong *)(lVar15 + *(longlong *)(param_1 + 6)) + 0xe) < 0) {
        *pcVar14 = '\x01';
      }
      else {
        cVar1 = *(char *)(*(longlong *)(lVar15 + *(longlong *)(param_1 + 6)) + 0xd);
        if (cVar17 < cVar1) {
          cVar17 = cVar1;
        }
      }
      iVar6 = iVar6 + 1;
      pcVar14 = pcVar14 + 1;
      lVar15 = lVar15 + 8;
      local_78 = cVar17;
    } while (iVar6 < *param_1);
  }
  do {
    if (local_78 < '\0') {
      free(_Memory);
      return 0;
    }
    local_74 = 0;
    pcVar14 = _Memory;
    if (0 < *param_1) {
      local_68 = 0;
      do {
        lVar15 = (longlong)param_6;
        piVar4 = *(int **)(local_68 + *(longlong *)(param_1 + 6));
        if ((*_Memory == '\0') && (local_78 <= *(char *)((longlong)piVar4 + 0xd))) {
          iVar13 = *param_4;
          iVar6 = 0;
          *_Memory = '\x01';
          if (0 < iVar13) {
            lVar16 = 0;
            do {
              iVar3 = **(int **)(lVar16 + *(longlong *)(param_4 + 8));
              iVar7 = iVar3 - *piVar4;
              if (iVar7 < 0) {
                iVar7 = *piVar4 - iVar3;
              }
              if (iVar7 < 10) {
                iVar3 = (*(int **)(lVar16 + *(longlong *)(param_4 + 8)))[1];
                iVar7 = iVar3 - piVar4[1];
                if (iVar7 < 0) {
                  iVar7 = piVar4[1] - iVar3;
                }
                if (iVar7 < 10) {
                  if (-1 < iVar6) goto LAB_18003171e;
                  break;
                }
              }
              iVar6 = iVar6 + 1;
              lVar16 = lVar16 + 8;
            } while (iVar6 < iVar13);
          }
          piVar8 = FUN_180030ce0((int *)param_2,piVar4,0x24,&local_54);
          iVar13 = 0;
          if (0 < *param_4) {
            plVar11 = *(longlong **)(param_4 + 10);
            do {
              if ((int *)*plVar11 == piVar8) {
                if (-1 < iVar13) goto LAB_18003171e;
                break;
              }
              iVar13 = iVar13 + 1;
              plVar11 = plVar11 + 1;
            } while (iVar13 < *param_4);
          }
          if (local_54 < 10) {
            iVar13 = 0;
            if (0 < lVar15) {
              puVar12 = (ulonglong *)(piVar4 + 4);
              do {
                iVar13 = iVar13 + (int)POPCOUNT(*(ulonglong *)
                                                 ((longlong)piVar8 + (0x10 - (longlong)(piVar4 + 4))
                                                 + (longlong)puVar12) ^ *puVar12);
                lVar15 = lVar15 + -1;
                puVar12 = puVar12 + 1;
              } while (lVar15 != 0);
            }
            if (param_5 < iVar13) goto LAB_180031547;
LAB_1800316c3:
            iVar6 = *param_4;
            lVar15 = (longlong)iVar6;
            piVar9 = (int *)0x0;
            *(int **)(*(longlong *)(param_4 + 8) + lVar15 * 8) = piVar4;
            *(int **)(*(longlong *)(param_4 + 10) + lVar15 * 8) = piVar8;
            *(int *)(*(longlong *)(param_4 + 2) + lVar15 * 4) = iVar13;
            iVar6 = iVar6 + 1;
            *param_4 = iVar6;
            pcVar14 = local_40;
            if ((int)*param_2 <= iVar6) break;
          }
          else {
LAB_180031547:
            iVar13 = param_4[5];
            FUN_1800310a0(*piVar4,piVar4[1],&local_58,&local_50,
                          (int)*(short *)((longlong)param_4 + 0x16),(int)(short)param_4[6],
                          (int)(short)iVar13,(uint)(ushort)uVar5,(uint)uVar2);
            iVar3 = local_50;
            iVar6 = local_58;
            if ((((local_4c <= local_58) && (local_4c <= local_50)) &&
                (local_58 <= (int)((uint)(ushort)uVar5 - local_4c))) &&
               (local_50 <= (int)((uint)uVar2 - local_4c))) {
              if ((piVar9 == (int *)0x0) &&
                 (piVar9 = (int *)FUN_180031240(param_2,param_4,param_7), pcVar14 = local_40,
                 piVar9 == (int *)0x0)) break;
              FUN_18004d980((undefined1 (*) [16])(piVar9 + 4),0,(longlong)local_48);
              sVar10 = *(short *)((longlong)piVar4 + 0xe) + (short)iVar13;
              if (sVar10 < 0) {
                sVar10 = sVar10 + 0x168;
              }
              else if (0x167 < sVar10) {
                sVar10 = sVar10 + -0x168;
              }
              *(short *)((longlong)piVar9 + 0xe) = sVar10;
              FUN_18003c720((longlong)piVar9,param_3,iVar6 << 0x10,iVar3 << 0x10,param_8);
              lVar15 = (longlong)param_6;
              iVar13 = 0;
              if (0 < lVar15) {
                puVar12 = (ulonglong *)(piVar4 + 4);
                do {
                  iVar13 = iVar13 + (int)POPCOUNT(*(ulonglong *)
                                                   (((longlong)(piVar9 + 4) - (longlong)(piVar4 + 4)
                                                    ) + (longlong)puVar12) ^ *puVar12);
                  lVar15 = lVar15 + -1;
                  puVar12 = puVar12 + 1;
                } while (lVar15 != 0);
              }
              if (iVar13 <= param_5) {
                *piVar9 = *piVar4;
                piVar9[1] = piVar4[1];
                piVar9[2] = piVar4[2];
                piVar8 = piVar9;
                goto LAB_1800316c3;
              }
            }
          }
        }
LAB_18003171e:
        local_74 = local_74 + 1;
        local_68 = local_68 + 8;
        _Memory = _Memory + 1;
        pcVar14 = local_40;
      } while (local_74 < *param_1);
    }
    _Memory = pcVar14;
    local_78 = local_78 + -1;
  } while( 1 );
}

int FUN_1800317b0(uint *param_1,uint *param_2,longlong param_3,undefined8 *param_4,int param_5,
                 char *param_6,longlong *param_7)
{
  static unsigned char _frame_backing[10512] __attribute__((aligned(16)));
  __builtin_memset(_frame_backing, 0, sizeof(_frame_backing));
  unsigned char *_frame = _frame_backing + 0x1000;

  int iVar1;
  short *psVar2;
  short *psVar3;
  ushort uVar4;
  int bVar5;
  ulonglong uVar6;
  short sVar7;
  int iVar8;
  uint uVar9;
  undefined8 uVar10;
  longlong *plVar11;
  int *piVar12;
  longlong *plVar13;
  void *pvVar14;
  uint uVar15;
  uint uVar16;
  char *pcVar17;
  ulonglong *puVar18;
  longlong lVar19;
  uint uVar20;
  int *piVar21;
  uint uVar22;
  int iVar23;
  ulonglong uVar24;
  ulonglong uVar25;
  ulonglong uVar26;
  longlong lVar27;
  ulonglong uVar28;
  uint *puVar29;
  uint *puVar30;
  longlong *plVar31;
  ulonglong *puVar32;
  uint uVar33;
  ulonglong uVar34;
  ulonglong uVar35;






























  uVar22 = *param_2;
  uVar15 = *param_1;
  (*((int*)(_frame+2168))) = *(int *)(param_3 + 0x40);
  uVar16 = uVar15;
  if ((int)uVar22 < (int)uVar15) {
    uVar16 = uVar22;
  }
  (*((int*)(_frame+2040))) = 0;
  if (((int)uVar22 < 1) || ((int)uVar15 < 1)) {
    if (param_4 != (undefined8 *)0x0) {
      *(undefined4 *)param_4 = 0;
    }
    return 0;
  }
  if (param_5 == 0) {
    (*((int*)(_frame+2144))) = *(int *)(param_3 + 0x48);
    if ((byte)param_2[2] < 3) {
      iVar23 = 3;
      if (1 < (byte)param_2[2]) {
        iVar23 = 6;
      }
    }
    else {
      iVar23 = 8;
    }
    (*((int*)(_frame+2128))) = 0x80;
  }
  else {
    (*((int*)(_frame+2128))) = 0x20;
    (*((int*)(_frame+2144))) = (int)((*(int *)(param_3 + 0x48) >> 0x1f & 3U) + *(int *)(param_3 + 0x48)) >> 2;
    iVar23 = (1 < (byte)param_2[2]) + 1;
  }
  iVar8 = *(int *)(param_3 + 0x44) * (*((int*)(_frame+2144))) + 0x200;
  (*((int*)(_frame+2120))) = (int)(iVar8 + (iVar8 >> 0x1f & 0x3ffU)) >> 10;
  (*((int*)(_frame+2124))) = iVar23;
  uVar10 = FUN_180035490(((uint*)(_frame+2064)),uVar16);
  if ((int)uVar10 != 0) {
    return -1000;
  }
  if ((*(int *)(param_3 + 100) == 1) && (uVar10 = FUN_180035490(((uint*)(_frame+2176)),uVar16), (int)uVar10 != 0))
  {
    if ((*((uint**)(_frame+2072))) != (uint *)0x0) {
      free((*((uint**)(_frame+2072))));
    }
    if ((*((longlong**)(_frame+2096))) != (longlong *)0x0) {
      free((*((longlong**)(_frame+2096))));
    }
    if ((*((longlong**)(_frame+2104))) == (longlong *)0x0) {
      return -1000;
    }
    free((*((longlong**)(_frame+2104))));
    return -1000;
  }
  (*((longlong*)(_frame+2136))) = (longlong)iVar23;
  (*((uint*)(_frame+2044))) = 0;
  uVar22 = (*((uint*)(_frame+2064)));
  if ((int)*param_1 < 1) {
    (*((uint*)(_frame+2056))) = ((uint*)(_frame+2176))[0];
  }
  else {
    pcVar17 = (char *)(**(longlong **)(param_1 + 6) + 0xc);
    (*((longlong*)(_frame+2152))) = 0;
    (*((uint*)(_frame+2060))) = (*((uint*)(_frame+2064)));
    (*((uint*)(_frame+2056))) = ((uint*)(_frame+2176))[0];
    do {
      uVar26 = 0xffffffffffffffff;
      uVar15 = 0x1e8;
      iVar23 = -1;
      (*((ulonglong*)(_frame+2160))) = 0xffffffffffffffff;
      uVar16 = 0x1e8;
      if (-1 < *(short *)(pcVar17 + 2)) {
        if (param_5 != 0) {
          uVar9 = (*((uint*)(_frame+2044))) & 0x80000003;
          if ((int)uVar9 < 0) {
            uVar9 = (uVar9 - 1 | 0xfffffffc) + 1;
          }
          if (uVar9 == 0) goto LAB_180031bee;
        }
        uVar25 = 0;
        (*((longlong**)(_frame+2232))) = *(longlong **)(param_2 + 6);
        uVar9 = ((uint*)(_frame+2176))[0];
        if (0 < (int)*param_2) {
          puVar32 = (ulonglong *)(*(*((longlong**)(_frame+2232))) + 0x10);
          uVar24 = uVar25;
          uVar34 = 0xffffffff;
          uVar33 = uVar15;
          do {
            uVar35 = uVar34;
            uVar22 = uVar16;
            uVar6 = (*((ulonglong*)(_frame+2160)));
            uVar15 = uVar33;
            if ((*pcVar17 == *(char *)((longlong)puVar32 + -4)) &&
               (-1 < *(short *)((longlong)puVar32 + -2))) {
              uVar9 = 0;
              uVar28 = uVar26;
              if (0 < (*((longlong*)(_frame+2136)))) {
                puVar18 = puVar32;
                lVar27 = (*((longlong*)(_frame+2136)));
                do {
                  uVar9 = uVar9 + (int)POPCOUNT(*(ulonglong *)
                                                 (pcVar17 + (4 - (longlong)puVar32) +
                                                 (longlong)puVar18) ^ *puVar18);
                  lVar27 = lVar27 + -1;
                  puVar18 = puVar18 + 1;
                  uVar28 = (*((ulonglong*)(_frame+2160)));
                } while (lVar27 != 0);
              }
              uVar26 = uVar24;
              uVar35 = uVar25;
              uVar22 = uVar9;
              uVar6 = uVar24;
              uVar15 = uVar16;
              if (((int)uVar16 <= (int)uVar9) &&
                 (uVar26 = uVar28, uVar35 = uVar34, uVar22 = uVar16, uVar6 = (*((ulonglong*)(_frame+2160))),
                 uVar15 = uVar33, (int)uVar9 < (int)uVar33)) {
                uVar15 = uVar9;
              }
            }
            (*((ulonglong*)(_frame+2160))) = uVar6;
            uVar16 = uVar22;
            iVar23 = (int)uVar35;
            uVar20 = (int)uVar25 + 1;
            uVar25 = (ulonglong)uVar20;
            uVar24 = uVar24 + 1;
            puVar32 = puVar32 + 10;
            uVar34 = uVar35;
            uVar22 = (*((uint*)(_frame+2060)));
            uVar9 = (*((uint*)(_frame+2056)));
            uVar33 = uVar15;
          } while ((int)uVar20 < (int)*param_2);
        }
        ((uint*)(_frame+2176))[0] = uVar9;
        if ((int)uVar16 <= (*((int*)(_frame+2144)))) {
          if (((*((int*)(_frame+2168))) == 0) || ((int)(uVar16 * 0x400) < (int)(uVar15 * (*((int*)(_frame+2168)))))) {
            FUN_180030c20((int *)((uint*)(_frame+2064)),(longlong)param_1,(*((uint*)(_frame+2044))),(longlong)param_2,iVar23,
                          uVar16);
            uVar26 = 0;
            uVar22 = (*((uint*)(_frame+2064)));
            if ((*(int *)(param_3 + 100) == 1) && (0 < (int)uVar9)) {
              uVar25 = uVar26;
              do {
                if ((*((longlong**)(_frame+2216)))[uVar26] == *(longlong *)(*(longlong *)(param_2 + 6) + (*((ulonglong*)(_frame+2160))) * 8)) {
                  if (-1 < (int)uVar25) {
                    (*((longlong**)(_frame+2216)))[uVar26] = 0;
                    (*((uint*)(_frame+2060))) = (*((uint*)(_frame+2064)));
                    goto LAB_180031bee;
                  }
                  break;
                }
                uVar15 = (int)uVar25 + 1;
                uVar25 = (ulonglong)uVar15;
                uVar26 = uVar26 + 1;
              } while ((int)uVar15 < (int)uVar9);
            }
            (*((uint*)(_frame+2060))) = (*((uint*)(_frame+2064)));
          }
          else {
            uVar25 = 0;
            if (((int)uVar16 < (*((int*)(_frame+2120)))) && (*(int *)(param_3 + 100) != 0)) {
              if (0 < (int)uVar22) {
                plVar11 = (*((longlong**)(_frame+2104)));
                uVar24 = uVar25;
                do {
                  if (*plVar11 == (*((longlong**)(_frame+2232)))[uVar26]) {
                    if (-1 < (int)uVar24) goto LAB_180031bee;
                    break;
                  }
                  uVar15 = (int)uVar24 + 1;
                  uVar24 = (ulonglong)uVar15;
                  plVar11 = plVar11 + 1;
                } while ((int)uVar15 < (int)uVar22);
              }
              if (0 < (int)uVar9) {
                uVar24 = uVar25;
                do {
                  if ((*((longlong**)(_frame+2216)))[uVar24] == (*((longlong**)(_frame+2232)))[uVar26]) {
                    if (-1 < (longlong)uVar24) {
                      if ((int)uVar16 < (int)(*((uint**)(_frame+2184)))[uVar24]) {
                        *(undefined8 *)((longlong)(*((void**)(_frame+2208))) + uVar24 * 8) =
                             *(undefined8 *)((*((longlong*)(_frame+2152))) + *(longlong *)(param_1 + 6));
                        (*((uint**)(_frame+2184)))[uVar24] = uVar16;
                      }
                      goto LAB_180031bee;
                    }
                    break;
                  }
                  uVar15 = (int)uVar25 + 1;
                  uVar25 = (ulonglong)uVar15;
                  uVar24 = uVar24 + 1;
                } while ((int)uVar15 < (int)uVar9);
              }
              lVar27 = (longlong)(int)uVar9;
              (*((uint*)(_frame+2056))) = uVar9 + 1;
              *(undefined8 *)((longlong)(*((void**)(_frame+2208))) + lVar27 * 8) =
                   *(undefined8 *)((*((longlong*)(_frame+2152))) + *(longlong *)(param_1 + 6));
              (*((longlong**)(_frame+2216)))[lVar27] = *(longlong *)(*(longlong *)(param_2 + 6) + uVar26 * 8);
              (*((uint**)(_frame+2184)))[lVar27] = uVar16;
              ((uint*)(_frame+2176))[0] = (*((uint*)(_frame+2056)));
            }
          }
        }
      }
LAB_180031bee:
      (*((longlong*)(_frame+2152))) = (*((longlong*)(_frame+2152))) + 8;
      (*((uint*)(_frame+2044))) = (*((uint*)(_frame+2044))) + 1;
      pcVar17 = pcVar17 + 0x50;
    } while ((int)(*((uint*)(_frame+2044))) < (int)*param_1);
  }
  lVar27 = 0;
  if (0 < (int)uVar22) {
    iVar23 = FUN_180030b40(((uint*)(_frame+2064)),*(int *)(param_3 + 0x4c));
    if (iVar23 < 2) {
      uVar22 = 0;
      if (0 < (int)(*((uint*)(_frame+2064)))) {
        uVar26 = (ulonglong)(*((uint*)(_frame+2064)));
        lVar19 = lVar27;
        plVar11 = (*((longlong**)(_frame+2104)));
        plVar31 = (*((longlong**)(_frame+2104)));
        do {
          if (((*((uint**)(_frame+2072)))[lVar27] >> 0xf & 1) == 0) {
            if (lVar27 != lVar19) {
              *(undefined8 *)(((longlong)(*((longlong**)(_frame+2096))) - (longlong)(*((longlong**)(_frame+2104)))) + (longlong)plVar11) =
                   *(undefined8 *)(((longlong)(*((longlong**)(_frame+2096))) - (longlong)(*((longlong**)(_frame+2104)))) + (longlong)plVar31);
              *plVar11 = *plVar31;
              (*((uint**)(_frame+2072)))[lVar19] = (*((uint**)(_frame+2072)))[lVar27];
            }
            uVar22 = uVar22 + 1;
            lVar19 = lVar19 + 1;
            plVar11 = plVar11 + 1;
          }
          else {
            (*((uint**)(_frame+2072)))[lVar27] = (*((uint**)(_frame+2072)))[lVar27] & 0xffff7fff;
          }
          lVar27 = lVar27 + 1;
          plVar31 = plVar31 + 1;
          uVar26 = uVar26 - 1;
        } while (uVar26 != 0);
      }
      iVar23 = 0;
      (*((uint*)(_frame+2064))) = uVar22;
      goto LAB_180031fec;
    }
    FUN_180030de0((int *)param_2,(*((short*)(_frame+2084))));
    iVar23 = *(int *)(param_3 + 0x50);
    iVar8 = FUN_180030900(((uint*)(_frame+2064)),0,iVar23);
    uVar26 = (ulonglong)(*((uint*)(_frame+2064)));
    sVar7 = (short)iVar8;
    if (0 < (int)(*((uint*)(_frame+2064)))) {
      uVar25 = (ulonglong)(*((uint*)(_frame+2064)));
      puVar29 = (*((uint**)(_frame+2072)));
      plVar11 = (*((longlong**)(_frame+2096)));
      do {
        if ((*puVar29 >> 0xf & 1) == 0) {
          psVar2 = *(short **)(((longlong)(*((longlong**)(_frame+2104))) - (longlong)(*((longlong**)(_frame+2096)))) + (longlong)plVar11);
          psVar3 = (short *)*plVar11;
          iVar8 = (int)sVar7;
          if (((int)*psVar3 - (int)*psVar2) + iVar8 < 0) {
            iVar8 = ((int)*psVar2 - (int)*psVar3) - iVar8;
          }
          else {
            iVar8 = ((int)*psVar3 - (int)*psVar2) + iVar8;
          }
          if (iVar23 < iVar8) {
            *puVar29 = *puVar29 | 0x8000;
          }
        }
        plVar11 = plVar11 + 1;
        puVar29 = puVar29 + 1;
        uVar25 = uVar25 - 1;
      } while (uVar25 != 0);
    }
    iVar23 = *(int *)(param_3 + 0x50);
    (*((short*)(_frame+2086))) = sVar7;
    iVar8 = FUN_180030900(((uint*)(_frame+2064)),4,iVar23);
    plVar11 = (*((longlong**)(_frame+2096)));
    puVar29 = (*((uint**)(_frame+2072)));
    (*((short*)(_frame+2088))) = (short)iVar8;
    if (0 < (int)(*((uint*)(_frame+2064)))) {
      uVar25 = uVar26;
      puVar30 = (*((uint**)(_frame+2072)));
      plVar31 = (*((longlong**)(_frame+2096)));
      do {
        if ((*puVar30 >> 0xf & 1) == 0) {
          lVar27 = *(longlong *)((longlong)plVar31 + ((longlong)(*((longlong**)(_frame+2104))) - (longlong)(*((longlong**)(_frame+2096)))));
          lVar19 = *plVar31;
          iVar8 = (int)(*((short*)(_frame+2088)));
          if (((int)*(short *)(lVar19 + 4) - (int)*(short *)(lVar27 + 4)) + iVar8 < 0) {
            iVar8 = ((int)*(short *)(lVar27 + 4) - (int)*(short *)(lVar19 + 4)) - iVar8;
          }
          else {
            iVar8 = ((int)*(short *)(lVar19 + 4) - (int)*(short *)(lVar27 + 4)) + iVar8;
          }
          if (iVar23 < iVar8) {
            *puVar30 = *puVar30 | 0x8000;
          }
        }
        plVar31 = plVar31 + 1;
        puVar30 = puVar30 + 1;
        uVar25 = uVar25 - 1;
      } while (uVar25 != 0);
    }
    uVar22 = 0;
    if (0 < (int)(*((uint*)(_frame+2064)))) {
      lVar27 = 0;
      lVar19 = 0;
      plVar31 = (*((longlong**)(_frame+2104)));
      plVar13 = (*((longlong**)(_frame+2104)));
      do {
        if (((*((uint**)(_frame+2072)))[lVar27] >> 0xf & 1) == 0) {
          if (lVar27 != lVar19) {
            *(undefined8 *)(((longlong)(*((longlong**)(_frame+2096))) - (longlong)(*((longlong**)(_frame+2104)))) + (longlong)plVar31) =
                 *(undefined8 *)(((longlong)(*((longlong**)(_frame+2096))) - (longlong)(*((longlong**)(_frame+2104)))) + (longlong)plVar13);
            *plVar31 = *plVar13;
            (*((uint**)(_frame+2072)))[lVar19] = (*((uint**)(_frame+2072)))[lVar27];
          }
          uVar22 = uVar22 + 1;
          lVar19 = lVar19 + 1;
          plVar31 = plVar31 + 1;
        }
        else {
          (*((uint**)(_frame+2072)))[lVar27] = (*((uint**)(_frame+2072)))[lVar27] & 0xffff7fff;
        }
        lVar27 = lVar27 + 1;
        plVar13 = plVar13 + 1;
        uVar26 = uVar26 - 1;
      } while (uVar26 != 0);
    }
    (*((uint*)(_frame+2064))) = uVar22;
    FUN_180030700((int *)param_2,(int)sVar7,(int)(*((short*)(_frame+2088))));
    if (((1 < (int)uVar22) && (*(int *)(param_3 + 100) == 1)) && (0 < (int)(*((uint*)(_frame+2056))))) {
      uVar26 = (ulonglong)(*((uint*)(_frame+2056)));
      plVar31 = (*((longlong**)(_frame+2216)));
      puVar30 = (*((uint**)(_frame+2184)));
      do {
        piVar21 = (int *)*plVar31;
        if (piVar21 != (int *)0x0) {
          piVar12 = *(int **)(((longlong)(*((void**)(_frame+2208))) - (longlong)(*((longlong**)(_frame+2216)))) + (longlong)plVar31);
          uVar15 = (uint)(piVar21[1] - piVar12[1]) >> 0x1f;
          if (((int)((piVar21[1] - piVar12[1] ^ uVar15) - uVar15) <= *(int *)(param_3 + 0x50)) &&
             (uVar15 = (uint)(*piVar21 - *piVar12) >> 0x1f,
             (int)((*piVar21 - *piVar12 ^ uVar15) - uVar15) <= *(int *)(param_3 + 0x50))) {
            iVar23 = (int)*(short *)((longlong)piVar21 + 0xe) -
                     (int)*(short *)((longlong)piVar12 + 0xe);
            uVar4 = (ushort)(iVar23 >> 0x1f);
            sVar7 = ((ushort)iVar23 ^ uVar4) - uVar4;
            if (0xb4 < sVar7) {
              sVar7 = 0x168 - sVar7;
            }
            if ((int)sVar7 <= *(int *)(param_3 + 0x4c)) {
              (*((uint*)(_frame+2064))) = uVar22 + 1;
              plVar11[(int)uVar22] = (longlong)piVar12;
              (*((longlong**)(_frame+2104)))[(int)uVar22] = *plVar31;
              puVar29[(int)uVar22] = *puVar30;
              uVar22 = (*((uint*)(_frame+2064)));
            }
          }
        }
        plVar31 = plVar31 + 1;
        puVar30 = puVar30 + 1;
        uVar26 = uVar26 - 1;
      } while (uVar26 != 0);
    }
  }
  iVar23 = (*((int*)(_frame+2040)));
  if (*(int *)(param_3 + 0x58) <= (int)uVar22) {
    if (param_7 == (longlong *)0x0) {
      iVar8 = 0;
      iVar23 = 0;
      piVar21 = (int *)**(undefined8 **)(param_1 + 6);
      if (0 < (int)*param_1) {
        do {
          plVar31 = (*((longlong**)(_frame+2104)));
          plVar11 = (*((longlong**)(_frame+2096)));
          uVar26 = 0;
          if (-1 < *(short *)((longlong)piVar21 + 0xe)) {
            if (0 < (int)uVar22) {
              plVar13 = (*((longlong**)(_frame+2096)));
              uVar25 = uVar26;
              do {
                if (*(int **)(((longlong)(*((longlong**)(_frame+2104))) - (longlong)(*((longlong**)(_frame+2096)))) + (longlong)plVar13) ==
                    piVar21) {
                  lVar27 = (*((longlong**)(_frame+2096)))[uVar25];
LAB_1800320a4:
                  if (lVar27 != 0) goto LAB_1800321f4;
                  break;
                }
                if ((int *)*plVar13 == piVar21) {
                  lVar27 = (*((longlong**)(_frame+2104)))[uVar25];
                  goto LAB_1800320a4;
                }
                uVar15 = (int)uVar26 + 1;
                uVar26 = (ulonglong)uVar15;
                uVar25 = uVar25 + 1;
                plVar13 = plVar13 + 1;
              } while ((int)uVar15 < (int)uVar22);
            }
            piVar12 = FUN_180030ce0((int *)param_2,piVar21,9,(int *)0x0);
            if (piVar12 != (int *)0x0) {
              if ((char)piVar12[3] == (char)piVar21[3]) {
                iVar23 = (int)*(short *)((longlong)piVar21 + 0xe) -
                         (int)*(short *)((longlong)piVar12 + 0xe);
                uVar4 = (ushort)(iVar23 >> 0x1f);
                sVar7 = ((ushort)iVar23 ^ uVar4) - uVar4;
                if (0xb4 < sVar7) {
                  sVar7 = 0x168 - sVar7;
                }
                uVar26 = 0;
                if ((int)sVar7 <= *(int *)(param_3 + 0x4c)) {
                  uVar15 = 0;
                  if (0 < (*((longlong*)(_frame+2136)))) {
                    puVar32 = (ulonglong *)(piVar21 + 4);
                    uVar25 = uVar26;
                    lVar27 = (*((longlong*)(_frame+2136)));
                    do {
                      uVar15 = (int)uVar25 +
                               (int)POPCOUNT(*(ulonglong *)
                                              ((longlong)piVar12 + (0x10 - (longlong)(piVar21 + 4))
                                              + (longlong)puVar32) ^ *puVar32);
                      uVar25 = (ulonglong)uVar15;
                      lVar27 = lVar27 + -1;
                      puVar32 = puVar32 + 1;
                    } while (lVar27 != 0);
                  }
                  uVar16 = uVar22;
                  if ((int)uVar15 < (*((int*)(_frame+2120)))) {
                    if (0 < (int)uVar22) {
                      plVar13 = plVar11;
                      uVar25 = uVar26;
                      do {
                        if (*(int **)(((longlong)plVar31 - (longlong)plVar11) + (longlong)plVar13)
                            == piVar12) {
                          lVar27 = plVar11[uVar25];
LAB_180032194:
                          if (lVar27 != 0) goto LAB_1800321f4;
                          break;
                        }
                        if ((int *)*plVar13 == piVar12) {
                          lVar27 = plVar31[uVar25];
                          goto LAB_180032194;
                        }
                        uVar16 = (int)uVar26 + 1;
                        uVar26 = (ulonglong)uVar16;
                        uVar25 = uVar25 + 1;
                        plVar13 = plVar13 + 1;
                      } while ((int)uVar16 < (int)uVar22);
                    }
                    uVar16 = uVar22 + 1;
                    plVar11[(int)uVar22] = (longlong)piVar21;
                    plVar31[(int)uVar22] = (longlong)piVar12;
                    (*((uint**)(_frame+2072)))[(int)uVar22] = uVar15;
                    (*((uint*)(_frame+2064))) = uVar16;
                  }
                  uVar22 = uVar16;
                  if (0x100 < (int)uVar15) {
                    if (*(char *)((longlong)piVar21 + 0xd) == -5) {
                      (*((int*)(_frame+2040))) = (*((int*)(_frame+2040))) - ((int)(uVar15 + ((int)uVar15 >> 0x1f & 3U)) >> 2);
                    }
                    else {
                      (*((int*)(_frame+2040))) = (*((int*)(_frame+2040))) - (int)uVar15 / 2;
                    }
                  }
                }
              }
              else {
                (*((int*)(_frame+2040))) = (*((int*)(_frame+2040))) + -2;
              }
            }
          }
LAB_1800321f4:
          iVar8 = iVar8 + 1;
          piVar21 = piVar21 + 0x14;
          iVar23 = (*((int*)(_frame+2040)));
        } while (iVar8 < (int)*param_1);
      }
    }
    else {
      FUN_180030ff0(param_7,*(ulonglong *)(param_2 + 4),(uint)(ushort)param_2[1],
                    (uint)*(ushort *)((longlong)param_2 + 6),param_3);
      uVar10 = FUN_1800312f0((int *)param_1,param_2,param_7,(int *)((uint*)(_frame+2064)),(*((int*)(_frame+2120))),(*((int*)(_frame+2124))),
                             param_6,param_3);
      uVar22 = (*((uint*)(_frame+2064)));
      iVar23 = (int)uVar10;
    }
  }
LAB_180031fec:
  plVar11 = (*((longlong**)(_frame+2096)));
  if (*(int *)(param_3 + 100) == 1) {
    if ((*((uint**)(_frame+2184))) != (uint *)0x0) {
      free((*((uint**)(_frame+2184))));
    }
    if ((*((void**)(_frame+2208))) != (void *)0x0) {
      free((*((void**)(_frame+2208))));
    }
    if ((*((longlong**)(_frame+2216))) != (longlong *)0x0) {
      free((*((longlong**)(_frame+2216))));
    }
  }
  if ((param_6 == (char *)0x0) || ((int)uVar22 < *(int *)(param_3 + 0x58))) {
    bVar5 = 0;
  }
  else {
    bVar5 = 1;
  }
  lVar19 = 0;
  lVar27 = (longlong)(int)uVar22;
  if (0 < (int)uVar22) {
    do {
      iVar8 = (*((int*)(_frame+2128))) - (*((uint**)(_frame+2072)))[lVar19];
      if (param_5 != 0) {
        iVar8 = iVar8 * 4;
      }
      iVar1 = *(int *)(param_3 + 0x94);
      if ((0 < iVar1) && (iVar8 < iVar1 * 2)) {
        iVar8 = iVar1 + iVar8 / 2;
      }
      iVar23 = iVar23 + iVar8;
      if (bVar5) {
        iVar1 = (int)(((*((longlong**)(_frame+2104)))[lVar19] - **(longlong **)(param_2 + 6)) / 0x50);
        if (param_6[iVar1] == '\x01') {
          (*((short*)(_frame+2112))) = (*((short*)(_frame+2112))) + (short)iVar8;
          (*((short*)(_frame+2114))) = (*((short*)(_frame+2114))) + 1;
        }
        else {
          param_6[iVar1] = '\x01';
        }
      }
      lVar19 = lVar19 + 1;
      plVar11 = (*((longlong**)(_frame+2096)));
    } while (lVar19 < lVar27);
  }
  if (iVar23 < 0) {
    iVar23 = 0;
  }
  if (param_4 == (undefined8 *)0x0) {
    if ((*((uint**)(_frame+2072))) != (uint *)0x0) {
      free((*((uint**)(_frame+2072))));
    }
    if ((*((longlong**)(_frame+2096))) != (longlong *)0x0) {
      free((*((longlong**)(_frame+2096))));
    }
    if ((*((longlong**)(_frame+2104))) != (longlong *)0x0) {
      free((*((longlong**)(_frame+2104))));
      return iVar23;
    }
    return iVar23;
  }
  (*((uint**)(_frame+2072))) = realloc((*((uint**)(_frame+2072))),lVar27 * 4);
  (*((longlong**)(_frame+2096))) = realloc(plVar11,lVar27 * 8);
  pvVar14 = realloc((*((longlong**)(_frame+2104))),lVar27 * 8);
  *param_4 = CONCAT44((*((undefined4*)(_frame+2068))),(*((uint*)(_frame+2064))));
  param_4[1] = (undefined8)(*((uint**)(_frame+2072)));
  param_4[2] = CONCAT26((*((short*)(_frame+2086))),CONCAT24((*((short*)(_frame+2084))),(*((undefined4*)(_frame+2080)))));
  param_4[3] = CONCAT62((*((undefined8*)(_frame+2090))),(*((short*)(_frame+2088))));
  param_4[4] = (undefined8)(*((longlong**)(_frame+2096)));
  param_4[5] = (undefined8)pvVar14;
  param_4[6] = CONCAT44((*((undefined4*)(_frame+2116))),CONCAT22((*((short*)(_frame+2114))),(*((short*)(_frame+2112)))));
  return iVar23;
}

int FUN_1800323e0(uint *param_1,uint *param_2,longlong param_3,undefined8 *param_4)
{
  int iVar1;
  longlong *_Memory;

  _Memory = (longlong *)0x0;
  if (*(longlong *)(param_2 + 4) != 0) {
    _Memory = calloc(1,0x18);
  }
  iVar1 = FUN_1800317b0(param_1,param_2,param_3,param_4,0,(char *)0x0,_Memory);
  if (_Memory != (longlong *)0x0) {
    if ((void *)*_Memory != (void *)0x0) {
      free((void *)*_Memory);
    }
    if ((void *)_Memory[1] != (void *)0x0) {
      free((void *)_Memory[1]);
    }
    if ((void *)_Memory[2] != (void *)0x0) {
      free((void *)_Memory[2]);
    }
    free(_Memory);
  }
  return iVar1;
}

int FUN_180032770(int *param_1,int *param_2,longlong param_3,longlong param_4,int param_5)
{
  int iVar1;
  char *_Memory;
  longlong *_Memory_00;
  uint *_Memory_01;
  int *_Memory_02;
  ulonglong uVar2;
  int iVar3;
  int iVar4;
  longlong lVar5;
  int local_res10;

  iVar4 = 0;
  _Memory_00 = (longlong *)0x0;
  local_res10 = 0;
  if (*param_2 < 1) {
    return 0;
  }
  _Memory = calloc((longlong)*param_2,1);
  if (_Memory == (char *)0x0) {
    iVar1 = -1000;
  }
  else {
    if (*(longlong *)(param_2 + 4) != 0) {
      _Memory_00 = calloc(1,0x18);
    }
    iVar3 = 0;
    iVar1 = 0;
    if (0 < *param_1) {
      lVar5 = 0;
      do {
        _Memory_01 = (uint *)FUN_1800353b0(param_2);
        _Memory_02 = calloc(1,0x38);
        if (_Memory_01 == (uint *)0x0) goto LAB_1800328f0;
        if (_Memory_02 == (int *)0x0) {
LAB_180032935:
          if (_Memory_01 != (uint *)0x0) {
            free(*(void **)(_Memory_01 + 4));
            free(_Memory_01);
          }
        }
        else {
          *(undefined8 *)(_Memory_01 + 4) = *(undefined8 *)(param_2 + 4);
          iVar1 = FUN_1800317b0(*(uint **)(*(longlong *)(param_1 + 2) + lVar5),_Memory_01,param_4,
                                (undefined8 *)_Memory_02,param_5,_Memory,_Memory_00);
          _Memory_01[4] = 0;
          _Memory_01[5] = 0;
          _Memory_02[4] = iVar1;
          if (*_Memory_02 < *(int *)(param_4 + 0x58)) {
            if (param_3 != 0) {
              *(undefined8 *)(lVar5 + *(longlong *)(param_3 + 0x10)) = 0;
            }
LAB_1800328f0:
            if (_Memory_02 != (int *)0x0) {
              if (*(void **)(_Memory_02 + 2) != (void *)0x0) {
                free(*(void **)(_Memory_02 + 2));
              }
              if (*(void **)(_Memory_02 + 8) != (void *)0x0) {
                free(*(void **)(_Memory_02 + 8));
              }
              if (*(void **)(_Memory_02 + 10) != (void *)0x0) {
                free(*(void **)(_Memory_02 + 10));
              }
              _Memory_02[2] = 0;
              _Memory_02[3] = 0;
              _Memory_02[8] = 0;
              _Memory_02[9] = 0;
              _Memory_02[10] = 0;
              _Memory_02[0xb] = 0;
              free(_Memory_02);
            }
            goto LAB_180032935;
          }
          if (param_5 == 0) {
            FUN_1800324a0(_Memory_02);
          }
          iVar4 = iVar4 + (*_Memory_02 - (int)*(short *)((longlong)_Memory_02 + 0x32));
          local_res10 = local_res10 + (iVar1 - (short)_Memory_02[0xc]);
          if (param_3 == 0) goto LAB_1800328f0;
          *(uint **)(lVar5 + *(longlong *)(param_3 + 0x18)) = _Memory_01;
          *(int **)(lVar5 + *(longlong *)(param_3 + 0x10)) = _Memory_02;
          *(int *)(param_3 + 4) = iVar4;
        }
        iVar3 = iVar3 + 1;
        lVar5 = lVar5 + 8;
        iVar1 = local_res10;
      } while (iVar3 < *param_1);
    }
    if (_Memory_00 != (longlong *)0x0) {
      if ((void *)*_Memory_00 != (void *)0x0) {
        free((void *)*_Memory_00);
      }
      if ((void *)_Memory_00[1] != (void *)0x0) {
        free((void *)_Memory_00[1]);
      }
      if ((void *)_Memory_00[2] != (void *)0x0) {
        free((void *)_Memory_00[2]);
      }
      free(_Memory_00);
    }
    if ((0 < iVar1) && (param_3 != 0)) {
      uVar2 = FUN_1800324f0(param_2,_Memory);
      *(int *)(param_3 + 8) = (int)uVar2;
    }
    free(_Memory);
  }
  return iVar1;
}

undefined8 FUN_180032d90(int *param_1,longlong param_2)
{
  byte bVar1;
  int *piVar2;
  ulonglong uVar3;
  short *psVar4;
  int iVar5;
  char cVar6;
  longlong lVar7;
  uint uVar8;
  uint uVar9;
  short local_res8 [4];

  cVar6 = '\0';
  bVar1 = *(byte *)(**(longlong **)(param_1 + 2) + 8);
  if ((*param_1 - 1U < 0x28) && (*(longlong **)(param_1 + 2) != (longlong *)0x0)) {
    uVar3 = FUN_180035bd0(param_1,local_res8,*(int *)(param_2 + 0x58));
    if (-1 < (int)uVar3) {
      iVar5 = *(int *)(param_2 + 0x88);
      uVar8 = (int)uVar3 + 0x20;
      uVar3 = (ulonglong)uVar8;
      if (iVar5 < (int)uVar8) {
        do {
          uVar8 = FUN_180032d40(param_1);
          if (iVar5 < (int)uVar3) {
            do {
              uVar8 = uVar8 + 10;
              if (0x1ffe < uVar8) break;
              lVar7 = (longlong)(*param_1 + -1);
              if (-1 < *param_1 + -1) {
                do {
                  iVar5 = 0;
                  piVar2 = *(int **)(*(longlong *)(param_1 + 2) + lVar7 * 8);
                  if (0 < *piVar2) {
                    psVar4 = (short *)(**(longlong **)(piVar2 + 6) + 0xe);
                    do {
                      if (((-1 < *psVar4) && (*(uint *)(psVar4 + -3) < uVar8)) &&
                         (*(char *)((longlong)psVar4 + -1) <= cVar6)) {
                        uVar9 = (int)uVar3 - (((int)((uint)bVar1 * 0xa2 + 7) >> 3) + 8);
                        uVar3 = (ulonglong)uVar9;
                        *psVar4 = -1;
                        if ((int)uVar9 <= *(int *)(param_2 + 0x88)) {
                          return 0;
                        }
                      }
                      iVar5 = iVar5 + 1;
                      psVar4 = psVar4 + 0x28;
                    } while (iVar5 < *piVar2);
                  }
                  lVar7 = lVar7 + -1;
                } while (-1 < lVar7);
              }
              iVar5 = *(int *)(param_2 + 0x88);
            } while (iVar5 < (int)uVar3);
          }
          cVar6 = cVar6 + '\x01';
        } while (cVar6 < '\x7f');
      }
      return 0;
    }
  }
  return 0xfffffc17;
}

void FUN_180033b50(longlong param_1,int param_2,int param_3,int *param_4,longlong param_5)
{
  longlong lVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  int *_Memory;
  uint *_Memory_00;
  void *pvVar5;
  ulonglong uVar6;
  undefined1 uVar7;
  uint uVar8;
  ulonglong uVar9;
  ulonglong uVar10;
  longlong lVar11;

  lVar11 = (longlong)param_3;
  _Memory = calloc(1,0x38);
  if (_Memory != (int *)0x0) {
    _Memory_00 = (uint *)FUN_1800353b0(param_4);
    lVar1 = (longlong)param_2 * 8;
    iVar4 = FUN_1800323e0(*(uint **)(lVar1 + *(longlong *)(param_1 + 0x48)),_Memory_00,param_5,
                          (undefined8 *)_Memory);
    iVar3 = *_Memory;
    uVar10 = 0;
    if ((iVar3 < 2) || (iVar4 < *(int *)(param_5 + 0x78))) {
      uVar7 = 0;
    }
    else {
      if (0xff < iVar3) {
        iVar3 = 0xff;
      }
      uVar7 = (undefined1)iVar3;
    }
    *(undefined1 *)(lVar11 + *(longlong *)(lVar1 + *(longlong *)(param_1 + 0x40))) = uVar7;
    if (*(byte *)(lVar11 + *(longlong *)(lVar1 + *(longlong *)(param_1 + 0x40))) < 2) {
      if (*(void **)(_Memory + 2) != (void *)0x0) {
        free(*(void **)(_Memory + 2));
      }
      if (*(void **)(_Memory + 8) != (void *)0x0) {
        free(*(void **)(_Memory + 8));
      }
      if (*(void **)(_Memory + 10) != (void *)0x0) {
        free(*(void **)(_Memory + 10));
      }
      _Memory[2] = 0;
      _Memory[3] = 0;
      _Memory[8] = 0;
      _Memory[9] = 0;
      _Memory[10] = 0;
      _Memory[0xb] = 0;
      free(_Memory);
    }
    else {
      uVar6 = uVar10;
      uVar9 = uVar10;
      if (0 < *_Memory) {
        do {
          cVar2 = *(char *)(*(longlong *)(uVar6 + *(longlong *)(_Memory + 8)) + 0xd);
          if (cVar2 < '\x7f') {
            *(char *)(*(longlong *)(uVar6 + *(longlong *)(_Memory + 8)) + 0xd) = cVar2 + '\x01';
          }
          cVar2 = *(char *)(*(longlong *)(*(longlong *)(_Memory + 10) + uVar6) + 0xd);
          if (cVar2 < '\x7f') {
            *(char *)(*(longlong *)(*(longlong *)(_Memory + 10) + uVar6) + 0xd) = cVar2 + '\x01';
          }
          uVar8 = (int)uVar9 + 1;
          uVar6 = uVar6 + 8;
          uVar9 = (ulonglong)uVar8;
        } while ((int)uVar8 < *_Memory);
      }
      uVar6 = uVar10;
      uVar9 = uVar10;
      if (0 < *_Memory) {
        do {
          uVar8 = (int)uVar6 + 1;
          *(longlong *)(*(longlong *)(_Memory + 10) + -8 + uVar9 + 8) =
               ((*(longlong *)(*(longlong *)(_Memory + 10) + uVar9) -
                **(longlong **)(_Memory_00 + 6)) / 0x50) * 0x50 +
               **(longlong **)(*(longlong *)(*(longlong *)(param_1 + 0x48) + lVar11 * 8) + 0x18);
          uVar6 = (ulonglong)uVar8;
          uVar9 = uVar9 + 8;
        } while ((int)uVar8 < *_Memory);
      }
      *(int **)(*(longlong *)(*(longlong *)(param_1 + 0x88) + lVar1) + lVar11 * 8) = _Memory;
      pvVar5 = malloc((longlong)(int)*_Memory_00);
      *(void **)(*(longlong *)(*(longlong *)(param_1 + 0x50) + lVar1) + lVar11 * 8) = pvVar5;
      uVar6 = uVar10;
      if (0 < (int)*_Memory_00) {
        do {
          uVar8 = (int)uVar10 + 1;
          uVar10 = (ulonglong)uVar8;
          *(undefined1 *)
           (uVar6 + *(longlong *)(*(longlong *)(*(longlong *)(param_1 + 0x50) + lVar1) + lVar11 * 8)
           ) = *(undefined1 *)
                (*(longlong *)(*(longlong *)(_Memory_00 + 6) + -8 + (uVar6 + 1) * 8) + 0xd);
          uVar6 = uVar6 + 1;
        } while ((int)uVar8 < (int)*_Memory_00);
      }
    }
    if (_Memory_00 != (uint *)0x0) {
      free(*(void **)(_Memory_00 + 4));
      free(_Memory_00);
    }
  }
  return;
}

int FUN_180033d80(uint *param_1,uint *param_2,longlong param_3)
{
  static unsigned char _frame_backing[10320] __attribute__((aligned(16)));
  __builtin_memset(_frame_backing, 0, sizeof(_frame_backing));
  unsigned char *_frame = _frame_backing + 0x1000;

  int iVar1;
  int iVar2;
  void *_Memory;
  undefined2 *puVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;







  iVar4 = 0;
  iVar5 = 0;
  _Memory = FUN_180032700((int *)param_2);
  iVar6 = 0;
  if (_Memory != (void *)0x0) {
    (*((undefined8*)(_frame+2040))) = 0;
    (*((void**)(_frame+2048))) = (void *)0x0;
    (*((undefined8*)(_frame+2056))) = 0;
    (*((undefined8*)(_frame+2064))) = 0;
    (*((void**)(_frame+2072))) = (void *)0x0;
    (*((void**)(_frame+2080))) = (void *)0x0;
    (*((undefined8*)(_frame+2088))) = 0;
    iVar1 = FUN_1800317b0(param_1,param_2,param_3,((undefined8*)(_frame+2040)),0,(char *)0x0,(longlong *)0x0);
    iVar6 = iVar4;
    if ((1 < (int)(*((undefined8*)(_frame+2040)))) && (iVar6 = iVar5, *(int *)(param_3 + 0x78) <= iVar1)) {
      iVar1 = *(int *)(param_3 + 0x74);
      iVar5 = iVar1 * 2 + 10;
      if (iVar1 != 0) {
        iVar2 = (int)(short)((*((undefined8*)(_frame+2056))) >> 0x30);
        if ((longlong)(*((undefined8*)(_frame+2056))) < 0) {
          iVar2 = -iVar2;
        }
        if (iVar2 < iVar1 * 4) {
          iVar2 = (int)(short)(*((undefined8*)(_frame+2064)));
          if ((short)(*((undefined8*)(_frame+2064))) < 0) {
            iVar2 = -iVar2;
          }
          if ((iVar2 < iVar1 * 4) &&
             (((int)(short)((*((undefined8*)(_frame+2056))) >> 0x20) < iVar5 ||
              (0x168 - iVar5 < (int)(short)((*((undefined8*)(_frame+2056))) >> 0x20))))) {
            iVar6 = 1;
          }
        }
      }
    }
    if ((*((void**)(_frame+2048))) != (void *)0x0) {
      free((*((void**)(_frame+2048))));
    }
    if ((*((void**)(_frame+2072))) != (void *)0x0) {
      free((*((void**)(_frame+2072))));
    }
    if ((*((void**)(_frame+2080))) != (void *)0x0) {
      free((*((void**)(_frame+2080))));
    }
    if (0 < (int)*param_2) {
      puVar3 = (undefined2 *)((longlong)_Memory + 8);
      puVar7 = (undefined4 *)**(undefined8 **)(param_2 + 6);
      do {
        iVar4 = iVar4 + 1;
        *puVar7 = *(undefined4 *)(puVar3 + -4);
        puVar7[1] = *(undefined4 *)(puVar3 + -2);
        *(undefined2 *)((longlong)puVar7 + 0xe) = *puVar3;
        puVar3 = puVar3 + 6;
        puVar7 = puVar7 + 0x14;
      } while (iVar4 < (int)*param_2);
    }
    free(_Memory);
  }
  return iVar6;
}

int * FUN_1800343d0(int *param_1,int *param_2,int *param_3,int param_4,longlong param_5)
{
  int iVar1;
  int iVar2;
  int iVar3;

  iVar1 = *param_1;
  FUN_180033480(param_1,param_2,param_3,param_4,param_5);
  iVar2 = *param_2;
  if (iVar2 != 0) {
    iVar3 = iVar2 + iVar1;
    if (0x1000 < iVar3) {
      iVar3 = 0x1000;
      iVar2 = 0x1000 - iVar1;
      *param_2 = iVar2;
    }
    param_1 = FUN_1800352f0(param_1,iVar3);
    if (param_1 != (int *)0x0) {
      FUN_18004d200(*(undefined8 **)(*(longlong *)(param_1 + 6) + (longlong)iVar1 * 8),
                    (undefined8 *)**(undefined8 **)(param_2 + 6),(longlong)iVar2 * 0x50);
      *param_1 = iVar3;
    }
  }
  return param_1;
}

/* ===== batch 7 ===== */

/* Ghidra intrinsic helpers */


/* ================ FUN_180034480 ================ */
undefined8 FUN_180034480(int *param_1,int *param_2,longlong param_3)

{
  static unsigned char _frame_backing[10384] __attribute__((aligned(16)));
  __builtin_memset(_frame_backing, 0, sizeof(_frame_backing));
  unsigned char *_frame = _frame_backing + 0x1000;

  char cVar1;
  uint *puVar2;
  uint *puVar3;
  longlong lVar4;
  int *piVar5;
  void *_Memory;
  void *_Memory_00;
  int iVar6;
  int *piVar7;
  undefined8 uVar8;
  void *pvVar9;
  longlong *plVar10;
  undefined2 *puVar11;
  longlong *plVar12;
  undefined4 *puVar13;
  ulonglong uVar14;
  longlong lVar15;
  int iVar16;
  ulonglong uVar17;
  uint uVar18;
  ulonglong uVar19;
  int iVar20;
  int local_res20;









  iVar6 = *(int *)(param_3 + 0x78);
  (*((int*)(_frame+2040))) = 0;
  if (param_2 == (int *)0x0) {
    return 0xfffffc16;
  }
  if (param_1 == (int *)0x0) {   /* guard: null featureset -> reject, don't segfault at *param_1 */
    return 0xfffffc16;
  }
  iVar16 = 0;
  iVar20 = 0;
  if (0 < *param_1) {
    lVar15 = 0;
    do {
      piVar7 = *(int **)(lVar15 + *(longlong *)(param_2 + 4));
      if ((piVar7 != (int *)0x0) && (iVar6 * 2 <= piVar7[4])) {
        iVar20 = iVar20 + 1;
        piVar7 = FUN_1800343d0(*(int **)(*(longlong *)(param_1 + 2) + lVar15),
                               *(int **)(lVar15 + *(longlong *)(param_2 + 6)),piVar7,1,param_3);
        if (piVar7 != (int *)0x0) {
          *(int **)(lVar15 + *(longlong *)(param_1 + 2)) = piVar7;
          pvVar9 = *(void **)(lVar15 + *(longlong *)(param_2 + 6));
          if (pvVar9 != (void *)0x0) {
            free(*(void **)((longlong)pvVar9 + 0x10));
            free(pvVar9);
          }
          *(undefined8 *)(lVar15 + *(longlong *)(param_2 + 6)) = 0;
        }
      }
      iVar16 = iVar16 + 1;
      lVar15 = lVar15 + 8;
    } while (iVar16 < *param_1);
    if (iVar20 != 0) {
      iVar6 = 0;
      if (0 < *param_1) {
        lVar15 = 0;
        do {
          if ((*(int **)(*(longlong *)(param_2 + 4) + lVar15) != (int *)0x0) &&
             (3 < **(int **)(*(longlong *)(param_2 + 4) + lVar15))) {
            if (-1 < iVar6) {
              if (*param_2 < 2) goto LAB_180034974;
              lVar15 = (longlong)iVar6;
              (*((longlong*)(_frame+2064))) = 0;
              local_res20 = 0;
              goto LAB_1800345f0;
            }
            break;
          }
          iVar6 = iVar6 + 1;
          lVar15 = lVar15 + 8;
        } while (iVar6 < *param_1);
      }
      uVar8 = FUN_180032d90(param_1,param_3);
      return uVar8;
    }
  }
  return 0xfffffc15;
LAB_1800345f0:
  if (((*((longlong*)(_frame+2064))) != lVar15) && (*(longlong *)(*(longlong *)(param_2 + 4) + (*((longlong*)(_frame+2064))) * 8) != 0)) {
    piVar7 = *(int **)(*(longlong *)(param_1 + 2) + (*((longlong*)(_frame+2064))) * 8);
    iVar6 = *piVar7;
    pvVar9 = malloc((longlong)iVar6 * 0xc);
    if (pvVar9 != (void *)0x0) {
      plVar10 = (longlong *)0x0;
      if (0 < iVar6) {
        puVar11 = (undefined2 *)((longlong)pvVar9 + 8);
        puVar13 = (undefined4 *)**(undefined8 **)(piVar7 + 6);
        plVar12 = plVar10;
        do {
          uVar18 = (int)plVar12 + 1;
          plVar12 = (longlong *)(ulonglong)uVar18;
          *(undefined4 *)(puVar11 + -4) = *puVar13;
          *(undefined4 *)(puVar11 + -2) = puVar13[1];
          *puVar11 = *(undefined2 *)((longlong)puVar13 + 0xe);
          puVar11 = puVar11 + 6;
          puVar13 = puVar13 + 0x14;
        } while ((int)uVar18 < *piVar7);
      }
      (*((ulonglong*)(_frame+2072))) = 0;
      (*((void**)(_frame+2080))) = (void *)0x0;
      (*((undefined8*)(_frame+2088))) = 0;
      (*((undefined8*)(_frame+2096))) = 0;
      (*((void**)(_frame+2104))) = (void *)0x0;
      (*((longlong**)(_frame+2112))) = (longlong *)0x0;
      (*((undefined8*)(_frame+2120))) = 0;
      puVar2 = *(uint **)(*(longlong *)(param_1 + 2) + (*((longlong*)(_frame+2064))) * 8);
      puVar3 = *(uint **)(*(longlong *)(param_1 + 2) + lVar15 * 8);
      if (*(longlong *)(puVar2 + 4) != 0) {
        plVar10 = calloc(1,0x18);
      }
      iVar6 = FUN_1800317b0(puVar3,puVar2,param_3,((ulonglong*)(_frame+2072)),0,(char *)0x0,plVar10);
      if (plVar10 != (longlong *)0x0) {
        if ((void *)*plVar10 != (void *)0x0) {
          free((void *)*plVar10);
        }
        if ((void *)plVar10[1] != (void *)0x0) {
          free((void *)plVar10[1]);
        }
        if ((void *)plVar10[2] != (void *)0x0) {
          free((void *)plVar10[2]);
        }
        free(plVar10);
      }
      plVar10 = (*((longlong**)(_frame+2112)));
      _Memory_00 = (*((void**)(_frame+2104)));
      if (iVar6 < *(int *)(param_3 + 0x78)) {
        iVar6 = 0;
        piVar7 = *(int **)(*(longlong *)(param_1 + 2) + (*((longlong*)(_frame+2064))) * 8);
        if (0 < *piVar7) {
          puVar11 = (undefined2 *)((longlong)pvVar9 + 8);
          puVar13 = (undefined4 *)**(undefined8 **)(piVar7 + 6);
          do {
            iVar6 = iVar6 + 1;
            *puVar13 = *(undefined4 *)(puVar11 + -4);
            puVar13[1] = *(undefined4 *)(puVar11 + -2);
            *(undefined2 *)((longlong)puVar13 + 0xe) = *puVar11;
            puVar11 = puVar11 + 6;
            puVar13 = puVar13 + 0x14;
          } while (iVar6 < *piVar7);
        }
      }
      else {
        if (0 < (int)(*((ulonglong*)(_frame+2072)))) {
          uVar17 = (*((ulonglong*)(_frame+2072))) & 0xffffffff;
          plVar12 = (*((longlong**)(_frame+2112)));
          do {
            lVar4 = *(longlong *)(((longlong)(*((void**)(_frame+2104))) - (longlong)(*((longlong**)(_frame+2112)))) + (longlong)plVar12);
            cVar1 = *(char *)(lVar4 + 0xd);
            if (cVar1 < '\x7f') {
              *(char *)(lVar4 + 0xd) = cVar1 + '\x01';
            }
            cVar1 = *(char *)(*plVar12 + 0xd);
            if (cVar1 < '{') {
              *(char *)(*plVar12 + 0xd) = cVar1 + '\x04';
            }
            plVar12 = plVar12 + 1;
            uVar17 = uVar17 - 1;
          } while (uVar17 != 0);
        }
        piVar7 = *(int **)(*(longlong *)(param_1 + 2) + lVar15 * 8);
        piVar5 = *(int **)(*(longlong *)(param_1 + 2) + (*((longlong*)(_frame+2064))) * 8);
        iVar6 = *piVar7;
        FUN_180033480(piVar7,piVar5,(int *)((ulonglong*)(_frame+2072)),1,param_3);
        iVar16 = *piVar5;
        if (iVar16 != 0) {
          iVar20 = iVar16 + iVar6;
          if (0x1000 < iVar20) {
            iVar20 = 0x1000;
            iVar16 = 0x1000 - iVar6;
            *piVar5 = iVar16;
          }
          piVar7 = FUN_1800352f0(piVar7,iVar20);
          if (piVar7 == (int *)0x0) goto LAB_1800348a7;
          FUN_18004d200(*(undefined8 **)(*(longlong *)(piVar7 + 6) + (longlong)iVar6 * 8),
                        (undefined8 *)**(undefined8 **)(piVar5 + 6),(longlong)iVar16 * 0x50);
          *piVar7 = iVar20;
        }
        if (piVar7 != (int *)0x0) {
          (*((int*)(_frame+2040))) = (*((int*)(_frame+2040))) + 1;
          *(int **)(*(longlong *)(param_1 + 2) + lVar15 * 8) = piVar7;
          _Memory = *(void **)(*(longlong *)(param_1 + 2) + (*((longlong*)(_frame+2064))) * 8);
          if (_Memory != (void *)0x0) {
            free(*(void **)((longlong)_Memory + 0x10));
            free(_Memory);
          }
          *(undefined8 *)(*(longlong *)(param_1 + 2) + (*((longlong*)(_frame+2064))) * 8) = 0;
        }
      }
LAB_1800348a7:
      free(pvVar9);
      if ((*((void**)(_frame+2080))) != (void *)0x0) {
        free((*((void**)(_frame+2080))));
      }
      if (_Memory_00 != (void *)0x0) {
        free(_Memory_00);
      }
      if (plVar10 != (longlong *)0x0) {
        free(plVar10);
      }
    }
  }
  uVar17 = 1;
  local_res20 = local_res20 + 1;
  (*((longlong*)(_frame+2064))) = (*((longlong*)(_frame+2064))) + 1;
  if (*param_1 <= local_res20) {
    if (0 < *param_1) {
      lVar15 = 0;
      uVar19 = uVar17;
      do {
        lVar4 = *(longlong *)(param_1 + 2);
        iVar6 = (int)uVar17;
        if ((*(longlong *)(lVar4 + lVar15) == 0) && (iVar6 < *param_1)) {
          plVar10 = (longlong *)(lVar4 + 8 + lVar15);
          uVar14 = uVar19;
          do {
            if (*plVar10 != 0) {
              *(undefined8 *)(lVar4 + lVar15) = *(undefined8 *)(lVar4 + uVar14 * 8);
              *(undefined8 *)(*(longlong *)(param_1 + 2) + uVar14 * 8) = 0;
              break;
            }
            uVar18 = (int)uVar17 + 1;
            uVar17 = (ulonglong)uVar18;
            uVar14 = uVar14 + 1;
            plVar10 = plVar10 + 1;
          } while ((int)uVar18 < *param_1);
        }
        uVar17 = (ulonglong)(iVar6 + 1);
        uVar19 = uVar19 + 1;
        lVar15 = lVar15 + 8;
      } while (iVar6 < *param_1);
    }
    *param_1 = *param_1 - (*((int*)(_frame+2040)));
LAB_180034974:
    uVar8 = FUN_180032d90(param_1,param_3);
    return uVar8;
  }
  goto LAB_1800345f0;
}

/* ================ FUN_1800349c0 ================ */
int * FUN_1800349c0(int *param_1,int *param_2,longlong param_3)

{
  int *piVar1;
  int *piVar2;
  undefined8 uVar3;

  piVar1 = param_1;
  if (param_1 == (int *)0x0) {
    piVar1 = calloc(1,0x150);
    if (piVar1 == (int *)0x0) {
      return (int *)0x0;
    }
    *(int **)(piVar1 + 2) = piVar1 + 4;
  }
  if ((*piVar1 < 0x28) && (piVar2 = FUN_1800353b0(param_2), piVar2 != (int *)0x0)) {
    *(int **)(*(longlong *)(piVar1 + 2) + (longlong)*piVar1 * 8) = piVar2;
    *piVar1 = *piVar1 + 1;
  }
  uVar3 = FUN_180032d90(param_1,param_3);
  if ((int)uVar3 < 0) {
    piVar1 = (int *)0x0;
  }
  return piVar1;
}

/* ================ FUN_180034a50 ================ */
int FUN_180034a50(int *param_1)

{
  undefined8 *puVar1;
  uint *puVar2;
  int iVar3;
  int iVar4;
  longlong *_Memory;
  longlong lVar5;
  longlong lVar6;
  int iVar7;
  int iVar8;
  longlong lVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  ulonglong uVar13;
  int iVar14;
  int local_res10;
  int local_res18;
  longlong local_50;

  iVar7 = 0;
  local_res10 = 0;
  local_res18 = 0;
  if (0 < *param_1) {
    local_50 = 0;
    do {
      iVar11 = 0x400;
      iVar14 = -0x400;
      puVar2 = *(uint **)(local_50 + *(longlong *)(param_1 + 2));
      iVar7 = 0;
      iVar10 = 0x400;
      iVar8 = -0x400;
      if (0 < (int)*puVar2) {
        lVar9 = 0;
        uVar13 = (ulonglong)*puVar2;
        do {
          puVar1 = (undefined8 *)(lVar9 + *(longlong *)(puVar2 + 6));
          lVar9 = lVar9 + 8;
          iVar12 = *(int *)*puVar1;
          iVar3 = ((int *)*puVar1)[1];
          if (iVar12 < iVar11) {
            iVar11 = iVar12;
          }
          if (iVar3 < iVar10) {
            iVar10 = iVar3;
          }
          if (iVar14 < iVar12) {
            iVar14 = iVar12;
          }
          if (iVar8 < iVar3) {
            iVar8 = iVar3;
          }
          uVar13 = uVar13 - 1;
        } while (uVar13 != 0);
      }
      iVar14 = ((int)((iVar14 - iVar11 >> 0x1f & 3U) + (iVar14 - iVar11)) >> 2) + 1;
      iVar8 = ((int)((iVar8 - iVar10 >> 0x1f & 3U) + (iVar8 - iVar10)) >> 2) + 1;
      _Memory = malloc((longlong)(iVar8 * iVar14 + iVar8 * 8));
      lVar9 = (longlong)iVar8;
      if (_Memory == (longlong *)0x0) {
        _Memory = (longlong *)0x0;
      }
      else {
        lVar6 = (longlong)_Memory + (longlong)(iVar8 * 8);
        lVar5 = 0;
        if (0 < lVar9) {
          do {
            _Memory[lVar5] = lVar6;
            lVar5 = lVar5 + 1;
            lVar6 = lVar6 + iVar14;
          } while (lVar5 < lVar9);
        }
        FUN_18004d980((undefined1 (*) [16])*_Memory,0,(longlong)(iVar8 * iVar14));
      }
      iVar12 = 0;
      if (0 < (int)*puVar2) {
        lVar5 = 0;
        do {
          iVar12 = iVar12 + 1;
          iVar3 = (*(int **)(lVar5 + *(longlong *)(puVar2 + 6)))[1] - iVar10;
          iVar4 = **(int **)(lVar5 + *(longlong *)(puVar2 + 6)) - iVar11;
          *(undefined1 *)
           ((longlong)((int)(iVar4 + (iVar4 >> 0x1f & 3U)) >> 2) +
           _Memory[(int)(iVar3 + (iVar3 >> 0x1f & 3U)) >> 2]) = 0xff;
          lVar5 = lVar5 + 8;
        } while (iVar12 < (int)*puVar2);
      }
      FUN_1800394f0(_Memory,iVar14,iVar8);
      lVar5 = 0;
      if (0 < lVar9) {
        do {
          lVar6 = 0;
          if (0 < iVar14) {
            do {
              if (*(char *)(_Memory[lVar5] + lVar6) == -1) {
                iVar7 = iVar7 + 1;
              }
              lVar6 = lVar6 + 1;
            } while (lVar6 < iVar14);
          }
          lVar5 = lVar5 + 1;
        } while (lVar5 < lVar9);
      }
      free(_Memory);
      local_res18 = local_res18 + 1;
      iVar7 = local_res10 + iVar7;
      local_50 = local_50 + 8;
      local_res10 = iVar7;
    } while (local_res18 < *param_1);
  }
  return iVar7;
}

/* ================ FUN_18002d350 ================ */
undefined8 FUN_18002d350(longlong *param_1,undefined8 param_2,ulonglong param_3,byte *param_4)

{
  uint uVar1;
  undefined8 uVar2;
  char *pcVar3;
  ulonglong uVar4;
  undefined8 *puVar5;
  int iVar6;
  undefined8 uVar7;
  ulonglong uVar8;
  ulonglong uVar9;

  pcVar3 = "991801==>";
  uVar2 = 3;
  FUN_18002b720(3,"991801==>",param_3,param_4);
  if (param_1 == (longlong *)0x0) {
    return 0xfffffc11;
  }
  if (DAT_18009707c == 7000) {
    if (5 < (int)param_1[2]) {
      return 0xfffffc00;
    }
    FUN_18002cf00(uVar2,pcVar3,param_3,param_4);
    if (DAT_1800a0460 != (int)param_1[2]) {
      DAT_1800970d0 = 4;
      _DAT_1800970c0 = _DAT_180089890;
      uRam00000001800970c8 = _UNK_180089898;
      DAT_1800a0460 = (int)param_1[2];
    }
    uVar4 = 0;
    if (0 < (int)param_1[2]) {
      puVar5 = &DAT_1800a0470;
      uVar8 = uVar4;
      uVar9 = uVar4;
      do {
        iVar6 = (int)uVar4;
        if ((*(undefined8 **)(uVar9 + *param_1) == (undefined8 *)0x0) ||
           (param_3 = (ulonglong)*(uint *)(uVar8 + param_1[1]),
           0x3c000 < *(uint *)(uVar8 + param_1[1]))) {
          param_4 = (byte *)(ulonglong)*(uint *)(param_1[1] + uVar8);
          FUN_18002b720(1,"991802==>%d,%d",uVar4,param_4);
          param_3 = uVar4;
        }
        else {
          param_4 = &DAT_1800a0478 + (longlong)iVar6 * 0x18;
          uVar1 = FUN_18002e020((undefined8 *)(&DAT_1800a0460 + ((longlong)iVar6 * 3 + 2) * 2),
                                *(undefined8 **)(uVar9 + *param_1),param_3,param_4);
          if (uVar1 == 0xfffffc0f) {
            pcVar3 = "991803==>Fail %d";
            uVar7 = 0xfffffc0f;
            uVar2 = 1;
            FUN_18002b720(1,"991803==>Fail %d",0xfffffc0f,param_4);
            FUN_18002cf00(uVar2,pcVar3,uVar7,param_4);
            DAT_18009707c = 7000;
            FUN_18002b720(3,"9918FE==>%d",0xfffffc0f,param_4);
            return 0xfffffc0f;
          }
          if ((uVar1 != 0) && ((int *)*puVar5 != (int *)0x0)) {
            FUN_180035630((int *)*puVar5);
            *puVar5 = 0;
          }
        }
        uVar4 = (ulonglong)(iVar6 + 1U);
        uVar9 = uVar9 + 8;
        puVar5 = puVar5 + 3;
        uVar8 = uVar8 + 4;
      } while ((int)(iVar6 + 1U) < (int)param_1[2]);
    }
    DAT_1800a0464 = (undefined4)param_1[2];
    DAT_18009707c = 0x1b5b;
    FUN_18002b720(3,"9918FF==>",param_3,param_4);
    return 0;
  }
  return 0xfffffc02;
}

/* ================ FUN_18002d530 ================ */
uint FUN_18002d530(int *param_1,uint *param_2,undefined4 *param_3,undefined8 param_4)

{
  uint uVar1;
  uint uVar2;
  uint *puVar3;
  ulonglong uVar4;
  ulonglong uVar5;
  uint uVar6;
  uint *puVar7;
  ulonglong uVar8;
  short local_res8 [4];

  FUN_18002b720(3,"991901==>",param_3,param_4);
  uVar1 = *param_2;
  puVar7 = (uint *)0x0;
  *param_3 = 0;
  puVar3 = calloc(1,((longlong)(int)uVar1 + 2) * 0x10);
  if (puVar3 != (uint *)0x0) {
    *puVar3 = uVar1;
    *(uint **)(puVar3 + 4) = puVar3 + 8;
    *(uint **)(puVar3 + 6) = puVar3 + 8 + (longlong)(int)uVar1 * 2;
    puVar7 = puVar3;
  }
  FUN_18002b720(3,"991902==>%d %d %d",(ulonglong)*param_2,(longlong)*param_1);
  uVar8 = DAT_18009d018;
  uVar1 = FUN_180032770((int *)param_2,param_1,(longlong)puVar7,DAT_18009d018,0);
  FUN_18002b720(3,"991903==>%d",(ulonglong)uVar1,uVar8);
  uVar2 = (uint)DAT_1800a04f0;
  if ((DAT_1800a04f6 == 1) && ((int)uVar2 < (int)uVar1)) {
    _DAT_1800a04f4 = 1;
  }
  if ((int)uVar2 < (int)uVar1) {
    uVar8 = (ulonglong)*(uint *)(DAT_18009d018 + 0x84);
    DAT_1800a04f8 = DAT_1800a04f8 - 0x50;
    uVar6 = (uint)DAT_1800a04f8;
    _DAT_1800a04f4 = CONCAT22(1,DAT_1800a04f4);
    if ((int)(uint)DAT_1800a04f8 < (int)*(uint *)(DAT_18009d018 + 0x84)) {
      FUN_18002b720(3,"991904==>%d < %d",(ulonglong)DAT_1800a04f8,uVar8);
      DAT_1800a04f8 = *(ushort *)(DAT_18009d018 + 0x84);
      uVar6 = (uint)DAT_1800a04f8;
    }
  }
  else {
    uVar6 = (uint)DAT_1800a04f8;
    _DAT_1800a04f4 = 0;
    uVar2 = (DAT_1800a04f0 >> 1) + uVar2;
    if (uVar6 <= uVar2) {
      uVar6 = (uint)(ushort)(DAT_1800a04f8 + 0x3c);
    }
    if (uVar2 < uVar6) {
      uVar6 = uVar2 & 0xffff;
    }
    DAT_1800a04f8 = (ushort)uVar6;
    if (puVar7 != (uint *)0x0) {
      uVar8 = (ulonglong)puVar7[2];
      FUN_18002b720(3,"991905==>%d %d",(ulonglong)uVar2,uVar8);
      uVar6 = (uint)DAT_1800a04f8;
    }
  }
  uVar5 = DAT_18009d018;
  if (((int)uVar6 < (int)uVar1) && ((int)puVar7[2] < *(int *)(DAT_18009d018 + 0x68))) {
    if ((0x27 < *param_2 - 1) || (*(longlong *)(param_2 + 2) == 0)) goto LAB_18002d777;
    uVar4 = FUN_180035bd0((int *)param_2,local_res8,*(int *)(DAT_18009d018 + 0x58));
    if (((int)uVar4 < 0) || (*(int *)(uVar5 + 0x88) <= (int)uVar4 + 0x20)) goto LAB_18002d777;
    uVar8 = (ulonglong)(_DAT_1800a04f4 & 0xffff);
    FUN_18002b720(3,"991906==>%d %d %d %d",3,uVar8);
    uVar5 = FUN_180034480((int *)param_2,(int *)puVar7,DAT_18009d018);
    if ((int)uVar5 == 0) {
      *param_3 = 1;
      goto LAB_18002d777;
    }
    FUN_18002b720(1,"991907==>%d ,Fail",uVar5 & 0xffffffff,uVar8);
  }
  *param_3 = 0;
LAB_18002d777:
  FUN_180035560((int *)puVar7);
  FUN_18002b720(3,"9919FF==>%d",(ulonglong)uVar1,uVar8);
  return uVar1;
}

/* ================ FUN_180034130 ================ */
int * FUN_180034130(uint *param_1,longlong param_2)

{
  void *_Memory;
  longlong lVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  int *piVar5;
  undefined8 uVar6;
  int *piVar7;
  int *piVar8;
  bool bVar9;

  lVar1 = DAT_18009d018;
  piVar8 = (int *)0x0;
  bVar9 = *(int *)(DAT_18009d018 + 0x90) != 0;
  piVar7 = piVar8;
  if ((!bVar9) && (piVar7 = (int *)0x0, *(int *)(DAT_18009d018 + 0x7c) == 1)) {
    if (*(uint **)(param_2 + 0x60) != (uint *)0x0) {
      iVar2 = FUN_180033d80(*(uint **)(param_2 + 0x60),param_1,DAT_18009d018);
      if (0 < iVar2) {
        piVar7 = (int *)0x3;
        goto LAB_1800341b9;
      }
      _Memory = *(void **)(param_2 + 0x60);
      if (_Memory != (void *)0x0) {
        free(*(void **)((longlong)_Memory + 0x10));
        free(_Memory);
      }
    }
    piVar4 = FUN_1800353b0((int *)param_1);
    *(int **)(param_2 + 0x60) = piVar4;
  }
LAB_1800341b9:
  iVar2 = *(int *)(param_2 + 4);
  if (*(int *)(lVar1 + 0x6c) < iVar2) {
    piVar4 = *(int **)(param_2 + 0x78);
    if (((piVar4 == (int *)0x0) && (piVar4 = (int *)FUN_180033210(param_2,lVar1), !bVar9)) &&
       (*(int *)(lVar1 + 0x7c) == 2)) {
      iVar2 = FUN_180034a50(piVar4);
      *(int *)(param_2 + 0x14) = iVar2;
    }
    iVar2 = *piVar4;
    piVar5 = calloc(1,((longlong)iVar2 + 2) * 0x10);
    if (piVar5 != (int *)0x0) {
      *piVar5 = iVar2;
      *(int **)(piVar5 + 4) = piVar5 + 8;
      *(int **)(piVar5 + 6) = piVar5 + 8 + (longlong)iVar2 * 2;
      piVar8 = piVar5;
    }
    iVar2 = FUN_180032770(piVar4,(int *)param_1,(longlong)piVar8,lVar1,(uint)bVar9);
    if (iVar2 < *(int *)(lVar1 + 0x78)) {
      piVar4 = FUN_1800349c0(piVar4,(int *)param_1,lVar1);
      if ((!bVar9) && (*(int *)(lVar1 + 0x7c) == 2)) {
        iVar2 = FUN_180034a50(piVar4);
        *(int *)(param_2 + 0x14) = iVar2;
      }
    }
    else {
      uVar6 = FUN_180034480(piVar4,piVar8,lVar1);
      if ((int)uVar6 == -0x3eb) {
        piVar4 = FUN_1800349c0(piVar4,(int *)param_1,lVar1);
      }
      if ((!bVar9) && (*(int *)(lVar1 + 0x7c) == 2)) {
        iVar2 = *(int *)(param_2 + 0x14);
        iVar3 = FUN_180034a50(piVar4);
        iVar3 = iVar3 - iVar2;
        *(int *)(param_2 + 0x14) = iVar2 + iVar3;
        if ((-1 < iVar3) && (iVar3 < *(int *)(lVar1 + 0x80))) {
          FUN_180035560(piVar8);
          return (int *)0x3;
        }
      }
    }
    FUN_180035560(piVar8);
  }
  else {
    piVar4 = FUN_1800353b0((int *)param_1);
    *(int **)(*(longlong *)(param_2 + 0x48) + (longlong)*(int *)(param_2 + 4) * 8) = piVar4;
    if (*(longlong *)(*(longlong *)(param_2 + 0x48) + (longlong)*(int *)(param_2 + 4) * 8) == 0) {
      piVar7 = (int *)0xfffffc18;
    }
    else {
      iVar3 = *(int *)(param_2 + 4) + 1;
      *(int *)(param_2 + 4) = iVar3;
      if (!bVar9) {
        piVar4 = piVar8;
        piVar5 = piVar8;
        if (0 < iVar3) {
          do {
            iVar3 = (int)piVar4;
            if (iVar3 != iVar2) {
              FUN_180033b50(param_2,iVar3,iVar2,(int *)param_1,lVar1);
              FUN_180033b50(param_2,iVar2,iVar3,
                            *(int **)((longlong)piVar5 + *(longlong *)(param_2 + 0x48)),lVar1);
            }
            piVar4 = (int *)(ulonglong)(iVar3 + 1U);
            piVar5 = piVar5 + 2;
          } while ((int)(iVar3 + 1U) < *(int *)(param_2 + 4));
        }
        if ((*(int *)(lVar1 + 0x7c) == 2) && (0 < iVar2)) {
          do {
            iVar3 = FUN_180033d80(*(uint **)(*(longlong *)(param_2 + 0x48) + (longlong)piVar8 * 8),
                                  param_1,lVar1);
            if (0 < iVar3) {
              piVar7 = (int *)0x3;
            }
            piVar8 = (int *)((longlong)piVar8 + 1);
          } while ((longlong)piVar8 < (longlong)iVar2);
        }
      }
    }
  }
  return piVar7;
}

/* ================ FUN_18002c880 ================ */
undefined4
FUN_18002c880(undefined8 *param_1,int param_2,ulonglong param_3,uint **param_4,uint *param_5)

{
  uint uVar1;
  longlong *_Memory;
  undefined8 uVar2;
  int *piVar3;
  char *pcVar4;
  longlong lVar5;
  int iVar6;
  longlong lVar7;
  ulonglong uVar8;
  int *piVar9;
  uint **ppuVar10;
  uint *_Memory_00;
  uint *local_res8;

  local_res8 = (uint *)0x0;
  ppuVar10 = param_4;
  FUN_18002b720(3,"991201==>",param_3,param_4);
  if (((param_1 == (undefined8 *)0x0) || (param_4 == (uint **)0x0)) || (param_5 == (uint *)0x0)) {
    return 0xfffffc11;
  }
  if (DAT_18009707c != 0x1b59) {
    return 0xfffffc02;
  }
  if ((*(char *)param_4 != 'R') || (*(char *)((longlong)param_4 + 1) != 'D')) {
    return 0xfffffc0e;
  }
  if (*(int *)((longlong)param_4 + 4) == 0) {
    return 0xfffffffd;
  }
  uVar1 = (uint)param_3;
  _Memory = FUN_18002e460(param_2,uVar1);
  if (_Memory == (longlong *)0x0) {
    return 0xfffffc18;
  }
  uVar8 = (ulonglong)(int)(param_2 * uVar1);
  FUN_18004d200((undefined8 *)*_Memory,param_1,uVar8);
  if (DAT_18009d018 == 0) {
    iVar6 = -0x3ea;
    _Memory_00 = (uint *)0x0;
  }
  else {
    ppuVar10 = &local_res8;
    uVar8 = param_3 & 0xffffffff;
    uVar2 = FUN_18003ce10(_Memory,param_2,uVar1,ppuVar10,DAT_18009d018);
    iVar6 = (int)uVar2;
    _Memory_00 = local_res8;
  }
  free(_Memory);
  if (iVar6 != 0) {
    FUN_18002b720(1,"991202==>Fail",uVar8,ppuVar10);
    return 0xfffffc05;
  }
  piVar9 = (int *)(ulonglong)*_Memory_00;
  FUN_18002b720(3,"991203==>%d",piVar9,ppuVar10);
  uVar1 = DAT_1800a0464;
  if ((int)*_Memory_00 < 0xb) {
    FUN_18002b720(3,"991204==>bad image",piVar9,ppuVar10);
    *param_5 = DAT_18009d014;
    free(*(void **)(_Memory_00 + 4));
    free(_Memory_00);
    return 0xffffffff;
  }
  if (*(int *)(param_4 + 2) != 0) {
    lVar5 = (longlong)(int)DAT_1800a0464;
    piVar9 = (int *)(ulonglong)DAT_1800a0464;
    FUN_18002b720(3,"991205==>%d",piVar9,ppuVar10);
    lVar7 = 0;
    if (0 < (int)uVar1) {
      do {
        if ((uint *)(&DAT_1800a0470)[(longlong)(int)(&DAT_1800970c0)[lVar7] * 3] == (uint *)0x0) {
          FUN_18002b720(1,"991206==>Fail",piVar9,ppuVar10);
        }
        else {
          uVar1 = FUN_18002d530((int *)_Memory_00,
                                (uint *)(&DAT_1800a0470)[(longlong)(int)(&DAT_1800970c0)[lVar7] * 3]
                                ,(undefined4 *)&local_res8,ppuVar10);
          piVar9 = (int *)(ulonglong)uVar1;
          FUN_18002b720(3,"99127==>%d",piVar9,ppuVar10);
          if (((int*)DAT_18009d018)[0x15] <= (int)uVar1) {
            FUN_18002b720(3,"991208==>%d %d",(ulonglong)uVar1,(ulonglong)(uint)((int*)DAT_18009d018)[0x15]);
            free(*(void **)(_Memory_00 + 4));
            free(_Memory_00);
            return 0xfffffc14;
          }
        }
        lVar7 = lVar7 + 1;
      } while (lVar7 < lVar5);
    }
  }
  piVar3 = FUN_180034130(_Memory_00,(longlong)param_4[3]);
  free(*(void **)(_Memory_00 + 4));
  free(_Memory_00);
  iVar6 = (int)piVar3;
  if (*(int *)((longlong)param_4 + 0xc) == 0) {
    if (iVar6 == 3) {
      FUN_18002b720(3,"991209==>redundant %d",(ulonglong)*(uint *)(param_4 + 4),ppuVar10);
      piVar9 = (int *)DAT_18009d018;
      *param_5 = ((((int*)DAT_18009d018)[0x1c] - *(int *)((longlong)param_4 + 4)) * 100) /
                 ((int*)DAT_18009d018)[0x1c];
      if (*(int *)(param_4 + 4) != 1) {
        return 0xfffffff8;
      }
      *(int *)((longlong)param_4 + 4) = *(int *)((longlong)param_4 + 4) + -1;
      if (0 < *(int *)((longlong)param_4 + 4)) {
        return 4;
      }
    }
    else {
      FUN_18002b720(3,"991210==>accept",piVar9,ppuVar10);
      *(int *)((longlong)param_4 + 4) = *(int *)((longlong)param_4 + 4) + -1;
      piVar9 = (int *)DAT_18009d018;
    }
    if (*(int *)((longlong)param_4 + 4) != 0) {
      uVar8 = (longlong)((piVar9[0x1c] - *(int *)((longlong)param_4 + 4)) * 100) /
              (longlong)piVar9[0x1c];
      pcVar4 = "991212==>%d";
      *param_5 = (uint)uVar8;
      piVar9 = (int *)(uVar8 & 0xffffffff);
LAB_18002cc68:
      FUN_18002b720(3,pcVar4,piVar9,ppuVar10);
      return 1;
    }
    pcVar4 = "991211==>done";
  }
  else {
    if (*(int *)((longlong)param_4 + 0xc) != 2) {
      FUN_18002b720(3,"9912FF==>",piVar9,ppuVar10);
      return 0xfffffffd;
    }
    if (*(longlong *)(param_4[3] + 0x1e) == 0) {
      if (iVar6 == 3) {
        piVar9 = (int *)(ulonglong)*(uint *)(param_4 + 4);
        FUN_18002b720(3,"991218==>redundant %d",piVar9,ppuVar10);
        *param_5 = 0;
        if (*(int *)(param_4 + 4) != 1) {
          return 0xfffffff8;
        }
        *(int *)((longlong)param_4 + 4) = *(int *)((longlong)param_4 + 4) + -1;
        if (0 < *(int *)((longlong)param_4 + 4)) {
          return 4;
        }
      }
      else {
        FUN_18002b720(3,"991219==>accept",piVar9,ppuVar10);
        *(int *)((longlong)param_4 + 4) = *(int *)((longlong)param_4 + 4) + -1;
      }
      if (*(int *)((longlong)param_4 + 4) != 0) {
        uVar8 = (longlong)((((int*)DAT_18009d018)[0x1c] - *(int *)((longlong)param_4 + 4)) * 100) /
                (longlong)((int*)DAT_18009d018)[0x1c];
        pcVar4 = "991221==>%d";
        DAT_18009d014 = (uint)uVar8;
        *param_5 = DAT_18009d014;
        piVar9 = (int *)(uVar8 & 0xffffffff);
        goto LAB_18002cc68;
      }
      pcVar4 = "991220==>done";
    }
    else {
      if (iVar6 == 3) {
        piVar9 = (int *)(ulonglong)*(uint *)(param_4 + 4);
        pcVar4 = "991213==>redundant %d";
        FUN_18002b720(3,"991213==>redundant %d",piVar9,ppuVar10);
        *param_5 = DAT_18009d014;
        if (*(int *)(param_4 + 4) != 1) {
          return 0xfffffff8;
        }
        *(int *)((longlong)param_4 + 4) = *(int *)((longlong)param_4 + 4) + -1;
        iVar6 = *(int *)((longlong)param_4 + 4);
        if (0 < iVar6) {
          return 4;
        }
      }
      else {
        pcVar4 = "991214==>accept";
        FUN_18002b720(3,"991214==>accept",piVar9,ppuVar10);
        *(int *)((longlong)param_4 + 4) = *(int *)((longlong)param_4 + 4) + -1;
        iVar6 = *(int *)((longlong)param_4 + 4);
      }
      if (iVar6 == 0) {
        pcVar4 = "991215==>done";
      }
      else {
        uVar1 = FUN_18002c6f0((longlong)param_4,pcVar4,piVar9,ppuVar10);
        if ((int)DAT_18009d014 < (int)uVar1) {
          if ((int)(uVar1 - DAT_18009d014) < 0x1a) {
            *param_5 = uVar1;
            DAT_18009d014 = uVar1;
          }
          else {
            *param_5 = DAT_18009d014 + 0x19;
            DAT_18009d014 = DAT_18009d014 + 0x19;
          }
        }
        else {
          *param_5 = DAT_18009d014 + 3;
          DAT_18009d014 = DAT_18009d014 + 3;
        }
        piVar9 = (int *)(ulonglong)*param_5;
        if (((int)*param_5 < 100) && (*(int *)((longlong)param_4 + 4) != 0)) {
          pcVar4 = "991217==>%d";
          goto LAB_18002cc68;
        }
        pcVar4 = "991216==>done";
      }
    }
  }
  FUN_18002b720(3,pcVar4,piVar9,ppuVar10);
  *param_5 = 100;
  DAT_18009707c = 0x1b5a;
  return 2;
}

/* ================ FUN_180033750 ================ */
int * FUN_180033750(int *param_1,int param_2,longlong *param_3,longlong param_4,int param_5,
                   int param_6,short param_7,int param_8,longlong param_9)

{
  int *piVar1;
  longlong lVar2;
  short sVar3;
  int *_Memory;
  int iVar4;
  int *piVar5;
  longlong lVar6;
  longlong lVar7;
  int iVar8;
  int iVar9;
  longlong *plVar10;
  int *local_res8;
  int local_40;
  longlong local_38;

  local_40 = 0;
  if (0 < *(int *)(param_4 + 4)) {
    local_38 = 0;
    local_res8 = param_1;
    do {
      if ((*(char *)(local_38 + *(longlong *)(param_4 + 0x38)) == '\0') &&
         (lVar6 = (longlong)param_2 * 8, param_1 = local_res8,
         *(char *)(local_38 + param_3[param_2]) != '\0')) {
        lVar2 = local_38 * 8;
        _Memory = *(int **)(lVar2 + *(longlong *)(param_4 + 0x48));
        piVar5 = *(int **)(lVar2 + *(longlong *)(*(longlong *)(param_4 + 0x88) + lVar6));
        FUN_1800329e0(_Memory,(short)piVar5[5]);
        iVar9 = piVar5[6];
        sVar3 = *(short *)((longlong)piVar5 + 0x16);
        iVar8 = 0;
        if (0 < *_Memory) {
          lVar7 = 0;
          do {
            iVar8 = iVar8 + 1;
            **(int **)(lVar7 + *(longlong *)(_Memory + 6)) =
                 **(int **)(lVar7 + *(longlong *)(_Memory + 6)) - (int)sVar3;
            piVar1 = (int *)(*(longlong *)(lVar7 + *(longlong *)(_Memory + 6)) + 4);
            *piVar1 = *piVar1 - (int)(short)iVar9;
            lVar7 = lVar7 + 8;
          } while (iVar8 < *_Memory);
        }
        lVar7 = 0;
        iVar9 = 0;
        if (0 < *_Memory) {
          do {
            iVar9 = iVar9 + 1;
            *(undefined1 *)(*(longlong *)(*(longlong *)(_Memory + 6) + -8 + (lVar7 + 1) * 8) + 0xd)
                 = *(undefined1 *)
                    (lVar7 + *(longlong *)
                              (*(longlong *)(lVar6 + *(longlong *)(param_4 + 0x50)) + lVar2));
            lVar7 = lVar7 + 1;
          } while (iVar9 < *_Memory);
        }
        iVar9 = 0;
        if (0 < *piVar5) {
          lVar6 = 0;
          do {
            plVar10 = (longlong *)(*(longlong *)(piVar5 + 10) + lVar6);
            iVar9 = iVar9 + 1;
            lVar6 = lVar6 + 8;
            *(longlong *)(*(longlong *)(piVar5 + 10) + -8 + lVar6) =
                 (*(longlong **)(_Memory + 6))
                 [(int)((*plVar10 - **(longlong **)(_Memory + 6)) / 0x50)];
          } while (iVar9 < *piVar5);
        }
        FUN_1800329e0(_Memory,param_7);
        iVar9 = 0;
        if (0 < *_Memory) {
          lVar6 = 0;
          do {
            iVar9 = iVar9 + 1;
            **(int **)(lVar6 + *(longlong *)(_Memory + 6)) =
                 **(int **)(lVar6 + *(longlong *)(_Memory + 6)) - param_5;
            piVar1 = (int *)(*(longlong *)(lVar6 + *(longlong *)(_Memory + 6)) + 4);
            *piVar1 = *piVar1 - param_6;
            lVar6 = lVar6 + 8;
          } while (iVar9 < *_Memory);
        }
        iVar9 = *local_res8;
        FUN_180033480(local_res8,_Memory,piVar5,0,param_9);
        iVar8 = *_Memory;
        piVar5 = local_res8;
        if (iVar8 != 0) {
          iVar4 = iVar8 + iVar9;
          if (0x1000 < iVar4) {
            iVar4 = 0x1000;
            iVar8 = 0x1000 - iVar9;
            *_Memory = iVar8;
          }
          piVar5 = FUN_1800352f0(local_res8,iVar4);
          if (piVar5 == (int *)0x0) goto LAB_180033a82;
          FUN_18004d200(*(undefined8 **)(*(longlong *)(piVar5 + 6) + (longlong)iVar9 * 8),
                        (undefined8 *)**(undefined8 **)(_Memory + 6),(longlong)iVar8 * 0x50);
          *piVar5 = iVar4;
        }
        if (piVar5 != (int *)0x0) {
          free(*(void **)(_Memory + 4));
          free(_Memory);
          *(undefined8 *)(lVar2 + *(longlong *)(param_4 + 0x48)) = 0;
          *(char *)(local_38 + *(longlong *)(param_4 + 0x38)) = (char)param_8 + '\x01';
          iVar9 = 0;
          lVar6 = 0;
          param_1 = piVar5;
          local_res8 = piVar5;
          if (0 < *(int *)(param_4 + 4)) {
            do {
              iVar9 = iVar9 + 1;
              *(undefined1 *)(local_38 + param_3[lVar6]) = 0;
              *(undefined1 *)(lVar6 + param_3[local_38]) = 0;
              lVar6 = lVar6 + 1;
            } while (iVar9 < *(int *)(param_4 + 4));
          }
        }
      }
LAB_180033a82:
      local_38 = local_38 + 1;
      local_40 = local_40 + 1;
    } while (local_40 < *(int *)(param_4 + 4));
  }
  while (plVar10 = param_3, iVar9 = FUN_1800333d0(param_3,param_4,param_8 + 1U), -1 < iVar9) {
    lVar6 = *(longlong *)
             (*(longlong *)(*(longlong *)(param_4 + 0x88) + (longlong)param_2 * 8) +
             (longlong)iVar9 * 8);
    param_1 = FUN_180033750(param_1,iVar9,plVar10,param_4,(int)*(short *)(lVar6 + 0x16),
                            (int)*(short *)(lVar6 + 0x18),*(short *)(lVar6 + 0x14),param_8 + 1U,
                            param_9);
  }
  return param_1;
}

/* ================ FUN_180033ef0 ================ */
undefined8 FUN_180033ef0(longlong param_1,longlong param_2)

{
  int iVar1;
  undefined8 *puVar2;
  int iVar3;
  longlong *plVar4;
  int *piVar5;
  longlong lVar6;
  int *piVar7;
  int *piVar8;
  longlong *_Memory;
  longlong lVar9;
  ulonglong uVar10;
  uint uVar11;
  ulonglong uVar12;

  iVar3 = *(int *)(param_1 + 0x10);
  puVar2 = *(undefined8 **)(param_1 + 0x40);
  plVar4 = FUN_18002e460(iVar3,iVar3);
  _Memory = (longlong *)0x0;
  if (plVar4 != (longlong *)0x0) {
    FUN_18004d200((undefined8 *)*plVar4,(undefined8 *)*puVar2,(longlong)(iVar3 * iVar3));
    _Memory = plVar4;
  }
  FUN_18004d980(*(undefined1 (**) [16])(param_1 + 0x38),0,(longlong)*(int *)(param_2 + 0x70));
  do {
    iVar3 = FUN_1800333d0(_Memory,param_1,0);
    if (iVar3 < 0) break;
    piVar7 = *(int **)(*(longlong *)(param_1 + 0x48) + (longlong)iVar3 * 8);
    lVar9 = (longlong)*piVar7;
    piVar5 = calloc(1,(longlong)(*piVar7 * 0x58 + 0x20));
    if (piVar5 == (int *)0x0) {
      piVar5 = (int *)0x0;
    }
    else {
      piVar8 = piVar5 + 8 + lVar9 * 2;
      lVar6 = 0;
      *(int **)(piVar5 + 6) = piVar5 + 8;
      if (0 < lVar9) {
        do {
          lVar6 = lVar6 + 1;
          *(int **)(*(longlong *)(piVar5 + 6) + -8 + lVar6 * 8) = piVar8;
          piVar8 = piVar8 + 0x14;
        } while (lVar6 < lVar9);
      }
      *piVar5 = *piVar7;
      *(short *)(piVar5 + 1) = (short)piVar7[1];
      *(undefined2 *)((longlong)piVar5 + 6) = *(undefined2 *)((longlong)piVar7 + 6);
      *(char *)(piVar5 + 2) = (char)piVar7[2];
      *(undefined1 *)((longlong)piVar5 + 9) = *(undefined1 *)((longlong)piVar7 + 9);
      *(undefined1 *)((longlong)piVar5 + 10) = *(undefined1 *)((longlong)piVar7 + 10);
      if ((*(undefined8 **)(piVar5 + 4) != (undefined8 *)0x0) &&
         (*(undefined8 **)(piVar7 + 4) != (undefined8 *)0x0)) {
        FUN_18004d200(*(undefined8 **)(piVar5 + 4),*(undefined8 **)(piVar7 + 4),
                      (longlong)
                      (int)((uint)*(ushort *)(piVar7 + 1) * (uint)*(ushort *)((longlong)piVar7 + 6))
                     );
      }
      FUN_18004d200((undefined8 *)**(undefined8 **)(piVar5 + 6),
                    (undefined8 *)**(undefined8 **)(piVar7 + 6),(longlong)*piVar7 * 0x50);
    }
    iVar1 = *piVar5;
    *(undefined1 *)((longlong)iVar3 + *(longlong *)(param_1 + 0x38)) = 1;
    piVar7 = FUN_180033750(piVar5,iVar3,_Memory,param_1,0,0,0,1,param_2);
    *(int **)(*(longlong *)(*(int **)(param_1 + 0x78) + 2) +
             (longlong)**(int **)(param_1 + 0x78) * 8) = piVar7;
    **(int **)(param_1 + 0x78) = **(int **)(param_1 + 0x78) + 1;
  } while (iVar1 < *piVar7);
  uVar10 = 0;
  free(_Memory);
  uVar12 = uVar10;
  if (0 < *(int *)(param_1 + 4)) {
    do {
      if (*(char *)(uVar10 + *(longlong *)(param_1 + 0x38)) == '\0') {
        *(undefined8 *)
         (*(longlong *)(*(int **)(param_1 + 0x78) + 2) + (longlong)**(int **)(param_1 + 0x78) * 8) =
             *(undefined8 *)(*(longlong *)(param_1 + 0x48) + uVar10 * 8);
        **(int **)(param_1 + 0x78) = **(int **)(param_1 + 0x78) + 1;
        *(undefined8 *)(*(longlong *)(param_1 + 0x48) + uVar10 * 8) = 0;
      }
      uVar11 = (int)uVar12 + 1;
      uVar10 = uVar10 + 1;
      uVar12 = (ulonglong)uVar11;
    } while ((int)uVar11 < *(int *)(param_1 + 4));
  }
  return 0;
}

/* ================ FUN_180033210 ================ */
longlong FUN_180033210(longlong param_1,longlong param_2)

{
  void *pvVar1;
  longlong lVar2;

  lVar2 = *(longlong *)(param_1 + 0x78);
  if (lVar2 == 0) {
    pvVar1 = calloc(1,0x150);
    if (pvVar1 != (void *)0x0) {
      *(longlong *)((longlong)pvVar1 + 8) = (longlong)pvVar1 + 0x10;
    }
    *(void **)(param_1 + 0x78) = pvVar1;
    FUN_180033ef0(param_1,param_2);
    FUN_180032ef0(param_1);
    lVar2 = *(longlong *)(param_1 + 0x78);
  }
  return lVar2;
}

/* ================ FUN_180033270 ================ */
int * FUN_180033270(void *param_1)

{
  int *piVar1;
  longlong lVar2;
  void *pvVar3;
  longlong lVar4;
  undefined8 uVar5;
  int *_Memory;
  int iVar6;
  int iVar7;
  longlong lVar8;
  longlong lVar9;

  lVar9 = DAT_18009d018;
  _Memory = *(int **)((longlong)param_1 + 0x78);
  if (_Memory == (int *)0x0) {
    pvVar3 = calloc(1,0x150);
    if (pvVar3 != (void *)0x0) {
      *(longlong *)((longlong)pvVar3 + 8) = (longlong)pvVar3 + 0x10;
    }
    *(void **)((longlong)param_1 + 0x78) = pvVar3;
    FUN_180033ef0((longlong)param_1,lVar9);
    FUN_180032ef0((longlong)param_1);
    _Memory = *(int **)((longlong)param_1 + 0x78);
  }
  FUN_1800331a0(param_1);
  if (_Memory != (int *)0x0) {
    iVar7 = 0;
    if (0 < *_Memory) {
      lVar8 = 0;
      do {
        iVar6 = 0;
        piVar1 = *(int **)(lVar8 + *(longlong *)(_Memory + 2));
        if (0 < *piVar1) {
          lVar4 = 0;
          do {
            lVar2 = *(longlong *)(*(longlong *)(piVar1 + 6) + lVar4);
            if (*(int *)(lVar9 + 0x8c) == 0) {
              *(char *)(lVar2 + 0xd) = *(char *)(lVar2 + 0xd) << 2;
            }
            else {
              *(undefined1 *)(lVar2 + 0xd) = 0xfb;
            }
            iVar6 = iVar6 + 1;
            lVar4 = lVar4 + 8;
          } while (iVar6 < *piVar1);
        }
        iVar7 = iVar7 + 1;
        lVar8 = lVar8 + 8;
      } while (iVar7 < *_Memory);
    }
    if ((*_Memory == 0) || (uVar5 = FUN_180032d90(_Memory,lVar9), (int)uVar5 < 0)) {
      iVar7 = 0;
      if (0 < *_Memory) {
        lVar9 = 0;
        do {
          pvVar3 = *(void **)(lVar9 + *(longlong *)(_Memory + 2));
          if (pvVar3 != (void *)0x0) {
            free(*(void **)((longlong)pvVar3 + 0x10));
            free(pvVar3);
          }
          iVar7 = iVar7 + 1;
          lVar9 = lVar9 + 8;
        } while (iVar7 < *_Memory);
      }
      free(_Memory);
      return (int *)0x0;
    }
  }
  return _Memory;
}

/* ================ FUN_18002dc40 ================ */
void * FUN_18002dc40(char *param_1,uint *param_2,ulonglong param_3,undefined *param_4)

{
  int *piVar1;
  ulonglong uVar2;
  uint *puVar3;

  FUN_18002b720(3,"992201==>",param_3,param_4);
  if (DAT_18009d028 != (void *)0x0) {
    return DAT_18009d028;
  }
  if ((((param_1 != (char *)0x0) && (param_2 != (uint *)0x0)) && (DAT_18009707c == 0x1b5a)) &&
     ((*param_1 == 'R' && (param_1[1] == 'D')))) {
    if (*(longlong *)(*(longlong *)(param_1 + 0x18) + 0x80) != 0) {
      FUN_18002b720(3,"992202==>",param_3,param_4);
      if (*(void **)(*(longlong *)(param_1 + 0x18) + 0x80) != (void *)0x0) {
        free(*(void **)(*(longlong *)(param_1 + 0x18) + 0x80));
      }
      *(undefined8 *)(*(longlong *)(param_1 + 0x18) + 0x80) = 0;
    }
    piVar1 = FUN_180033270(*(void **)(param_1 + 0x18));
    if (piVar1 == (int *)0x0) {
      FUN_18002b720(1,"992202==>Fail",param_3,param_4);
    }
    else {
      param_4 = &DAT_1800a04e0;
      puVar3 = param_2;
      uVar2 = FUN_18002e350(piVar1,(longlong *)&DAT_18009d028,param_2,&DAT_1800a04e0);
      if ((int)uVar2 == 0) {
        FUN_180035630(piVar1);
        param_1[8] = '\x01';
        param_1[9] = '\0';
        param_1[10] = '\0';
        param_1[0xb] = '\0';
        FUN_18002b720(3,"9922FF==>",puVar3,param_4);
        return DAT_18009d028;
      }
      param_3 = uVar2 & 0xffffffff;
      FUN_18002b720(1,"992203==>Fail %d",param_3,param_4);
    }
    if (DAT_18009d028 != (void *)0x0) {
      free(DAT_18009d028);
      DAT_18009d028 = (void *)0x0;
    }
    if (piVar1 != (int *)0x0) {
      FUN_180035630(piVar1);
    }
    *param_2 = 0;
    FUN_18002b720(3,"9922FE==>NULL",param_3,param_4);
    return (void *)0x0;
  }
  return (void *)0x0;
}

