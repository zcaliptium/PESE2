 
#include "StdH.h"
#include "GameMP/SEColors.h"

#include <Engine/Graphics/DrawPort.h>

#include <EntitiesMP/Player.h>
#include <EntitiesMP/PlayerView.h>
#include <EntitiesMP/PlayerWeapons.h>
#include <EntitiesMP/MusicHolder.h>
#include <EntitiesMP/EnemyBase.h>
#include <EntitiesMP/EnemyCounter.h>
#include <EntitiesMP/PlayerBot.h>
#include <EntitiesMP/EnemySpawner.h>
#include <EntitiesMP/PESpawnerEG.h>
#include <EntitiesMP/Display.h>

#define ENTITY_DEBUG

// armor & health constants 
// NOTE: these _do not_ reflect easy/tourist maxvalue adjustments. that is by design!
#define TOP_ARMOR  100
#define TOP_HEALTH 100


// cheats
extern INDEX cht_bEnable;
extern INDEX cht_bGod;
extern INDEX cht_bFly;
extern INDEX cht_bGhost;
extern INDEX cht_bInvisible;
extern FLOAT cht_fTranslationMultiplier;

// interface control
extern INDEX hud_bShowInfo;
extern INDEX hud_bShowLatency;
extern INDEX hud_bShowMessages;
extern INDEX hud_iShowPlayers;
extern INDEX hud_iSortPlayers;
extern FLOAT hud_fOpacity;
extern FLOAT hud_fScaling;
extern FLOAT hud_tmWeaponsOnScreen;
extern INDEX hud_bShowMatchInfo;

// mod chit
extern INDEX hud_bShowEnemyRadar;
extern INDEX hud_bShowPlayerRadar;
extern INDEX hud_bShowPlayerRadar2;
extern INDEX hud_bColorCritterRadar;
extern INDEX hud_bShowScore;
extern INDEX dwk_bShowEnemyCount;

// info from overlord
extern INDEX hud_bShowPEInfo;
// INDEX ol_ctEnemiesAlive;
//extern INDEX ol_ctEvents;
//extern INDEX ol_iEventCount;
//extern INDEX ol_ctSpawnersActive;
//extern INDEX ol_ctCTSpawnersActive;
//extern INDEX ol_ctPESpawnersActive;
//extern INDEX ol_ctSpawnersInQueue;
//extern FLOAT ol_fSpawnersMaxAge;
//extern FLOAT ol_fLag;
//extern INDEX ol_ctSpawnersTriggered;
//extern INDEX ol_ctSpawnersFinished;
//extern INDEX ol_ctSpawnersTotal;

// player statistics sorting keys
enum SortKeys {
  PSK_SCORE   = 1,
  PSK_HEALTH  = 2,
  PSK_LVL_FRAGS = 3,
  PSK_MANA    = 4, 
  PSK_FRAGS   = 5,
  PSK_DEATHS  = 6,
};

// where is the bar lowest value
enum BarOrientations {
  BO_LEFT  = 1,
  BO_RIGHT = 2, 
  BO_UP    = 3,
  BO_DOWN  = 4,
};

extern const INDEX aiWeaponsRemap[19];

// maximal mana for master status
#define MANA_MASTER 10000

// drawing variables
static const CPlayer *_penPlayer;
static CPlayerWeapons *_penWeapons;
static CDrawPort *_pDP;
static PIX   _pixDPWidth, _pixDPHeight;
static FLOAT _fResolutionScaling;
static FLOAT _fCustomScaling;
static ULONG _ulAlphaHUD;
static COLOR _colHUD;
static COLOR _colHUDText;
static TIME  _tmNow = -1.0f;
static TIME  _tmLast = -1.0f;
static CFontData _fdNumbersFont;
//static INDEX ctEnemiesLast = 0;

// array for pointers of all players
extern CPlayer *_apenPlayers[NET_MAXGAMEPLAYERS] = {0};

// status bar textures
static CTextureObject _toHealth;
static CTextureObject _toOxygen;
static CTextureObject _toScore;
static CTextureObject _toHiScore;
static CTextureObject _toMessage;
static CTextureObject _toMana;
static CTextureObject _toFrags;
static CTextureObject _toDeaths;
static CTextureObject _toArmorSmall;
static CTextureObject _toArmorMedium;
static CTextureObject _toArmorLarge;

// ammo textures                    
static CTextureObject _toAShells;
static CTextureObject _toABullets;
static CTextureObject _toARockets;
static CTextureObject _toAGrenades;
static CTextureObject _toANapalm;
static CTextureObject _toAElectricity;
static CTextureObject _toAIronBall;
static CTextureObject _toASniperBullets;
static CTextureObject _toASeriousBomb;
static CTextureObject _toADaBomb;
// weapon textures
static CTextureObject _toWKnife;
static CTextureObject _toWColt;
static CTextureObject _toWSingleShotgun;
static CTextureObject _toWDoubleShotgun;
static CTextureObject _toWTommygun;
static CTextureObject _toWSniper;
static CTextureObject _toWChainsaw;
static CTextureObject _toWMinigun;
static CTextureObject _toWRocketLauncher;
static CTextureObject _toWGrenadeLauncher;
static CTextureObject _toWFlamer;
static CTextureObject _toWLaser;
static CTextureObject _toWIronCannon;

// powerup textures (ORDER IS THE SAME AS IN PLAYER.ES!)
#define MAX_POWERUPS 4
static CTextureObject _atoPowerups[MAX_POWERUPS];
// tile texture (one has corners, edges and center)
static CTextureObject _toTile;
static CTextureObject _toSniperLine;
static CTextureObject _toPlayer;
static CTextureObject _toPlayerRadar;
static CTextureObject _toEnemyDot;
static CTextureObject _toN;
static CTextureObject _toE;
static CTextureObject _toS;
static CTextureObject _toW;

//static CTextureObject _toIHealthBar;
//static CTextureObject _toIHealth;

// all info about color transitions
struct ColorTransitionTable {
  COLOR ctt_colFine;      // color for values over 1.0
  COLOR ctt_colHigh;      // color for values from 1.0 to 'fMedium'
  COLOR ctt_colMedium;    // color for values from 'fMedium' to 'fLow'
  COLOR ctt_colLow;       // color for values under fLow
  FLOAT ctt_fMediumHigh;  // when to switch to high color   (normalized float!)
  FLOAT ctt_fLowMedium;   // when to switch to medium color (normalized float!)
  BOOL  ctt_bSmooth;      // should colors have smooth transition
};
static struct ColorTransitionTable _cttHUD;


// ammo's info structure
struct AmmoInfo {
  CTextureObject    *ai_ptoAmmo;
  struct WeaponInfo *ai_pwiWeapon1;
  struct WeaponInfo *ai_pwiWeapon2;
  INDEX ai_iAmmoAmmount;
  INDEX ai_iMaxAmmoAmmount;
  INDEX ai_iLastAmmoAmmount;
  TIME  ai_tmAmmoChanged;
  BOOL  ai_bHasWeapon;
};

// weapons' info structure
struct WeaponInfo {
  enum WeaponType  wi_wtWeapon;
  CTextureObject  *wi_ptoWeapon;
  struct AmmoInfo *wi_paiAmmo;
  BOOL wi_bHasWeapon;
};

extern struct WeaponInfo _awiWeapons[18];
static struct AmmoInfo _aaiAmmo[9] = {
  { &_toAShells,        &_awiWeapons[4],  &_awiWeapons[5],  0, 0, 0, -9, FALSE }, //  0
  { &_toABullets,       &_awiWeapons[6],  &_awiWeapons[7],  0, 0, 0, -9, FALSE }, //  1
  { &_toARockets,       &_awiWeapons[8],  NULL,             0, 0, 0, -9, FALSE }, //  2
  { &_toAGrenades,      &_awiWeapons[9],  NULL,             0, 0, 0, -9, FALSE }, //  3
  { &_toANapalm,        &_awiWeapons[11], NULL,             0, 0, 0, -9, FALSE }, //  4
  { &_toAElectricity,   &_awiWeapons[12], NULL,             0, 0, 0, -9, FALSE }, //  5
  { &_toAIronBall,      &_awiWeapons[14], NULL,             0, 0, 0, -9, FALSE }, //  6
  { &_toASniperBullets, &_awiWeapons[13], NULL,             0, 0, 0, -9, FALSE }, //  7
  { &_toADaBomb,			  &_awiWeapons[15], NULL,             0, 0, 0, -9, FALSE }, //  15
};
static const INDEX aiAmmoRemap[9] = { 0, 1, 2, 3, 4, 7, 5, 6, 8 };

struct WeaponInfo _awiWeapons[18] = {
  { WEAPON_NONE,            NULL,                 NULL,         FALSE },   //  0
  { WEAPON_KNIFE,           &_toWKnife,           NULL,         FALSE },   //  1
  { WEAPON_COLT,            &_toWColt,            NULL,         FALSE },   //  2
  { WEAPON_DOUBLECOLT,      &_toWColt,            NULL,         FALSE },   //  3
  { WEAPON_SINGLESHOTGUN,   &_toWSingleShotgun,   &_aaiAmmo[0], FALSE },   //  4
  { WEAPON_DOUBLESHOTGUN,   &_toWDoubleShotgun,   NULL,					FALSE },   //  5
  { WEAPON_TOMMYGUN,        &_toWTommygun,        &_aaiAmmo[8], FALSE },   //  6
  { WEAPON_MINIGUN,         &_toWMinigun,         &_aaiAmmo[1], FALSE },   //  7
  { WEAPON_ROCKETLAUNCHER,  &_toWRocketLauncher,  &_aaiAmmo[2], FALSE },   //  8
  { WEAPON_GRENADELAUNCHER, &_toWGrenadeLauncher, &_aaiAmmo[3], FALSE },   //  9
  { WEAPON_CHAINSAW,        &_toWChainsaw,        NULL,         FALSE },   // 10
  { WEAPON_FLAMER,          &_toWFlamer,          &_aaiAmmo[4], FALSE },   // 11
  { WEAPON_LASER,           &_toWLaser,           &_aaiAmmo[5], FALSE },   // 12
  { WEAPON_SNIPER,          &_toWSniper,          &_aaiAmmo[7], FALSE },   // 13
  { WEAPON_IRONCANNON,      &_toWIronCannon,      &_aaiAmmo[6], FALSE },   // 14
//{ WEAPON_PIPEBOMB,        &_toWPipeBomb,        &_aaiAmmo[3], FALSE },   // 15
//{ WEAPON_GHOSTBUSTER,     &_toWGhostBuster,     &_aaiAmmo[5], FALSE },   // 16
//{ WEAPON_NUKECANNON,      &_toWNukeCannon,      &_aaiAmmo[7], FALSE },   // 17
  { WEAPON_NONE,            NULL,                 NULL,         FALSE },   // 15
  { WEAPON_NONE,            NULL,                 NULL,         FALSE },   // 16
  { WEAPON_NONE,            NULL,                 NULL,         FALSE },   // 17
};


// compare functions for qsort()
static int qsort_CompareLvlFrags( const void *ppPEN0, const void *ppPEN1) {
  CPlayer &en0 = **(CPlayer**)ppPEN0;
  CPlayer &en1 = **(CPlayer**)ppPEN1;
  SLONG sl0 = en0.m_psLevelStats.ps_iKills;
  SLONG sl1 = en1.m_psLevelStats.ps_iKills;
  if(      sl0<sl1) return +1;
  else if( sl0>sl1) return -1;
  else              return  0;
}

static int qsort_CompareScores( const void *ppPEN0, const void *ppPEN1) {
  CPlayer &en0 = **(CPlayer**)ppPEN0;
  CPlayer &en1 = **(CPlayer**)ppPEN1;
  SLONG sl0 = en0.m_psGameStats.ps_iScore;
  SLONG sl1 = en1.m_psGameStats.ps_iScore;
  if(      sl0<sl1) return +1;
  else if( sl0>sl1) return -1;
  else              return  0;
}

static int qsort_CompareHealth( const void *ppPEN0, const void *ppPEN1) {
  CPlayer &en0 = **(CPlayer**)ppPEN0;
  CPlayer &en1 = **(CPlayer**)ppPEN1;
  SLONG sl0 = (SLONG)ceil(en0.GetHealth());
  SLONG sl1 = (SLONG)ceil(en1.GetHealth());
  if(      sl0<sl1) return +1;
  else if( sl0>sl1) return -1;
  else              return  0;
}

static int qsort_CompareManas( const void *ppPEN0, const void *ppPEN1) {
  CPlayer &en0 = **(CPlayer**)ppPEN0;
  CPlayer &en1 = **(CPlayer**)ppPEN1;
  SLONG sl0 = en0.m_iMana;
  SLONG sl1 = en1.m_iMana;
  if(      sl0<sl1) return +1;
  else if( sl0>sl1) return -1;
  else              return  0;
}

static int qsort_CompareDeaths( const void *ppPEN0, const void *ppPEN1) {
  CPlayer &en0 = **(CPlayer**)ppPEN0;
  CPlayer &en1 = **(CPlayer**)ppPEN1;
  SLONG sl0 = en0.m_psGameStats.ps_iDeaths;
  SLONG sl1 = en1.m_psGameStats.ps_iDeaths;
  if(      sl0<sl1) return +1;
  else if( sl0>sl1) return -1;
  else              return  0;
}

static int qsort_CompareFrags( const void *ppPEN0, const void *ppPEN1) {
  CPlayer &en0 = **(CPlayer**)ppPEN0;
  CPlayer &en1 = **(CPlayer**)ppPEN1;
  SLONG sl0 = en0.m_psGameStats.ps_iKills;
  SLONG sl1 = en1.m_psGameStats.ps_iKills;
  if(      sl0<sl1) return +1;
  else if( sl0>sl1) return -1;
  else              return -qsort_CompareDeaths(ppPEN0, ppPEN1);
}

static int qsort_CompareLatencies( const void *ppPEN0, const void *ppPEN1) {
  CPlayer &en0 = **(CPlayer**)ppPEN0;
  CPlayer &en1 = **(CPlayer**)ppPEN1;
  SLONG sl0 = (SLONG)ceil(en0.m_tmLatency);
  SLONG sl1 = (SLONG)ceil(en1.m_tmLatency);
  if(      sl0<sl1) return +1;
  else if( sl0>sl1) return -1;
  else              return  0;
}

