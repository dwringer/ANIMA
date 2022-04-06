/*
  CrewUnitGroup class 
    :: CrewUnitGroup -> UnitGroup -> ObjectRoot

  Methods:
    assign unit role  :: assign unit or vehicle to crew in specified role
    auto_assign units :: assign vehicles and units automatically
    board             :: assignAs- and orderGetIn for all units
    board_instant     :: instantly assignAs- and moveIn- all designated units 
    is_serviceable    :: return whether crew has vehicle(s) and driver(s) alive

      This class is used to represent a dynamic vehicle crew (and any assigned
  vehicles).  Add units to the group, assign one or more vehicles, and,
  optionally, set one or more specific unit position assignments.  Then, the
  group can be commanded to board, unload, or any number of other tasks (TBD)

  Example:
    CG = ["CrewUnitGroup"] call fnc_new;
    [CG, "assign", player, "driver"] call fnc_tell;
    [CG, "assign", gunman, "gunner"] call fnc_tell;
    [CG, "assign", carman, "cargo"] call fnc_tell;
    [CG, "assign", car, "vehicle"] call fnc_tell;
    [CG, "board_instant"] call fnc_tell;
    // the designated units are now ordered and moved into the vehicle

*/

#define ROLE_ARRAY_ALIST [["vehicle", "vehicles"],  \
			  ["driver", "drivers"],  \
			  ["gunner", "gunners"],  \
			  ["cargo", "cargo"]]


DEFCLASS("CrewUnitGroup") ["_self"] DO {
	/* Instantiate crew and prepare role arrays */
	SUPER("UnitGroup", _self);
	[[["_pair", "_obj"], {
                [_obj, "_setf", _pair select 1, []] call fnc_tell
         }] call fnc_lambda,
 	 ROLE_ARRAY_ALIST,
	 [_self]] call fnc_mapwith;
        _self
} ENDCLASS;


DEFMETHOD("CrewUnitGroup", "assign") ["_self", "_unit", "_role"] DO {
	/* Add unit to group (if not member) and add to role array */
	private ["_aname", "_arr"];
	if ((({_x == _unit}
              count ([_self, "_getf", "units"] call fnc_tell)) == 0)) then {
		[_self, "add", _unit] call fnc_tell;			
	};
	for "_i" from 0 to ((count ROLE_ARRAY_ALIST) - 1) do {
		scopeName "removeCurrentAssignment";
		_aname = (ROLE_ARRAY_ALIST select _i) select 1;
		_arr = [_self, "_getf", _aname] call fnc_tell;
                if ({_unit == _x} count _arr > 0) then {
                        _arr = _arr - [_unit];
		        [_self, "_setf", _aname, _arr] call fnc_tell;
		        breakOut "removeCurrentAssignment";
                };
	};
	[_self, "_push_attr",
	 [_role, ROLE_ARRAY_ALIST] call fnc_alist_get,
	 _unit] call fnc_tell
} ENDMETHOD;

DEFMETHOD("CrewUnitGroup", "init_units") ["_self", "_units"] DO {
  private ["_vehicle"];
  private _vehicles = [];
  private _men = [];
  {
    _vehicle = vehicle _x;
    if (((_vehicle isKindOf "LandVehicle") or
	 (_vehicle isKindOf "Air")) and
	(not (_vehicle in _vehicles))) then {
	_vehicles = _vehicles + [_vehicle];
    } else {
        _men = _men + [_vehicle];
    };
  } forEach _units;
  // Now sort all men by leadership status/rank:
  _men = [[["_pair"], { _pair select 0 }] call fnc_lambda,
          [[[["_x"], {
              private ["_n"];
	      if (_x == (leader (group _x))) then {
	        _n = -1
	      } else {
	        switch (rank _x) do {
		  case "PRIVATE":    { _n = 6 };
		  case "CORPORAL":   { _n = 5 };
	          case "SERGEANT":   { _n = 4 };
	          case "LIEUTENANT": { _n = 3 };
	          case "CAPTAIN":    { _n = 2 };
	          case "MAJOR":      { _n = 1 };
		  case "COLONEL":    { _n = 0 };
	        };
	      };
              [_x, _n]
	    }] call fnc_lambda,
	    _men] call fnc_map,
	   [["_a", "_b"], {
	     (_a select 1) < (_b select 1)
	   }] call fnc_lambda] call fnc_sorted] call fnc_map;
  _self setVariable ["men", _men];
  _self setVariable ["vehicles", _vehicles];
} ENDMETHOD;

