//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT, sirlemonhead

This file is part of PCExhumed.

PCExhumed is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "compat.h"
#include "baselayer.h"
#include "renderlayer.h" // for win_gethwnd()
#include "build.h"
#include "cache1d.h"
#include "fx_man.h"
#include "keyboard.h"
#include "control.h"
#include "engine.h"
#include "exhumed.h"
#include "config.h"
#include "sound.h"
#include "init.h"
#include "object.h"
#include "player.h"
#include "random.h"
#include "snake.h"
#include "trigdat.h"
#include "sequence.h"
#include "cd.h"

#define kMaxFXVolume   255

#if 0
extern "C" {
#include "usrhooks.h"
#include "music.h"
#include "multivoc.h"
#include "fx_man.h"
#include "midi.h"
#include "mpu401.h"
}
#endif

short nSoundsPlaying = 0;
short nAmbientChannel = -1;

short nStopSound;
short nStoneSound;
short nSwitchSound;

short nLocalEyeSect;
short nElevSound;
short nCreepyTimer;

const char *SoundFiles[kMaxSoundFiles] =
{
  "spl_big",
  "spl_smal",
  "bubble_l",
  "grn_drop",
  "p_click",
  "grn_roll",
  "cosprite",
  "m_chant0",
  "anu_icu",
  "item_reg",
  "item_spe", // 10
  "item_key",
  "torch_on", // 12
  "jon_bnst",
  "jon_gasp",
  "jon_land",
  "jon_gags",
  "jon_fall",
  "jon_drwn",
  "jon_air1",
  "jon_glp1", // 20
  "jon_bbwl",
  "jon_pois",
  "amb_ston",
  "cat_icu",
  "bubble_h",
  "set_land",
  "jon_hlnd",
  "jon_laf2",
  "spi_jump",
  "jon_scub", // 30
  "item_use",
  "tr_arrow",
  "swi_foot",
  "swi_ston",
  "swi_wtr1",
  "tr_fire",
  "m_skull5",
  "spi_atak",
  "anu_hit",
  "fishdies", // 40
  "scrp_icu",
  "jon_wade",
  "amb_watr",
  "tele_1",
  "wasp_stg",
  "res",
  "drum4",
  "rex_icu",
  "m_hits_u",
  "q_tail", // 50
  "vatr_mov",
  "jon_hit3",
  "jon_t_2", // 53
  "jon_t_1",
  "jon_t_5",
  "jon_t_6",
  "jon_t_8",
  "jon_t_4",
  "rasprit1",
  "jon_fdie", // 60
  "wijaf1",
  "ship_1",
  "saw_on",
  "ra_on",
  "amb_ston", // 65
  "vatr_stp", // 66
  "mana1",
  "mana2",
  "ammo",
  "pot_pc1", // 70?
  "pot_pc2",
  "weapon",
  "alarm",
  "tick1",
  "scrp_zap", // 75
  "jon_t_3",
  "jon_laf1",
  "blasted",
  "jon_air2" // 79
};

short StaticSound[kMaxSoundFiles];

// TODO - temp location. Needs to exist within config file
//
// Sound variables
//
//int FXDevice;
//int MusicDevice;
//int FXVolume;
//int MusicVolume;
//int SoundToggle;
//int MusicToggle;
//int VoiceToggle;
//int AmbienceToggle;
//fx_blaster_config BlasterConfig;
//int NumVoices;
//int NumChannels;
//int NumBits;
//int MixRate;
//int32 MidiPort;
//int ReverseStereo;

int nNextFreq;
int nTotalSoundBytes;
int nSoundCount;
short nSwirlyFrames;

short nDistTable[256];

struct ActiveSound
{
    short nSprite;
    short nSound;
    short f_4; // volume
    short nAngle;
    short f_8; // volume
    short f_a;
    short f_c;
    int hFX; // ASS FX_() handle
    int f_12; // pitch
    int f_16; // pitch
    int nClock;
    int x;
    int y;
    int z;
    short nSector;
    short nChannel;
};

ActiveSound sActiveSound[kMaxActiveSounds];

