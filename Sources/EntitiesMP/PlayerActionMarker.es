407
%{
#include "StdH.h"
extern INDEX hud_bShowPEInfo;
%}

uses "EntitiesMP/Marker";
uses "EntitiesMP/Player";
uses "EntitiesMP/Trigger";
uses "EntitiesMP/PlayerBot";

enum PlayerAutoAction {
  1 PAA_RUN               "Run",              // run to this marker
  2 PAA_WAIT              "Wait",             // wait given time
  3 PAA_USEITEM           "UseItem",          // use some item here
  4 PAA_STOREWEAPON       "StoreWeapon",      // put current weapon away
  5 PAA_DRAWWEAPON        "DrawWeapon",       // put current weapon back in hand
  6 PAA_LOOKAROUND        "LookAround",       // wait given time and look around
  7 PAA_RUNANDSTOP        "RunAndStop",       // run to this marker and stop at it
  8 PAA_RECORDSTATS       "RecordStats",      // before end of level animation - record statistics
  9 PAA_ENDOFGAME         "EndOfGame",        // the game has ended - go to computer, then to menu
 10 PAA_SHOWSTATS         "ShowStats",        // after end of level animation - show statistics
 11 PAA_APPEARING         "Appearing",        // appearing with stardust effect
 12 PAA_WAITFOREVER       "WaitForever",      // wait trigger
 13 PAA_TELEPORT          "Teleport",         // teleport to new location
 14 PAA_PICKITEM          "PickItem",         // pick item
 15 PAA_FALLDOWN          "FallDown",         // falling from broken bridge
 16 PAA_FALLTOABYSS       "FallToAbyss",      // falling down into an abyss
 17 PAA_RELEASEPLAYER     "ReleasePlayer",    // return control to player from camera and similar
 18 PAA_STARTCOMPUTER     "StartComputer",    // invoke netricsa
 19 PAA_TRAVELING_IN_BEAM "TravelingInBeam",  // traveling in space shpi beam
 20 PAA_LOGO_FIRE_MINIGUN "LogoFireMinigun",  // fire minigun in logo sequence
 21 PAA_STARTCREDITS      "StartCredits",     // start credits printout
 22 PAA_STARTINTROSCROLL  "StartIntroScroll", // start intro text scroll
 23 PAA_STOPSCROLLER      "StopScroller",     // stop intro scroll, or end-of-game credits
 24 PAA_NOGRAVITY         "NoGravity",        // deactivate gravity influence for player
 25 PAA_TURNONGRAVITY     "TurnOnGravity",    // turn on gravity
 26 PAA_LOGO_FIRE_INTROSE "SE Logo Fire",       // for SE intro
 27 PAA_INTROSE_SELECT_WEAPON "SE Logo draw weapon",
 28 PAA_STOPANDWAIT       "StopAndWait",      // stop immediately and wait
};

class CPlayerActionMarker: CMarker {
name      "PlayerActionMarker";
thumbnail "Thumbnails\\PlayerActionMarker.tbn";

properties:
  1 enum PlayerAutoAction m_paaAction "Action" 'A' = PAA_RUN, // what to do here
  2 FLOAT m_tmWait "Wait" 'W' = 0.0f, // how long to wait (if wait action)
  3 CEntityPointer m_penDoorController "Door for item" 'D', // where to use the item (if use action)
  4 CEntityPointer m_penTrigger "Trigger" 'G',  // triggered when player gets here
  5 FLOAT m_fSpeed "Speed" 'S' = 1.0f,    // how fast to run towards marker
  6 CEntityPointer m_penItem "Item to pick" 'I',

components:
  1 model   MODEL_MARKER     "Models\\Editor\\PlayerActionMarker.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\PlayerActionMarker.tex",

functions:

  const CTString &GetDescription(void) const {
    CTString strAction = PlayerAutoAction_enum.NameForValue(INDEX(m_paaAction));
    if (m_penTarget==NULL) {
      ((CTString&)m_strDescription).PrintF("%s (%s)-><none>", m_strName, strAction);
    } else {
      ((CTString&)m_strDescription).PrintF("%s (%s)->%s", m_strName, strAction, 
        m_penTarget->GetName());
    }
    return m_strDescription;
  }

  /* Check if entity can drop marker for making linked route. */
  BOOL DropsMarker(CTFileName &fnmMarkerClass, CTString &strTargetProperty) const {
    fnmMarkerClass = CTFILENAME("Classes\\PlayerActionMarker.ecl");
    strTargetProperty = "Target";
    return TRUE;
  }

	// send an event to the TGP trigger to send an event to the new world link
  void SendEventToWorldLink(void)
  {
    {FOREACHINDYNAMICCONTAINER(GetWorld()->wo_cenEntities, CEntity, iten) {
			CEntity *pen = iten;
      if (IsDerivedFromClass(pen, "Trigger")) {
        CTrigger *penTrigger = (CTrigger *)pen;
        if (penTrigger->m_strName=="Game End") {
          penTrigger->SendEvent(ETrigger());
        }
      }
    }}
  }


  /* Handle an event, return false if the event is not handled. */
  BOOL HandleEvent(const CEntityEvent &ee)
  {
    // if triggered
    if (ee.ee_slEvent==EVENTCODE_ETrigger) {
      ETrigger &eTrigger = (ETrigger &)ee;
      // if triggered by a player
      if( IsDerivedFromClass(eTrigger.penCaused, "Player")) {
				if (hud_bShowPEInfo) {
					CPrintF("PlayerActionMarker: %s, Triggered by: Player\n", m_strDescription);
				}
        // send it event to start auto actions from here
        EAutoAction eAutoAction;
        eAutoAction.penFirstMarker = this;
        eTrigger.penCaused->SendEvent(eAutoAction);
				// send the TGP world link event
        if (m_strName=="Marker RecStats Coop") {
          SendEventToWorldLink();
        }
      }
      // if triggered by a player bot
      else if( IsDerivedFromClass(eTrigger.penCaused, "PlayerBot")) {
				if (hud_bShowPEInfo) {
					CPrintF("PlayerActionMarker %s, Triggered by: Player Bot\n", m_strDescription);
				}
				CEntity *pen = eTrigger.penCaused;
				// send event to the bot's owner
				CPlayerBot *penBot = (CPlayerBot *)pen;
        EAutoAction eAutoAction;
        eAutoAction.penFirstMarker = this;
        penBot->m_penOwner->SendEvent(eAutoAction);
				// send the TGP world link event
        if (m_strName=="Marker RecStats Coop") {
          SendEventToWorldLink();
        }
      } 
			// if somehow we ended up being triggered by something else, really bad idea?
			else {
				if (hud_bShowPEInfo) {
					CPrintF("PlayerActionMarker: %s, Triggered by: Other ... Do fixup to player?\n", m_strDescription);
				}
				// get a player to send the event to
				/*CEntity *penCaused = FixupCausedToPlayer(this, eTrigger.penCaused);
        EAutoAction eAutoAction;
        eAutoAction.penFirstMarker = this;
        penCaused->SendEvent(eAutoAction);
				// send the TGP world link event
        if (m_strName=="Marker RecStats Coop") {
          SendEventToWorldLink();
        }*/
      }
      return TRUE;
    }
    return FALSE;
  }


procedures:

  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    m_tmWait = ClampDn(m_tmWait, 0.05f);

		//CPrintF("PlayerMarker Main()\n");

    return;
  }
};
