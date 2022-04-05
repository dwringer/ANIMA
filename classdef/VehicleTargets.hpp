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
			    ["radius", _radius],
			    ["allowSingleBins", true]]
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
			    call fnc_euclidean_distance) < 5
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

DEFMETHOD("VehicleTargets", "spawn") ["_self", "_alive"] DO {
    private ["_logic", "_losA", "_losB", "_classes", "_vehicle"];
    if (isNil "_alive") then { _alive = false };
    private _logics = [_self, "get_logics"] call fnc_tell;
    private _vehicles = [];
    {
	_logic = _x;
	_classes  = _self getVariable "vehicleClasses";
	_vehicle  = ((_classes select ([0, count _classes] call fnc_randint))
		     createVehicle (position _logic));
	private _targets = _logic getVariable "targets";
	if (not isNil "_targets") then {
	    _targets = [_targets, [["_a", "_b"], {
		_losA = [_a, [_logic], 1.6, false] call fnc_LOS_to_array;
		_losB = [_b, [_logic], 1.6, false] call fnc_LOS_to_array;
		if (((_losA <= 0) and (_losB <= 0)) or
		    ((0 < _losA) and (0 < _losB))) then {
	            (_logic distance _a) < (_logic distance _b)
		} else {
		    (_losB < _losA)
		};
	    }] call fnc_lambda] call fnc_sorted;
	    _vehicle setDir (((_vehicle getDir (_targets select 0)) - 22.5)
			     + (random 45));
	} else {
	    _vehicle setDir (random 360);
	};
	if (_alive) then {
	    createVehicleCrew _vehicle;
	    _vehicle allowCrewInImmobile true;
	};
	_vehicle engineOn true;
	_vehicles = _vehicles + [_vehicle];
	[_logic, "hide"] call fnc_tell;
	deleteVehicle _logic;
    } forEach _logics;
} ENDMETHODV;