#if 0
int AIL_allocate_sample_handle(int);
int AIL_allocate_sequence_handle(int);
void AIL_startup(void);
void AIL_set_preference(int, int);
int AIL_install_DIG_INI(void);
int AIL_install_MDI_INI(void);
void AIL_shutdown(void);
void AIL_end_sequence(int);
void AIL_set_sequence_volume(int, int, int);
void AIL_init_sequence(int, char*, int);
void AIL_start_sequence(int);
void AIL_set_sequence_loop_count(int, int);
void AIL_set_sequence_volume(int, int, int);
void AIL_set_sample_playback_rate(int, int);
void AIL_end_sample(int);
int AIL_sample_status(int);
void AIL_set_sample_volume(int, int);
void AIL_set_sample_pan(int, int);
void AIL_init_sample(int);
void AIL_set_sample_file(int, char*, int);
void AIL_start_sample(int);
void AIL_set_sample_loop_count(int, int);
int AIL_sample_playback_rate(int);
char AIL_error[256];
#endif
char pMusicBuf[45000];
char szSoundName[kMaxSounds][kMaxSoundNameLen];
char *SoundBuf[kMaxSounds];
int SoundLen[kMaxSounds];
char SoundLock[kMaxSounds];

extern char message_text[80];
extern short message_timer;

int S, dig, mdi, handle;
int nActiveSounds;

short nLocalSectFlags;
short nLocalChan;

uint8_t nPanTable[] = {
    0, 2, 4, 6, 8, 10, 12, 14,
    16, 18, 20, 22, 24, 26, 28, 30,
    32, 34, 36, 38, 40, 42, 44, 46,
    48, 50, 52, 54, 56, 58, 60, 62,
    64, 66, 68, 70, 72, 74, 76, 78,
    80, 82, 84, 86, 88, 90, 92, 94,
    96, 98, 100, 102, 104, 106, 108, 110,
    112, 114, 116, 118, 120, 122, 124, 128,
    128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128
};

void CalcASSPan(int nPan, int nVolume, int *pLeft, int *pRight)
{
    if (nPan < 0)
        nPan = 0;
    else if (nPan > 127)
        nPan = 127;

    if (nVolume < 0)
        nVolume = 0;
    else if (nVolume > 127)
        nVolume = 127;

    *pLeft = mulscale6(nPanTable[nPan], nVolume);
    *pRight = mulscale6(nPanTable[127-nPan], nVolume);
}

void ASSCallback(intptr_t num)
{
    if ((int32_t)num == MUSIC_ID) {
        return;
    }

    // TODO: add mutex?
    if ((int32_t)num == -1)
        handle = -1;
    else if ((int32_t)num >= 0 && (int32_t)num < kMaxActiveSounds)
        sActiveSound[num].hFX = -1;
}

void CreateDistTable(void)
{
    int eax = 260;

    for (int i = 0; i < 256; i++)
    {
        if (eax > 65280)
        {
            nDistTable[i] = 255;
        }
        else
        {
            nDistTable[i] = eax >> 8;

            eax = (eax * eax) >> 8;
        }
    }
}

void InitSoundInfo(void)
{
    int i;
    memset(sActiveSound, 255, sizeof(sActiveSound));

    for (i = 0; i < kMaxActiveSounds; i++)
    {
        sActiveSound[i].nChannel = 0;
        sActiveSound[i].hFX = -1;
    }
    nActiveSounds = i;

    handle = -1;
}

void InitFX(void)
{
#ifdef MIXERTYPEWIN
    void* initdata = (void*)win_gethwnd(); // used for DirectSound
#else
    void* initdata = NULL;
#endif

    dig = 0;

    if (bNoSound)
        return;

    if (FX_Init(NumVoices, NumChannels, MixRate, initdata) != FX_Ok)
    {
        DebugOut("Error initializing sound card!\n");
        initprintf("Error initializing sound card!\n");
        DebugOut("ERROR: %s\n", FX_ErrorString(FX_Error));
        return;
    }

    dig = 1;

    FX_SetCallBack(ASSCallback);

    CreateDistTable();
    InitSoundInfo();

    nTotalSoundBytes = 0;
    nSoundCount = 0;
    nCreepyTimer = kCreepyCount;

    SetMasterFXVolume(FXVolume);

#if 0
    int status = FX_Init(FXDevice, NumVoices, NumChannels, NumBits, MixRate);
    if (status == FX_Ok)
    {
        FX_SetVolume(FXVolume);

        if (ReverseStereo == 1) {
            FX_SetReverseStereo(!FX_GetReverseStereo());
        }
    }

    if (status != FX_Ok) {
//		Error("Sound startup error: %s", FX_ErrorString(FX_Error));
    }

    status = FX_SetCallBack(testcallback);

    if (status != FX_Ok) {
//		Error("Sound startup error: %s", FX_ErrorString(FX_Error));
    }
#endif
}

