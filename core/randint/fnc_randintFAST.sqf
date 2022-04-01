///////////////////////////// fnc_randint ////////////////////////// 2022-03-25
/*  Random integer using the Arma's PRNG directly.  */                 //
//////////////////////////////////////////////////////                ////
private [              //                                            //////
        "_min",        // int [IN   /B1]                            ///  ///
        "_max"         // int [IN/A1/B2]                           ///    /// 
                                                                  ///      ///
];  /////////////////////////////////////// <dwringer@gmail.com> ///        ///
switch (count _this) do {
        case 1: {
                _min = 0;
		_max = _this select 0;
	};
	case 2: {
		_min = _this select 0;
		_max = _this select 1;
	};
};
(_min + (floor (random (_max - _min))))  // RETURN ////////////////////////////
