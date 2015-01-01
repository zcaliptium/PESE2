302
%{
#include "StdH.h"
%}

uses "EntitiesMP/Marker";

class CEnemyMarker: CMarker {
name      "Enemy Marker";
thumbnail "Thumbnails\\EnemyMarker.tbn";

properties:
  1 FLOAT m_fWaitTime = 0.0f,     // time to wait(or do anything) until go to another marker
  3 RANGE m_fMarkerRange        "Marker Range" 'M' = 0.0f,  // range around marker (marker doesn't have to be hit directly)

 11 RANGE m_fPatrolAreaInner    "Patrol Area Inner" 'R' = 0.0f,     // patrol area inner circle
 12 RANGE m_fPatrolAreaOuter    "Patrol Area Outer" 'E' = 0.0f,     // patrol area outer circle
 13 FLOAT m_fPatrolTime         "Patrol Time" 'P' = 0.0f,           // time to patrol around
 14 enum BoolEType m_betRunToMarker  "Run to marker" 'O' = BET_IGNORE,   // run to marker
 15 enum BoolEType m_betFly     "Fly" 'F' = BET_IGNORE,             // fly if you can
 16 enum BoolEType m_betBlind   "Blind" 'B' = BET_IGNORE,
 17 enum BoolEType m_betDeaf    "Deaf"  'D' = BET_IGNORE,

 18 BOOL m_bStartTactics          "Start Tactics" = FALSE, 

 20 CEntityPointer m_penTargeted,

components:
  1 model   MODEL_MARKER     "Models\\Editor\\EnemyMarker.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\EnemyMarker.tex"

functions:
  /* Check if entity is moved on a route set up by its targets. */
  BOOL MovesByTargetedRoute(CTString &strTargetProperty) const {
    strTargetProperty = "Target";
    return TRUE;
  };
  
  /* Check if entity can drop marker for making linked route. */
  BOOL DropsMarker(CTFileName &fnmMarkerClass, CTString &strTargetProperty) const {
    fnmMarkerClass = CTFILENAME("Classes\\EnemyMarker.ecl");
    strTargetProperty = "Target";
    return TRUE;
  }

  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if( slPropertyOffset == offsetof(CMarker, m_penTarget))
    {
      if (IsOfClass(penTarget, "Enemy Marker")) { return TRUE; }
      else { return FALSE; }
    }   
    return CEntity::IsTargetValid(slPropertyOffset, penTarget);
  }

  // render particles
  /*void RenderParticles(void) {
    if(m_penTarget!=NULL) {
      // render one lightning toward enemy
      FLOAT3D vSource = GetPlacement().pl_PositionVector;
      vSource += FLOAT3D(0, 0.5f, 0);
      FLOAT3D vTarget = m_penTarget->GetPlacement().pl_PositionVector;
      vTarget += FLOAT3D(0, 0.5f, 0);
      COLOR colLine = C_RED|CT_OPAQUE;
      Particles_Raycast(vSource, vTarget, 64, 0.5f, colLine);
    }
  }*/

procedures:
  Main() {
    InitAsEditorModel();
		//InitAsModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
		//SetPhysicsFlags(EPF_MODEL_ITEM);
    SetCollisionFlags(ECF_IMMATERIAL);
    
    if (m_strName=="Marker") {
      m_strName="Enemy Marker";
    }

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);
    return;
  }
};