DEFMETHOD("CrewUnitGroup", "make_assignments") ["_self", "_asPassengers"] DO {
  private ["_vehicle", "_slots", "_fn_slot", "_crewTurrets"];
  private _men = _self getVariable "men";
  private _vehicles = _self getVariable "vehicles";
  private _commandedVehicles = [];
  private _commanderSlots    = [];
  private _gunnerSlots       = [];
  private _turretSlots       = [];
  private _personTurretSlots = [];
  private _cargoSlots        = [];
  if (isNil "_asPassengers") then {
    _asPassengers = false;
  };
  {
    _vehicle = _x;
    _crewTurrets = [_vehicle call BIS_fnc_vehicleCrewTurrets,
		    1, 0] call fnc_subseq;
    _fn_slot = [["_obj", "_type", "_cidx", "_tpath", "_pturret", "_aidx"], {
      private _entry = [_vehicle, _cidx, _aidx, _tpath];
      switch (_type) do {
	  case "commander": {
	    if (_vehicle in _commandedVehicles) then {
		if (_pturret) then {
		    _personTurretSlots = _personTurretSlots + [_entry];
		} else {
		    if (not _asPassengers) then {
			if (_tpath in _crewTurrets) then {
			    _turretSlots     = _turretSlots       + [_entry];
			    _crewTurrets = _crewTurrets - [_tpath];
			};
		    };
		};
	    } else {
		if (not _asPassengers) then {
		    _commanderSlots = _commanderSlots + [_entry];
		    if (_tpath in _crewTurrets) then {
			_crewTurrets = _crewTurrets - [_tpath];
		    };
		};
		_commandedVehicles = _commandedVehicles + [_vehicle];
	    };
	  };
          case "gunner":    {
	    if (not _asPassengers) then {
	      if (_tpath in _crewTurrets) then {
		_gunnerSlots = _gunnerSlots    + [_entry];
		_crewTurrets = _crewTurrets - [_tpath];
	      };
//		    else {
//		_gunnerSlots = _gunnerSlots +
//		  [[_vehicle, _cidx, _aidx, _crewTurrets select 0]];
//		_crewTurrets = [_crewTurrets, 1, 0] call fnc_subseq;
//	      };
	    };
	  };
	  case "turret":    {
	    if (_pturret) then {
	      _personTurretSlots = _personTurretSlots + [_entry];
	    } else {
	      if (not _asPassengers) then {
		if (_tpath in _crewTurrets) then {
		  _turretSlots     = _turretSlots       + [_entry];
		  _crewTurrets = _crewTurrets - [_tpath];
		};
	      };
	    };
	  };
          case "cargo":     {     _cargoSlots = _cargoSlots     + [_entry] };
      };
    }] call fnc_lambda;
    // _slots = fullCrew [_vehicle, "", true];
    // // Cargo-filtered slots need to be first so we can count action indices:
    // private _filteredSlots = fullCrew [_vehicle, "cargo", true];
    // _slots = _filteredSlots + (_slots - _filteredSlots);
    _slots = ((fullCrew [_vehicle, "cargo", true]) +
	      (fullCrew [_vehicle, "driver", true]) +
              (fullCrew [_vehicle, "commander", true]) +
	      (fullCrew [_vehicle, "gunner", true]) +
	      (fullCrew [_vehicle, "turret", true]));
    for "_i" from 0 to ((count _slots) - 1) do {
      ((_slots select _i)+[_i]) call _fn_slot;
    };
  } forEach _vehicles;
  private _commanders = [];
  if (0 < (count _commanderSlots)) then {
      _commanders = [_men, 0, count _commanderSlots] call fnc_subseq;
  };
  private _remain = [_men, count _commanders, count _men] call fnc_subseq;
  private _drivers = [];
  if (not _asPassengers) then {
    _drivers = [_remain + _commanders,
		0, count _vehicles] call fnc_subseq;
    _remain = _remain - _drivers;
  };
  private _gunners = [];
  if ((count _gunnerSlots) < ((count _remain) + (count _commanders))) then {
    if (0 < (count _gunnerSlots)) then {
      _gunners = [_remain + _commanders, 0, count _gunnerSlots] call fnc_subseq;
    };
  } else {
    _gunners = _remain + _commanders;
  };
  _remain = _remain - _gunners;
  _commanders = (_commanders - _gunners) - _drivers;
  if ((count _commanderSlots) < (count _commanders)) then {
    _remain = ([_commanders, count _commanderSlots, 0] call fnc_subseq)
      + _remain;
    if (0 < (count _commanderSlots)) then {
      _commanders = [_commanders, 0, count _commanderSlots] call fnc_subseq;
    } else {
      _commanders = [];
    };
  };
   private _turrets = [];
  if ((count _turretSlots) < (count _remain)) then {
    if (0 < (count _turretSlots)) then {
      _turrets = [_remain, 0, count _turretSlots] call fnc_subseq;
    };  
  } else {
    _turrets = _remain;
  };
  _remain = _remain - _turrets;
  private _personTurrets = [];
  if ((count _personTurretSlots) < (count _remain)) then {
    if (0 < (count _personTurretSlots)) then {
      _personTurrets = [_remain, 0, count _personTurretSlots] call fnc_subseq;
    };
  } else {
    _personTurrets = _remain;
  };
  _remain = _remain - _personTurrets;
  private _cargo = [];
  if ((count _cargoSlots) < (count _remain)) then {
    if (0 < (count _cargoSlots)) then {
      _cargo = [_remain, 0, count _cargoSlots] call fnc_subseq;
    };
  } else {
    _cargo = _remain;
  };
  _remain = _remain - _cargo;
  [["commanders", [[["_a", "_b"], {[_a, _b]}] call fnc_lambda,
		  _commanders, _commanderSlots] call fnc_map],
   ["drivers", [[["_a", "_b"], {[_a, [_b, -1, -1, []]]}] call fnc_lambda,
	       _drivers, _vehicles] call fnc_map],
   ["gunners", [[["_a", "_b"], {[_a, _b]}] call fnc_lambda,
	       _gunners, _gunnerSlots] call fnc_map],
   ["turrets", [[["_a", "_b"], {[_a, _b]}] call fnc_lambda,
	       _turrets, _turretSlots] call fnc_map],
   ["personTurrets", [[["_a", "_b"], {[_a, _b]}] call fnc_lambda,
		     _personTurrets, _personTurretSlots] call fnc_map],
   ["cargo", [[["_a", "_b"], {[_a, _b]}] call fnc_lambda,
	     _cargo, _cargoSlots] call fnc_map]
   //   ["remain", _remain]
   ]
} ENDMETHODV;

