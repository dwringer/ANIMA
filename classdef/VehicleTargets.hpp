///////////////////////////////////////////////////////////////////////////////
//  VehicleTargets class  //
////////////////////////////
//
//  This creates a zone of vehicles, strategically placed by certain criteria.
//   By default, these are chosen from a list of various CUP OPFOR armor.
//   Alternatively, vehicles can be placed inside the trigger within the
//   EDEN editor, in which case they will be deleted at mission start and
//   vehicles of those types will be spawned.
//
///////////////////////////////////////////////////////////////////////////////
//
//  Instances of this class are formed from a 2-D or 3-D trigger in the editor,
//   with Activation set to None (or, if placing vehicles inside, their side),
//   and Condition set to true:
//   _thread = [thisTrigger, triggerList] spawn {
//     params ["_trigger", "_list"];
//     waitUntil { not isNil "ClassesInitialized" };
//     <instance> = ["VehicleTargets", _trigger, _list, <n-vehicles>,
//                   (<visible-to-objects>, <hide-from-objects>)] call fnc_new;
//   };
//
//  The last two parameters are optional lists of units or gamelogics and will
//   greatly improve positioning at a steep cost in run time. The former is a
//   list of objects which are to have LOS to spawned vehicles, while the
//   latter is a list of objects which should NOT have LOS. These may be used,
//   for instance, to mark approaches and possible ambush positions.
//
//  Once initialized, the VehicleTargets instance must be run with:
//   [<instance>, "spawn"] call fnc_tell;
//
//  That will create empty vehicles through the area, with the engines on.
//   This "spawn" method has two additional optional parameters:
//   [VTInstance, "spawn", <crewed?>, <priority?>] call fnc_tell;
//
//  The "crewed" parameter tells whether to use createVehicleCrew to spawn
//   a crew of the vehicle's faction inside it, thus enabling AI behavior.
//   [VTInstance, "spawn", true] call fnc_tell;
//
//  By default ("priority"), spawned vehicles will face the first object in
//   <visible-to-objects> to which they have LOS, or if no list was provided,
//   they will face random directions. If the second "spawn" parameter is
//   false then instead they'll choose whichever target has the clearest LOS:
//   [VTInstance, "spawn", true, false] call fnc_tell;
//
//  In the event a spawned vehicle has no LOS to any of the targets, then
//   it will always face the first one in <visible-to-objects>.
//
///////////////////////////////////////////////////////////////////////////////
DEFCLASS("VehicleTargets") ["_self", "_trigger", "_triggerList", "_count",
			    "_observers", "_avoid"] DO {
    if (0 < (count _triggerList)) then {
        private ["_vehicle", "_type"];
        private _classNames = [];
	{
	    _vehicle = _x;
	    if (_vehicle isKindOf "LandVehicle") then {
		_type = typeOf _vehicle;
		if (not (_type in _classNames)) then {
		    _classNames = _classNames + [_type];
		};
		{ deleteVehicle _x } forEach (crew _vehicle) + [_vehicle];
	    };
	} forEach _triggerList;
	_self setVariable ["vehicleClasses", _classNames];
    } else {
	_self setVariable ["vehicleClasses", [
            "CUP_O_MTLB_pk_TKA",
	    "CUP_O_2S6_RU",
	    "CUP_O_BRDM2_CHDKZ",
	    "CUP_O_BMP1_TKA",
	    "CUP_O_BMP2_TKA",
	    "CUP_O_T72_SLA",
	    "CUP_O_T34_TKA",
	    "CUP_O_T55_TK"
        ]];
    };
    private _pos    = position _trigger;
    private _triggerAreaSpec = triggerArea _trigger;
    private _radius = ((_triggerAreaSpec select 0) min
		       (_triggerAreaSpec select 1));
    private _finder = ["LocationFinder", 5 max (1.6*(_count))] call fnc_new;
    [_finder, "configure", [["center", [_pos]],
			    ["radius", _radius * (0.408 + (random 0.42))],
			    ["allowSingleBins", false]]
    ] call fnc_tell;
    private _assignments = [["trigger", _trigger]];
    private _objectives = [
	[true,  0,    1,    '[_x]',
	 [["_x"], {
	     private _trig = _x getVariable "trigger";
	     if (_x inArea _trig) then { 1 } else { 0 };
	 }] call fnc_lambda  ] call fnc_to_cost_function,
	[true,  0.15, 0.75, '[_x, 1.5, 2, 0.4]',
	 fnc_check_los_grid  ] call fnc_to_cost_function,
	OPT_fnc_surface_is_not_water,
	OPT_fnc_distance_to_roads
    ];
    if (not isNil "_observers") then {
	_assignments = _assignments + [["targets", _observers]];
	_objectives = _objectives + [
	    OPT_fnc_LOS_to_targets,
	    OPT_fnc_partial_LOS_to_targets,
	    OPT_fnc_distance_from_targets,
	    OPT_fnc_long_distance_to_targets
	];
    };
    if (not isNil "_avoid") then {
	_assignments = _assignments + [["avoid", _avoid]];
	_objectives = _objectives + [
	    [false, 0, 1,
	     '[_x, _x getVariable "avoid", 1.6, true]',
	     fnc_LOS_to_array] call fnc_to_cost_function,
	    [true, 25, 50,
	     '[position _x,
               ([[["_y"], {position _y}] call fnc_lambda,
	         _x getVariable "avoid"] call fnc_map)
	        call fnc_vector_mean]',
	     fnc_euclidean_distance] call fnc_to_cost_function
	];
    };
    [_finder, "configure",
     [["objectives", _objectives],
      ["assignments", _assignments]]] call fnc_tell;
    _self setVariable ["finder", _finder];
    _self setVariable ["count", _count];
    _self setVariable ["trigger", _trigger];
    _self
} ENDCLASSV;

