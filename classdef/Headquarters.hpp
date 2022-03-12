DEFCLASS("Headquarters") (["_self", "_track_units"]) DO {
	_self setVariable ["_tracked", []];
	_self setVariable ["_groups", []];
	_self setVariable ["_dispatched", []];
	_self setVariable ["_vehicles", []];
	_self setVariable ["_air", []];
	_self setVariable ["_men", []];
	_self setVariable ["_transports", []];
	_self setVariable ["_passengers", []];
	_self setVariable ["_pairs", []];
	_self setVariable ["_armory", ["Armory"] call fnc_new];
	if (not (isNil "_track_units")) then {
		[_self, "track", _track_units] call fnc_tell;
	};
	_self
} ENDCLASS;

DEFMETHOD("Headquarters", "track") ["_self", "_units"] DO {
	private ["_group", "_groups", "_vehicles", "_air", "_men",
		 "_transports", "_passengers", "_pairs", "_unitKind",
		 "_fn_getKindOf", "_fn_registerCrew"];
	_fn_getKindOf = [["_x"], {  // TODO: if x is kindof y then y (map)
	    if (_x isKindOf "Man") then { "Man" } else {
		if (_x isKindOf "Land") then { "Land" } else {
	    	    if (_x isKindOf "Ship") then { "Ship" } else {
			if (_x isKindOf "Air") then { "Air" }}}}
	}] call fnc_lambda;
	_groups     = _self getVariable "_groups";
	_tracked    = _self getVariable "_tracked";
	_vehicles   = _self getVariable "_vehicles";
	_air        = _self getVariable "_air";
	_men        = _self getVariable "_men";
	_transports = _self getVariable "_transports";
	_passengers = _self getVariable "_passengers";
	_pairs      = _self getVariable "_pairs";
	{
		_group = group _x;
		_groups = _groups - [_group];
		_groups append [_group];
		_unitKind = [_x] call _fn_getKindOf;
		_fn_registerCrew = [["_crewMemberArray"], {
					private ["_unit", "_unitGroup", "_role"];
					_unit      = _crewMemberArray select 0;
					_unitGroup = group _unit;
					_role      = _crewMemberArray select 1;
					_men = _men - [_unit];
					_men append [_unit];
					if (not (_unitGroup == _group)) then {
						_groups = _groups - [_unitGroup];
						_groups append [_unitGroup];
						_passengers = _passengers - [_unitGroup];
						_passengers append [_unitGroup];
						_transports = _transports - [_group];
						_transports append [_group];
						_pairs = _pairs - [[_group, _unitGroup]];
						_pairs append [[_group, _unitGroup]];
					};
				   }] call fnc_lambda;
		switch (_unitKind) do {
			case "Man": {
				_men = _men - [_x];
				_men append [_x];
			};
			case  "Air": {
				_air = _air - [_x];
				_air append [_x]; // TODO: landVehicles + vehicles, more air fns
				[_fn_registerCrew, fullCrew _x] call fnc_map;
			};
			case "Land": {
				_vehicles = _vehicles - [_x];
				_vehicles append [_x];
				[_fn_registerCrew, fullCrew _x] call fnc_map;
			};
			case "Ship": { };
			default      { };
		};
	} forEach _units;
	_tracked = _tracked - _units;
	_tracked append _units;
	_self setVariable ["_side",       side (_men select 0)];
	_self setVariable ["_groups",	  _groups];
	_self setVariable ["_tracked",	  _tracked];
	_self setVariable ["_vehicles",	  _vehicles];
	_self setVariable ["_air",	  _air];
	_self setVariable ["_men",	  _men];
	_self setVariable ["_transports", _transports];
	_self setVariable ["_passengers", _passengers];
	_self setVariable ["_pairs",      _pairs];
} ENDMETHOD;

DEFMETHOD("Headquarters", "untrack") ["_self", "_units"] DO {
	private ["_tracked"];
	_tracked = _self getVariable "_tracked";
	_tracked = _tracked - _units;
	_self setVariable ["_groups",  []];
	_self setVariable ["_tracked", []];
	if (0 < (count _tracked)) then {
		[_self, "track", _tracked] call fnc_tell;
	};
} ENDMETHOD;

DEFMETHOD("Headquarters", "dispatch") ["_self", "_n", "_locations", "_wp_config",
				       "_no_transports"] DO {
	private ["_groups", "_dispatched", "_behavior", "_speed", "_transports",
		 "_fn_mean_position", "_fn_add", "_fn_sort", "_fn_reduce_if_exists"];
	if (isNil "_wp_config") then {
		_behavior = "COMBAT";
		_speed    = "NORMAL";
	} else {
		_behavior = "AWARE";
		_speed    = "FULL";
	};
	_fn_mean_position = [["_units"], {
		([[["_x"], {position _x}] call fnc_lambda, _units] call fnc_map)
		call fnc_vector_mean
	}] call fnc_lambda;
	_fn_add = [["_a", "_b"], {_a + _b}] call fnc_lambda;
	_fn_reduce_if_exists = [["_fn", "_arr", "_extra_vars"], {
				 if (isNil "_extra_vars") then { _extra_vars = [] };
				 if (not (isNil "_arr")) then {
					 if (0 < (count _arr)) then {
						 [_fn, _arr, _extra_vars] call fnc_reduce;
					 };
				  };
				}] call fnc_vlambda;
	_locations = [_locations, 0, _n min (count _locations)] call fnc_subseq;
	_fn_sort = [["_a", "_b"], {
		       ([_fn_add,
			 [[["_x"], {([units _a] call _fn_mean_position)
				    distance _x}] call fnc_lambda,
			 _locations] call fnc_map] call _fn_reduce_if_exists) < //fnc_reduce) <
		       ([_fn_add,
			 [[["_x"], {([units _b] call _fn_mean_position)
				    distance _x}] call fnc_lambda,
			 _locations] call fnc_map] call _fn_reduce_if_exists)  // fnc_reduce)
		   }] call fnc_lambda;
	_dispatched = _self getVariable "_dispatched";
	_groups = (_self getVariable "_groups") - _dispatched;
	_groups = ([_groups,     _fn_sort] call fnc_sorted) +
  		  ([_dispatched, _fn_sort] call fnc_sorted);
	if (not (isNil "_no_transports")) then {
		_transports = _self getVariable "_transports";
		_groups = _groups - _transports;
	};
	_groups = [_groups, 0, _n] call fnc_subseq;
	_n = (_n min (count _groups)) min (count _locations);
	_locations = [_locations, 0, _n] call fnc_subseq;	
	[[["_loc", "_grp"], {
		private ["_wp"];
		_wp = ["Waypoint", _behavior, "RED",  15,
		       "Seek & Destroy", 
		       "WEDGE",  0, _loc, _speed, 
		       ["true", ""], 
		       [0, 0, 0], "SAD", true] call fnc_new;
		if ((not (isNil "_wp_config")) and
		    (typeName _wp_config == "ARRAY")) then {
			[_wp, "configure", _wp_config] call fnc_tell;
		};
		if (not (isNil "_grp")) then {
			if (_grp in _dispatched) then {
				 [_wp, "to_group", _grp, -1] call fnc_tell;
			} else {
				_grp setCurrentWaypoint (
				 [_wp, "to_group", _grp, 0] call fnc_tell);
			};
		};
	}] call fnc_lambda,
	_locations,
        _groups] call fnc_map;
	_self setVariable ["_dispatched", _dispatched - _groups + _groups];
} ENDMETHODV;