DEFMETHOD("CrewUnitGroup", "assignments_from") ["_self",
						"_units",
						"_asPassengers"] DO {
  if (isNil "_asPassengers") then { _asPassengers = false };
  [_self, "init_units", _units] call fnc_tell;
  ([_self, "make_assignments", _asPassengers] call fnc_tell)
} ENDMETHODV;

DEFMETHOD("CrewUnitGroup", "mount_up") ["_self", "_asPassengers"] DO {
  private ["_type", "_maps", "_map", "_unit", "_mapping", "_vehicle",
	   "_cargoIndex", "_cargoActionIndex", "_turretPath"];
  if (isNil "_asPassengers") then { _asPassengers = false };
  private _assignments = [_self, "make_assignments",
			  _asPassengers] call fnc_tell;
  private _assigned = [];
  {
    _type  = _x select 0;
    _maps  = _x select 1;
    for "_i" from 0 to ((count _maps) - 1) do {
      _map = _maps select _i;
      _unit    = _map select 0;
      _mapping = _map select 1;
      _vehicle          = _mapping select 0;
      _cargoIndex       = _mapping select 1;
      _cargoActionIndex = _mapping select 2;
      _turretPath       = _mapping select 3;
      switch (_type) do {
	  case "commanders": { _unit assignAsCommander _vehicle };
	  case "drivers": { _unit assignAsDriver _vehicle };
	  case "gunners";
          case "turrets": { _unit assignAsTurret [_vehicle, _turretPath] };
          case "personTurrets": {
            private _thread = [_unit, _vehicle, _turretPath, _cargoActionIndex]
	                       spawn {
              params ["_unit", "_vehicle", "_turretPath", "_cargoActionIndex"];
	      while { 8 <= (_unit distance _vehicle) } do {
		_unit doMove (position _vehicle);
		waitUntil { sleep 0.5;
		  (((_unit distance _vehicle) < 8) or
			     ((speed _unit) == 0))
		};
	      };
	      [_unit] allowGetIn true;
	      _unit action ["getInTurret", _vehicle, _turretPath];
	      _unit assignAsTurret [_vehicle, _turretPath];
	    };
	  };
	  case "cargo": {
	    private _thread = [_unit, _vehicle, _cargoIndex, _cargoActionIndex]
	                       spawn {
              params ["_unit", "_vehicle", "_cargoIndex", "_cargoActionIndex"];
	      while { 8 <= (_unit distance _vehicle) } do {
		_unit doMove (position _vehicle);
		waitUntil { sleep 0.5;
		            (((_unit distance _vehicle) < 8) or
			     ((speed _unit) == 0))
		};
	      };
	      [_unit] allowGetIn true;
	      _unit assignAsCargo _vehicle;
	      _unit action ["getInCargo", _vehicle, _cargoActionIndex];
	    };
	  };
      };
      _assigned = _assigned + [_unit];
    };
  } forEach _assignments;
  _assigned orderGetIn true;
} ENDMETHODV;