void UnInitFX()
{
    FX_Shutdown();
}

#if 0
void InitMusic()
{
    if (bNoSound)
    {
        mdi = 0;
        return;
    }

    mdi = AIL_install_MDI_INI();
    if (!mdi)
    {
        initprintf("AIL_install midi failed\n");
        return;
    }
    S = AIL_allocate_sequence_handle(mdi);
    if (!S)
    {
        initprintf("Midi handle failed");
        return;
    }
}

void UnInitMusic()
{
    if (mdi)
        AIL_end_sequence(S);
}

void FadeSong()
{
    AIL_set_sequence_volume(S, 0, 1000);
}

int LoadSong(const char *song)
{
    FILE *f;
    if (!mdi)
        return 0;

    AIL_end_sequence(S);
    f = fopen(song, "rb");
    if (!f)
    {
        sprintf(message_text, "Unable to midi open file '%s'!\n", song);
        message_timer = 1000;
        return 0;
    }

    if (fread(pMusicBuf, sizeof(pMusicBuf), 1, f) == sizeof(pMusicBuf))
    {
        sprintf(message_text, "Programmer error: not enough memory allocated for song file.\n");
        message_timer = 1000;
        fclose(f);
        return 0;
    }
    fclose(f);

    AIL_init_sequence(S, pMusicBuf, 0);
    AIL_start_sequence(S);
    AIL_set_sequence_loop_count(S, 0);
    AIL_set_sequence_volume(S, gMusicVolume>>1, 0);
}
#else
void InitMusic()
{
}

void UnInitMusic()
{
}

void FadeSong()
{
}

int LoadSong(const char *song)
{
    UNREFERENCED_PARAMETER(song);
    return 0;
}
#endif

int LoadSound(const char *sound)
{
    if (!dig)
        //return 0;
        return -1;

    int i;

    for (i = 0; i < nSoundCount; i++)
    {
        if (!Bstrncasecmp(szSoundName[i], sound, kMaxSoundNameLen))
            return i;
    }

    if (i >= kMaxSounds)
        bail2dos("Too many sounds being loaded... increase array size\n");

    strncpy(szSoundName[i], sound, kMaxSoundNameLen);

    char buffer[BMAX_PATH];
    buffer[0] = 0;

    strncat(buffer, szSoundName[i], kMaxSoundNameLen);

    char *pzTail = buffer + strlen(buffer);
    while (*pzTail == ' ' && pzTail != buffer)
        *pzTail-- = '\0';

    static const char *soundfileformats[] = { ".ogg", ".wav", ".voc" };
    char sndfilename[BMAX_PATH];
    buildvfs_kfd hSnd = buildvfs_kfd_invalid;

    for (auto fmt : soundfileformats)
    {
        strcpy(sndfilename, buffer);
        strcat(sndfilename, fmt);

        hSnd = kopen4loadfrommod(sndfilename, 0);
        if (hSnd != buildvfs_kfd_invalid)
            break;
    }

    if (hSnd != buildvfs_kfd_invalid)
    {
        int nSize = kfilelength(hSnd);
        SoundLock[i] = 255; // TODO: implement cache lock properly
        SoundLen[i] = nSize;
        g_cache.allocateBlock((intptr_t*)&SoundBuf[i], nSize, &SoundLock[i]);

        if (!SoundBuf[i])
            bail2dos("Error allocating buf '%s' to %lld  (size=%ld)!\n", buffer, (intptr_t)&SoundBuf[i], nSize);

        if (kread(hSnd, SoundBuf[i], nSize) != nSize)
            bail2dos("Error reading '%s'!\n", buffer);
    }
    else
    {
        if (!ISDEMOVER) {
            bail2dos("Unable to open sound '%s'!\n", buffer); // demo tries to load sound files it doesn't have
        }

        SoundBuf[i] = NULL;
        SoundLen[i] = 0;
        return -1;
    }

    kclose(hSnd);
    nSoundCount++;
    return i;
}

#if 0
void FreeSounds(void)
{
    int i;
    for (i = 0; i < nSoundCount; i++)
        free(SoundBuf[i]);
}
#endif

void LoadFX(void)
{
    if (!dig)
        return;

    int i;
    for (i = 0; i < kMaxSoundFiles; i++)
    {
        StaticSound[i] = LoadSound(SoundFiles[i]);

        if (StaticSound[i] < 0)
            bail2dos("error loading static sound #%d ('%.8s')\n", i, SoundFiles[i]);
    }
}