// prepare color transitions
static void PrepareColorTransitions( COLOR colFine, COLOR colHigh, COLOR colMedium, COLOR colLow,
                                     FLOAT fMediumHigh, FLOAT fLowMedium, BOOL bSmooth)
{
  _cttHUD.ctt_colFine     = colFine;
  _cttHUD.ctt_colHigh     = colHigh;   
  _cttHUD.ctt_colMedium   = colMedium;
  _cttHUD.ctt_colLow      = colLow;
  _cttHUD.ctt_fMediumHigh = fMediumHigh;
  _cttHUD.ctt_fLowMedium  = fLowMedium;
  _cttHUD.ctt_bSmooth     = bSmooth;
}



// calculates shake ammount and color value depanding on value change
#define SHAKE_TIME (2.0f)
static COLOR AddShaker( PIX const pixAmmount, INDEX const iCurrentValue, INDEX &iLastValue,
                        TIME &tmChanged, FLOAT &fMoverX, FLOAT &fMoverY)
{
  // update shaking if needed
  fMoverX = fMoverY = 0.0f;
  const TIME tmNow = _pTimer->GetLerpedCurrentTick();
  if( iCurrentValue != iLastValue) {
    iLastValue = iCurrentValue;
    tmChanged  = tmNow;
  } else {
    // in case of loading (timer got reseted)
    tmChanged = ClampUp( tmChanged, tmNow);
  }
  
  // no shaker?
  const TIME tmDelta = tmNow - tmChanged;
  if( tmDelta > SHAKE_TIME) return NONE;
  ASSERT( tmDelta>=0);
  // shake, baby shake!
  const FLOAT fAmmount    = _fResolutionScaling * _fCustomScaling * pixAmmount;
  const FLOAT fMultiplier = (SHAKE_TIME-tmDelta)/SHAKE_TIME *fAmmount;
  const INDEX iRandomizer = (INDEX)(tmNow*511.0f)*fAmmount*iCurrentValue;
  const FLOAT fNormRnd1   = (FLOAT)((iRandomizer ^ (iRandomizer>>9)) & 1023) * 0.0009775f;  // 1/1023 - normalized
  const FLOAT fNormRnd2   = (FLOAT)((iRandomizer ^ (iRandomizer>>7)) & 1023) * 0.0009775f;  // 1/1023 - normalized
  fMoverX = (fNormRnd1 -0.5f) * fMultiplier;
  fMoverY = (fNormRnd2 -0.5f) * fMultiplier;
  // clamp to adjusted ammount (pixels relative to resolution and HUD scale
  fMoverX = Clamp( fMoverX, -fAmmount, fAmmount);
  fMoverY = Clamp( fMoverY, -fAmmount, fAmmount);
  if( tmDelta < SHAKE_TIME/3) return C_WHITE;
  else return NONE;
//return FloatToInt(tmDelta*4) & 1 ? C_WHITE : NONE;
}


// get current color from local color transitions table
static COLOR GetCurrentColor( FLOAT fNormalizedValue)
{
  // if value is in 'low' zone just return plain 'low' alert color
  if( fNormalizedValue < _cttHUD.ctt_fLowMedium) return( _cttHUD.ctt_colLow & 0xFFFFFF00);
  // if value is in out of 'extreme' zone just return 'extreme' color
  if( fNormalizedValue > 1.0f) return( _cttHUD.ctt_colFine & 0xFFFFFF00);
 
  COLOR col;
  // should blend colors?
  if( _cttHUD.ctt_bSmooth)
  { // lets do some interpolations
    FLOAT fd, f1, f2;
    COLOR col1, col2;
    UBYTE ubH,ubS,ubV, ubH2,ubS2,ubV2;
    // determine two colors for interpolation
    if( fNormalizedValue > _cttHUD.ctt_fMediumHigh) {
      f1   = 1.0f;
      f2   = _cttHUD.ctt_fMediumHigh;
      col1 = _cttHUD.ctt_colHigh;
      col2 = _cttHUD.ctt_colMedium;
    } else { // fNormalizedValue > _cttHUD.ctt_fLowMedium == TRUE !
      f1   = _cttHUD.ctt_fMediumHigh;
      f2   = _cttHUD.ctt_fLowMedium;
      col1 = _cttHUD.ctt_colMedium;
      col2 = _cttHUD.ctt_colLow;
    }
    // determine interpolation strength
    fd = (fNormalizedValue-f2) / (f1-f2);
    // convert colors to HSV
    ColorToHSV( col1, ubH,  ubS,  ubV);
    ColorToHSV( col2, ubH2, ubS2, ubV2);
    // interpolate H, S and V components
    ubH = (UBYTE)(ubH*fd + ubH2*(1.0f-fd));
    ubS = (UBYTE)(ubS*fd + ubS2*(1.0f-fd));
    ubV = (UBYTE)(ubV*fd + ubV2*(1.0f-fd));
    // convert HSV back to COLOR
    col = HSVToColor( ubH, ubS, ubV);
  }
  else
  { // simple color picker
    col = _cttHUD.ctt_colMedium;
    if( fNormalizedValue > _cttHUD.ctt_fMediumHigh) col = _cttHUD.ctt_colHigh;
  }
  // all done
  return( col & 0xFFFFFF00);
}



// fill array with players' statistics (returns current number of players in game)
extern INDEX SetAllPlayersStats( INDEX iSortKey)
{
  // determine maximum number of players for this session
  INDEX iPlayers    = 0;
  INDEX iMaxPlayers = _penPlayer->GetMaxPlayers();
  CPlayer *penCurrent;
  // loop thru potentional players 
  for( INDEX i=0; i<iMaxPlayers; i++)
  { // ignore non-existent players
    penCurrent = (CPlayer*)&*_penPlayer->GetPlayerEntity(i);
    if( penCurrent==NULL) continue;
    // fill in player parameters
    _apenPlayers[iPlayers] = penCurrent;
    // advance to next real player
    iPlayers++;
  }
  // sort statistics by some key if needed
  switch( iSortKey) {
  case PSK_LVL_FRAGS: qsort( _apenPlayers, iPlayers, sizeof(CPlayer*), qsort_CompareLvlFrags);  break;
  case PSK_SCORE:			qsort( _apenPlayers, iPlayers, sizeof(CPlayer*), qsort_CompareScores);		break;
  case PSK_HEALTH:		qsort( _apenPlayers, iPlayers, sizeof(CPlayer*), qsort_CompareHealth);		break;
  case PSK_MANA:			qsort( _apenPlayers, iPlayers, sizeof(CPlayer*), qsort_CompareManas);			break;
  case PSK_FRAGS:			qsort( _apenPlayers, iPlayers, sizeof(CPlayer*), qsort_CompareFrags);			break;
  case PSK_DEATHS:		qsort( _apenPlayers, iPlayers, sizeof(CPlayer*), qsort_CompareDeaths);		break;
  default:  break;  // invalid or NONE key specified so do nothing
  }
  // all done
  return iPlayers;
}



// ----------------------- drawing functions

// draw border with filter
static void HUD_DrawBorder( FLOAT fCenterX, FLOAT fCenterY, FLOAT fSizeX, FLOAT fSizeY, COLOR colTiles)
{
  // determine location
  const FLOAT fCenterI  = fCenterX*_pixDPWidth  / 640.0f;
  const FLOAT fCenterJ  = fCenterY*_pixDPHeight / (480.0f * _pDP->dp_fWideAdjustment);
  const FLOAT fSizeI    = _fResolutionScaling*fSizeX;
  const FLOAT fSizeJ    = _fResolutionScaling*fSizeY;
  const FLOAT fTileSize = 8*_fResolutionScaling*_fCustomScaling;
  // determine exact positions
  const FLOAT fLeft  = fCenterI  - fSizeI/2 -1; 
  const FLOAT fRight = fCenterI  + fSizeI/2 +1; 
  const FLOAT fUp    = fCenterJ  - fSizeJ/2 -1; 
  const FLOAT fDown  = fCenterJ  + fSizeJ/2 +1;
  const FLOAT fLeftEnd  = fLeft  + fTileSize;
  const FLOAT fRightBeg = fRight - fTileSize; 
  const FLOAT fUpEnd    = fUp    + fTileSize; 
  const FLOAT fDownBeg  = fDown  - fTileSize; 
  // prepare texture                 
  colTiles |= _ulAlphaHUD;
  // put corners
  _pDP->InitTexture( &_toTile, TRUE); // clamping on!
  _pDP->AddTexture( fLeft, fUp,   fLeftEnd, fUpEnd,   colTiles);
  _pDP->AddTexture( fRight,fUp,   fRightBeg,fUpEnd,   colTiles);
  _pDP->AddTexture( fRight,fDown, fRightBeg,fDownBeg, colTiles);
  _pDP->AddTexture( fLeft, fDown, fLeftEnd, fDownBeg, colTiles);
  // put edges
  _pDP->AddTexture( fLeftEnd,fUp,    fRightBeg,fUpEnd,   0.4f,0.0f, 0.6f,1.0f, colTiles);
  _pDP->AddTexture( fLeftEnd,fDown,  fRightBeg,fDownBeg, 0.4f,0.0f, 0.6f,1.0f, colTiles);
  _pDP->AddTexture( fLeft,   fUpEnd, fLeftEnd, fDownBeg, 0.0f,0.4f, 1.0f,0.6f, colTiles);
  _pDP->AddTexture( fRight,  fUpEnd, fRightBeg,fDownBeg, 0.0f,0.4f, 1.0f,0.6f, colTiles);
  // put center
  _pDP->AddTexture( fLeftEnd, fUpEnd, fRightBeg, fDownBeg, 0.4f,0.4f, 0.6f,0.6f, colTiles);
  _pDP->FlushRenderingQueue();
}


// draw icon texture (if color = NONE, use colortransitions structure)
static void HUD_DrawIcon( FLOAT fCenterX, FLOAT fCenterY, CTextureObject &toIcon,
                          COLOR colDefault, FLOAT fNormValue, BOOL bBlink)
{
  // determine color
  COLOR col = colDefault;
  if( col==NONE) col = GetCurrentColor( fNormValue);
  // determine blinking state
  if( bBlink && fNormValue<=(_cttHUD.ctt_fLowMedium/2)) {
    // activate blinking only if value is <= half the low edge
    INDEX iCurrentTime = (INDEX)(_tmNow*4);
    if( iCurrentTime&1) col = C_vdGRAY;
  }
  // determine location
  const FLOAT fCenterI = fCenterX*_pixDPWidth  / 640.0f;
  const FLOAT fCenterJ = fCenterY*_pixDPHeight / (480.0f * _pDP->dp_fWideAdjustment);
  // determine dimensions
  CTextureData *ptd = (CTextureData*)toIcon.GetData();
  const FLOAT fHalfSizeI = _fResolutionScaling*_fCustomScaling * ptd->GetPixWidth()  *0.5f;
  const FLOAT fHalfSizeJ = _fResolutionScaling*_fCustomScaling * ptd->GetPixHeight() *0.5f;
  // done
  _pDP->InitTexture( &toIcon);
  _pDP->AddTexture( fCenterI-fHalfSizeI, fCenterJ-fHalfSizeJ,
                    fCenterI+fHalfSizeI, fCenterJ+fHalfSizeJ, col|_ulAlphaHUD);
  _pDP->FlushRenderingQueue();
}

// draw text (or numbers, whatever)
static void HUD_DrawText( FLOAT fCenterX, FLOAT fCenterY, const CTString &strText,
                          COLOR colDefault, FLOAT fNormValue)
{
  // determine color
  COLOR col = colDefault;
  if( col==NONE) col = GetCurrentColor( fNormValue);
  // determine location
  PIX pixCenterI = (PIX)(fCenterX*_pixDPWidth  / 640.0f);
  PIX pixCenterJ = (PIX)(fCenterY*_pixDPHeight / (480.0f * _pDP->dp_fWideAdjustment));
  // done
  _pDP->SetTextScaling( _fResolutionScaling*_fCustomScaling);
  _pDP->PutTextCXY( strText, pixCenterI, pixCenterJ, col|_ulAlphaHUD);
}


// draw text (or numbers, whatever)
static void HUD_DrawTextSmall( FLOAT fCenterX, FLOAT fCenterY, const CTString &strText,
                          COLOR colDefault, FLOAT fNormValue)
{
  // determine color
  COLOR col = colDefault;
  if( col==NONE) col = GetCurrentColor( fNormValue);
  // determine location
  PIX pixCenterI = (PIX)(fCenterX*_pixDPWidth  / 640.0f);
  PIX pixCenterJ = (PIX)(fCenterY*_pixDPHeight / (480.0f * _pDP->dp_fWideAdjustment));
  // done
  _pDP->SetTextScaling( _fResolutionScaling*_fCustomScaling*0.7f);
  _pDP->PutTextCXY( strText, pixCenterI, pixCenterJ, col|_ulAlphaHUD);
}


// draw icon texture (if color = NONE, use colortransitions structure)
static void HUD_DrawIconSmall( FLOAT fCenterX, FLOAT fCenterY, CTextureObject &toIcon,
                          COLOR colDefault, FLOAT fNormValue, BOOL bBlink)
{
  // determine color
  COLOR col = colDefault;
  if( col==NONE) col = GetCurrentColor( fNormValue);
  // determine blinking state
  if( bBlink && fNormValue<=(_cttHUD.ctt_fLowMedium/2)) {
    // activate blinking only if value is <= half the low edge
    INDEX iCurrentTime = (INDEX)(_tmNow*4);
    if( iCurrentTime&1) col = C_vdGRAY;
  }
  // determine location
  const FLOAT fCenterI = fCenterX*_pixDPWidth  / 640.0f;
  const FLOAT fCenterJ = fCenterY*_pixDPHeight / (480.0f * _pDP->dp_fWideAdjustment);
  // determine dimensions
  CTextureData *ptd = (CTextureData*)toIcon.GetData();
  const FLOAT fHalfSizeI = _fResolutionScaling*_fCustomScaling * ptd->GetPixWidth()  *0.4f;
  const FLOAT fHalfSizeJ = _fResolutionScaling*_fCustomScaling * ptd->GetPixHeight() *0.4f;
  // done
  _pDP->InitTexture( &toIcon);
  _pDP->AddTexture( fCenterI-fHalfSizeI, fCenterJ-fHalfSizeJ,
                    fCenterI+fHalfSizeI, fCenterJ+fHalfSizeJ, col|_ulAlphaHUD);
  _pDP->FlushRenderingQueue();
}