DEFMETHOD("CrewUnitGroup", "mount_instant") ["_self", "_asPassengers"] DO {
  private ["_type", "_maps", "_map", "_unit", "_mapping", "_vehicle",
	   "_cargoIndex", "_cargoActionIndex", "_turretPath"];
  if (isNil "_asPassengers") then { _asPassengers = false };
  private _assignments = [_self, "make_assignments",
			  _asPassengers] call fnc_tell;
  {
    _type  = _x select 0;
    _maps  = _x select 1;
    for "_i" from 0 to ((count _maps) - 1) do {
      _map = _maps select _i;
      _unit    = _map select 0;
      _mapping = _map select 1;
      _vehicle          = _mapping select 0;
      _cargoIndex       = _mapping select 1;
      _cargoActionIndex = _mapping select 2;
      _turretPath       = _mapping select 3;
      switch (_type) do {
	  case "commanders": {
	      _unit assignAsCommander _vehicle;
	      _unit moveInCommander   _vehicle;
	      _vehicle setEffectiveCommander _unit;
	  };
	  case "drivers": {    _unit assignAsDriver    _vehicle;
                  	       _unit moveInDriver      _vehicle; };
          case "gunners";
          case "turrets";
	  case "personTurrets": {
	    _unit assignAsTurret [_vehicle, _turretPath];
	    _unit moveInTurret   [_vehicle, _turretPath];
	  };
	  case "cargo": {      _unit assignAsCargo     _vehicle;
                               _unit moveInCargo       _vehicle; };
      };
    };
  } forEach _assignments;
} ENDMETHOD;

