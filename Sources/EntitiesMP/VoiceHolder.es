231
%{
#include "StdH.h"
extern INDEX ent_bReportBrokenChains;
//#include "EntitiesMP/Player.h"
static const CPlayer *_penPlayer;
//extern CPlayer *_apenPlayers[NET_MAXGAMEPLAYERS];
%}

uses "EntitiesMP/Player";

class CVoiceHolder : CRationalEntity {
name      "VoiceHolder";
thumbnail "Thumbnails\\VoiceHolder.tbn";
features  "HasName", "IsTargetable";

properties:

  1 CTString m_strName          "Name" 'N' = "VoiceHolder",
  3 CTString m_strDescription = "",
  2 CTFileName m_fnmMessage  "Message" 'M' = CTString(""),
  5 BOOL m_bActive "Active" 'A' = TRUE,
  6 INDEX m_ctMaxTrigs            "Max trigs" 'X' = 1, // how many times could trig

  {
    CAutoPrecacheSound m_aps;
  }

components:

  1 model   MODEL_MARKER     "Models\\Editor\\VoiceHolder.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\VoiceHolder.tex"

functions:

  void Precache(void)
  {
    m_aps.Precache(m_fnmMessage);
  }

  const CTString &GetDescription(void) const {
    ((CTString&)m_strDescription).PrintF("%s", m_fnmMessage.FileName());
    return m_strDescription;
  }

	void SendEvents(void)
	{
    EVoiceMessage eMsg;
    eMsg.fnmMessage = m_fnmMessage;
	  // loop thru potentional players 
	  for( INDEX i=0; i<16; i++) { 
		  // ignore non-existent players
      CPlayer *penPlayer = (CPlayer*)&*_penPlayer->GetPlayerEntity(i);
		  if (penPlayer==NULL) { continue; }
			penPlayer->SendEvent(eMsg);
			//penPlayer->SayVoiceMessage(eMsg.fnmMessage);
			//SendToTarget(penPlayer, eMsg, this);
    }
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
    wait() {
      on (ETrigger eTrigger): {
        if (!m_bActive) {
          resume;
        }
        CEntity *penCaused = FixupCausedToPlayer(this, eTrigger.penCaused);
        EVoiceMessage eMsg;
        eMsg.fnmMessage = m_fnmMessage;
				penCaused->SendEvent(eMsg);
				//SendEvents();

        // if max trig count is used for counting
        if(m_ctMaxTrigs > 0) {
          // decrease count
          m_ctMaxTrigs-=1;
          // if we trigged max times
          if( m_ctMaxTrigs <= 0) {
            // cease to exist
            Destroy();
            stop;
          }
        } else {
					resume;
				}
      }
      on (EActivate): {
        m_bActive = TRUE;
        resume;
      }
      on (EDeactivate): {
        m_bActive = FALSE;
        resume;
      }
    }

		//autowait(1.0f);
		//Destroy();
    return;
  }
};