// draw bar
static void HUD_DrawBar( FLOAT fCenterX, FLOAT fCenterY, PIX pixSizeX, PIX pixSizeY,
                         enum BarOrientations eBarOrientation, COLOR colDefault, FLOAT fNormValue)
{
  // determine color
  COLOR col = colDefault;
  if( col==NONE) col = GetCurrentColor( fNormValue);
  // determine location and size
  PIX pixCenterI = (PIX)(fCenterX*_pixDPWidth  / 640.0f);
  PIX pixCenterJ = (PIX)(fCenterY*_pixDPHeight / (480.0f * _pDP->dp_fWideAdjustment));
  PIX pixSizeI   = (PIX)(_fResolutionScaling*pixSizeX);
  PIX pixSizeJ   = (PIX)(_fResolutionScaling*pixSizeY);
  // fill bar background area
  PIX pixLeft  = pixCenterI-pixSizeI/2;
  PIX pixUpper = pixCenterJ-pixSizeJ/2;
  // determine bar position and inner size
  switch( eBarOrientation) {
  case BO_UP:
    pixSizeJ *= fNormValue;
    break;
  case BO_DOWN:
    pixUpper  = pixUpper + (PIX)ceil(pixSizeJ * (1.0f-fNormValue));
    pixSizeJ *= fNormValue;
    break;
  case BO_LEFT:
    pixSizeI *= fNormValue;
    break;
  case BO_RIGHT:
    pixLeft   = pixLeft + (PIX)ceil(pixSizeI * (1.0f-fNormValue));
    pixSizeI *= fNormValue;
    break;
  }
  // done
  _pDP->Fill( pixLeft, pixUpper, pixSizeI, pixSizeJ, col|_ulAlphaHUD);
}

static void DrawRotatedQuad( class CTextureObject *_pTO, FLOAT fX, FLOAT fY, FLOAT fSize, ANGLE aAngle, COLOR col)
{
  FLOAT fSinA = Sin(aAngle);
  FLOAT fCosA = Cos(aAngle);
  FLOAT fSinPCos = fCosA*fSize+fSinA*fSize;
  FLOAT fSinMCos = fSinA*fSize-fCosA*fSize;
  FLOAT fI0, fJ0, fI1, fJ1, fI2, fJ2, fI3, fJ3;

  fI0 = fX-fSinPCos;  fJ0 = fY-fSinMCos;
  fI1 = fX+fSinMCos;  fJ1 = fY-fSinPCos;
  fI2 = fX+fSinPCos;  fJ2 = fY+fSinMCos;
  fI3 = fX-fSinMCos;  fJ3 = fY+fSinPCos;
  
  _pDP->InitTexture( _pTO);
  _pDP->AddTexture( fI0, fJ0, 0, 0, col,   fI1, fJ1, 0, 1, col,
                    fI2, fJ2, 1, 1, col,   fI3, fJ3, 1, 0, col);
  _pDP->FlushRenderingQueue();  

}

static void DrawAspectCorrectTextureCentered( class CTextureObject *_pTO, FLOAT fX, FLOAT fY, FLOAT fWidth, COLOR col)
{
  CTextureData *ptd = (CTextureData*)_pTO->GetData();
  FLOAT fTexSizeI = ptd->GetPixWidth();
  FLOAT fTexSizeJ = ptd->GetPixHeight();
  FLOAT fHeight = fWidth*fTexSizeJ/fTexSizeJ;
  
  _pDP->InitTexture( _pTO);
  _pDP->AddTexture( fX-fWidth*0.5f, fY-fHeight*0.5f, fX+fWidth*0.5f, fY+fHeight*0.5f, 0, 0, 1, 1, col);
  _pDP->FlushRenderingQueue();
}

// draw sniper mask
static void HUD_DrawSniperMask( void )
{
  // determine location
  const FLOAT fSizeI = _pixDPWidth;
  const FLOAT fSizeJ = _pixDPHeight;
  const FLOAT fCenterI = fSizeI/2;  
  const FLOAT fCenterJ = fSizeJ/2; 

  COLOR colMask = C_WHITE|CT_OPAQUE;

  // crosshair lines
  _pDP->InitTexture( &_toSniperLine);
  _pDP->AddTexture( 0, fSizeJ/2-1, fSizeI, fSizeJ/2, 1.0f, 0.0f, 0.0f, 1.0f, C_WHITE|50); // horiz line
  _pDP->AddTexture( fSizeI/2-1, 0, fSizeI/2, fSizeJ, 1.0f, 0.0f, 0.0f, 1.0f, C_WHITE|50); // vert line
  _pDP->FlushRenderingQueue();

	// prepare for output of distance
  CTString strTmp;
  FLOAT _fYResolutionScaling = (FLOAT)_pixDPHeight/480.0f;
  FLOAT fDistance = _penWeapons->m_fRayHitDistance;

	// print it out
  if (_fResolutionScaling>=1.0f)
  {
    if (_fResolutionScaling<=1.3f) {
      _pDP->SetFont( _pfdConsoleFont);
      _pDP->SetTextAspect( 1.0f);
      _pDP->SetTextScaling(1.0f);
    } else {
      _pDP->SetFont( _pfdDisplayFont);
      _pDP->SetTextAspect( 1.0f);
      _pDP->SetTextScaling(0.7f*_fYResolutionScaling);
    } 
    if (fDistance>1500.0f) { strTmp.PrintF("---.-");           }
    else if (TRUE)         { strTmp.PrintF("%.1f", fDistance); }
    _pDP->PutTextC( strTmp, fCenterI, fCenterJ+50.0f*_fYResolutionScaling, C_RED|220);
  }
}

static void DrawPlayerRadarMask(void)
{
	// player view and absolute orientation
	ANGLE aAngle = ((CEntity&)*_penPlayer).GetPlacement().pl_OrientationAngle(1);
	CPlacement3D plView = ((CPlayer &)*_penPlayer).en_plViewpoint;
	aAngle += plView.pl_OrientationAngle(1);
	// draw the main mask
	DrawRotatedQuad(&_toPlayerRadar, _pixDPWidth*0.947f, _pixDPHeight*0.89f, 32.0f*(_pixDPWidth /640.0f), aAngle, C_WHITE|230);

	FLOAT fPosX;
	FLOAT fPosZ;
	FLOAT fD = 205.0f*(_pixDPWidth /640.0f);
	FLOAT fWidthOffset = _pixDPWidth*0.9475f;
	FLOAT fHeightOffset = _pixDPHeight*0.89f;
	FLOAT iTextureOffset = 3.0f*(_pixDPWidth /640.0f);
	CPlacement3D plN;
	CPlacement3D plE;
	CPlacement3D plS;
	CPlacement3D plW; 
  plN = plE = plS = plW = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
	plN.pl_PositionVector -= FLOAT3D(CosFast(aAngle)*fD, 0, SinFast(aAngle)*fD);
	plE.pl_PositionVector -= FLOAT3D(CosFast(aAngle+90)*fD, 0, SinFast(aAngle+90)*fD);
	plS.pl_PositionVector -= FLOAT3D(CosFast(aAngle+180)*fD, 0, SinFast(aAngle+180)*fD);
	plW.pl_PositionVector -= FLOAT3D(CosFast(aAngle+270)*fD, 0, SinFast(aAngle+270)*fD);
	
	// draw N
	fPosX = plN.pl_PositionVector(1)/8 + fWidthOffset; 
	fPosZ = plN.pl_PositionVector(3)/8 + fHeightOffset;	
	_pDP->InitTexture( &_toN);
  _pDP->AddTexture(fPosX-iTextureOffset, fPosZ-iTextureOffset, fPosX+iTextureOffset, fPosZ+iTextureOffset, C_WHITE|200);
	_pDP->FlushRenderingQueue();
	// draw E
	fPosX = plE.pl_PositionVector(1)/8 + fWidthOffset; 
	fPosZ = plE.pl_PositionVector(3)/8 + fHeightOffset;	
	_pDP->InitTexture( &_toE);
  _pDP->AddTexture(fPosX-iTextureOffset, fPosZ-iTextureOffset, fPosX+iTextureOffset, fPosZ+iTextureOffset, C_WHITE|200);
	_pDP->FlushRenderingQueue();
	// draw S
	fPosX = plS.pl_PositionVector(1)/8 + fWidthOffset; 
	fPosZ = plS.pl_PositionVector(3)/8 + fHeightOffset;	
	_pDP->InitTexture( &_toS);
  _pDP->AddTexture(fPosX-iTextureOffset, fPosZ-iTextureOffset, fPosX+iTextureOffset, fPosZ+iTextureOffset, C_WHITE|200);
	_pDP->FlushRenderingQueue();
	// draw W
	fPosX = plW.pl_PositionVector(1)/8 + fWidthOffset;  
	fPosZ = plW.pl_PositionVector(3)/8 + fHeightOffset;	
	_pDP->InitTexture( &_toW);
  _pDP->AddTexture(fPosX-iTextureOffset, fPosZ-iTextureOffset, fPosX+iTextureOffset, fPosZ+iTextureOffset, C_WHITE|200);
	_pDP->FlushRenderingQueue();
}

// helper functions

// fill weapon and ammo table with current state
static void FillWeaponAmmoTables(void)
{
  // ammo quantities
  _aaiAmmo[0].ai_iAmmoAmmount    = _penWeapons->m_iShells;
  _aaiAmmo[0].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxShells;
  _aaiAmmo[1].ai_iAmmoAmmount    = _penWeapons->m_iBullets;
  _aaiAmmo[1].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxBullets;
  _aaiAmmo[2].ai_iAmmoAmmount    = _penWeapons->m_iRockets;
  _aaiAmmo[2].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxRockets;
  _aaiAmmo[3].ai_iAmmoAmmount    = _penWeapons->m_iGrenades;
  _aaiAmmo[3].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxGrenades;
  _aaiAmmo[4].ai_iAmmoAmmount    = _penWeapons->m_iNapalm;
  _aaiAmmo[4].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxNapalm;
  _aaiAmmo[5].ai_iAmmoAmmount    = _penWeapons->m_iElectricity;
  _aaiAmmo[5].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxElectricity;
  _aaiAmmo[6].ai_iAmmoAmmount    = _penWeapons->m_iIronBalls;
  _aaiAmmo[6].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxIronBalls;
  _aaiAmmo[7].ai_iAmmoAmmount    = _penWeapons->m_iSniperBullets;
  _aaiAmmo[7].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxSniperBullets;
  _aaiAmmo[8].ai_iAmmoAmmount    = _penWeapons->m_iNukeBalls;
  _aaiAmmo[8].ai_iMaxAmmoAmmount = _penWeapons->m_iMaxNukeBalls;

  // prepare ammo table for weapon possesion
  INDEX i, iAvailableWeapons = _penWeapons->m_iAvailableWeapons;
  for( i=0; i<8; i++) _aaiAmmo[i].ai_bHasWeapon = FALSE;
  // weapon possesion
  for( i=WEAPON_NONE+1; i<WEAPON_LAST; i++)
  {
    if( _awiWeapons[i].wi_wtWeapon!=WEAPON_NONE)
    {
      // regular weapons
      _awiWeapons[i].wi_bHasWeapon = (iAvailableWeapons&(1<<(_awiWeapons[i].wi_wtWeapon-1)));
      if( _awiWeapons[i].wi_paiAmmo!=NULL) _awiWeapons[i].wi_paiAmmo->ai_bHasWeapon |= _awiWeapons[i].wi_bHasWeapon;
    }
  }
}