DEFMETHOD("CrewUnitGroup", "auto_assign") ["_self", "_units"] DO {
	/* Automatically assign roles to array of units + vehicle(s) */
	private ["_vehicles", "_drivers", "_gunners", "_unit", "_vehicle",
		 "_vehicleIndex"];
	_vehicles = [];
	_drivers = [];
	_gunners = [];
	for "_i" from 0 to ((count _units) - 1) do {
		_unit = _units select _i;
		if ((_unit emptyPositions "driver") > 0) then {
			_vehicles = _vehicles + [_unit];
		};
	};
	_units = _units - _vehicles;
	for "_i" from 0 to ((count _vehicles) - 1) do {
		if (_i < (count _units)) then {
			_drivers = _drivers + [_units select _i];
		};
	};
	_units = _units - _drivers;
	_vehicleIndex = 0;
	while {((count _units) > 0) and
	       (_vehicleIndex < (count _vehicles))} do {
		_vehicle = _vehicles select _vehicleIndex;
		for "_i" from 0 to ((count (allTurrets _vehicle)) - 1) do {
			if (_i <= (count _units)) then {
				_gunners = _gunners + [_units select _i];
			};
		};
		_units = _units - _gunners;
		_vehicleIndex = _vehicleIndex + 1;
	};
	[[["_arr", "_role", "_cg"], {
		if (((count _arr) > 0) and
		    (not isNil "_arr")) then {
			[[["_u", "_r", "_g"], {
				if ((not isNil "_u") and
				    (not isNil "_r") and
				    (not isNil "_g")) then {
					[_g, "assign", _u, _r] call fnc_tell;
				};
			 }] call fnc_lambda,
			 _arr,
			 [_role, _cg]] call fnc_mapwith;
		};
         }] call fnc_lambda,
	 [_vehicles, _drivers, _gunners, _units],
	 ["vehicle", "driver", "gunner", "cargo"],
	 [_self]] call fnc_mapwith;
} ENDMETHOD;

DEFMETHOD("CrewUnitGroup", "init_in_place") ["_self", "_men"] DO {
  private ["_man", "_vehicle", "_vehicles", "_drivers", "_gunners", "_cargo", "_data", "_unit", "_role"];
  _vehicles = [];
  _drivers  = [];
  _gunners  = [];
  _cargo    = [];
  for "_i" from 0 to ((count _men) - 1) do {
      _man  = _men select _i;
      _vehicle = vehicle _man;
      if (not (_vehicle in _vehicles)) then {_vehicles append [_vehicle]};
  };
  for "_i" from 0 to ((count _vehicles) - 1) do {
    _vehicle = _vehicles select _i;
    _crew = fullCrew _vehicle;
    for "_i" from 0 to ((count _crew) - 1) do {
	_data = _crew select _i;
	_unit = _data select 0;
	if (not (isNull _unit)) then {
	    _role = _data select 1;
	    switch (_role) do {
		case "driver": {
		  _drivers append [_unit];
		};
	        case "gunner": {
		  _gunners append [_unit];
		};
                case "turret": {
		  _gunners append [_unit];
		};
		default {
		  _cargo append [_unit];
		};
	      };
	  };
    };
  };
  [[["_arr", "_role", "_cg"], {
	if (((count _arr) > 0) and
	    (not isNil "_arr")) then {
	    [[["_u", "_r", "_g"], {
		  if ((not isNil "_u") and
		      (not isNil "_r") and
		      (not isNil "_g")) then {
		      [_g, "assign", _u, _r] call fnc_tell;
		    };
		}] call fnc_lambda,
	      _arr,
	      [_role, _cg]] call fnc_mapwith;
	  };
      }] call fnc_lambda,
    [_vehicles, _drivers, _gunners, _cargo],
    ["vehicle", "driver", "gunner", "cargo"],
    [_self]] call fnc_mapwith;
  
    [_self, "board_instant"] call fnc_tell;
  
} ENDMETHOD;


thread_turret_monitor = [["_man", "_vehicle"], {
	/* Watch stored list of gunners and man turret once all are dead */
	waitUntil {([[["_a", "_b"], {_a * _b}] call fnc_lambda,
 		     [[["_x"], {
			     if (isNil "_x") then {1} else {
				     if (alive _x) then {0} else {1}
			     }
		      }] call fnc_lambda,
		     _man getVariable "monitored"] call fnc_map
		    ] call fnc_reduce) == 1
	};
	sleep 2 + (random 2);
	if (not (_man in (crew _vehicle))) then {
		_man doMove (position _vehicle);
		waitUntil {(_man distance _vehicle) < 8};
		_man assignAsGunner _vehicle;
		[_man] orderGetIn true;
	} else {
		doGetOut _man;
		waitUntil {not (_man in (crew _vehicle))};
		_man assignAsGunner _vehicle;
		_man moveInGunner _vehicle;
	};
}] call fnc_lambda;