DEFMETHOD("VehicleTargets", "get_logics") ["_self"] DO {
    private _logics = [];
    private _count = _self getVariable "count";
    private _trigger = _self getVariable "trigger";
    while { (count _logics) < _count } do {
	_logics = _logics +
	    ([_self getVariable "finder", "run",
	      _count - (count _logics), 3] call fnc_tell);
	_logics = [[["_logic"], {
	             private _check = 
	               (({
			   ([position _x, position _logic]
			    call fnc_euclidean_distance) < 7
		        } count _logics) < 2) and
	               (_logic inArea _trigger);
	             if (not _check) then {
			 [_logic, "hide"] call fnc_tell;
			 deleteVehicle _logic;
		     };
	             _check
                  }] call fnc_lambda,
		   _logics] call fnc_filter;
    };
    if (_count < (count _logics)) then {
	{
	    [_x, "hide"] call fnc_tell;
	    deleteVehicle _x;
	} forEach ([_logics, _count, 0] call fnc_subseq);
    };
    ([_logics, 0, _count] call fnc_subseq)
} ENDMETHOD;

DEFMETHOD("VehicleTargets", "spawn") ["_self", "_alive", "_priority"] DO {
    private ["_logic", "_position", "_losA", "_losB", "_vehicle", "_target"];
    if (isNil "_alive"   ) then {    _alive = false };
    if (isNil "_priority") then { _priority = true  };
    private _logics = [_self, "get_logics"] call fnc_tell;
    private _classes  = _self getVariable "vehicleClasses";
    private _vehicles = [];
    private _toSpawn = [];
    {
	_logic = _x;
	private _targets = _logic getVariable "targets";
	if (not isNil "_targets") then { // Sort them by which one to face:
	    _targets = [_targets, [["_a", "_b"], {
		_losA = [_a, [_logic], 1.6, false] call fnc_LOS_to_array;
		_losB = [_b, [_logic], 1.6, false] call fnc_LOS_to_array;
		if (_priority) then { // Return the first (with any LOS):
		    if (((_losA <= 0) and (_losB <= 0)) or
			((0 < _losA) and (0 < _losB))) then {
			false
		    } else {
			(_losB < _losA)
		    };
		} else { // Return the best LOS, or first if none:
		    if ((_losA <= 0) and (_losB <= 0)) then {
			false
		    } else {
			(_losB < _losA)
		    };
		};
	    }] call fnc_lambda] call fnc_sorted;
	    _target = _targets select 0;
	} else {
	    _target = objNull;
	};
	_toSpawn = _toSpawn + [[position _logic, _target]];
	[_logic, "hide"] call fnc_tell;
	deleteVehicle _logic;
    } forEach _logics;
    {
	_position = _x select 0;
	_position = [_position, 0, 10, 5, 0, 0.25, 0, [], [_position]]
                     call BIS_fnc_findSafePos;
	_target   = _x select 1;
	_vehicle  = ((_classes select ([0, count _classes] call fnc_randint))
		     createVehicle _position);
	if (isNull _target) then {
	    _vehicle setDir (random 360);
        } else {
	    _vehicle setDir (((_vehicle getDir _target) - 5.625)
			     + (random 11.25));
	};
	if (_alive) then {
	    createVehicleCrew _vehicle;
	    _vehicle allowCrewInImmobile true;
	    if (not isNull _target) then {
		_vehicle doWatch _target;
	    };
	};
	_vehicle engineOn true;
	_vehicles = _vehicles + [_vehicle];
    } forEach _toSpawn;
    _vehicles
} ENDMETHODV;