DEFMETHOD("Headquarters", "dispatch_transports") ["_self", "_n", "_locations", "_wp_config", "_nearest"] DO {
	private ["_groups", "_dispatched", "_behavior",
		 "_speed", "_pairs", "_passengers",
		 "_fn_mean_position", "_fn_add", "_fn_sort",
		 "_fn_reduce_if_exists"];
	if (isNil "_wp_config") then {
		_behavior = "AWARE";
		_speed    = "NORMAL";
	} else {
		_behavior = "CARELESS";
		_speed    = "FULL";
	};
        if (isNil "_nearest") then { _nearest = false; };
	_fn_mean_position = [["_units"], {
		([[["_x"], {position _x}] call fnc_lambda, _units] call fnc_map)
		call fnc_vector_mean
	}] call fnc_lambda;
	_fn_add = [["_a", "_b"], {_a + _b}] call fnc_lambda;
	_fn_sort = [["_a", "_b"], {
		       ([_fn_add,
			 [[["_x"], {([units _a] call _fn_mean_position)
				    distance _x}] call fnc_lambda,
				 _locations] call fnc_map] call _fn_reduce_if_exists) <  //fnc_reduce) <
		       ([_fn_add,
			 [[["_x"], {([units _b] call _fn_mean_position)
				    distance _x}] call fnc_lambda,
			 _locations] call fnc_map] call _fn_reduce_if_exists)  // fnc_reduce)
		   }] call fnc_lambda;
	_fn_reduce_if_exists = [["_fn", "_arr", "_extra_vars"], {
				 if (isNil "_extra_vars") then { _extra_vars = [] };
				 if (not (isNil "_arr")) then {
					 if (0 < (count _arr)) then {
						 [_fn, _arr, _extra_vars] call fnc_reduce;
					 };
				  };
				}] call fnc_vlambda;	
	_dispatched = _self getVariable "_dispatched";
	_pairs  = [_self, "available_pairs"] call fnc_tell;
	_pairs = ([_pairs, [["_a", "_b"], {
				[_a select 0, _b select 0] call _fn_sort
			    }] call fnc_lambda] call fnc_sorted);
	_groups = [];
	_passengers = [];
	{
		_groups     append [_x select 0];
		_passengers append [_x select 1];
	} forEach _pairs;
	_n = (_n min (count _groups)) min (count _locations);
	_groups      = [_groups,     0, _n] call fnc_subseq;
	_passsengers = [_passengers, 0, _n] call fnc_subseq;
	if ((_nearest isEqualTo true) or
	    ((not (_nearest isEqualTo false)) and
	     ((random 1) < _nearest))) then {
	    _locations = [_locations, 
			  [["_a", "_b"], {
			      private ["_apos"];
			      _apos = position ((units (_groups select 0)) select 0);
			      (([_apos, position _a] call fnc_euclidean_distance) <
			       ([_apos, position _b] call fnc_euclidean_distance))
				}] call fnc_lambda] call fnc_sorted;
	};
	_locations = [_locations, 0, _n] call fnc_subseq;
	[[["_loc", "_grp", "_passenger"], {
		private ["_wp", "_wpPassenger"];
		if ((not (isNil "_grp")) and (not (_grp in _dispatched))) then {
			_wp = ["Waypoint", _behavior, "GREEN",  15,
			       "Unload Passengers", 
			       "FILE",  0, _loc, _speed, 
			       ["true", ""], 
			       [0, 0, 0], "TR UNLOAD", true] call fnc_new;
			if ((not (isNil "_wp_config")) and
			    (typeName _wp_config == "ARRAY")) then {
				[_wp, "configure", _wp_config] call fnc_tell;
			};
			_grp setCurrentWaypoint (
				 [_wp, "to_group", _grp, 0] call fnc_tell);
			_dispatched append [_grp];
			if (not (isNil "_passenger")) then {
				_wpPassenger = ["Waypoint", _behavior,
						"YELLOW", 15, "Get Out", 
						"WEDGE",  0, _loc, _speed, 
						["true", ""], 
						[0, 0, 0], "GETOUT",
						true] call fnc_new;
				if ((not (isNil "_wp_config")) and
				    (typeName _wp_config == "ARRAY")) then {
					[_wpPassenger, "configure",
					 _wp_config] call fnc_tell;
				};
					_passenger setCurrentWaypoint (
						 [_wpPassenger, "to_group",
						  _passenger, 0] call fnc_tell);
					if (not (_passenger in _dispatched)) then {
						_dispatched append [_passenger];
					};
			};
		};
	}] call fnc_lambda,
	_locations,
	_groups,
	_passengers] call fnc_map;
	_self setVariable ["_dispatched", _dispatched];
} ENDMETHOD;

DEFMETHOD("Headquarters", "radio") ["_self", "_msg"] DO {
	[BLUFOR, "HQ"] sideChat _msg;
} ENDMETHOD;

DEFMETHOD("Headquarters", "available") ["_self"] DO {
	(count (_self getVariable "_groups")) -
	(count (_self getVariable "_dispatched"));
} ENDMETHOD;

DEFMETHOD("Headquarters", "available_groups") ["_self"] DO {
	(_self getVariable "_groups") -
	(_self getVariable "_dispatched");
} ENDMETHOD;

DEFMETHOD("Headquarters", "available_transports") ["_self"] DO {
	(_self getVariable "_transports") -
	(_self getVariable "_dispatched");
} ENDMETHOD;

DEFMETHOD("Headquarters", "available_pairs") ["_self"] DO {
	private ["_dispatched"];
	_dispatched = _self getVariable "_dispatched";
	[[["_pair"], {not ((_pair select 0) in _dispatched)}] call fnc_lambda,
	 _self getVariable "_pairs"] call fnc_filter;
} ENDMETHOD;

DEFMETHOD("Headquarters", "moveable_vehicles") ["_self", "_cull"] DO {
	private ["_vehicles", "_transports"];
	_vehicles   = (_self getVariable "_vehicles");
	if (isNil "_cull") then {
		[[["_x"], {alive (driver _x) and (canMove _x)}] call fnc_lambda,
		 _vehicles] call fnc_filter;
	} else {
		_transports = _self getVariable "_transports";
		[[["_x"], {
			private ["_driver", "_moveable"];
			_driver = driver _x;
			_moveable = (not (isNil "_driver")) and (canMove _x);
			if (not _moveable) then {
				_vehicles   = _vehicles   - [_x];
				_transports = _transports - [group _x];
			};
			_moveable
		 }] call fnc_lambda,
		 _self getVariable "_vehicles"] call fnc_map;
		_self setVariable ["_vehicles",   _vehicles];
		_self setVariable ["_transports", _transports];
		_vehicles
	};
} ENDMETHOD;