DEFMETHOD("CrewUnitGroup", "board") ["_self"] DO {
	/* Move to vehicle(s) and embark */
	private ["_vehicles", "_drivers", "_gunners", "_cargos", "_positions",
             	 "_vlen", "_vehicle", "_driver", "_gunner", "_cargo",
          	 "_turret", "_turretPositions", "_turrets", "_cargoSeats",
	         "_assignmentIndex", "_monitored", "_monitor"];
	_vehicles = [_self, "_getf", "vehicles"] call fnc_tell;
	_drivers = [_self, "_getf", "drivers"] call fnc_tell;
	_gunners = [_self, "_getf", "gunners"] call fnc_tell;
	_cargos = [_self, "_getf", "cargo"] call fnc_tell;
	_gunners = _gunners + _cargos;
	_vlen = count _vehicles;
	_cargoSeats = [];
	for "_i" from 0 to (_vlen - 1) do {
		scopeName "vehicleBoarding";
		_vehicle = _vehicles select _i;
		_positions = fullCrew [_vehicle, "", true];
		_driver = _drivers select 0;
		_drivers = _drivers - [_driver];
		_driver assignAsDriver _vehicle;
		[_driver] orderGetIn true;
		_turrets = allTurrets _vehicle;
		_turretPositions = [];
		for "_i" from 0 to ((count _turrets) - 1) do {
			_turret = _turrets select _i;
			if ((count _turret) == 1) then {
				_turretPositions = _turretPositions + [[_i]];
			} else {
				for "_j" from 0 to ((count _turret) - 1) do {
					_turretPositions = _turretPositions +
							   [[_i, _j]];
				};
			};
		};
		_assignmentIndex = 0;		
		while {_assignmentIndex < ((count _gunners) min
					   (count _turretPositions))} do {
			(_gunners select _assignmentIndex) assignAsTurret
	                         [_vehicle,
				  _turretPositions select _assignmentIndex];
			[_gunners select _assignmentIndex] orderGetIn true;
			_assignmentIndex = _assignmentIndex + 1;
		};
		_gunners = [_gunners, _assignmentIndex, 0] call fnc_subseq;
		_cargoSeats = _cargoSeats +
		              [(count _positions) - (_assignmentIndex + 1)];
		if (((count _drivers) == 0) and
                    (_i < (_vlen - 1)) and
		    ((count _gunners) > 0)) then {
			_drivers = _drivers + [_gunners select 0];
			_gunners = [_gunners, 1, 0] call fnc_subseq;
		};
		if ((count _drivers) == 0) then {
			breakOut "vehicleBoarding";
		};
        };
	for "_i" from 0 to (_vlen - 1) do {
		if ((count _cargoSeats) > _i) then {
			_vehicle = _vehicles select _i;
			_monitored = [gunner _vehicle];
			for "_j" from 0 to ((_cargoSeats select _i) - 1) do {
				if ((count _gunners) > 0) then {
					_cargo = _gunners select 0;
					_cargo setVariable ["monitored",
							    _monitored];
					_cargo assignAsCargo _vehicle;
					[_cargo] orderGetIn true;
					_gunners = [_gunners, 1, 0] call
					           fnc_subseq;
					_monitored = _monitored + [_cargo];
					_monitor = [_cargo, _vehicle] spawn
						thread_turret_monitor;
					_cargo setVariable ["monitor",
							    _monitor];
				};
		        };
		};
	};
} ENDMETHOD;