void GetSpriteSoundPitch(short nSector, int *pVolume, int *pPitch)
{
    if (nSector < 0)
        return;

    if ((SectFlag[nSector] ^ nLocalSectFlags) & kSectUnderwater)
    {
        *pVolume >>= 1;
        *pPitch -= 1200;
    }
}

void BendAmbientSound(void)
{
    if (nAmbientChannel < 0)
        return;

    ActiveSound *pASound = &sActiveSound[nAmbientChannel];
    //AIL_set_sample_playback_rate(pASound->f_e, nDronePitch+11000);

    if (pASound->hFX > 0)
        FX_SetFrequency(pASound->hFX, nDronePitch+11000);
}

void CheckAmbience(short nSector)
{
    if (SectSound[nSector] != -1)
    {
        short nSector2 = SectSoundSect[nSector];
        walltype *pWall = &wall[sector[nSector2].wallptr];

        if (nAmbientChannel < 0)
        {
            PlayFXAtXYZ(SectSound[nSector]|0x4000, pWall->x, pWall->y, sector[nSector2].floorz, nSector);
            return;
        }

        ActiveSound *pASound = &sActiveSound[nAmbientChannel];

        if (nSector == nSector2)
        {
            spritetype *pSprite = &sprite[PlayerList[0].nSprite];
            pASound->x = pSprite->x;
            pASound->y = pSprite->y;
            pASound->z = pSprite->z;
        }
        else
        {
            pASound->x = pWall->x;
            pASound->y = pWall->y;
            pASound->z = sector[nSector2].floorz;
        }
    }
    else if (nAmbientChannel != -1)
    {
        if (sActiveSound[nAmbientChannel].hFX > 0)
            FX_StopSound(sActiveSound[nAmbientChannel].hFX);

        sActiveSound[nAmbientChannel].hFX = -1;
        nAmbientChannel = -1;
    }
}

int GetDistFromDXDY(int dx, int dy)
{
    int nSqr = dx*dx+dy*dy;
    return (nSqr>>3)-(nSqr>>5);
}

#if 0
void MuteSounds(void)
{
    if (!dig)
        return;

    int i;
    ActiveSound *pASound = sActiveSound;
    pASound++;
    for (i = 1; i < kMaxActiveSounds; i++, pASound++)
    {
        if (pASound->f_e > -1 && FX_SoundActive(pASound->f_e))
            FX_SetPan(pASound->f_e, 0, 0, 0);
    }
}

void SetChanPan(int nSound, int nPan)
{
    AIL_set_sample_pan(sActiveSound[nSound].f_e, nPan);
}
#endif

void SoundBigEntrance(void)
{
    StopAllSounds();
    ActiveSound *pASound = sActiveSound;

    for (int i = 0; i < 4; i++, pASound++)
    {
#if 0
        AIL_init_sample(pASound->f_e);
        short nPitch = i*512-1200;
        pASound->f_16 = nPitch;
        AIL_set_sample_playback_rate(pASound->f_e, 11000+nPitch);
        AIL_set_sample_file(pASound->f_e, SoundBuf[12], -1);
        AIL_set_sample_volume(pASound->f_e, 200);
        AIL_set_sample_pan(pASound->f_e, 63-(i&1)*127);
        AIL_start_sample(pASound->f_e);
#endif
        short nPitch = i*512-1200;
        pASound->f_16 = nPitch;
        int nLeft, nRight;
        CalcASSPan(63-(i&1)*127, 200, &nLeft, &nRight);

        if (pASound->hFX > 0)
            FX_StopSound(pASound->hFX);

        pASound->hFX = FX_Play(SoundBuf[kSoundTorchOn], SoundLen[kSoundTorchOn], -1, 0, 0, max(nLeft, nRight), nLeft, nRight, 0, fix16_one, i);

        if (pASound->hFX > 0)
            FX_SetFrequency(pASound->hFX, 11000+nPitch);
    }
}

