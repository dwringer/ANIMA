#ifndef __MKCIVS_H__
#define __MKCIVS_H__
/* Initialize ambient civilians: */
Index_civGroups = 0;
civArray = [];
civKilled = 0;
Mutex_mkCivs      = false;  // Guards  mkCiv execution at large
Mutex_civTriggers = false;  // Guards  Index_civGroups
Mutex_civArray    = false;  // Guards  civArray
Mutex_civKilled   = false;  // Guardds civKilled

fnc_mkcivs = compile preprocessfile "mkcivs\fnc_mkcivs.sqf";
fnc_civLeaders = compile preprocessfile "mkcivs\fnc_civLeaders.sqf";
#endif

///////////////////// MKCIVS module and support classes: //////////////////////
#include <..\mkcivs\ambush_fns.h>
#include <..\mkcivs\WeaponCache.hpp>
#include <..\mkcivs\Victim.hpp>
#include <..\mkcivs\CivilianZone.hpp>
#include <..\mkcivs\KillZone.hpp>
///////////////////////////////////////////////////////////////////////////////