DEFMETHOD("Headquarters", "living_men") ["_self", "_cull"] DO {
	private ["_men"];
	_men = + (_self getVariable "_men");
	if (isNil "_cull") then {
		[[["_x"], {alive _x}] call fnc_lambda, _men] call fnc_filter;
	} else {
		[[["_x"], {
			private ["_alive"];
			if (not (alive _x)) then {
				_men = _men - [_x];
			};
		 }] call fnc_lambda,
		_self getVariable "_men"] call fnc_map;
		_men
	};
} ENDMETHOD;

DEFMETHOD("Headquarters", "men_from_trigger") ["_self", "_trigger_list"] DO {
	private ["_thisKind", "_grp", "_groups", "_fn_getKindOf"];
	_fn_getKindOf = [["_x"], {  // TODO: if x is kindof y then y (map)
			  private ["_kinds", "_kind"];
			  _kinds = ["Man", "Land", "Ship", "Air"];
			  for "_i" from 0 to ((count _kinds) - 1) do {
				  scopeName "TestKinds";
				  _kind = _kinds select _i;
				  if (_x isKindOf _kind) then {
					  breakOut "TestKinds";
				  };
			  };
			  _kind
			 }] call fnc_lambda;
	_groups = [];
	{
		//		switch ([_x] call _fn_getKindOf) do {
		_thisKind = [_x] call _fn_getKindOf;
		if ((_thisKind == "Land") or (_thisKind == "Air") or
		    (_thisKind == "Ship")) then {
			[[["_crewMemberArray"], {
				_grp = group (_crewMemberArray select 0);
				if (not (_grp in _groups)) then {
					_groups append [_grp];
				};
			 }] call fnc_lambda,
			 fullCrew _x] call fnc_map;
		} else {
			_grp = group _x;
			if (not (_grp in _groups)) then {
				_groups append [_grp];
			};
		};

	} forEach _trigger_list;
	hint format ["groups found: %1
from men: %2", _groups, _trigger_list];
	[[["_x"], {(group _x) in _groups}] call fnc_lambda,
	 [_self, "living_men"] call fnc_tell] call fnc_filter
} ENDMETHOD;

DEFMETHOD("Headquarters", "purge") ["_self"] DO {  // TODO: fix vehicles?
	private ["_men", "_groups"];
	[_self, "moveable_vehicles", true] call fnc_tell;
	_men = [_self, "living_men", true] call fnc_tell;
	_groups = [];
	[[["_grp"], {
		private ["_living"];
		_living = 0;
		[[["_x"], {
			if (_x in _men) then { _living = _living + 1; };
			nil
		 }] call fnc_lambda, units _grp] call fnc_map;
		if (0 < _living) then {	_groups append [_grp]; }
		                 else { deleteGroup _grp;      };
		nil
	 }] call fnc_lambda,
	 _self getVariable "_groups"] call fnc_map;
	_self setVariable ["_groups", _groups];
} ENDMETHOD;

DEFMETHOD("Headquarters", "retrack") ["_self"] DO {
	private ["_dispatchedLeaders", "_dispatched", "_transportLeaders",
		 "_keepTransports", "_transports"];
	_self setVariable ["_dispatched",
			   [[["_x"], {not (isNil "_x")}] call fnc_lambda,
			    _self getVariable "_dispatched"] call fnc_filter];
	_self setVariable ["_transports",
			   [[["_x"], {not (isNil "_x")}] call fnc_lambda,
			    _self getVariable "_transports"] call fnc_filter];
	_dispatchedLeaders = [[["_x"], {leader _x}] call fnc_lambda,
		              _self getVariable "_dispatched"] call fnc_map;
	_dispatched = [];
	_transportLeaders = [[["_x"], {leader _x}] call fnc_lambda,
			     _self getVariable "_transports"] call fnc_map;
	{_self setVariable [_x, []]} forEach ["_groups",
					      "_air",
					      "_men",
					      "_transports",
					      "_passengers",
					      "_pairs"];
	[_self, "track", _self getVariable "_tracked"] call fnc_tell;
	{
		if (alive _x) then { _dispatched append [group _x]; };
	} forEach _dispatchedLeaders;
	_keepTransports = [[["_x"], {group _x}] call fnc_lambda,
			   _transportLeaders] call fnc_map;
	_transports = _self getVariable "_transports";
	_transports = _transports - _keepTransports;
	_transports append _keepTransports;
	_self setVariable ["_dispatched", _dispatched];
	_self setVariable ["_transports", _transports];
} ENDMETHOD;

DEFMETHOD("Headquarters", "leaders") ["_self", "_named_only"] DO {
	private ["_result", "_acc"];
	if ( isNil "_named_only" ) then { _named_only = false };
	_result = [[["_x"], {_x == (leader (group _x))}] call fnc_lambda,
		   _self getVariable "_men"] call fnc_filter;
	if (_named_only) then {
		_acc = [];
		{
			if (not (1 < (count ((format ["%1", _x]) splitString " "))))
			then { _acc append [_x] };
		} forEach _result;
		_result = _acc;
	};
	_result
} ENDMETHOD;

DEFMETHOD("Headquarters", "regroupify") ["_self", "_groups", "_waypoints"] DO {
	private ["_fn_group_position_id", "_transports", "_addTransports"];
	if ( isNil "_groups" ) then { _groups = _self getVariable "_groups" };
	_fn_group_position_id = {
		private ["_vvn", "_str"];
		_vvn = vehicleVarName _this;
		_this setVehicleVarName "";
		_str = str _this;
		_this setVehicleVarName _vvn;
		parseNumber (_str select [(_str find ":") + 1])
	};
	_addTransports = [];
	_transports = _self getVariable "_transports";
	[[["_grp"], {
		private ["_group", "_leader", "_units", "_leaderTarget"];
		_leader = leader _grp;
		_units  = units  _grp - [_leader];
		_gpids  = [[["_x"],
			      {_x call _fn_group_position_id}] call fnc_lambda,
			   _units] call fnc_map;
		_group  = createGroup (side _leader);
		if ( _grp in _transports ) then { _addTransports append [_group] };
		if ( not (isNil "_waypoints") ) then {
			[[["_wpt"], {
				private ["_wp"];
				_wp = [_wpt, "to_group", _group, 0] call fnc_tell;
			}] call fnc_lambda, _waypoints] call fnc_mapnil;
		};
		_leaderTarget = assignedTarget _leader;
		_leader joinAsSilent [_group, _leader call _fn_group_position_id];
		_leader doTarget _leaderTarget;
		[[["_unit", "_gpid"], {
		    private ["_previousTarget"];
		    _previousTarget = assignedTarget _unit;
		    _unit joinAsSilent [_group, _gpid];
		    _unit doTarget _previousTarget;
	         }] call fnc_lambda,
		 _units, _gpids] call fnc_map;
		deleteGroup _grp;
	 }] call fnc_lambda,
	 _groups] call fnc_map;
	[_self, "retrack"] call fnc_tell;
	_transports = (_self getVariable "_transports") - _addTransports;
	_transports append _addTransports;
	_self setVariable ["_transports", _transports];
} ENDMETHODV;

DEFMETHOD("Headquarters", "approach_targets") ["_self",
					       "_targets",
					       "_commit",
					       "_wp_config",
                                               "_nearest"] DO {
	private ["_image", "_locs", "_nextLocs", "_pairs",
		 "_locationFinder", "_numToDispatch", "_numDispatched"];
	_pairs = [_self, "available_pairs"] call fnc_tell;
	if ((isNil "_commit") or
	    (not (typeName _commit == "SCALAR"))) then { _commit = count _pairs };
	if (isNil "_wp_config") then { _wp_config = true };
        if (isNil "_nearest")   then { _nearest   = false };
	_image = ["TargetImpression", _targets] call fnc_new;
	_locationFinder = ["LocationFinder"] call fnc_new;
	[_locationFinder, "configure",
	 [["color",          "ColorYellow"],
	  ["populationSize", 7],
	  ["radius",         300],
	  ["shape",          [7] call fnc_make_ring]]] call fnc_tell;
	_locs = [];
	_numDispatched = 0;
	while { count _locs < _commit } do {
		[_locationFinder, "obj_tactical_approach",
		 _image getVariable "_logics", true] call fnc_tell;
		_nextLocs = [_locationFinder, "run", 3, 3] call fnc_tell;
		_nextLocs = [[["_x"], {
			       not ((surfaceIsWater (position _x)) or
				    (800 <
				     [[["_a", "_b"], {_a min _b}] call fnc_lambda,
				      [[["_y"], {
					 [position _x,
					  position _y] call fnc_euclidean_distance
				       }] call fnc_lambda,
				       _image getVariable "_logics"] call fnc_map
				      ] call fnc_reduce))
			     }] call fnc_lambda, _nextLocs] call fnc_filter;
		_locs append _nextLocs;
		_numToDispatch = (count _nextLocs) min
			           ((count _pairs) - _numDispatched);
		[_self, "dispatch_transports",
		 (count _nextLocs) min (count _pairs),
		 _nextLocs, _wp_config, _nearest] call fnc_tell;
		_numDispatched = _numDispatched + _numToDispatch;
	};
	[_image, "dispose"] call fnc_tell;
} ENDMETHODV;

DEFMETHOD("Headquarters", "attack_targets") ["_self",
					     "_targets",
					     "_commit",
					     "_wp_config"] DO {
        private ["_image", "_locs", "_nextLocs", "_avail", "_locationFinder",
		 "_dispatchCount", "_color"];
	_image = ["TargetImpression", _targets] call fnc_new;
	/* Exit if the target array is empty by this point: */
	if ((count (_image getVariable "_logics")) <= 0) exitWith {
		[_image, "dispose"] call fnc_tell;
		true
	}; /* Otherwise, continue: */
	_avail = [_self, "available"] call fnc_tell;
	if ((isNil "_commit") or
	    (not (typeName _commit == "SCALAR"))) then { _commit = _avail };
	if (isNil "_wp_config") then { _wp_config = true };
	_locationFinder = ["LocationFinder"] call fnc_new;
	if (blufor == side ((_self getVariable "_men") select 0)) then {
	    _color = "ColorBlue";
	} else {
	  _color = "ColorRed";
	};
	[_locationFinder, "configure", 
	 [["color",          _color], 
	  ["populationSize", 7], 
	  ["radius",         125],
	  ["shape",          [7] call fnc_make_ring]]] call fnc_tell;
	 _locs = [];
	 while {count _locs < _commit} do {
	   [_locationFinder, 
	    ["obj_tactical_advantage", 
	     "obj_sniper_nests"] select ([2] call fnc_randint), 
	    _image getVariable "_logics", true] call fnc_tell;
	   _nextLocs = [_locationFinder, "run", 3, 3] call fnc_tell;
	   _nextLocs = [[["_x"], {not (surfaceIsWater (position _x))}] 
			call fnc_lambda, _nextLocs] call fnc_filter;     
	   _locs append _nextLocs;
	   _availableCount = ([_self, "available"] call fnc_tell);
	   if (_availableCount < _commit) then {
	     _dispatchCount = _commit min (count _nextLocs);
           } else {
	     _dispatchCount = ((count _nextLocs) min _availableCount)
                               min _commit;
	   };
	   if (0 < (count _nextLocs)) then {
		   [_self, "dispatch",
		    _dispatchCount,
		    _nextLocs, _wp_config, true] call fnc_tell;
	   };
	 };
	 [_image, "dispose"] call fnc_tell;
} ENDMETHODV;

DEFMETHOD("Headquarters", "safety_protocol") ["_self", "_helicopter"] DO {
	_helicopter allowCrewInImmobile true;
	{
		removeBackpack (_x select 0);
		(_x select 0) addBackpack "B_Parachute";
	} forEach (fullCrew _helicopter);  //  [_helicopter, "cargo", false]);
	if ((vehicle player) == _helicopter) then {
		[player, _helicopter] spawn
		([["_man", "_heli"], {
		     private ["_chute"];
		     waitUntil {
			     (not (alive _man)) or
			     ((_man == vehicle _man) and
			      (3 < ((getPosATL _heli) select 2)))
		     };
		     _man setVariable ["safetySleepDone", false];
		     _man spawn {
			 sleep 4;
			 _this setVariable ["safetySleepDone", true];
		     };
		     waitUntil {
			     (8 < ([getPosATL _man,
				    getPosATL _heli] call fnc_euclidean_distance)) or
			     (_man getVariable "safetySleepDone")
		     };
		     if ((backpack (_this select 0)) == "B_Parachute") then {
			     removeBackpack _man;
		     };
		     if (15 < ((getPosATL  _man) select 2) ) then {
			     _chute = "Steerable_Parachute_F" createVehicle [0, 0, 100];
			     _chute setPosATL (_man modelToWorld [0, 0, 0]);
			     _man moveInDriver _chute;
		     };
		  }] call fnc_lambda);
	};
} ENDMETHOD;

DEFMETHOD("Headquarters", "insurgency_arm") ["_self",
					     "_obj",
					     "_radius",
					     "_type",
					     "_chance",
					     "_watch",
					     "_engage",
					     "_ratio"] DO {
	private ["_killzone"];
	if (isNil "_type"   ) then { _type    = "units" };  // intersections | roads | units | group
	if (isNil "_chance" ) then { _chance  = 1.0     };
	if (isNil "_watch") then { _watch = [_obj] };
	if (isNil "_ratio") then { _ratio = 0.618; };
	
	_killzone = [_obj, "KillZone",
		     _type, _radius, _chance, _watch,
		     _self getVariable "_side"] call fnc_instance;
	[_killzone, "arm", 7, 3] call fnc_tell;
	if (not (isNil "_engage")) then {
		[_self, "insurgency_engage",
		 _killzone, 200, _ratio] call fnc_tell;
	};
	_killzone
} ENDMETHODV;

DEFMETHOD("Headquarters", "insurgency_engage") ["_self", "_zone", "_pull_radius", "_percentage"] DO {
	if (isNil "_pull_radius") then { _pull_radius = 150; };
	if (isNil "_percentage" ) then { _percentage  = 1.0; };
	[_zone, "engage", _pull_radius, _percentage] call fnc_tell;
} ENDMETHODV;
	
 DEFMETHOD("Headquarters", "woodland_vests") ["_self", "_men"] DO {
	 private ["_fn"];
	 if (isNil "_men") then { _men = [_self, "living_men"] call fnc_tell; };
	 [_self getVariable "_armory", "change_vests",
	  _men, "V_I_G_resistanceLeader_F"] call fnc_tell;
 } ENDMETHOD;

DEFMETHOD("Headquarters", "land_transports") ["_self",  // TODO: refactor to use
					       "_position",  //   position finder
					       "_helicopters",
					       "_enemies",
					       "_search_radius",
					       "_search_steps",
					       "_search_population",
					       "_search_shape"] DO {
	private ["_pad", "_solutions", "_group", "_solutionIndex", "_wp",
		 "_pads", "_confirmed", "_locationFinder", "_dispatched", //"_assignments",// "_optimizations",
		 "_fn_distanceSort"];
	if (isNil "_search_radius") then {
		_search_radius = 7.5;
	};
	if (isNil "_search_steps") then {
		_search_steps = ceil sqrt (2 * count _helicopters);
	};
	if (isNil "_search_population") then {
		_search_population = floor (7 * (sqrt (count _helicopters)));
	};
	if (isNil "_search_shape") then {
		_search_shape = //[_search_population] call
			fnc_make_concentric_rings;
	};
	_fn_distanceSort = [["_a", "_b"], {
				   ([_a, 1,
				     ["Building", "House", "Church", "Chapel",
				      "Rock", "Bunker", "Fortress", "Fountain",
				      "Lighthouse", "Fuelstation", "Hospital",
				      "Busstop", "Ruin", "Rocks",
				      "Tree", "Small Tree", "Forest Border",
				      "Forest Triangle", "Forest Square",
				      "Forest"],
				     40]
	       call fnc_average_distance_to_nearest_terrain_objects) >
				   ([_b, 1,
				     ["Building", "House", "Church", "Chapel",
				      "Rock", "Bunker", "Fortress", "Fountain",
				      "Lighthouse", "Fuelstation", "Hospital",
				      "Busstop", "Ruin", "Rocks",
				      "Tree", "Small Tree", "Forest Border",
				      "Forest Triangle", "Forest Square",
				      "Forest"],
				     40]
	       call fnc_average_distance_to_nearest_terrain_objects)
	}] call fnc_lambda;
	_solutions      = [];
	_solutionIndex  = 0;
	_pads           = [];
	_locationFinder = ["LocationFinder",
			   _search_population, _search_shape] call fnc_new;
	[_locationFinder, "set_location", [_position]] call fnc_tell;
	[_locationFinder, "configure", [["radius", _search_radius]]] call fnc_tell;
	if (isNil "_enemies") then {
		[_locationFinder, "obj_landing_zones"] call fnc_tell;
	} else {
		[_locationFinder, "obj_landing_zones", _enemies] call fnc_tell;
	};
	while {(count _pads) < (count _helicopters)} do {
		_confirmed = false;
		while {not _confirmed} do {
			_confirmed = true;
			for "_i" from 0 to ((count _pads) - 1) do {
				scopeName "skipClose";
				if (((_pads select _i) distance
				     (_solutions select _solutionIndex)) < 25)
				then {
					_solutionIndex = _solutionIndex + 1;
					_confirmed = false;
					breakOut "skipClose";
				};
			};
		};
		if (_solutionIndex >= (count _solutions)) then {
			_solutions = [_locationFinder, "run",
				      ceil ((count _helicopters) / .618),
				      _search_steps] call fnc_tell;
			_solutions = [[["_s"], {
					([_s] call ([false, 0.35, 1.5,
						     '[_x, 3, 2]',
						     fnc_check_level]
						    call fnc_to_cost_function))
					< 0.1
				      }] call fnc_lambda,
				      _solutions] call fnc_filter;
			_solutions = [_solutions,
				      _fn_distanceSort] call fnc_sorted;
			_solutionIndex = 0;
		} else {
			_pad = "Land_HelipadEmpty_F" createVehicle
			       (position (_solutions select _solutionIndex));
			_solutionIndex = _solutionIndex + 1;
			_pads = _pads + [_pad];
		};
	};
	_dispatched = _self getVariable "_dispatched";
	for "_i" from 0 to ((count _helicopters) - 1) do {
		_pad = _pads select _i;
		_group = group ((crew (_helicopters select _i)) select 0);
		_dispatched append [_group]; 
		_wp = _group addWaypoint [position _pad,
					  currentWaypoint _group];
		_wp waypointAttachVehicle _pad;
		_wp setWaypointType      "TR UNLOAD";  // "MOVE";  // "TR UNLOAD" if passengers...
		_wp setWaypointBehaviour "CARELESS"; // Otherwise, to land:
		_wp setWaypointStatements ["true", "
		    _thread = this spawn {
		      while { (not (isNull _this)) and 
                              (alive (driver _this)) and 
                              (not (unitReady _this)) } do {
			  sleep 1;
		      };
		      if ((not (isNull _this)) and (alive (driver _this))) then {
		  	  (vehicle _this) land 'LAND';
		      };
		    };"

		];
		_group setCurrentWaypoint _wp;
	};
	_self setVariable ["_dispatched", _dispatched];
} ENDMETHODV;

DEFMETHOD("Headquarters", "land_helicopters") ["_self",  // TODO: refactor to use
					       "_position",  //   position finder
					       "_helicopters",
					       "_enemies",
					       "_search_radius",
					       "_search_steps",
					       "_search_population",
					       "_search_shape"] DO {
	private ["_pad", "_solutions", "_group", "_solutionIndex", "_wp",
		 "_pads", "_confirmed", "_locationFinder", "_dispatched", //"_assignments",// "_optimizations",
		 "_fn_distanceSort"];
	if (isNil "_search_radius") then {
		_search_radius = 7.5;
	};
	if (isNil "_search_steps") then {
		_search_steps = ceil sqrt (2 * count _helicopters);
	};
	if (isNil "_search_population") then {
		_search_population = floor (7 * (sqrt (count _helicopters)));
	};
	if (isNil "_search_shape") then {
		_search_shape = //[_search_population] call
			fnc_make_concentric_rings;
	};
	_fn_distanceSort = [["_a", "_b"], {
				   ([_a, 1,
				     ["Building", "House", "Church", "Chapel",
				      "Rock", "Bunker", "Fortress", "Fountain",
				      "Lighthouse", "Fuelstation", "Hospital",
				      "Busstop", "Ruin", "Rocks",
				      "Tree", "Small Tree", "Forest Border",
				      "Forest Triangle", "Forest Square",
				      "Forest"],
				     40]
	       call fnc_average_distance_to_nearest_terrain_objects) >
				   ([_b, 1,
				     ["Building", "House", "Church", "Chapel",
				      "Rock", "Bunker", "Fortress", "Fountain",
				      "Lighthouse", "Fuelstation", "Hospital",
				      "Busstop", "Ruin", "Rocks",
				      "Tree", "Small Tree", "Forest Border",
				      "Forest Triangle", "Forest Square",
				      "Forest"],
				     40]
	       call fnc_average_distance_to_nearest_terrain_objects)
	}] call fnc_lambda;
	_solutions      = [];
	_solutionIndex  = 0;
	_pads           = [];
	_locationFinder = ["LocationFinder",
			   _search_population, _search_shape] call fnc_new;
	[_locationFinder, "set_location", [_position]] call fnc_tell;
	[_locationFinder, "configure", [["radius", _search_radius]]] call fnc_tell;
	if (isNil "_enemies") then {
		[_locationFinder, "obj_landing_zones"] call fnc_tell;
	} else {
		[_locationFinder, "obj_landing_zones", _enemies] call fnc_tell;
	};
	while {(count _pads) < (count _helicopters)} do {
		_confirmed = false;
		while {not _confirmed} do {
			_confirmed = true;
			for "_i" from 0 to ((count _pads) - 1) do {
				scopeName "skipClose";
				if (((_pads select _i) distance
				     (_solutions select _solutionIndex)) < 25)
				then {
					_solutionIndex = _solutionIndex + 1;
					_confirmed = false;
					breakOut "skipClose";
				};
			};
		};
		if (_solutionIndex >= (count _solutions)) then {
			_solutions = [_locationFinder, "run",
				      ceil ((count _helicopters) / .618),
				      _search_steps] call fnc_tell;
			_solutions = [[["_s"], {
					([_s] call ([false, 0.35, 1.5,
						     '[_x, 3, 2]',
						     fnc_check_level]
						    call fnc_to_cost_function))
					< 0.1
				      }] call fnc_lambda,
				      _solutions] call fnc_filter;
			_solutions = [_solutions,
				      _fn_distanceSort] call fnc_sorted;
			_solutionIndex = 0;
		} else {
			_pad = "Land_HelipadEmpty_F" createVehicle
			       (position (_solutions select _solutionIndex));
			_solutionIndex = _solutionIndex + 1;
			_pads = _pads + [_pad];
		};
	};
	_dispatched = _self getVariable "_dispatched";
	for "_i" from 0 to ((count _helicopters) - 1) do {
		_pad = _pads select _i;
		_group = group ((crew (_helicopters select _i)) select 0);
		_dispatched append [_group]; 
		_wp = _group addWaypoint [position _pad,
					  currentWaypoint _group];
		_wp waypointAttachVehicle _pad;
		_wp setWaypointType      "MOVE";  // "TR UNLOAD" if passengers...
		_wp setWaypointBehaviour "CARELESS"; // Otherwise, to land:
		_wp setWaypointStatements ["true", "
		    _thread = this spawn {
		      while { (not (isNull _this)) and 
			      (alive (driver _this)) and 
			      (not (unitReady _this)) } do {
			sleep 1;
		      };
		      if ((not (isNull _this)) and (alive (driver _this))) then {
		  	  (vehicle _this) land 'LAND';
		      };
		    };"
		];
		_group setCurrentWaypoint _wp;
	};
	_self setVariable ["_dispatched", _dispatched];
} ENDMETHODV;

DEFMETHOD("Headquarters", "watch_for_smoke") ["_self", "_helicopters", "_whose"] DO {
	private ["_fn_eh_fired"];  // , "_fn_projectile_follow"];
	if (isNil "_whose") then { _whose = [player]; };
	_fn_eh_fired = [["_unit", "_weapon", "_muzzle", "_mode", "_ammo",
			 "_magazine", "_projectile", "_gunner"], {
			  private ["_strMagazine", "_fn_projectile_follow"];
			  _fn_projectile_follow = [["_unit", "_projectile"], {
				private ["_speed"];
				_speed = speed _projectile;
				while { 1 < (abs _speed) } do {
					sleep 1;
					_speed = speed _projectile;
				};
				[_unit getVariable "_hqForSmoke", "land_helicopters",
				 position _projectile,
				 _unit getVariable "_helicoptersForSmoke"] call fnc_tell;
		          }] call fnc_lambda;
			  _strMagazine = format ["%1", _magazine];
			  if (("smoke" in _strMagazine) or
			      ("Smoke" in _strMagazine) or
			      ("SMOKE" in _strMagazine)) then {
					  [_unit, _projectile] spawn _fn_projectile_follow;
			  };
			}] call fnc_lambda;
	{ _x setVariable     ["_hqForSmoke",          _self];
	  _x setVariable     ["_helicoptersForSmoke", _helicopters];
	  _x addEventHandler ["Fired",                _fn_eh_fired]; } forEach _whose;
} ENDMETHODV;

DEFMETHOD("Headquarters", "orbit_position") ["_self",
					     "_position",
					     "_helicopters",
					     "_radius",
					     "_altitude",
					     "_bearing",
					     "_ccw",
					     "_num_points",
					     "_star"] DO {
        private ["_waypoints", "_shape", "_waypoint", "_groups", "_group", "_ccwMult", "_fn"];
	if ( isNil "_radius"     ) then {     _radius = 420    };
	if ( isNil "_altitude"   ) then {   _altitude = 210    };
	if ( isNil "_ccw"        ) then {        _ccw = false  };
	if ( isNil "_num_points" ) then { _num_points = 6      };
	if ( isNil "_star"       ) then {       _star = false  };

        if (_star) then { _fn = fnc_make_star; } else { _fn = fnc_make_ring; };

	if ( isNil "_bearing"    ) then {
		if (_ccw) then {
			_ccwMult = -1;
		} else {
			_ccwMult = 1;
		};
		_bearing = (180 +
			    ((_helicopters select 0) getRelDir _position) +
 			    (direction (_helicopters select 0)) +
			    _ccwMult * (360.0 / _num_points)) mod 360;
	} else {
		if ((typeName _bearing) == "BOOL") then {
			if (_ccw) then {
				_ccwMult = -1;
			} else {
				_ccwMult = 1;
			};
			_bearing = (180 +
				    ((_helicopters select 0) getRelDir _position) +
				    (direction (_helicopters select 0)) +
				    _ccwMult * (360.0 / _num_points)) mod 360;
		};
	};
	_waypoints = [];
	if ( _ccw ) then {
		_bearing = (_bearing + (360.0 / _num_points)) mod 360;
	};
	_shape     = [[_num_points] call _fn,
		       _radius,
		       _bearing] call fnc_scale_and_rotate;
	if not ( _ccw ) then { reverse _shape };
	{
		if (count _waypoints == 0) then {
			_waypoint = ["Waypoint",
				     "CARELESS", "YELLOW", "", "", "", 0,
				     [[["_component", "_offset"],
				       { _component + _offset }] call fnc_lambda,
				      _position, _x] call fnc_map,
				     "LIMITED", "", "", "CYCLE", ""] call fnc_new;
			_waypoints append [_waypoint];
		};
		_waypoint = ["Waypoint",
			     "CARELESS", "YELLOW", "", "", "", 0,
			     [[["_component", "_offset"],
			       { _component + _offset }] call fnc_lambda,
			      _position, _x] call fnc_map,
			     "LIMITED", "", "", "MOVE", ""] call fnc_new;
		_waypoints append [_waypoint];
	} forEach _shape;
       // _waypoint = ["Waypoint",
       // 		    "CARELESS", "YELLOW", "", "", "", 0,
       // 		    [[["_component", "_offset"],
       // 		      { _component + _offset }] call fnc_lambda,
       // 		     _position, _x] call fnc_map,
       // 		    "LIMITED", "", "", "CYCLE", ""] call fnc_new;
       //	_waypoints append [_waypoint];
	_groups = [];
	{
		_group = group _x;
		if ( not (_group in _groups) ) then { _groups append [_group] };
		_x flyInHeight _altitude;
	} forEach _helicopters;
	[_self, "regroupify", _groups, _waypoints] call fnc_tell;
	[_self, "retrack"] call fnc_tell;
} ENDMETHODV;

DEFMETHOD("Headquarters", "holding_arc") ["_self",
					  "_focus_group",
					  "_helicopter",
					  "_angle_cw",
					  "_angle_ccw",
					  "_radius_inner",
					  "_radius_outer",
					  "_altitude",
					  "_ccw",
					  "_n",
					  "_recur"] DO {
	private ["_units", "_position", "_relDir", "_heliIdle", "_doArc"];
	if (isNil "_n"    ) then { _n     = 1     };
	if (isNil "_recur") then { _recur = false };
	if (not _recur) then { /* if this is the first iteration, set up vars */
		_heliIdle = _helicopter getVariable "arcIdle";
		if (isNil "_heliIdle") then {
			_heliIdle = true;
			_helicopter setVariable ["arcIdle", true];
		} else { /* flag and wait for previous run to finish */
			if (not _heliIdle) then {
				hint "not arcIdle!";
				_helicopter setVariable ["_doHoldingArc", false];
				waitUntil { sleep 2;
					    _helicopter getVariable "arcIdle" };
				hint "arc idle!";
			};
		};
		_helicopter setVariable ["arcIdle", false];
		_helicopter setVariable ["_doHoldingArc", true];
		_doArc = true;

		/* now make sure we get to within bounds to begin */
		_units = [[["_x"], {alive _x}] call fnc_lambda,
			  units _focus_group] call fnc_filter;
		_position = ([[["_x"], {position _x}] call fnc_lambda,
			      _units] call fnc_map) call fnc_vector_mean;
		_relDir = (180 + (_helicopter getRelDir _position) +
			   (direction _helicopter)) mod 360;
		if ((_angle_cw < _relDir) or (_relDir < _angle_ccw)) then {
			if ((abs (((360 + _relDir) - _angle_cw) mod 360))
			    < (abs ((360 + _angle_ccw - _relDir) mod 360))) then {
				[_self, "orbit_group", _focus_group, _helicopter,
				 _radius_outer, _altitude, true] spawn fnc_tell;					
			} else {
				[_self, "orbit_group", _focus_group, _helicopter,
				 _radius_outer, _altitude, false] spawn fnc_tell;					
			};
			waitUntil {
				sleep 1;
				_units = [[["_x"], {alive _x}] call fnc_lambda,
					  units _focus_group] call fnc_filter;
				_position = ([[["_x"], {position _x}] call fnc_lambda,
					      _units] call fnc_map) call fnc_vector_mean;
				_relDir = (180 + (_helicopter getRelDir _position) +
					   (direction _helicopter)) mod 360;
				_doArc = _helicopter getVariable "_doHoldingArc";
				((_relDir > (_angle_ccw + (8 min (_angle_cw-_angle_ccw)/5))) and
				 (_relDir < (_angle_cw  - (8 min (_angle_cw-_angle_ccw)/5)))) or
				(not _doArc)  // (_helicopter getVariable "_doHoldingArc"))
			};
		};
	} else { _doArc = true };  // minimal setup for reiterations
	if (_doArc and (_helicopter getVariable "_doHoldingArc")) then {
		[_self, "orbit_group", _focus_group, _helicopter,
		 _radius_inner, _altitude, not _ccw] spawn fnc_tell;
		waitUntil {
			sleep 1;
			_units = [[["_x"], {alive _x}] call fnc_lambda,
				  units _focus_group] call fnc_filter;
			_position = ([[["_x"], {position _x}] call fnc_lambda,
				      _units] call fnc_map) call fnc_vector_mean;
			_relDir = (180 + (_helicopter getRelDir _position) +
				   (direction _helicopter)) mod 360;
			hint format ["%1", _relDir];
			_doArc = _helicopter getVariable "_doHoldingArc";
			(_ccw and (_relDir > _angle_cw)) or
			((not _ccw) and (_relDir < _angle_ccw)) or
			(not _doArc)  // (_helicopter getVariable "_doHoldingArc"))
		};
	};
	if (_doArc and (_helicopter getVariable "_doHoldingArc")) then {
		[_self, "orbit_group", _focus_group, _helicopter,
		 _radius_outer, _altitude, _ccw] spawn fnc_tell;
		waitUntil {
			sleep 1;
			_units = [[["_x"], {alive _x}] call fnc_lambda,
				  units _focus_group] call fnc_filter;
			_position = ([[["_x"], {position _x}] call fnc_lambda,
				      _units] call fnc_map) call fnc_vector_mean;
			_relDir = (180 + (_helicopter getRelDir _position) +
				   (direction _helicopter)) mod 360;
			_doArc = _helicopter getVariable "_doHoldingArc";
			(_ccw and (_relDir < _angle_ccw)) or
			((not _ccw) and (_relDir > _angle_cw)) or
			(not _doArc)  // (_helicopter getVariable "_doHoldingArc"))
		};
	};
	if (_doArc and (_helicopter getVariable "_doHoldingArc")) then {
		switch (typeName _n) do {
			case "BOOL": {
				if (_n) then {
					[_self, "holding_arc", _focus_group, _helicopter,
					 _angle_cw, _angle_ccw, _radius_inner, _radius_outer,
					 _altitude, _ccw, _n, true] spawn fnc_tell;
				};
			};
			default {
				if (1 < _n) then {
					[_self, "holding_arc", _focus_group, _helicopter,
					 _angle_cw, _angle_ccw, _radius_inner, _radius_outer,
					 _altitude, _ccw, _n - 1, true] spawn fnc_tell;
				};
			};
		};
	} else {
		_helicopter setVariable ["arcIdle", true];
	};
} ENDMETHODV;

DEFMETHOD("Headquarters", "orbit_group") ["_self",
					  "_group_units_or_pos",
					  "_helicopter",
					  "_radius",
					  "_altitude",
					  "_ccw",
					  "_points",
					  "_star",
					  "_tolerance"] DO {
	private ["_units", "_position", "_lastPos", "_ccwMult", "_heliIdle", "_group"];

	_heliIdle = _helicopter getVariable "orbitIdle";
	if (isNil "_heliIdle") then {
		_heliIdle = true;
		_helicopter setVariable ["orbitIdle", true];
	} else {
		if (not _heliIdle) then {
			hint "not heliIdle!";
			_helicopter setVariable ["_doOrbit", false];
			waitUntil { sleep 0.5; _helicopter getVariable "orbitIdle" };
			hint "orbit idle!";
		};
	};

	if ( isNil "_radius"    ) then {    _radius = 400   };
	if ( isNil "_altitude"  ) then {  _altitude = 55    };
	if ( isNil "_ccw"       ) then {       _ccw = false };
	if ( isNil "_points"    ) then {    _points = 6     };
	if ( isNil "_star"      ) then {      _star = false };
	if ( isNil "_tolerance" ) then { _tolerance = 75    };
	if ( _ccw ) then { _ccwMult = -1 } else { _ccwMult = 1 };
	if (not ((typeName _group_units_or_pos) == "ARRAY")) then {
		_units = [[["_x"], {alive _x}] call fnc_lambda,
			  units _group_units_or_pos] call fnc_filter;
	} else {
	        if ((typeName (_group_units_or_pos select 0)) == "SCALAR") then {
			_group = createGroup sideLogic;
			_units = [_group createUnit ["LOGIC",
						     _group_units_or_pos,
						     [], 0, ""]];
		} else {
			_units = [[["_x"], {alive _x}] call fnc_lambda,
				  _group_units_or_pos] call fnc_filter;
		};
	};

	_helicopter setVariable ["orbitIdle", false];
	_helicopter setVariable ["_doOrbit",  true ];

	_helicopter setVariable ["_updateOrbitSleepDone", true];
	while { (0 < (count _units)) and
		(canMove _helicopter) and
		(alive (driver _helicopter)) and
		(_helicopter getVariable "_doOrbit") } do {
		scopeName "OrbitRefresh";
		if (not (isNil "_position")) then { _lastPos = _position };
		_position = ([[["_x"], {position _x}] call fnc_lambda,
			      _units] call fnc_map) call fnc_vector_mean;
		if (isNil "_lastPos") then {		     
			[_self, "orbit_position", _position,
			 [_helicopter], _radius, _altitude,
			 true, _ccw, _points, _star] call fnc_tell;
		} else {
			if (_tolerance < ([_position,
				           _lastPos] call fnc_euclidean_distance)) then {
			[_self, "orbit_position", _position,
			 [_helicopter], _radius, _altitude,
			 true, _ccw, _points, _star] call fnc_tell;
			};
		};
		_helicopter setVariable ["_updateOrbitSleepDone", false];
		_helicopter spawn {
		        private ["_unbroken"];
			_unbroken = true;
			for "_i" from 1 to 10 do {
			  scopeName "WaitForRefresh";
			  sleep 2;
			  if ( not (_this getVariable "_doOrbit") ) then {
				  _unbroken = false;
				  breakOut "WaitForRefresh";
			  };
			};
			if (_unbroken) then {
				_this setVariable ["_updateOrbitSleepDone", true];
			};
		};
		waitUntil { (_helicopter getVariable "_updateOrbitSleepDone") or
			    (not (_helicopter getVariable "_doOrbit")) };
		if (_helicopter getVariable "_updateOrbitSleepDone") then {
			_helicopter setVariable ["_updateOrbitSleepDone", false];
			if (not ((typeName _group_units_or_pos) == "ARRAY")) then {
				_units = [[["_x"], {alive _x}] call fnc_lambda,
					  units _group_units_or_pos] call fnc_filter;
			} else {
				if (not ((typeName (_group_units_or_pos select 0)) == "SCALAR")) then {
					_units = [[["_x"], {alive _x}] call fnc_lambda,
						  _group_units_or_pos] call fnc_filter;
				};
			};			
		} else {
			breakOut "OrbitRefresh";
		};
	};

	_helicopter setVariable ["_eolOrbitSleepDone", false];
	_helicopter spawn {
	   sleep 2;
	   _this setVariable ["_eolOrbitSleepDone", true];
	};
	waitUntil { (_helicopter getVariable "_updateOrbitSleepDone") or
	            (_helicopter getVariable "_eolOrbitSleepDone")
	};
	
	if ((typeName _group_units_or_pos) == "ARRAY") then {
		if ((typeName (_group_units_or_pos select 0)) == "SCALAR") then {
			deleteVehicle (_units select 0);
			deleteGroup _group;
		};
	};
	
	_helicopter setVariable ["orbitIdle", true];
	
} ENDMETHODV;

DEFMETHOD("Headquarters", "abort_pattern") ["_self", "_helicopter", "_recur"] DO {
	private ["_busy"];
	_busy = _helicopter getVariable "_eolAbortSleepDone";
	if (isNil "_busy" ) then {  _busy = false } else { _busy = true };
	if (isNil "_recur") then { _recur = false };
	if (_recur or (not _busy)) then {
 	    [_self, _helicopter] spawn
	    ([["_hq", "_heli"], 
	      {
	       private ["_idle"];
	       _idle = _heli getVariable "arcIdle";
	       if (isNil "_idle") then { _heli setVariable ["arcIdle", true] };
	       _idle = nil;
	       _idle = _heli getVariable "orbitIdle";
	       if (isNil "_idle") then { _heli setVariable ["orbitIdle", true] };
	       _heli setVariable ["_doOrbit", false];
	       _heli setVariable ["_doHoldingArc", false];

	       _heli setVariable ["_eolAbortSleepDone", false];
	       _heli spawn {
		  sleep 2;
		  _this setVariable ["_eolAbortSleepDone", true];
	       };
	       waitUntil {
		       sleep 0.5;
		       ((_heli getVariable "orbitIdle") and
			(_heli getVariable "arcIdle")) or
		       _heli getVariable "_eolAbortSleepDone"
	       };
	       if (_heli getVariable "_eolAbortSleepDone") then {
		       [_hq, "abort_pattern", _heli, true] call fnc_tell;
	       } else {
		       sleep 2;
		       hint "patterns aborted, helicopter idle";
		       _heli setVariable ["_eolAbortSleepDone", nil];
	       };
	      }] call fnc_lambda);
	};
} ENDMETHOD;
