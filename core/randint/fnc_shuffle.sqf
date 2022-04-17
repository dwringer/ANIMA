////////////////////////////// fnc_shuffle ///////////////////////// 2022-04-16
/*  Return a shuffled copy of an array  */
//////////////////////////////////////////
private [           //
         "_arr",    // Array [IN]                                      //
//       "_newArr"  // Array [OUT]                                    //// 
         "_temp",   //                                               ////// 
         "_swapi"   //                                              ///  ///  
                    //                                             ///    ///
                    //                                            ///      ///
];  /////////////////////////////////////// <dwringer@gmail.com> ///        ///
_arr = _this;

private ["_j", "_swap"];
for "_i" from ((count _arr) - 1) to 1 step -1 do {
  _swap = _arr select _i;
  _j = [_i+1] call fnc_randint;
  _arr set [_i, _arr select _j];
  _arr set [_j, _swap];
};
_arr;  // RETURN ///////////////////////////////////////////////////////////