void StartSwirly(int nActiveSound)
{
    ActiveSound *pASound = &sActiveSound[nActiveSound];
    pASound->nAngle &= kAngleMask;

    //AIL_init_sample(pASound->f_e);
    short nPitch = nNextFreq - RandomSize(9);
    pASound->f_16 = nPitch;

    nNextFreq = 25000 - RandomSize(10) * 6;
    if (nNextFreq > 32000)
        nNextFreq = 32000;

    //AIL_set_sample_file(pASound->f_e, SoundBuf[StaticSound[kSoundMana1]], -1);

    int nVolume = nSwirlyFrames+1;
    if (nVolume >= 220)
        nVolume = 220;

    pASound->f_4 = nVolume;

    int nPan = pASound->nAngle & 127;
    int nLeft, nRight;

    CalcASSPan(nPan, nVolume, &nLeft, &nRight);

    if (pASound->hFX > 0)
        FX_StopSound(pASound->hFX);

    pASound->hFX = FX_Play(SoundBuf[StaticSound[kSoundMana1]], SoundLen[StaticSound[kSoundMana1]], -1, 0, 0, max(nLeft, nRight), nLeft, nRight, 0, fix16_one, nActiveSound);

//    if (pASound->hFX > 0)
//y        FX_SetFrequency(pASound->hFX, nPitch);

#if 0
    AIL_set_sample_volume(pASound->f_e, nVolume);
    AIL_set_sample_playback_rate(pASound->f_e, nPitch);
    AIL_set_sample_loop_count(pASound->f_e, 1);
    AIL_set_sample_pan(pASound->f_e, pASound->f_6);
    AIL_start_sample(pASound->f_e);
#endif
}

void StartSwirlies()
{
    StopAllSounds();

    nNextFreq = 19000;
    nSwirlyFrames = 0;

    for (int i = 1; i <= 4; i++)
        StartSwirly(i);
}

void UpdateSwirlies()
{
    nSwirlyFrames++;
    ActiveSound* pASound = sActiveSound;
    pASound++;

    for (int i = 1; i <= 4; i++, pASound++)
    {
        if (pASound->hFX <= 0 || !FX_SoundActive(pASound->hFX))
            StartSwirly(i);

        if (pASound->hFX > 0)
        {
            int nLeft, nRight;
            int nPan = 64+(Sin((int)totalclock<<(4+i))>>8);
            CalcASSPan(nPan, pASound->f_4, &nLeft, &nRight);
            FX_SetPan(pASound->hFX, max(nLeft, nRight), nLeft, nRight);
            FX_SetFrequency(pASound->hFX, pASound->f_16);
        }
        //AIL_set_sample_pan(pASound->f_e, 64+(Sin((int)totalclock<<(4+i))>>8));
    }
}

void UpdateSounds()
{
    if (!dig || nFreeze)
        return;

    nLocalSectFlags = SectFlag[nPlayerViewSect[nLocalPlayer]];
//    spritetype *pSprite = &sprite[PlayerList[nLocalPlayer].nSprite];
    int x, y;
    short ang;

    if (nSnakeCam >= 0)
    {
        Snake *pSnake = &SnakeList[nSnakeCam];
        spritetype *pSnakeSprite = &sprite[pSnake->nSprites[0]];
        x = pSnakeSprite->x;
        y = pSnakeSprite->y;
        ang = pSnakeSprite->ang;
    }
    else
    {
        x = initx;
        y = inity;
        ang = inita;
    }

    ActiveSound* pASound = sActiveSound;
    pASound++;

    for (int i = 1; i < kMaxActiveSounds; i++, pASound++)
    {
        if (FX_SoundValidAndActive(pASound->hFX))
        {
            short nSoundSprite = pASound->nSprite;
            int dx, dy;
            if (nSoundSprite < 0)
            {
                dx = x-pASound->x;
                dy = y-pASound->y;
            }
            else
            {
                dx = x-sprite[nSoundSprite].x;
                dy = y-sprite[nSoundSprite].y;
            }

            dx >>= 8;
            dy >>= 8;

            short nSoundAng;
            if ((dx|dy) == 0)
                nSoundAng = 0;
            else
                nSoundAng = AngleDelta(GetMyAngle(dx, dy), ang, 1024);

            int nDist = GetDistFromDXDY(dx, dy);
            if (nDist >= 255)
            {
                FX_StopSound(pASound->hFX);
                if (pASound->f_a&0x4000)
                    nAmbientChannel = -1;
                return;
            }

            int nVolume = kMaxFXVolume - nDist;
            if (nVolume < 0)
                nVolume = 0;
            if (nVolume > kMaxFXVolume)
                nVolume = kMaxFXVolume;

            int nPitch = pASound->f_16;
            short nSoundSect;
            if (nSoundSprite >= 0)
            {
                if (nSoundSprite == nLocalSpr)
                    nSoundSect = nPlayerViewSect[nLocalPlayer];
                else
                    nSoundSect = sprite[nSoundSprite].sectnum;
            }
            else
                nSoundSect = pASound->nSector;

            GetSpriteSoundPitch(nSoundSect, &nVolume, &nPitch);

            if (pASound->f_12 != nPitch)
                pASound->f_12 = nPitch;

            if (nVolume != pASound->f_4 || nSoundAng != pASound->nAngle)
            {
                int nLeft, nRight;
                int nPan = 63+(Sin(nSoundAng+1023)>>8);
                CalcASSPan(nPan, nVolume, &nLeft, &nRight);
                FX_SetPan(pASound->hFX, nVolume, nLeft, nRight);

                if (nPitch < 0)
                    FX_SetFrequency(pASound->hFX, 7000);

                pASound->f_4 = nVolume;
                pASound->nAngle = nSoundAng;
            }
        }
    }
}

