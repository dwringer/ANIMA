#include <include\core.h>
#include <classdef\ObjectRoot.hpp>
#include <classdef\PostProcessing.hpp>
#include <include\optimizer.h>
#include <include\mkcivs.h>
//#include <classdef\Dictionary.hpp>
#include <classdef\Waypoint.hpp>
#include <classdef\TargetImpression.hpp>
#include <classdef\LocationFinder.hpp>
#include <classdef\Armory.hpp>
#include <classdef\UnitGroup.hpp>
#include <classdef\CrewUnitGroup.hpp>
#include <classdef\Headquarters.hpp>

#include <classdef\LogicalGroup.hpp>

private ["_randomizerThread"];

ClassesInitialized = true;

// Burn some randoms while initializing. Theoretically pointless.
//  99% sure this is some kind of OCD ritual rather than necessary.
_randomizerThread = [] spawn {
	private ["_rand"];
	for "_i" from 0 to 160 do {
		_rand = ([[1, 2, 3, _i]] call fnc_choose) select 0;
	};
	sleep 0.05;
};

PostProcessing = ["PostProcessing",
  ["golden_autumn",
   "movie_day",
   "middle_east",
   "real_is_brown_2",
   "tropical",
   "ofp_palette",
   "none"] select ([7] call fnc_randint)
] call fnc_new;

[([["sunrise", "sunset", "sunset", "sunset"],
   1] call fnc_choose) select 0] execVM "misc\randomTime.sqf";
[] execVM "misc\randomWeather.sqf";
_nil = [] spawn { sleep 0.02; forceWeatherChange };


TOV_fnc_EndConvoy = [["_convoyScript", "_convoyGroup"], {
  terminate _convoyScript; 
  {
    (vehicle _x) limitSpeed 5000;
    (vehicle _x) setUnloadInCombat [true, false];
  } forEach (units _convoyGroup);
  _convoyGroup enableAttack true;
}] call fnc_lambda;


TOV_fnc_SimpleConvoy = { 
  params ["_convoyGroup",
	  ["_convoySpeed",      50],
	  ["_convoySeparation", 50],
	  ["_pushThrough",      true]];
  if (_pushThrough) then {
      _convoyGroup enableAttack !(_pushThrough);
      {
	(vehicle _x) setUnloadInCombat [false, false];
      } forEach (units _convoyGroup);
    };
  _convoyGroup setFormation "COLUMN";
  {
    (vehicle _x) limitSpeed _convoySpeed*1.15;
    (vehicle _x) setConvoySeparation _convoySeparation;
  } forEach (units _convoyGroup);
  (vehicle leader _convoyGroup) limitSpeed _convoySpeed;
  while {sleep 5; !isNull _convoyGroup} do {
    {
      if ((speed vehicle _x < 5) &&
	  (_pushThrough || (behaviour _x != "COMBAT"))) then {
          (vehicle _x) doFollow (leader _convoyGroup);
	};	
    } forEach (units _convoyGroup)-(crew (vehicle (leader _convoyGroup)))-allPlayers;
    {
      (vehicle _x) setConvoySeparation _convoySeparation;
    } forEach (units _convoyGroup);
  }; 
};

fnc_emergency_disembark = [["_leader"], { 
  private _thread = _leader spawn {  
    private _vehicle = vehicle _this; 
    doStop _vehicle;  
    waitUntil { (speed _vehicle) < 3 };  
    { sleep (0.25 + (random 0.25));  
      [_x] allowGetIn false;  
      unassignVehicle _x;  
      _x action ["getout", vehicle _x];
    } forEach (units (group _this)); 
    (group _this) setBehaviour "COMBAT";
    _vehicle doFollow (leader (group (driver _vehicle))); 
  };  
}] call fnc_lambda;

fnc_regroup_vehicle = [["_vehicle"], {
  private ["_joins"];
  private _commander          = objNull;
  private _gunner             = objNull;
  private _driver             = driver _vehicle;
  private _group              = createGroup (side _driver);
  private _effectiveCommander = effectiveCommander _vehicle;
  private _fullCrew           = fullCrew _vehicle;
  private _turrets            = [];
  private _cargo              = [];
  private _crew_fn = [["_unit",
                       "_role",
	               "_cargoIndex",
	               "_turretPath",
	               "_personTurret"], {
    if ((group _unit) == (group _driver)) then {
      switch (_role) do {
        case "driver":    {    _driver = _unit };
        case "commander": { _commander = _unit };
        case "gunner":    {    _gunner = _unit };
        case "turret":    {   _turrets = _turrets + [[_unit, _turretPath]] };
        case "cargo":     {     _cargo = _cargo   + [_unit] };
      };
    };
  }] call fnc_vlambda;
  { _x call _crew_fn } forEach _fullCrew;
  if (isNull _effectiveCommander) then {
    _joins = [];
  } else {
    _joins = [_effectiveCommander];
  };
  { if ((not (isNull _x)) and
        (not (_x == _effectiveCommander))) then { 
      _joins = _joins + [_x]; 
    }; 
  } forEach [_commander, _driver, _gunner];
  _joins joinSilent _group;
  if (not isNull _effectiveCommander) then {
    _vehicle setEffectiveCommander _effectiveCommander;
  };
  if (0 < (count _turrets)) then {
    ([[["_pair"], { _pair select 0 }] call fnc_lambda,
      _turrets] call fnc_map) joinSilent _group;
  };
  if (0 < (count _cargo)) then { _cargo joinSilent _group };
  _group
}] call fnc_lambda;

fnc_regroup_vehicles = {
  private _groups = [];
  {_groups = _groups + [[_x] call fnc_regroup_vehicle]} forEach _this;
  _groups
};

fnc_units = {
  private _acc = [];
  { _acc = _acc + (units _x) } forEach _this;
  _acc
};

// _thread = [] spawn {  
//   [Thread_Convoy, Opf_Group_Convoy] spawn TOV_fnc_EndConvoy; 
//   { [_x] call fnc_emergency_disembark } 
//    forEach ([Opf_HQ_ConvoyInfantry, "leaders"] call fnc_tell);
//   [Blu_HQ_WeaponsSquad, "attack_targets", units Opf_Group_Convoy, 1]
//    call fnc_tell;
//   waitUntil { not isNil "Detected_BLUFOR" };
//   private _groups = [Opf_APC_1, Opf_APC_2] call fnc_regroup_vehicles;
//   Opf_HQ_IFV = ["Headquarters", _groups call fnc_units] call fnc_new;
//   [Opf_HQ_ConvoyInfantry, "attack_targets", Detected_BLUFOR, 8] call fnc_tell;  
//   [Opf_HQ_IFV, "attack_targets", Detected_BLUFOR, 2] call fnc_tell;  
// }

//fnc_attach_strobe = [["_unit"], {

fnc_attach_strobe = {
  params ["_unit"];
  private ["_thread"];
  _thread =  _unit spawn {
    private ["_light_obj"];
//    sleep (random 2.65);
    while { alive _this } do {
      _light_obj = "MS_Strobe_Ammo_1" createVehicle [0,0,0];
      _light_obj attachTo [_this, [0,-0.03,0.07], "LeftShoulder"];
      sleep 300;
      if (not isNull _light_obj) then { deleteVehicle _light_obj };
    };
 };
};

fnc_clear_waypoints = [["_group"], {
    private ["_waypoints"];
    while {
      _waypoints = waypoints _group;
      0 < (count _waypoints)
    } do {
      deleteWaypoint (_waypoints select 0);
    };
}] call fnc_lambda;