DEFMETHOD("CrewUnitGroup", "board_instant") ["_self"] DO {
	/* Instantly embark */
	private ["_vehicles", "_drivers", "_gunners", "_cargos", "_positions",
             	 "_vlen", "_vehicle", "_driver", "_gunner", "_cargo",
          	 "_turret", "_turretPositions", "_turrets", "_cargoSeats",
	         "_assignmentIndex", "_monitored", "_monitor"];
	_vehicles = [_self, "_getf", "vehicles"] call fnc_tell;
	_drivers = [_self, "_getf", "drivers"] call fnc_tell;
	_gunners = [_self, "_getf", "gunners"] call fnc_tell;
	_cargos = [_self, "_getf", "cargo"] call fnc_tell;
	_gunners = _gunners + _cargos;
	_vlen = count _vehicles;
	_cargoSeats = [];
	for "_i" from 0 to (_vlen - 1) do {
		scopeName "vehicleBoarding";
		_vehicle = _vehicles select _i;
		_positions = fullCrew [_vehicle, "", true];
		_driver = _drivers select 0;
		_drivers = _drivers - [_driver];
		_driver assignAsDriver _vehicle;
		_driver moveInDriver _vehicle;
		_turrets = allTurrets _vehicle;
		_turretPositions = [];
		for "_i" from 0 to ((count _turrets) - 1) do {
			_turret = _turrets select _i;
			if ((count _turret) == 1) then {
				_turretPositions = _turretPositions + [[_i]];
			} else {
				for "_j" from 0 to ((count _turret) - 1) do {
					_turretPositions = _turretPositions +
							   [[_i, _j]];
				};
			};
		};
		_assignmentIndex = 0;
		while {_assignmentIndex < ((count _gunners) min
					   (count _turretPositions))} do {
			(_gunners select _assignmentIndex) moveInTurret
			[_vehicle, (_turretPositions select _assignmentIndex)];
			_assignmentIndex = _assignmentIndex + 1;
		};
		_gunners = [_gunners, _assignmentIndex, 0] call fnc_subseq;
		_cargoSeats = _cargoSeats +
		              [(count _positions) - (_assignmentIndex + 1)];
		if (((count _drivers) == 0) and
                    (_i < (_vlen - 1)) and
		    ((count _gunners) > 0)) then {
			_drivers = _drivers + [_gunners select 0];
			_gunners = [_gunners, 1, 0] call fnc_subseq;
		};
		if ((count _drivers) == 0) then {
			breakOut "vehicleBoarding";
		};
        };
	for "_i" from 0 to (_vlen - 1) do {
		if ((count _cargoSeats) > _i) then {
			_vehicle = _vehicles select _i;
			_monitored = [gunner _vehicle];
			for "_j" from 0 to ((_cargoSeats select _i) - 1) do {
				if ((count _gunners) > 0) then {
					_cargo = _gunners select 0;
					_cargo setVariable ["monitored",
							    _monitored];
					_cargo assignAsCargo _vehicle;
					_cargo moveInCargo _vehicle;
					_gunners = [_gunners, 1, 0]
						    call fnc_subseq;
					_monitored = _monitored + [_cargo];
					_monitor = [_cargo, _vehicle] spawn
 						    thread_turret_monitor;
					_cargo setVariable ["monitor",
							    _monitor];
				};
		        };
		};
	};
} ENDMETHOD;


DEFMETHOD("CrewUnitGroup", "is_serviceable") ["_self"] DO {
	/* Return whether the group can be considered at all serviceable */
	private ["_sequestered", "_result"];
	if (({canMove _x} count (_self getVariable "vehicles")) == 0) then {
		false
	} else {
		_sequestered = _self getVariable "sequestered";
		if (isNil "_sequestered") then {_sequestered = false;};
		if (_sequestered) then {
			_result = (count (_self getVariable "drivers")) >=
				  ({canMove _x}
                                    count (_self getVariable "vehicles"));
		} else {
			if (({alive _x}
                              count (_self getVariable "drivers")) <
			    ({canMove _x}
                              count (_self getVariable "vehicles"))) then {
				[_self, "auto_assign",
				 _self getVariable "units"] call fnc_tell;
			};
			_result = ({alive _x}
                                    count (_self getVariable "drivers")) >=
				  ({canMove _x}
                                    count (_self getVariable "vehicles"));
		};
		_result
	}
} ENDMETHOD;