void UpdateCreepySounds()
{
    if (levelnum == 20 || nFreeze)
        return;

    spritetype *pSprite = &sprite[PlayerList[nLocalPlayer].nSprite];
    nCreepyTimer--;

    if (nCreepyTimer <= 0)
    {
        if (nCreaturesLeft > 0 && !(SectFlag[nPlayerViewSect[nLocalPlayer]] & 0x2000))
        {
            int vsi = seq_GetFrameSound(SeqOffsets[kSeqCreepy], totalmoves % SeqSize[SeqOffsets[kSeqCreepy]]);

            if (vsi >= 0 && (vsi&0x1ff) < kMaxSounds)
            {
                int vdx = (totalmoves+32)&31;

                if (totalmoves & 1)
                    vdx = -vdx;

                int vax = (totalmoves+32)&63;

                if (totalmoves & 2)
                    vax = -vax;

                PlayFXAtXYZ(vsi, pSprite->x+vdx, pSprite->y+vax, pSprite->z, pSprite->sectnum);
            }
        }
        nCreepyTimer = kCreepyCount;
    }
}

int LocalSoundPlaying(void)
{
    if (!dig)
        return 0;

    return sActiveSound[nLocalChan].hFX > 0 && FX_SoundActive(sActiveSound[nLocalChan].hFX);
}

int GetLocalSound(void)
{
    if (LocalSoundPlaying() == -1)
        return -1;

    return sActiveSound[nLocalChan].nSound & 0x1ff;
}

void UpdateLocalSound(void)
{
    if (!dig)
        return;

    if (sActiveSound[nLocalChan].hFX > 0)
        FX_SetPan(sActiveSound[nLocalChan].hFX, kMaxFXVolume, kMaxFXVolume, kMaxFXVolume);
}

void StopLocalSound(void)
{
    if (!dig)
        return;

    if (nLocalChan == nAmbientChannel)
        nAmbientChannel = -1;

    if (LocalSoundPlaying())
        FX_StopSound(sActiveSound[nLocalChan].hFX);
}

void SetLocalChan(int nChannel)
{
    nLocalChan = nChannel;
}

void PlaySound(int nSound)
{
    if (!dig)
        return;

    if (nSound < 0 || nSound >= kMaxSounds || !SoundBuf[nSound])
    {
        initprintf("PlaySound: Invalid sound nSound == %i\n", nSound);
        return;
    }

    int bLoop = SoundBuf[nSound][26] == 6;

    if (handle >= 0)
        FX_StopSound(handle);

    handle = FX_Play(SoundBuf[nSound], SoundLen[nSound], bLoop ? 0 : -1, 0, 0, kMaxFXVolume, kMaxFXVolume, kMaxFXVolume, 0, fix16_one, -1);

#if 0
    AIL_init_sample(handle);
    AIL_set_sample_file(handle, SoundBuf[nSound], -1);
    AIL_set_sample_volume(handle, kMaxFXVolume >>1);

    if (SoundBuf[nSound][26] == 6)
        AIL_set_sample_loop_count(handle, 0);

    AIL_start_sample(handle);
#endif
}

void SetMasterFXVolume(int nVolume)
{
    if (nVolume > 255)
        nVolume = 255;
    if (nVolume < 0)
        nVolume = 0;

    FX_SetVolume(nVolume);
}