// Fragman's Display code
/*extern void DrawBars( const CPlayer *penPlayerCurrent, CDrawPort *pdpCurrent, CDisplay *pen, BOOL bBars){ 

	// no player - no info, sorry 
	if( penPlayerCurrent==NULL || (penPlayerCurrent->GetFlags()&ENF_DELETED)) return; 
	
	
	// find last values in case of predictor 
	CPlayer *penLast = (CPlayer*)penPlayerCurrent; 
	if( penPlayerCurrent->IsPredictor()) penLast = (CPlayer*)(((CPlayer*)penPlayerCurrent)->GetPredicted()); 
	ASSERT( penLast!=NULL); 
	if( penLast==NULL) return; // !!!! just in case 
	CTFileName fnIcon=pen->m_fnIcon; 
	CTextureObject toIcon; 
	if(fnIcon!=""){ 
		toIcon.SetData_t(fnIcon); 
		((CTextureData*)toIcon .GetData())->Force(TEX_CONSTANT); 
	} 
	// cache local variables 
	hud_fOpacity = 1;//Clamp( hud_fOpacity, 0.1f, 1.0f); 
	hud_fScaling = Clamp( hud_fScaling, 0.5f, 1.2f); 
	_penPlayer  = penPlayerCurrent; 
	_penWeapons = (CPlayerWeapons*)&*_penPlayer->m_penWeapons; 
	_pDP        = pdpCurrent; 
	_pixDPWidth   = _pDP->GetWidth(); 
	_pixDPHeight  = _pDP->GetHeight(); 
	_fCustomScaling     = 0.6f; 
	_fResolutionScaling = (FLOAT)_pixDPWidth /640.0f; 
	_colHUD     = 0x4C80BB00; 
	_colHUDText = SE_COL_ORANGE_LIGHT; 
	_ulAlphaHUD = NormFloatToByte(hud_fOpacity); 
	_tmNow = _pTimer->CurrentTick(); 
	
	// determine hud colorization; 
	COLOR colMax = SE_COL_BLUEGREEN_LT; 
	COLOR colTop = SE_COL_ORANGE_LIGHT; 
	COLOR colMid = LerpColor(colTop, C_RED, 0.5f); 
	
	// adjust borders color in case of spying mode 
	COLOR colBorder = _colHUD;  
	
	// prepare font and text dimensions 
	CTString strValue; 
	PIX pixCharWidth; 
	FLOAT fValue, fCol, fRow; 
	_pDP->SetFont( &_fdNumbersFont); 
	pixCharWidth = _fdNumbersFont.GetWidth() + _fdNumbersFont.GetCharSpacing() +1; 
	FLOAT fChrUnit = pixCharWidth * _fCustomScaling; 
	
	const PIX pixTopBound    = 6; 
	const PIX pixLeftBound   = 6; 
	const PIX pixBottomBound = (480 * _pDP->dp_fWideAdjustment) -pixTopBound; 
	const PIX pixRightBound  = 640-pixLeftBound; 
	FLOAT fOneUnit  = (32+0) * _fCustomScaling;  // unit size 
	FLOAT fAdvUnit  = (32+4) * _fCustomScaling;  // unit advancer 
	FLOAT fNextUnit = (32+8) * _fCustomScaling;  // unit advancer 
	FLOAT fHalfUnit = fOneUnit * 0.5f; 
	
	if(bBars){ 
		FLOAT fMax=pen->m_fMax; 
		fValue = Clamp( (FLOAT)abs(pen->m_fValue), 0.0f,fMax); 
		FLOAT fFrame=_toIHealthBar.GetCurrentAnimLength()*(fValue/fMax)*0.975; 
		_toIHealthBar.PlayAnim(0,AOF_LOOPING); 
		_toIHealthBar.OffsetPhase(-fFrame); 
		_fCustomScaling=0.6f/2.75f; 
		//draw animated bar part (the actual bar itsself) 
		if(fValue>0.0f){ 
			fRow = pixBottomBound*pen->m_fPosY +1; 
			fCol = pixRightBound*pen->m_fPosX   +fHalfUnit*5.5 -4; 
			HUD_DrawIcon(fCol,fRow, _toIHealthBar,C_WHITE,1.0f,FALSE); 
		}      
		strValue=""; 
		strValue.PrintF( "%d", (SLONG)ceil(fValue)); 
		//draw overlay bar 
		fRow = pixBottomBound*pen->m_fPosY; 
		fCol = pixRightBound*pen->m_fPosX+fHalfUnit*5.5; 
		HUD_DrawIcon( fCol, fRow, _toIHealth, C_WHITE, 1, false); 
		if(fnIcon!=""){ 
			//Draw Bar Icon 
			fRow = pixBottomBound*pen->m_fPosY; 
			fCol = pixRightBound*pen->m_fPosX-fOneUnit;      
			HUD_DrawIcon(fCol, fRow, toIcon,C_WHITE,1.0f,FALSE); 
		} 
		_fCustomScaling=0.6f; 
		HUD_DrawText( fCol, fRow, strValue, C_WHITE, 1); 
	}else{ 
		if(fnIcon!=""){ 
			//Draw Bar Icon 
			fRow = pixBottomBound*pen->m_fPosY; 
			fCol = pixRightBound*pen->m_fPosX-fOneUnit;      
			HUD_DrawIcon(fCol, fRow, toIcon,C_WHITE,1.0f,FALSE); 
		} 
		if(pen->m_eft==EFT_NORMAL){ 
			pdpCurrent->SetFont( _pfdDisplayFont); 
		}else if(pen->m_eft==EFT_XBOX){ 
			pdpCurrent->SetFont( &_fdNumbersFont); 
		}else if(pen->m_eft==EFT_CONSOLE){ 
			pdpCurrent->SetFont( _pfdConsoleFont); 
		} 
		pdpCurrent->SetTextScaling( pen->m_fScale); 
		pdpCurrent ->SetTextAspect( 1.0f); 
		fRow = pixBottomBound*pen->m_fPosY; 
		fCol = pixRightBound*pen->m_fPosX; 
		HUD_DrawText(fCol,fRow,pen->m_strParsed,C_WHITE,1); 
	}        
 }*/
// End of Fragman's Display code

//<<<<<<< DEBUG FUNCTIONS >>>>>>>

#ifdef ENTITY_DEBUG
CRationalEntity *DBG_prenStackOutputEntity = NULL;
#endif
void HUD_SetEntityForStackDisplay(CRationalEntity *pren)
{
#ifdef ENTITY_DEBUG
  DBG_prenStackOutputEntity = pren;
#endif
  return;
}

#ifdef ENTITY_DEBUG
static void HUD_DrawEntityStack()
{
  CTString strTemp;
  PIX pixFontHeight;
  ULONG pixTextBottom;

  if (tmp_ai[9]==12345)
  {
    if (DBG_prenStackOutputEntity!=NULL)
    {
      pixFontHeight = _pfdConsoleFont->fd_pixCharHeight;
      pixTextBottom = _pixDPHeight*0.83;
      _pDP->SetFont( _pfdConsoleFont);
      _pDP->SetTextScaling( 1.0f);
    
      INDEX ctStates = DBG_prenStackOutputEntity->en_stslStateStack.Count();
      strTemp.PrintF("-- stack of '%s'(%s)@%gs\n", DBG_prenStackOutputEntity->GetName(),
        DBG_prenStackOutputEntity->en_pecClass->ec_pdecDLLClass->dec_strName,
        _pTimer->CurrentTick());
      _pDP->PutText( strTemp, 1, pixTextBottom-pixFontHeight*(ctStates+1), _colHUD|_ulAlphaHUD);
      
      for(INDEX iState=ctStates-1; iState>=0; iState--) {
        SLONG slState = DBG_prenStackOutputEntity->en_stslStateStack[iState];
        strTemp.PrintF("0x%08x %s\n", slState, 
          DBG_prenStackOutputEntity->en_pecClass->ec_pdecDLLClass->HandlerNameForState(slState));
        _pDP->PutText( strTemp, 1, pixTextBottom-pixFontHeight*(iState+1), _colHUD|_ulAlphaHUD);
      }
    }
  }
}
#endif
//<<<<<<< DEBUG FUNCTIONS >>>>>>>

// main

