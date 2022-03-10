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