void PlayLocalSound(short nSound, short nRate)
{
    if (!SoundToggle)
        return;

    if (!dig)
        return;

    if (nSound < 0 || nSound >= kMaxSounds || !SoundBuf[nSound])
    {
        initprintf("PlayLocalSound: Invalid sound nSound == %i, nRate == %i\n", nSound, nRate);
        return;
    }

    if (nLocalChan == nAmbientChannel)
        nAmbientChannel = -1;

    int bLoop = SoundBuf[nSound][26] == 6;

    ActiveSound* pASound = &sActiveSound[nLocalChan];

    if (pASound->hFX > 0)
        FX_StopSound(pASound->hFX);

    pASound->hFX = FX_Play(SoundBuf[nSound], SoundLen[nSound], bLoop ? 0 : -1, 0, 0, kMaxFXVolume, kMaxFXVolume, kMaxFXVolume, 0, fix16_one, nLocalChan);

    if (nRate)
    {
        int nFreq = 0;
        FX_GetFrequency(pASound->hFX, &nFreq);
        FX_SetFrequency(pASound->hFX, nFreq+nRate);
    }
#if 0
    AIL_init_sample(pASound->f_e);
    AIL_set_sample_file(pASound->f_e, SoundBuf[nSound], -1);
    AIL_set_sample_volume(pASound->f_e, kMaxFXVolume >>1);

    if (nRate)
        AIL_set_sample_playback_rate(pASound->f_e, AIL_sample_playback_rate(pASound->f_e)+nRate);

    if (SoundBuf[nSound][26] == 6)
        AIL_set_sample_loop_count(pASound->f_e, 0);

    AIL_start_sample(pASound->f_e);
#endif
    pASound->nSound = nSound;
    SetLocalChan(0);
}

void PlayTitleSound(void)
{
    PlayLocalSound(StaticSound[kSoundItemSpecial], 0);
}

void PlayLogoSound(void)
{
    PlayLocalSound(StaticSound[kSoundJonLaugh2], 7000);
}

void PlayGameOverSound(void)
{
    PlayLocalSound(StaticSound[kSoundJonLaugh2], 0);
}

int soundx, soundy, soundz;
short soundsect;