// render interface (frontend) to drawport
// (units are in pixels for 640x480 resolution - for other res HUD will be scalled automatically)
extern void DrawHUD( const CPlayer *penPlayerCurrent, CDrawPort *pdpCurrent, BOOL bSnooping, const CPlayer *penPlayerOwner)
{
  // no player - no info, sorry
  if( penPlayerCurrent==NULL || (penPlayerCurrent->GetFlags()&ENF_DELETED)) return;
  
  // if snooping and owner player ins NULL, return
  if ( bSnooping && penPlayerOwner==NULL) return;

  // find last values in case of predictor
  CPlayer *penLast = (CPlayer*)penPlayerCurrent;
  if( penPlayerCurrent->IsPredictor()) penLast = (CPlayer*)(((CPlayer*)penPlayerCurrent)->GetPredicted());
  ASSERT( penLast!=NULL);
  if( penLast==NULL) return; // !!!! just in case

  // cache local variables
  hud_fOpacity = Clamp( hud_fOpacity, 0.1f, 1.0f);
  //0.5f = Clamp( 0.5f, 0.5f, 0.6f);
  _penPlayer  = penPlayerCurrent;
  _penWeapons = (CPlayerWeapons*)&*_penPlayer->m_penWeapons;
  _pDP        = pdpCurrent;
  _pixDPWidth   = _pDP->GetWidth();
  _pixDPHeight  = _pDP->GetHeight();
  _fCustomScaling     = 0.5f;
  _fResolutionScaling = (FLOAT)_pixDPWidth /640.0f;
//  _colHUD     = 0x4C80BB00;
  _colHUD     = 0x80808000;
  _colHUDText = SE_COL_ORANGE_LIGHT;
  _ulAlphaHUD = NormFloatToByte(hud_fOpacity);
  _tmNow = _pTimer->CurrentTick();

  // determine hud colorization;
  COLOR colMax = C_GRAY;
  COLOR colTop = SE_COL_ORANGE_LIGHT;
  COLOR colMid = LerpColor(colTop, C_RED, 0.5f);

  // adjust borders color in case of spying mode
  COLOR colBorder = _colHUD; 
  
  if( bSnooping) {
    colBorder = SE_COL_ORANGE_NEUTRAL;
    if( ((ULONG)(_tmNow*5))&1) {
      //colBorder = (colBorder>>1) & 0x7F7F7F00; // darken flash and scale
      colBorder = SE_COL_ORANGE_DARK;
      _fCustomScaling *= 0.933f;
    }
  }

  // draw sniper mask (original mask even if snooping)
  if (((CPlayerWeapons*)&*penPlayerOwner->m_penWeapons)->m_iCurrentWeapon!=WEAPON_CHAINSAW
    &&((CPlayerWeapons*)&*penPlayerOwner->m_penWeapons)->m_bSniping) {
    HUD_DrawSniperMask();
  } 
   
  // prepare font and text dimensions
  CTString strValue;
  PIX pixCharWidth;
  FLOAT fValue, fNormValue, fCol, fRow;
  _pDP->SetFont( &_fdNumbersFont);
  pixCharWidth = _fdNumbersFont.GetWidth() + _fdNumbersFont.GetCharSpacing() +1;
  FLOAT fChrUnit = pixCharWidth * _fCustomScaling;

  const PIX pixTopBound    = 6;
  const PIX pixLeftBound   = 6;
  const PIX pixBottomBound = (480 * _pDP->dp_fWideAdjustment) -pixTopBound;
  const PIX pixRightBound  = 640-pixLeftBound;
  FLOAT fOneUnit  = (32+0) * _fCustomScaling;  // unit size
  FLOAT fAdvUnit  = (32+4) * _fCustomScaling;  // unit advancer
  FLOAT fNextUnit = (32+8) * _fCustomScaling;  // unit advancer
  FLOAT fHalfUnit = fOneUnit * 0.5f;
  FLOAT fMoverX, fMoverY;
  COLOR colDefault;

   // prepare and draw health info
  fRow = pixBottomBound-4;
  fCol = pixLeftBound+8;

	FLOAT fMultiplier = 0.5f;
	if (GetSP()->sp_bUsePEHealth) {
		fMultiplier = 1.0f;
	}
  fValue = ClampDn( _penPlayer->GetHealth(), 0.0f);  // never show negative health
  fNormValue = fValue/TOP_HEALTH;
  strValue.PrintF( "%d", (SLONG)ceil(fValue));
  HUD_DrawIcon( fCol, fRow, _toHealth, C_RED|255, fNormValue, TRUE);
  if ( fValue <= 100.5f*fMultiplier ) {
	  colDefault = C_RED|255;
  } else if ( fValue <= 200.5f*fMultiplier ) {
	  colDefault = C_YELLOW|255;
  } else {
	  colDefault = C_GREEN|255;
  }
  HUD_DrawText( fCol+30, fRow, strValue, colDefault, fNormValue);

	// prepare and draw armor info
	fValue = _penPlayer->m_fArmor;
  fValue = ClampDn( _penPlayer->m_fArmor, 0.0f);  // never show negative armor
	fNormValue = fValue/TOP_ARMOR;
	strValue.PrintF( "%d", (SLONG)ceil(fValue));
	if (fValue<=100.5f*fMultiplier) {
		if (fValue<=0.0f) {
			colDefault = C_dGRAY|255;
		} else {
			colDefault = C_RED|255;
		}
		HUD_DrawIcon( fCol+70, fRow, _toArmorSmall, C_WHITE|255, fNormValue, FALSE);
	} else if (fValue<=200.5f*fMultiplier) {
		colDefault = C_YELLOW;
		HUD_DrawIcon( fCol+70, fRow, _toArmorMedium, C_WHITE|255, fNormValue, FALSE);
	} else {
		colDefault = C_GREEN;
		HUD_DrawIcon( fCol+70, fRow, _toArmorLarge, C_WHITE|255, fNormValue, FALSE);
	}
	HUD_DrawText( fCol+100, fRow, strValue, colDefault, fNormValue);

  // prepare output strings and formats depending on game type
	// taken out, redundent info
  INDEX iFrags  = _penPlayer->m_psGameStats.ps_iKills;
  INDEX iDeaths = _penPlayer->m_psGameStats.ps_iDeaths;
  INDEX iLevelFrags  = _penPlayer->m_psLevelStats.ps_iKills;

	FLOAT fRow2 = pixTopBound+10;
	FLOAT fCol2 = pixLeftBound+220;

	// prepare and draw kills info 
	strValue.PrintF( "%d", iLevelFrags);
	//HUD_DrawIconSmall(fCol2, fRow2, _toFrags, C_WHITE|255, 1.0f, FALSE);
	HUD_DrawTextSmall(fCol2+46, fRow2, strValue, C_WHITE|255, 1.0f);

	strValue.PrintF( "%d", iFrags);
	//HUD_DrawIconSmall(fCol2+60, fRow2, _toFrags, C_WHITE|255, 1.0f, FALSE);
	HUD_DrawTextSmall(fCol2+100, fRow2, strValue, C_WHITE|255, 1.0f);

	// prepare and draw deaths info
  strValue.PrintF( "%d", iDeaths);
  //HUD_DrawIconSmall(fCol2+130, fRow2, _toDeaths, C_WHITE|255, 1.0f, FALSE);
  HUD_DrawTextSmall(fCol2+148, fRow2, strValue, C_WHITE|255, fNormValue);

  // setup font
  _pfdDisplayFont->SetVariableWidth();
  _pDP->SetFont( _pfdDisplayFont);
  _pDP->SetTextScaling( (_fResolutionScaling+1)*0.35f);
  CTString strDes = "Level Kills     Total Kills     Deaths";
  _pDP->PutTextC( strDes, _pixDPWidth*0.5f, _pixDPHeight*0.005f, C_RED|CT_OPAQUE);

	// prepare and draw score info if desired
	/*if (hud_bShowScore) {
		// prepare and draw score
		strValue.PrintF( "%d", iScore);
		fRow = pixTopBound  +8;
		fCol = pixLeftBound+250;
		HUD_DrawIcon(fCol,    fRow, _toScore, C_dGREEN|255, 1.0f, FALSE);
		HUD_DrawText(fCol+70, fRow, strValue, C_mlGRAY|255, 1.0f);
	}*/

  // prepare and draw ammo and weapon info
  CTextureObject *ptoCurrentAmmo=NULL, *ptoCurrentWeapon=NULL, *ptoWantedWeapon=NULL;
  INDEX iCurrentWeapon = _penWeapons->m_iCurrentWeapon;
  INDEX iWantedWeapon  = _penWeapons->m_iWantedWeapon;
  // determine corresponding ammo and weapon texture component
  ptoCurrentWeapon = _awiWeapons[iCurrentWeapon].wi_ptoWeapon;
  ptoWantedWeapon  = _awiWeapons[iWantedWeapon].wi_ptoWeapon;

  AmmoInfo *paiCurrent = _awiWeapons[iCurrentWeapon].wi_paiAmmo;
  if( paiCurrent!=NULL) ptoCurrentAmmo = paiCurrent->ai_ptoAmmo;

  // draw complete weapon info if knife isn't current weapon
  if( ptoCurrentAmmo!=NULL && !GetSP()->sp_bInfiniteAmmo) {
    // determine ammo quantities
    FLOAT fMaxValue = _penWeapons->GetMaxAmmo();
    fValue = _penWeapons->GetAmmo();
    fNormValue = fValue / fMaxValue;
    strValue.PrintF( "%d", (SLONG)ceil(fValue));
    PrepareColorTransitions( colMax, colTop, colMid, C_RED, 0.30f, 0.15f, FALSE);
    BOOL bDrawAmmoIcon = _fCustomScaling<=1.0f;
    // draw ammo, value and weapon
    FLOAT fRow1 = pixBottomBound-35;
    FLOAT fCol1 = 275;
    colDefault = AddShaker( 4, fValue, penLast->m_iLastAmmo, penLast->m_tmAmmoChanged, fMoverX, fMoverY);
    HUD_DrawBorder( fCol1+fMoverX, fRow1+fMoverY, fOneUnit, fOneUnit, colBorder);
    fCol1 += fAdvUnit+fChrUnit*2.5f -fHalfUnit;
    HUD_DrawBorder( fCol1, fRow1, fChrUnit*5, fOneUnit, colBorder);
    if( bDrawAmmoIcon) {
      fCol1 += fAdvUnit+fChrUnit*2.5f -fHalfUnit;
      HUD_DrawBorder( fCol1, fRow1, fOneUnit, fOneUnit, colBorder);
      HUD_DrawIcon( fCol1, fRow1, *ptoCurrentAmmo, C_WHITE /*_colHUD*/, fNormValue, TRUE);
      fCol1 -= fAdvUnit+fChrUnit*2.5f -fHalfUnit;
    }
		_pDP->SetFont( &_fdNumbersFont);
		_pDP->SetTextScaling( (_fResolutionScaling+1)*0.5f);
    HUD_DrawText( fCol1, fRow1, strValue, NONE, fNormValue);
    fCol1 -= fAdvUnit+fChrUnit*2.5f -fHalfUnit;
    HUD_DrawIcon( fCol1+fMoverX, fRow1+fMoverY, *ptoCurrentWeapon, C_WHITE /*_colHUD*/, fNormValue, !bDrawAmmoIcon);
  } else if( ptoCurrentWeapon!=NULL) {
    // draw only knife or colt icons (ammo is irrelevant)
/*    fRow = pixBottomBound-fHalfUnit;
    fCol = 205 + fHalfUnit;
    HUD_DrawBorder( fCol, fRow, fOneUnit, fOneUnit, colBorder);
    HUD_DrawIcon(   fCol, fRow, *ptoCurrentWeapon, C_WHITE, fNormValue, FALSE);*/
  }


  // display all ammo infos
  INDEX i;
  FLOAT fAdv;
  COLOR colIcon, colBar;
  //PrepareColorTransitions( colMax, colTop, colMid, C_RED, 0.5f, 0.25f, FALSE);
  PrepareColorTransitions( C_GREEN, C_GREEN, colMid, C_RED, 0.5f, 0.25f, FALSE);
  // reduce the size of icon slightly
  _fCustomScaling = ClampDn( _fCustomScaling*0.8f, 0.5f);
  const FLOAT fOneUnitS  = fOneUnit  *0.8f;
  const FLOAT fAdvUnitS  = fAdvUnit  *0.8f;
  const FLOAT fNextUnitS = fNextUnit *0.8f;
  const FLOAT fHalfUnitS = fHalfUnit *0.8f;

  // prepare postition and ammo quantities
  fRow = pixBottomBound-27;
  fCol = pixLeftBound + 8;
  const FLOAT fBarPos = fHalfUnitS*0.7f;
  FillWeaponAmmoTables();

  FLOAT fBombCount = penPlayerCurrent->m_iSeriousBombCount;
  BOOL  bBombFiring = FALSE;
  // draw serious bomb
#define BOMB_FIRE_TIME 1.5f
  if (penPlayerCurrent->m_tmSeriousBombFired+BOMB_FIRE_TIME>_pTimer->GetLerpedCurrentTick()) {
    fBombCount++;
    //if (fBombCount>3) { fBombCount = 3; }
    bBombFiring = TRUE;
  }
  if (fBombCount>0) {
    fNormValue = (FLOAT) fBombCount / 5.0f;
    if (fNormValue>1) { fNormValue = 1.0f; }
    COLOR colBombBorder = _colHUD;
    COLOR colBombIcon = C_WHITE;
    COLOR colBombBar = C_YELLOW; 
    if (fBombCount<3) { colBombBar = C_RED; }
    if (fBombCount>5) { colBombBar = C_GREEN; }
    if (bBombFiring) { 
      FLOAT fFactor = (_pTimer->GetLerpedCurrentTick() - penPlayerCurrent->m_tmSeriousBombFired)/BOMB_FIRE_TIME;
      colBombBorder = LerpColor(colBombBorder, C_RED, fFactor);
      colBombIcon = LerpColor(colBombIcon, C_RED, fFactor);
      colBombBar = LerpColor(colBombBar, C_RED, fFactor);
    }
    //HUD_DrawBorder( fCol +fHalfUnitS,         fRow-40, fOneUnitS, fOneUnitS, colBombBorder);
    HUD_DrawIcon(   fCol,         fRow, _toASeriousBomb, colBombIcon, fNormValue, FALSE);
    HUD_DrawBar(    fCol+fBarPos, fRow, fOneUnitS/5, fOneUnitS-2, BO_DOWN, colBombBar, fNormValue);
    // advance to next position
    fRow -= 22;
  }

  // draw powerup(s) if needed
  PrepareColorTransitions( C_GREEN, C_GREEN, colMid, C_RED, 0.5f, 0.25f, FALSE);
  //static void PrepareColorTransitions( COLOR colFine, COLOR colHigh, COLOR colMedium, COLOR colLow,
  //                                   FLOAT fMediumHigh, FLOAT fLowMedium, BOOL bSmooth)
  TIME *ptmPowerups = (TIME*)&_penPlayer->m_tmInvisibility;
  TIME *ptmPowerupsMax = (TIME*)&_penPlayer->m_tmInvisibilityMax;
  for( i=0; i<MAX_POWERUPS; i++)
  {
    // skip if not active
    const TIME tmDelta = ptmPowerups[i] - _tmNow;
    if( tmDelta<=0) continue;
    fNormValue = tmDelta / 360.0f;
    if (fNormValue>1) { fNormValue = 1.0f; }
    // draw icon and a little bar
    //HUD_DrawBorder( fCol,         fRow, fOneUnitS, fOneUnitS, colBorder);
    HUD_DrawIcon(   fCol,         fRow, _atoPowerups[i], C_WHITE /*_colHUD*/, fNormValue, TRUE);
    HUD_DrawBar(    fCol+fBarPos, fRow, fOneUnitS/5, fOneUnitS-2, BO_DOWN, NONE, fNormValue);
    // play sound if icon is flashing
    if(fNormValue<=(_cttHUD.ctt_fLowMedium/6)) {
      // activate blinking only if value is <= half the low edge
      INDEX iLastTime = (INDEX)(_tmLast*4);
      INDEX iCurrentTime = (INDEX)(_tmNow*4);
      if(iCurrentTime&1 & !(iLastTime&1)) {
        ((CPlayer *)penPlayerCurrent)->PlayPowerUpSound();
      }
    }
    // advance to next position
    fRow -= 22;
  }

  // loop thru all ammo types
  if (!GetSP()->sp_bInfiniteAmmo) {
	  FLOAT fRow2 = pixBottomBound-4;
	  FLOAT fCol2 = pixLeftBound+223;
    for( INDEX ii=8; ii>=0; ii--) {
      i = aiAmmoRemap[ii];
      // if no ammo and hasn't got that weapon - just skip this ammo
      AmmoInfo &ai = _aaiAmmo[i];
      ASSERT( ai.ai_iAmmoAmmount>=0);
      if( ai.ai_iAmmoAmmount==0 && !ai.ai_bHasWeapon) continue;
      // display ammo info
      colIcon = C_WHITE /*_colHUD*/;
      fCol2 += 18;
      if( ai.ai_iAmmoAmmount==0) colIcon = C_mdGRAY;
      if( ptoCurrentAmmo == ai.ai_ptoAmmo) colIcon = C_WHITE; 
      fNormValue = (FLOAT)ai.ai_iAmmoAmmount / ai.ai_iMaxAmmoAmmount;
      colBar = AddShaker( 4, ai.ai_iAmmoAmmount, ai.ai_iLastAmmoAmmount, ai.ai_tmAmmoChanged, fMoverX, fMoverY);
      HUD_DrawBorder( fCol2,         fRow2, fOneUnitS, fOneUnitS, colBorder);
      HUD_DrawIcon(   fCol2,         fRow2, *_aaiAmmo[i].ai_ptoAmmo, colIcon, fNormValue, FALSE);
      HUD_DrawBar(    fCol2+fBarPos, fRow2, fOneUnitS/5, fOneUnitS-2, BO_DOWN, colBar, fNormValue);
      // advance to next position
      fCol -= fAdvUnitS;  
    }
  }

  // if weapon change is in progress
  _fCustomScaling = 0.5f;
  hud_tmWeaponsOnScreen = Clamp( hud_tmWeaponsOnScreen, 0.0f, 10.0f);   
  if( (_tmNow - _penWeapons->m_tmWeaponChangeRequired) < hud_tmWeaponsOnScreen) {
    // determine number of weapons that player has
    INDEX ctWeapons = 0;
    for( i=WEAPON_NONE+1; i<WEAPON_LAST; i++) {
      if( _awiWeapons[i].wi_wtWeapon!=WEAPON_NONE && _awiWeapons[i].wi_wtWeapon!=WEAPON_DOUBLECOLT &&
          _awiWeapons[i].wi_bHasWeapon) ctWeapons++;
    }
    // display all available weapons
    fRow = pixBottomBound - fHalfUnit - 3*fNextUnit;
    fCol = 320.0f - (ctWeapons*fAdvUnit-fHalfUnit)/2.0f;
    // display all available weapons
    for( INDEX ii=WEAPON_NONE+1; ii<WEAPON_LAST; ii++) {
      i = aiWeaponsRemap[ii];
      // skip if hasn't got this weapon
      if( _awiWeapons[i].wi_wtWeapon==WEAPON_NONE || _awiWeapons[i].wi_wtWeapon==WEAPON_DOUBLECOLT
         || !_awiWeapons[i].wi_bHasWeapon) continue;
      // display weapon icon
      COLOR colBorder = C_GRAY|255;
      colIcon = 0xccddff00;
      // weapon that is currently selected has different colors
      if( ptoWantedWeapon == _awiWeapons[i].wi_ptoWeapon) {
        colIcon = 0xffcc0000;
        colBorder = 0xffcc0000;
      }
      // no ammo
      if( _awiWeapons[i].wi_paiAmmo!=NULL && _awiWeapons[i].wi_paiAmmo->ai_iAmmoAmmount==0) {
        HUD_DrawBorder( fCol, fRow, fOneUnit, fOneUnit, 0x22334400);
        HUD_DrawIcon(   fCol, fRow, *_awiWeapons[i].wi_ptoWeapon, 0x22334400, 1.0f, FALSE);
      // yes ammo
      } else {
        HUD_DrawBorder( fCol, fRow, fOneUnit, fOneUnit, colBorder);
        HUD_DrawIcon(   fCol, fRow, *_awiWeapons[i].wi_ptoWeapon, colIcon, 1.0f, FALSE);
      }
      // advance to next position
      fCol += fAdvUnit;
    }
  }


  // reduce icon sizes a bit
  const FLOAT fUpperSize = ClampDn(_fCustomScaling*0.5f, 0.5f)/_fCustomScaling;
  _fCustomScaling*=fUpperSize;
  ASSERT( _fCustomScaling>=0.5f);
  fChrUnit  *= fUpperSize;
  fOneUnit  *= fUpperSize;
  fHalfUnit *= fUpperSize;
  fAdvUnit  *= fUpperSize;
  fNextUnit *= fUpperSize;

  // draw oxygen info if needed
  BOOL bOxygenOnScreen = FALSE;
  fValue = _penPlayer->en_tmMaxHoldBreath - (_pTimer->CurrentTick() - _penPlayer->en_tmLastBreathed);
  if( _penPlayer->IsConnected() && (_penPlayer->GetFlags()&ENF_ALIVE) && fValue<30.0f) { 
    // prepare and draw oxygen info
    fRow = pixTopBound + fOneUnit + fNextUnit;
    fCol = 280.0f;
    fAdv = fAdvUnit + fOneUnit*4/2 - fHalfUnit;
    PrepareColorTransitions( colMax, colTop, colMid, C_RED, 0.5f, 0.25f, FALSE);
    fNormValue = fValue/30.0f;
    fNormValue = ClampDn(fNormValue, 0.0f);
    HUD_DrawBorder( fCol,      fRow, fOneUnit,         fOneUnit, colBorder);
    HUD_DrawBorder( fCol+fAdv, fRow, fOneUnit*4,       fOneUnit, colBorder);
    HUD_DrawBar(    fCol+fAdv, fRow, fOneUnit*4*0.975, fOneUnit*0.9375, BO_LEFT, NONE, fNormValue);
    HUD_DrawIcon(   fCol,      fRow, _toOxygen, C_WHITE /*_colHUD*/, fNormValue, TRUE);
    bOxygenOnScreen = TRUE;
  }

  // draw boss energy if needed
  if( _penPlayer->m_penMainMusicHolder!=NULL) {
    CMusicHolder &mh = (CMusicHolder&)*_penPlayer->m_penMainMusicHolder;
    fNormValue = 0;

    if( mh.m_penBoss!=NULL && (mh.m_penBoss->en_ulFlags&ENF_ALIVE)) {
      CEnemyBase &eb = (CEnemyBase&)*mh.m_penBoss;
      ASSERT( eb.m_fMaxHealth>0);
      fValue = eb.GetHealth();
      fNormValue = fValue/eb.m_fMaxHealth;
    }
    if( mh.m_penCounter!=NULL) {
      CEnemyCounter &ec = (CEnemyCounter&)*mh.m_penCounter;
      if (ec.m_iCount>0) {
        fValue = ec.m_iCount;
        fNormValue = fValue/ec.m_iCountFrom;
      }
    }
    if( fNormValue>0) {
      // prepare and draw boss energy info
      //PrepareColorTransitions( colMax, colTop, colMid, C_RED, 0.5f, 0.25f, FALSE);
      PrepareColorTransitions( colMax, colMax, colTop, C_RED, 0.5f, 0.25f, FALSE);
      
      fRow = pixTopBound + fOneUnit + fNextUnit;
      fCol = 184.0f;
      fAdv = fAdvUnit+ fOneUnit*16/2 -fHalfUnit;
      if( bOxygenOnScreen) fRow += fNextUnit;
      HUD_DrawBorder( fCol,      fRow, fOneUnit,          fOneUnit, colBorder);
      HUD_DrawBorder( fCol+fAdv, fRow, fOneUnit*16,       fOneUnit, colBorder);
      HUD_DrawBar(    fCol+fAdv, fRow, fOneUnit*16*0.995, fOneUnit*0.9375, BO_LEFT, NONE, fNormValue);
      HUD_DrawIcon(   fCol,      fRow, _toHealth, C_WHITE /*_colHUD*/, fNormValue, FALSE);
    }
  }


  // determine scaling of normal text and play mode
  const FLOAT fTextScale  = (_fResolutionScaling+1) *0.5f;
  const BOOL bSinglePlay  =  GetSP()->sp_bSinglePlayer;
  const BOOL bCooperative =  GetSP()->sp_bCooperative && !bSinglePlay;
  const BOOL bScoreMatch  = !GetSP()->sp_bCooperative && !GetSP()->sp_bUseFrags;
  const BOOL bFragMatch   = !GetSP()->sp_bCooperative &&  GetSP()->sp_bUseFrags;
  COLOR colMana, colFrags, colDeaths, colScore;
  INDEX iScoreSum = 0;

  // if not in single player mode, we'll have to calc (and maybe printout) other players' info
  if( !bSinglePlay)
  {
    // set font and prepare font parameters
    _pfdDisplayFont->SetVariableWidth();
    _pDP->SetFont( _pfdDisplayFont);
    _pDP->SetTextScaling( fTextScale*0.75f);
    FLOAT fCharHeight = (_pfdDisplayFont->GetHeight())*fTextScale*0.8f;
    // generate and sort by mana list of active players
    BOOL bMaxScore=TRUE, bMaxMana=TRUE, bMaxFrags=TRUE, bMaxDeaths=TRUE;
    hud_iSortPlayers = Clamp( hud_iSortPlayers, -1L, 6L);
    SortKeys eKey = (SortKeys)hud_iSortPlayers;
    if (hud_iSortPlayers==-1) {
//      eKey = PSK_FRAGS;
			eKey = PSK_LVL_FRAGS;
    }
    INDEX iPlayers = SetAllPlayersStats(eKey);
		INDEX j = 0;
    // loop thru players 
    for( INDEX i=0; i<iPlayers; i++)
    { // get player name and mana
      CPlayer *penPlayer = _apenPlayers[i];

      colScore = C_WHITE;
      const CTString strName = penPlayer->GetPlayerName();
      //const CTString strNameU = penPlayer->GetPlayerName().Undecorated();
      const INDEX iHealth = ClampDn( (INDEX)ceil( penPlayer->GetHealth()), 0L);
      const INDEX iScore  = penPlayer->m_psGameStats.ps_iScore;
      const INDEX iMana   = penPlayer->m_iMana;
      const INDEX iFrags  = penPlayer->m_psGameStats.ps_iKills;
      const INDEX iDeaths = penPlayer->m_psGameStats.ps_iDeaths;
      const INDEX iLevelFrags  = penPlayer->m_psLevelStats.ps_iKills;
      CTString strScore, strMana, strFrags, strDeaths, strLvlFrags;
      strScore.PrintF(  "%d", iScore);
      strMana.PrintF(   "%d", iMana);
      strFrags.PrintF(  "%d", iFrags); 
      strDeaths.PrintF( "%d", iDeaths);
      strLvlFrags.PrintF(  "%d", iLevelFrags);
      // detemine corresponding colors
      colMana = colScore = colFrags = colDeaths = C_WHITE;
			if( i==0)  colMana = colFrags = colDeaths = C_BLUE; // current leader
      if( penPlayer==penPlayerCurrent) colMana = colFrags = colDeaths = C_RED; // current player
      if( iHealth<=0) colMana = colFrags = colDeaths = C_dGRAY; // darken if dead
      // eventually print it out
      if( hud_iShowPlayers==1 || hud_iShowPlayers==-1 && !bSinglePlay) {
        // printout location and info aren't the same for deathmatch and coop play
        const FLOAT fCharWidth = (PIX)((_pfdDisplayFont->GetWidth()-1) *fTextScale);
        if( bScoreMatch) {
					_pDP->PutTextR( strName+" :", _pixDPWidth-8*fCharWidth,			fCharHeight*i+6, colScore |255);
          _pDP->PutTextC( strScore,     _pixDPWidth-6*fCharWidth,			fCharHeight*i+6, colMana  |255);
          _pDP->PutText(  "/",          _pixDPWidth-4*fCharWidth,			fCharHeight*i+6, C_vlGRAY |255);
          _pDP->PutTextC( strMana,      _pixDPWidth-2*fCharWidth,			fCharHeight*i+6, colMana  |255);
        } else {
					_pDP->PutTextR( strName+" :", _pixDPWidth-10*fCharWidth,		fCharHeight*i+6, colScore |255);
          _pDP->PutTextC( strLvlFrags,  _pixDPWidth-8.5f*fCharWidth,	fCharHeight*i+6, colDeaths|255);
          _pDP->PutText(  "/",          _pixDPWidth-7*fCharWidth,			fCharHeight*i+6, _colHUD  |255);
          _pDP->PutTextC( strFrags,     _pixDPWidth-4.75f*fCharWidth, fCharHeight*i+6, colFrags |255);
          _pDP->PutText(  "/",          _pixDPWidth-3*fCharWidth,			fCharHeight*i+6, _colHUD  |255);
          _pDP->PutTextC( strDeaths,    _pixDPWidth-1.5f*fCharWidth,	fCharHeight*i+6, colFrags |255);
        }
      }
    }

    // draw remaining time if time based death- or scorematch
    if ((bScoreMatch || bFragMatch) && hud_bShowMatchInfo){
      CTString strLimitsInfo="";  
      if (GetSP()->sp_iTimeLimit>0) {
        FLOAT fTimeLeft = ClampDn(GetSP()->sp_iTimeLimit*60.0f - _pNetwork->GetGameTime(), 0.0f);
        strLimitsInfo.PrintF("%s^cFFFFFF%s: %s\n", strLimitsInfo, TRANS("TIME LEFT"), TimeToString(fTimeLeft));
      }
      extern INDEX SetAllPlayersStats( INDEX iSortKey);
      // fill players table
      const INDEX ctPlayers = SetAllPlayersStats(bFragMatch?5:3); // sort by frags or by score
      // find maximum frags/score that one player has
      INDEX iMaxFrags = LowerLimit(INDEX(0));
      INDEX iMaxScore = LowerLimit(INDEX(0));
      {for(INDEX iPlayer=0; iPlayer<ctPlayers; iPlayer++) {
        CPlayer *penPlayer = _apenPlayers[iPlayer];
        iMaxFrags = Max(iMaxFrags, penPlayer->m_psLevelStats.ps_iKills);
        iMaxScore = Max(iMaxScore, penPlayer->m_psLevelStats.ps_iScore);
      }}
      if (GetSP()->sp_iFragLimit>0) {
        INDEX iFragsLeft = ClampDn(GetSP()->sp_iFragLimit-iMaxFrags, INDEX(0));
        strLimitsInfo.PrintF("%s^cFFFFFF%s: %d\n", strLimitsInfo, TRANS("FRAGS LEFT"), iFragsLeft);
      }
      if (GetSP()->sp_iScoreLimit>0) {
        INDEX iScoreLeft = ClampDn(GetSP()->sp_iScoreLimit-iMaxScore, INDEX(0));
        strLimitsInfo.PrintF("%s^cFFFFFF%s: %d\n", strLimitsInfo, TRANS("SCORE LEFT"), iScoreLeft);
      }
      _pfdDisplayFont->SetFixedWidth();
      _pDP->SetFont( _pfdDisplayFont);
      _pDP->SetTextScaling( fTextScale*0.8f );
      _pDP->SetTextCharSpacing( -2.0f*fTextScale);
      _pDP->PutText( strLimitsInfo, 5.0f*_pixDPWidth/640.0f, 10.0f*_pixDPWidth/640.0f, C_WHITE|CT_OPAQUE);
    }

    // prepare color for local player printouts
    bMaxScore  ? colScore  = C_WHITE : colScore  = C_lGRAY;
    bMaxMana   ? colMana   = C_WHITE : colMana   = C_lGRAY;
    bMaxFrags  ? colFrags  = C_WHITE : colFrags  = C_lGRAY;
    bMaxDeaths ? colDeaths = C_WHITE : colDeaths = C_lGRAY;
  }

  // printout player latency if needed
  if( hud_bShowLatency) {
    CTString strLatency;
    strLatency.PrintF( "%4.0f", _penPlayer->m_tmLatency*1000.0f);
    _pDP->SetFont( _pfdDisplayFont);
    _pDP->SetTextScaling( fTextScale);
    _pDP->PutTextR( strLatency, _pixDPWidth *0.912f, _pixDPHeight*0.974f, C_dGREEN|255);
		strLatency.PrintF( "ms");
    _pDP->SetTextScaling( fTextScale*0.85f);
    _pDP->PutTextR( strLatency, _pixDPWidth *0.935f, _pixDPHeight*0.978f, C_dGREEN|255);
  }

  // restore font defaults
  _pfdDisplayFont->SetVariableWidth();
  _pDP->SetFont( &_fdNumbersFont);
  _pDP->SetTextCharSpacing(1);

  /*if (hud_bShowPEInfo) {
    // setup font
    _pDP->SetTextScaling( fTextScale*0.7f );
    _pDP->SetTextCharSpacing( fTextScale*0.7f );
    // define strings for output
    CTString strPEInfo;
    strPEInfo.PrintF( "Activation Events Sent: %d\n", ol_ctEvents);
    _pDP->PutText( strPEInfo, _pixDPWidth*0.01f, _pixDPHeight*0.77f, C_WHITE|CT_OPAQUE);
    strPEInfo.PrintF( "Activation Event Count: %d", ol_iEventCount);
    _pDP->PutText( strPEInfo, _pixDPWidth*0.01f, _pixDPHeight*0.80f, C_WHITE|CT_OPAQUE);
  }

  if (hud_bShowPEInfo) {
    // define strings for output
    CTString PEInfo, strLeader, strPlayer;
    // setup font
    _pfdDisplayFont->SetVariableWidth();
    _pDP->SetFont( _pfdDisplayFont);
    _pDP->SetTextScaling( fTextScale*0.8f );
    _pDP->SetTextCharSpacing( fTextScale*0.8f );

    INDEX ctEnemiesAlive, ctSpawnersActive,ctSpawnersInQueue;
		ctEnemiesAlive = ctSpawnersActive = ctSpawnersInQueue = 0;
		FLOAT fLag, fSpawnersMaxAge;
		fLag = 0.0f;
		fSpawnersMaxAge = 0.0f;
		INDEX iAlpha = 255;
    // scan the level's entities and if the entity is an Player, pen the bastard so we can trace his ass
    {FOREACHINDYNAMICCONTAINER(((CEntity&)*_penPlayer).GetWorld()->wo_cenEntities, CEntity, iten) { 
      CEntity *pen = iten;
      if (IsDerivedFromClass(pen, "Enemy Base")) { 
        CEnemyBase *penEnemy = (CEnemyBase *)pen;
        if (penEnemy->m_bHasBeenSpawned) {
          ctEnemiesAlive++;
					fLag += penEnemy->m_fLagger;
        }
			}
      if (IsOfClass(pen, "Enemy Spawner")) { 
        CEnemySpawner *penSpawner = (CEnemySpawner *)pen;
        if (penSpawner->m_bIsActive) {
          ctSpawnersActive++;
					FLOAT fAge = _pTimer->CurrentTick() - penSpawner->m_tmTriggered;
					if (fAge>fSpawnersMaxAge) {
						fSpawnersMaxAge = fAge;
					}
        }
			}
		}}

    // Headers
    PEInfo.PrintF(  "^c9999ff Enemies");
    _pDP->PutText( PEInfo, _pixDPWidth*0.01f, _pixDPHeight*0.39f, C_RED|iAlpha);

    PEInfo.PrintF(  "^c9900ff Spawners");
    _pDP->PutText( PEInfo, _pixDPWidth*0.01f, _pixDPHeight*0.47f, C_RED|iAlpha);

    //PEInfo.PrintF(  "^c99ffff Players");
    //_pDP->PutText( PEInfo, _pixDPWidth*0.01f, _pixDPHeight*0.59f, C_WHITE|iAlpha);

    // resize font
    _pDP->SetTextScaling( fTextScale*0.7f );
    _pDP->SetTextCharSpacing( fTextScale*0.7f );
    
    // INFO
    // enemies
     PEInfo.PrintF(  "Alive: %d", ctEnemiesAlive);
    _pDP->PutText( PEInfo, _pixDPWidth*0.01f, _pixDPHeight*0.42f, C_WHITE|iAlpha);

     PEInfo.PrintF(  "Lag Factor: %.2f", fLag);
    _pDP->PutText( PEInfo, _pixDPWidth*0.01f, _pixDPHeight*0.44f, C_WHITE|iAlpha);

    // spawners
    PEInfo.PrintF(  "Total Active: %d", ctSpawnersActive);
    _pDP->PutText( PEInfo, _pixDPWidth*0.01f, _pixDPHeight*0.50f, C_WHITE|iAlpha);

    PEInfo.PrintF(  "Max Age: %.2f", fSpawnersMaxAge);
    _pDP->PutText( PEInfo, _pixDPWidth*0.01f, _pixDPHeight*0.52f, C_WHITE|iAlpha);*/

    //PEInfo.PrintF(  "In Queue: %d", ctSpawnersInQueue);
    //_pDP->PutText( PEInfo, _pixDPWidth*0.01f, _pixDPHeight*0.33f, C_WHITE|iAlpha);


	  // loop thru players to find leader
    /*INDEX iPlayers = SetAllPlayersStats(PSK_LVL_FRAGS);
    FLOAT fPosHeight = 0.61f;
		for(INDEX i=0; i<iPlayers; i++) { 
			CPlayer *penLeader = _apenPlayers[i];
			CPlayer *penPlayer = _apenPlayers[i]; 
			CPlacement3D plLeader;
			CPlacement3D plPlayer;
			if (i<1) {
				// get leaders position 
  			plLeader.pl_PositionVector = penLeader->GetPlacement().pl_PositionVector;
        CTString strLeader = penLeader->GetPlayerName().Undecorated();
        strLeader.PrintF(  "%s", strLeader);
        _pDP->PutText( strLeader, _pixDPWidth*0.01f, _pixDPHeight*fPosHeight, C_WHITE|iAlpha);
        fPosHeight +=0.02f;
			} else {
				// get player's position
				plPlayer.pl_PositionVector = penPlayer->GetPlacement().pl_PositionVector; 
				FLOAT DistanceTotal = (plLeader.pl_PositionVector-plPlayer.pl_PositionVector).Length();
        CTString strName = penPlayer->GetPlayerName().Undecorated();
        strPlayer.PrintF(  "%s : %.0f", strName, DistanceTotal);
        _pDP->PutText( strPlayer, _pixDPWidth*0.01f, _pixDPHeight*fPosHeight, C_WHITE|iAlpha);
        fPosHeight +=0.02f;
      }
    }
  }*/

  if (hud_bShowPlayerRadar || hud_bShowEnemyRadar || dwk_bShowEnemyCount) {

    FLOAT fScaling = (FLOAT)_pixDPWidth /640.0f;

    // draw 'the enemy radar is on' text
    if (hud_bShowEnemyRadar) {
      // setup font
      _pfdDisplayFont->SetVariableWidth();
      _pDP->SetFont( _pfdDisplayFont);
      _pDP->SetTextScaling( fTextScale*0.6f );
      CTString strOn = "Enemy";
      _pDP->PutTextC( strOn, _pixDPWidth*0.98f, _pixDPHeight*0.76f, C_RED|CT_OPAQUE);
      strOn = "Radar";
      _pDP->PutTextC( strOn, _pixDPWidth*0.98f, _pixDPHeight*0.78f, C_RED|CT_OPAQUE);
      strOn = "is ON";
      _pDP->PutTextC( strOn, _pixDPWidth*0.98f, _pixDPHeight*0.8f, C_RED|CT_OPAQUE);
    }

    // initialize and draw the player radar mask
    if (hud_bShowPlayerRadar) {
			DrawPlayerRadarMask();
    }

    // setup font
    _pDP->SetFont(_pfdDisplayFont);
    _pDP->SetTextAspect(1.0f);
    _pDP->SetTextScaling(0.35f*fScaling);

    INDEX ctEnemies = 0;
    {FOREACHINDYNAMICCONTAINER(((CEntity&)*_penPlayer).GetWorld()->wo_cenEntities, CEntity, iten) { 
      CEntity *pen = iten;
			if (IsDerivedFromClass(pen, "Enemy Base")) { 
				CEnemyBase *penEnemy = (CEnemyBase *)pen;
				if (penEnemy->m_bHasBeenSpawned) {
					ctEnemies++;
					if (hud_bShowEnemyRadar) {
						if (penEnemy->m_penEnemy==NULL)	{ continue; }
						// get the inititial position and place it at (0,0,0)
						CPlacement3D plEnemy;  
						plEnemy.pl_PositionVector = penEnemy->GetPlacement().pl_PositionVector; 
						CPlacement3D plPlayer;  
						plPlayer.pl_PositionVector = ((CEntity&)*_penPlayer).GetPlacement().pl_PositionVector; 
						if( (plEnemy.pl_PositionVector - plPlayer.pl_PositionVector).Length() < 500) {
							// get it's position relative to the position of the player
							plEnemy.AbsoluteToRelativeSmooth(((CEntity&)*_penPlayer).GetPlacement());
							// X  & Y positions of the enemy (dot) relative to the player
							FLOAT EnemyX = plEnemy.pl_PositionVector(1);
							FLOAT EnemyZ = plEnemy.pl_PositionVector(3);					
							// position it in the center 
							FLOAT posX = EnemyX*2+_pDP->GetWidth()/2; 
							FLOAT posZ = EnemyZ*2+_pDP->GetHeight()/2;					
							// default dot color, initial enemy health
							COLOR colRdot = C_WHITE|255;
							COLOR colBdot = C_BLACK|255;
							FLOAT m_fEnemyHealth = 0.0f;
							// set the health of this enemy
							m_fEnemyHealth = ((CEnemyBase*)pen)->GetHealth() / ((CEnemyBase*)pen)->m_fMaxHealth;
							if( m_fEnemyHealth>0.7f) { 
								if ( penEnemy->m_penEnemy == penLast) {
									colRdot = C_MAGENTA|255;
								} else {
									colRdot = C_GREEN|255;
								}
							} else if( m_fEnemyHealth<=0.7f && m_fEnemyHealth>0.4f) { 
								if ( penEnemy->m_penEnemy == penLast) {
									colRdot = C_CYAN|255;
								} else {
									colRdot = C_YELLOW|255;
								}
							} else if( m_fEnemyHealth<=0.4f && m_fEnemyHealth>0.01f) { 
								if ( penEnemy->m_penEnemy == penLast) {
									colRdot = C_BLUE|255;
								} else {
									colRdot = C_RED|255; 
								}
							} else if( m_fEnemyHealth<=0.01f) { 
								colRdot = colBdot = C_WHITE|0; 
							}				
							// initialize and draw the dots
							_pDP->InitTexture( &_toEnemyDot);
							_pDP->AddTexture(posX-2, posZ-2, posX+2, posZ+2, colRdot);
							// all done trash it and start over
							_pDP->FlushRenderingQueue(); 
						}
					}
				}
			}
      if (hud_bShowPlayerRadar) {
		    if (IsDerivedFromClass(pen, "Player")) {
			    CPlayer *penPlayer = (CPlayer *)pen;
					if (penPlayer!=penPlayerCurrent) {
						if((penPlayer->GetPlacement().pl_PositionVector-((CEntity&)*_penPlayer).GetPlacement().pl_PositionVector).Length()<400
							&& (penPlayer->GetPlacement().pl_PositionVector-((CEntity&)*_penPlayer).GetPlacement().pl_PositionVector).Length()>15) { 
							if (penPlayer==penPlayerCurrent) { continue; }
							// set the players orientation angle to (0,0,0)
							CPlacement3D plPlayer;  
							plPlayer.pl_PositionVector = penPlayer->GetPlacement().pl_PositionVector; 
							plPlayer.pl_OrientationAngle = FLOAT3D(0,0,0); 

							// get it's position relative to the position of the player
							plPlayer.AbsoluteToRelativeSmooth(((CEntity&)*_penPlayer).GetPlacement()); 

							// X  & Y positions of the player relative to the owner
							FLOAT PlayerX = plPlayer.pl_PositionVector(1);
							FLOAT PlayerZ = plPlayer.pl_PositionVector(3);

							// position it relative to the center of the radar mask
							FLOAT posX = (PlayerX/6)+(_pixDPWidth*0.9475f); 
							FLOAT posZ = (PlayerZ/6)+(_pixDPHeight*0.89f);
							
							// default dot color, health
							COLOR colRdot = C_WHITE|255;
							FLOAT m_fPlayerHealth = 0.0f;

							// set the health of this player
							m_fPlayerHealth = ((CPlayer*)pen)->GetHealth() / ((CPlayer*)pen)->m_fMaxHealth;
							const CTString strName = penPlayer->GetPlayerName();
            
							if( m_fPlayerHealth>0.5f) { 
								colRdot = C_GREEN|255;
							} else if( m_fPlayerHealth<=0.5f && m_fPlayerHealth>0.25f) { 
								colRdot = C_YELLOW|255;
							} else if( m_fPlayerHealth<=0.25f && m_fPlayerHealth>0.01f) { 
								colRdot = C_RED|255;  
							} else if( m_fPlayerHealth<=0.00f) { 
								colRdot = C_BLACK|100; 
							} 
							CTString strTmp;    
							// put name under icon
							strTmp.PrintF("%s", strName); 
							if( m_fPlayerHealth<=0.01f) { 
								_pDP->PutTextC( strTmp, posX, posZ+4*fScaling, C_WHITE|200);
							} else {
								_pDP->PutTextC( strTmp, posX, posZ+4*fScaling, C_WHITE|255);
							}
							// initialize and draw the player icon
							_pDP->InitTexture( &_toPlayer);
							_pDP->AddTexture( posX-3*fScaling, posZ-3*fScaling, posX+3*fScaling, posZ+3*fScaling, colRdot );
							// all done trash it and start over
							_pDP->FlushRenderingQueue(); 
						}
					}
        } 
        if (IsDerivedFromClass(pen, "PlayerBot")) {
          // if these are the owners bots, draw them
          if (_penPlayer->m_penBot1==pen || _penPlayer->m_penBot2==pen || _penPlayer->m_penBot3==pen
            || _penPlayer->m_penBot4==pen || _penPlayer->m_penBot5==pen) { 

			      CPlayerBot *penPlayerBot = (CPlayerBot*)pen;
						if((penPlayerBot->GetPlacement().pl_PositionVector-((CEntity&)*_penPlayer).GetPlacement().pl_PositionVector).Length()<400) {
							// get the inititial position and place it at (0,0,0)
							CPlacement3D plPlayerBot;  
							plPlayerBot.pl_PositionVector = penPlayerBot->GetPlacement().pl_PositionVector; 
							plPlayerBot.pl_OrientationAngle = FLOAT3D(0,0,0);

							// get it's position relative to the position of the player
							plPlayerBot.AbsoluteToRelativeSmooth(((CEntity&)*_penPlayer).GetPlacement());

							// X  & Y positions of the player relative to the owner
							FLOAT BotX = plPlayerBot.pl_PositionVector(1);
							FLOAT BotZ = plPlayerBot.pl_PositionVector(3);

							FLOAT posBotX = (BotX/6)+(_pixDPWidth*0.9475f); 
							FLOAT posBotZ = (BotZ/6)+(_pixDPHeight*0.89f);

							COLOR colRdotBot = C_BLUE|255;
							CTString strTmpBot;  

							// put name under icon
							strTmpBot = penPlayerBot->m_strViewName; 
							_pDP->PutTextC( strTmpBot, posBotX, posBotZ+4*fScaling, C_WHITE|255);
							// initialize and draw the player bot icon
							_pDP->InitTexture( &_toPlayer);
							_pDP->AddTexture( posBotX-3*fScaling, posBotZ-3*fScaling, posBotX+3*fScaling, posBotZ+3*fScaling, colRdotBot);
							// all done trash it and start over
							_pDP->FlushRenderingQueue(); 
						}
          }
		    }
      }
    }} 

    //ctEnemiesLast = ctEnemies;
    if (dwk_bShowEnemyCount) {
      // define string for output
      CTString strEnemies;

      // setup font
			_pfdDisplayFont->SetVariableWidth();
			_pDP->SetFont( _pfdDisplayFont);
			_pDP->SetTextScaling( ( _fResolutionScaling+1)*0.37f);

			// print first two strings
			_pDP->SetTextCharSpacing( ( _fResolutionScaling+1)*0.25f); // set a different spacing width
      strEnemies.PrintF(  "Enemies");
      _pDP->PutTextC( strEnemies, _pixDPWidth*0.26f, _pixDPHeight*0.964f, C_RED|255);
			_pDP->SetTextCharSpacing( ( _fResolutionScaling+1)*0.8f);
      strEnemies.PrintF(  "Alive:");
      _pDP->PutTextC( strEnemies, _pixDPWidth*0.26f, _pixDPHeight*0.981f, C_RED|255);

      // setup font
			_pfdDisplayFont->SetVariableWidth();
			_pDP->SetFont( &_fdNumbersFont);
			_pDP->SetTextCharSpacing(1);
			
			// print number
			strValue.PrintF( "%d", ctEnemies);
			HUD_DrawText(pixLeftBound+200, pixBottomBound-4, strValue, C_WHITE|255, fNormValue);
    }
  }


  // setup font
	/*_pfdDisplayFont->SetVariableWidth();
	_pDP->SetFont( _pfdDisplayFont);

	FLOAT fMinDistance = 100.0f;
	CEntity *penChosenOne = NULL;
	// get player position
  CPlacement3D plPlayer = ((CEntity&)*_penPlayer).GetPlacement();	

  {FOREACHINDYNAMICCONTAINER(((CEntity&)*_penPlayer).GetWorld()->wo_cenEntities, CEntity, iten) { 
    CEntity *pen = iten;
    if (IsDerivedFromClass(pen, "PESpawnerEG")) { 
      CPESpawnerEG *penSpawner = (CPESpawnerEG *)pen;
      // get spawner position
      CPlacement3D plSpawner = penSpawner->GetPlacement();
			// get distance to spawner
			FLOAT fDistance = (plPlayer.pl_PositionVector - plSpawner.pl_PositionVector).Length();
			// if close enough
			if (fDistance<100) {
				// get direction to the entity
				FLOAT3D vDelta = plSpawner.pl_PositionVector - plPlayer.pl_PositionVector;
				// find front view vector
				CEntity *penView = NULL;
				if (((CPlayer &)*_penPlayer).m_pen3rdPersonView!=NULL) {
					penView = ((CPlayer &)*_penPlayer).m_pen3rdPersonView;
				}
				FLOAT3D vFront = FLOAT3D(0,0,0);
				if (penView!=NULL) {
					vFront = -((CPlayerView &)*penView).GetRotationMatrix().GetColumn(3);
				} else {
					vFront = -((CPlayer &)*_penPlayer).GetRotationMatrix().GetColumn(3);
				}
				// make dot product to determine if you can see target (view angle)
				FLOAT fDotProduct = (vDelta/vDelta.Length())%vFront;
				if (fDotProduct>=CosFast(10.0f)) {
					if (fDistance<fMinDistance) {
						fMinDistance = fDistance;
						penChosenOne = pen;
					}
				}
			}
		}
  }} 

	if (penChosenOne) {
		FLOAT fSize = 50.0f;
		//CPrintF("fSize: %.3f\n", fSize);
		// get it's position relative to the position of the player
		CPlacement3D plSpawner = penChosenOne->GetPlacement();
		//CPlacement3D plSpawner2 = plSpawner;
		//plSpawner2.AbsoluteToRelativeSmooth(plPlayer);
		// X  & Y positions of the spawner relative to the player
		//FLOAT SpawnerX = plSpawner2.pl_PositionVector(1);
		//FLOAT SpawnerZ = plSpawner2.pl_PositionVector(3);					
		// position it in the center 
		FLOAT posX = _pDP->GetWidth()/2; 
		FLOAT posZ = _pDP->GetHeight()/2*0.9f;	
		CTString strTmp;  
		strTmp.PrintF("( %.3f, %.3f, %.3f )", 
			plSpawner.pl_PositionVector(1), plSpawner.pl_PositionVector(2), plSpawner.pl_PositionVector(3));
		_pDP->SetTextScaling( ( _fResolutionScaling+1)*fSize*0.01f);
		_pDP->PutTextC( strTmp, posX, posZ, C_RED|255);
		// all done trash it and start over
		_pDP->FlushRenderingQueue(); 
	}*/

  /*FLOAT fMinDistance = 100.0f;
	CEnemySpawner *penChosenOne = NULL;

	// get player position
  CPlacement3D plPlayer = ((CEntity&)*_penPlayer).GetPlacement();	
	{FOREACHINDYNAMICCONTAINER(((CEntity&)*_penPlayer).GetWorld()->wo_cenEntities, CEntity, iten) { 
    CEntity *pen = iten;
    if (IsDerivedFromClass(pen, "Enemy Spawner")) { 
      CEnemySpawner *penSpawner = (CEnemySpawner *)pen;
			penSpawner->m_bHighlight = FALSE;
      // get spawner position
      CPlacement3D plSpawner = penSpawner->GetPlacement();
			// get distance to spawner
			FLOAT fDistance = (plPlayer.pl_PositionVector - plSpawner.pl_PositionVector).Length();
			// if close enough, set up and render text and texture
			if (fDistance<100) {
				// get direction to the entity
				FLOAT3D vDelta = plSpawner.pl_PositionVector - plPlayer.pl_PositionVector;
				// find front vector
				CEntity *penView = ((CPlayer &)*_penPlayer).m_pen3rdPersonView;
				FLOAT3D vFront = -((CPlayerView &)*penView).GetRotationMatrix().GetColumn(3);
				//FLOAT3D vFront = -((CEntity&)*_penPlayer).GetRotationMatrix().GetColumn(3);
				// make dot product to determine if you can see target (view angle)
				FLOAT fDotProduct = (vDelta/vDelta.Length())%vFront;
				if (fDotProduct>=CosFast(20.0f)) {
					if (fDistance<fMinDistance) {
						fMinDistance = fDistance;
						penChosenOne = penSpawner;
					}
				}
			}
		}
  }} 

	if (penChosenOne != NULL) {
    // setup font
		_pfdDisplayFont->SetVariableWidth();
		_pDP->SetFont( _pfdDisplayFont);
		_pDP->SetTextScaling( ( _fResolutionScaling+1)*0.37f);

		penChosenOne->m_bHighlight = TRUE;
		INDEX ctLoops = 0;
		FLOAT fHeightOffset = 0.40f;
		CTString strInfo = "";
		CTString strInfoOut = "";
		CTString strEntityInfo = penChosenOne->GetSpawnerInfo();
		while (strEntityInfo != "") {
			strInfo = strEntityInfo;
			strInfo.OnlyFirstLine();
			strInfoOut.PrintF("%s", strInfo);
			_pDP->PutText(strInfoOut, _pixDPWidth*0.01f, _pixDPHeight*fHeightOffset, C_WHITE|255);
			//CPrintF("strInfoOut: %s\n", strInfoOut);
			//CPrintF("strEntityInfo: %s\n", strEntityInfo);
			strInfo += "\n";
			strEntityInfo.ReplaceSubstr(strInfo, "");
			ctLoops++;
			fHeightOffset += 0.02f;
			if (ctLoops > 10) {
				strEntityInfo = "";
			}
		}
	}*/

  #ifdef ENTITY_DEBUG
  // if entity debug is on, draw entity stack
  HUD_DrawEntityStack();
  #endif

  // draw cheat modes
  if( GetSP()->sp_ctMaxPlayers==1) {
    INDEX iLine=1;
    ULONG ulAlpha = sin(_tmNow*16)*96 +128;
    PIX pixFontHeight = _pfdConsoleFont->fd_pixCharHeight;
    const COLOR colCheat = _colHUDText;
    _pDP->SetFont( _pfdConsoleFont);
    _pDP->SetTextScaling( 1.0f);
    const FLOAT fchtTM = cht_fTranslationMultiplier; // for text formatting sake :)
    if( fchtTM > 1.0f)  { _pDP->PutTextR( "turbo",     _pixDPWidth-1, _pixDPHeight-pixFontHeight*iLine, colCheat|ulAlpha); iLine++; }
    if( cht_bInvisible) { _pDP->PutTextR( "invisible", _pixDPWidth-1, _pixDPHeight-pixFontHeight*iLine, colCheat|ulAlpha); iLine++; }
    if( cht_bGhost)     { _pDP->PutTextR( "ghost",     _pixDPWidth-1, _pixDPHeight-pixFontHeight*iLine, colCheat|ulAlpha); iLine++; }
    if( cht_bFly)       { _pDP->PutTextR( "fly",       _pixDPWidth-1, _pixDPHeight-pixFontHeight*iLine, colCheat|ulAlpha); iLine++; }
    if( cht_bGod)       { _pDP->PutTextR( "god",       _pixDPWidth-1, _pixDPHeight-pixFontHeight*iLine, colCheat|ulAlpha); iLine++; }
  }

  // in the end, remember the current time so it can be used in the next frame
  _tmLast = _tmNow;

}



// initialize all that's needed for drawing the HUD
extern void InitHUD(void)
{
  // try to
  try {
    // initialize and load HUD numbers font
    DECLARE_CTFILENAME( fnFont, "Fonts\\Numbers3.fnt");
    _fdNumbersFont.Load_t( fnFont);
    //_fdNumbersFont.SetCharSpacing(0);

    // initialize status bar textures
    _toHealth.SetData_t(  CTFILENAME("TexturesMP\\Interface\\HSuper.tex"));
    _toOxygen.SetData_t(  CTFILENAME("TexturesMP\\Interface\\Oxygen-2.tex"));
    _toFrags.SetData_t(   CTFILENAME("TexturesMP\\Interface\\IBead.tex"));
    _toDeaths.SetData_t(  CTFILENAME("TexturesMP\\Interface\\ISkull.tex"));
    _toScore.SetData_t(   CTFILENAME("TexturesMP\\Interface\\IScore.tex"));
    _toHiScore.SetData_t( CTFILENAME("TexturesMP\\Interface\\IHiScore.tex"));
    _toMessage.SetData_t( CTFILENAME("TexturesMP\\Interface\\IMessage.tex"));
    _toMana.SetData_t(    CTFILENAME("TexturesMP\\Interface\\IValue.tex"));
    _toArmorSmall.SetData_t(  CTFILENAME("TexturesMP\\Interface\\ArSmall.tex"));
    _toArmorMedium.SetData_t(   CTFILENAME("TexturesMP\\Interface\\ArMedium.tex"));
    _toArmorLarge.SetData_t(   CTFILENAME("TexturesMP\\Interface\\ArStrong.tex"));

    // initialize ammo textures                    
    _toAShells.SetData_t(        CTFILENAME("TexturesMP\\Interface\\AmShells.tex"));
    _toABullets.SetData_t(       CTFILENAME("TexturesMP\\Interface\\AmBullets.tex"));
    _toARockets.SetData_t(       CTFILENAME("TexturesMP\\Interface\\AmRockets.tex"));
    _toAGrenades.SetData_t(      CTFILENAME("TexturesMP\\Interface\\AmGrenades.tex"));
    _toANapalm.SetData_t(        CTFILENAME("TexturesMP\\Interface\\AmFuelReservoir.tex"));
    _toAElectricity.SetData_t(   CTFILENAME("TexturesMP\\Interface\\AmElectricity.tex"));
    _toAIronBall.SetData_t(      CTFILENAME("TexturesMP\\Interface\\AmCannonBall.tex"));
    _toASniperBullets.SetData_t( CTFILENAME("TexturesMP\\Interface\\AmSniperBullets.tex"));
    _toASeriousBomb.SetData_t(   CTFILENAME("TexturesMP\\Interface\\AmSeriousBomb.tex"));
		_toADaBomb.SetData_t(				 CTFILENAME("TexturesMP\\Interface\\AmDaBomb.tex"));
    // initialize weapon textures
    _toWKnife.SetData_t(           CTFILENAME("TexturesMP\\Interface\\WKnife.tex"));
    _toWColt.SetData_t(            CTFILENAME("TexturesMP\\Interface\\WColt.tex"));
    _toWSingleShotgun.SetData_t(   CTFILENAME("TexturesMP\\Interface\\WSingleShotgun.tex"));
    _toWDoubleShotgun.SetData_t(   CTFILENAME("TexturesMP\\Interface\\WDoubleShotgun.tex"));
    _toWTommygun.SetData_t(        CTFILENAME("TexturesMP\\Interface\\WTommygun.tex"));
    _toWMinigun.SetData_t(         CTFILENAME("TexturesMP\\Interface\\WMinigun.tex"));
    _toWRocketLauncher.SetData_t(  CTFILENAME("TexturesMP\\Interface\\WRocketLauncher.tex"));
    _toWGrenadeLauncher.SetData_t( CTFILENAME("TexturesMP\\Interface\\WGrenadeLauncher.tex"));
    _toWLaser.SetData_t(           CTFILENAME("TexturesMP\\Interface\\WLaser.tex"));
    _toWIronCannon.SetData_t(      CTFILENAME("TexturesMP\\Interface\\WCannon.tex"));
    _toWChainsaw.SetData_t(        CTFILENAME("TexturesMP\\Interface\\WChainsaw.tex"));
    _toWSniper.SetData_t(          CTFILENAME("TexturesMP\\Interface\\WSniper.tex"));
    _toWFlamer.SetData_t(          CTFILENAME("TexturesMP\\Interface\\WFlamer.tex"));
        
    // initialize powerup textures (DO NOT CHANGE ORDER!)
    _atoPowerups[0].SetData_t( CTFILENAME("TexturesMP\\Interface\\PInvisibility.tex"));
    _atoPowerups[1].SetData_t( CTFILENAME("TexturesMP\\Interface\\PInvulnerability.tex"));
    _atoPowerups[2].SetData_t( CTFILENAME("TexturesMP\\Interface\\PSeriousDamage.tex"));
    _atoPowerups[3].SetData_t( CTFILENAME("TexturesMP\\Interface\\PSeriousSpeed.tex"));
    _toSniperLine.SetData_t( CTFILENAME("Textures\\Interface\\SniperLine.tex"));
    _toPlayer.SetData_t( CTFILENAME("Textures\\Interface\\Player.tex"));
    _toPlayerRadar.SetData_t(CTFILENAME("Textures\\Interface\\PlayerRadar.tex"));
    _toEnemyDot.SetData_t(CTFILENAME("Textures\\Interface\\EnemyDot.tex"));

    _toN.SetData_t(CTFILENAME("TexturesMP\\Interface\\N.tex"));
    _toE.SetData_t(CTFILENAME("TexturesMP\\Interface\\E.tex"));
    _toS.SetData_t(CTFILENAME("TexturesMP\\Interface\\S.tex"));
    _toW.SetData_t(CTFILENAME("TexturesMP\\Interface\\W.tex"));

		//_toIHealthBar.SetData_t(CTFILENAME("TexturesMP\\Interface\\IHealthBar.tex"));
		//_toIHealth.SetData_t(CTFILENAME("TexturesMP\\Interface\\IHealth.tex"));

    // initialize tile texture
    _toTile.SetData_t( CTFILENAME("Textures\\Interface\\Tile.tex"));
    
    // set all textures as constant
    ((CTextureData*)_toHealth .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toOxygen .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toFrags  .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toDeaths .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toScore  .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toHiScore.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toMessage.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toMana   .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toArmorSmall.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toArmorMedium.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toArmorLarge.GetData())->Force(TEX_CONSTANT);

    ((CTextureData*)_toAShells       .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toABullets      .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toARockets      .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toAGrenades     .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toANapalm       .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toAElectricity  .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toAIronBall     .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toASniperBullets.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toASeriousBomb  .GetData())->Force(TEX_CONSTANT);
		((CTextureData*)_toADaBomb			 .GetData())->Force(TEX_CONSTANT);

    ((CTextureData*)_toWKnife          .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWColt           .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWSingleShotgun  .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWDoubleShotgun  .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWTommygun       .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWRocketLauncher .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWGrenadeLauncher.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWChainsaw       .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWLaser          .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWIronCannon     .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWSniper         .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWMinigun        .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toWFlamer         .GetData())->Force(TEX_CONSTANT);
    
    ((CTextureData*)_atoPowerups[0]		.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_atoPowerups[1]		.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_atoPowerups[2]   .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_atoPowerups[3]		.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toTile						.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toSniperLine			.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toPlayer         .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toPlayerRadar    .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toEnemyDot       .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toN							.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toE							.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toS							.GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toW							.GetData())->Force(TEX_CONSTANT);
		//((CTextureData*)_toIHealthBar			.GetData())->Force(TEX_CONSTANT);
		//((CTextureData*)_toIHealth				.GetData())->Force(TEX_CONSTANT);
  }
  catch( char *strError) {
    FatalError( strError);
  }

}


// clean up
extern void EndHUD(void)
{

}