short PlayFX2(unsigned short nSound, short nSprite)
{
    if (!SoundToggle)
        return -1;

    if (!dig)
        return -1;

    if ((nSound&0x1ff) >= kMaxSounds || !SoundBuf[(nSound&0x1ff)])
    {
        initprintf("PlayFX2: Invalid sound nSound == %i, nSprite == %i\n", nSound, nSprite);
        return -1;
    }

    nLocalSectFlags = SectFlag[nPlayerViewSect[nLocalPlayer]];
    short v1c = 0;
    short vcx = 0;

    if (nSprite >= 0)
    {
        v1c = nSprite & 0x2000;
        vcx = nSprite & 0x4000;
        nSprite &= 0xfff;
        soundx = sprite[nSprite].x;
        soundy = sprite[nSprite].y;
        soundz = sprite[nSprite].z;
        soundsect = sprite[nSprite].sectnum;
    }

    int dx = (initx - soundx) >> 8;
    int dy = (inity - soundy) >> 8;

    short nSoundAng = 0;

    if (dx || dy) {
        nSoundAng = AngleDelta(GetMyAngle(dx, dy), inita, 1024);
    }

    int nDist = GetDistFromDXDY(dx, dy);
    if (nDist >= 255)
    {
        if (nSprite > -1)
            StopSpriteSound(nSprite);

        return -1;
    }

    int nVolume;

    if (!v1c)
    {
        nVolume = kMaxFXVolume - nDist;
        if (nVolume <= 0)
        {
            if (nSprite > -1)
                StopSpriteSound(nSprite);
            return -1;
        }
        if (nVolume > kMaxFXVolume)
            nVolume = kMaxFXVolume;
    }
    else
        nVolume = kMaxFXVolume;

    short vc = nSound & (~0x1ff);
    short v4 = nSound & 0x2000;
    short v8 = nSound & 0x1000;
    short v14 = nSound & 0x4000;
    short v10 = (nSound & 0xe00)>>9;
    int v2c = 0x7fffffff;
    ActiveSound* v38 = NULL;
    ActiveSound* v28 = NULL;
    ActiveSound* vdi = NULL;
    nSound &= 0x1ff;

    short v20;

    if (v8 || v14)
        v20 = 1000;
    else if (nSprite != -1 && vcx)
        v20 = 2000;
    else
        v20 = 0;

    ActiveSound* pASound = sActiveSound;
    pASound++;

    for (int i = 1; i < kMaxActiveSounds; i++, pASound++)
    {
        if (pASound->hFX <= 0 || !FX_SoundActive(pASound->hFX))
            vdi = pASound;
        else if (v20 >= pASound->f_c)
        {
            if (v2c > pASound->nClock && pASound->f_c <= v20)
            {
                v28 = pASound;
                v2c = pASound->nClock;
            }
            if (!v8)
            {
                if (nSound == pASound->nSound)
                {
                    if (v4 == 0 && nSprite == pASound->nSprite)
                        return -1;

                    if (v20 >= pASound->f_c)
                        v38 = pASound;
                }
                else if (nSprite == pASound->nSprite)
                {
                    if (v4 || nSound != pASound->nSound)
                    {
                        vdi = pASound;
                        break;
                    }
                }
            }
        }
    }

    if (!vdi)
    {
        if (v38)
            vdi = v38;
        else if (v28)
            vdi = v28;
    }

    if (FX_SoundValidAndActive(vdi->hFX))
    {
        FX_StopSound(vdi->hFX);
        if (vdi->nChannel == nAmbientChannel)
            nAmbientChannel = -1;
    }

    int nPitch;
    if (v10)
        nPitch = -(totalmoves&((1<<v10)-1))*16;
    else
        nPitch = 0;

    if (vdi)
    {
        vdi->f_16 = nPitch;
        vdi->f_8 = nVolume;

        if (nSprite < 0)
        {
            vdi->x = soundx;
            vdi->y = soundy;
            vdi->nSector = soundsect;
        }

        GetSpriteSoundPitch(soundsect, &nVolume, &nPitch);

        vdi->f_12 = nPitch;
        vdi->f_4 = nVolume;
        vdi->nAngle = nSoundAng;
        vdi->nSprite = nSprite;
        vdi->nSound = nSound;
        vdi->nClock = (int)totalclock;
        vdi->f_c = v20;
        vdi->f_a = vc;
        short nPan = 63+(Sin(nSoundAng+1023)>>8);
        int nLeft, nRight;

        int bLoop = SoundBuf[nSound][26] == 6;

        CalcASSPan(nPan, nVolume>>1, &nLeft, &nRight);

        vdi->hFX = FX_Play(SoundBuf[nSound], SoundLen[nSound], bLoop ? 0 : -1, 0, 0, max(nLeft, nRight), nLeft, nRight, 0, fix16_one, vdi-sActiveSound);

        if (nPitch)
        {
            int nFreq = 0;
            FX_GetFrequency(vdi->hFX, &nFreq);
            FX_SetFrequency(vdi->hFX, nFreq+nPitch);
        }
#if 0
        AIL_init_sample(vdi->f_e);
        AIL_set_sample_file(vdi->f_e, SoundBuf[nSound], -1);
        AIL_set_sample_pan(vdi->f_e, nPan);
        AIL_set_sample_volume(vdi->f_e, nVolume>>1);

        if (SoundBuf[nSound][26] == 6)
            AIL_set_sample_loop_count(vdi->f_e, 0);

        if (nPitch)
            AIL_set_sample_playback_rate(vdi->f_e, AIL_sample_playback_rate(vdi->f_e)+nPitch);

        AIL_start_sample(vdi->f_e);
#endif
        if (v14)
            nAmbientChannel = v14;

        // Nuke: added nSprite >= 0 check
        if (nSprite != nLocalSpr && nSprite >= 0 && (sprite[nSprite].cstat & 257))
            nCreepyTimer = kCreepyCount;

        return v14;
    }
    return -1;
}

short PlayFXAtXYZ(unsigned short nSound, int x, int y, int z, int nSector)
{
    soundx = x;
    soundy = y;
    soundz = z;
    soundsect = nSector & 0x3fff;
    short nSnd = PlayFX2(nSound, -1);

    if (nSnd > -1 && (nSector & 0x4000))
        sActiveSound[nSnd].f_c = 2000;

    return nSnd;
}

short D3PlayFX(unsigned short nSound, short nSprite)
{
    return PlayFX2(nSound, nSprite);
}

void StopSpriteSound(short nSprite)
{
    if (!dig)
        return;

    for (int i = 0; i < kMaxActiveSounds; i++)
    {
        if (FX_SoundValidAndActive(sActiveSound[i].hFX) && nSprite == sActiveSound[i].nSprite)
        {
            FX_StopSound(sActiveSound[i].hFX);
            return;
        }
    }
}

void StopAllSounds(void)
{
    if (!dig)
        return;

    for (int i = 0; i < kMaxActiveSounds; i++)
    {
        if (sActiveSound[i].hFX > 0)
            FX_StopSound(sActiveSound[i].hFX);
        // AIL_end_sample(sActiveSound[i].hFX);
    }

    nSoundsPlaying = 0;
    nAmbientChannel = -1;
}
