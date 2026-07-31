{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 9,
			"minor" : 0,
			"revision" : 9,
			"architecture" : "x64",
			"modernui" : 1
		}
,
		"classnamespace" : "box",
		"rect" : [ 100.0, 100.0, 760.0, 420.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 10.0, 10.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "enabled", 1 ], [ "allowEmptySelection", 1 ], [ "selectionMode", "custom" ], [ "count", 1 ], [ "loadingIndex", 0 ], [ "buttonModes", "momentary", "toggle", "toggle" ], [ "layout", "horizontal" ], [ "labels", "Match EQ" ] ],
					"filename" : "../../Shared/Interface/ButtonGroup/ButtonGroupControl.js",
					"id" : "match-control",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 120.0, 230.0, 90.0, 30.0 ],
					"presentation" : 0,
					"presentation_rect" : [ 0.0, 0.0, 90.0, 30.0 ],
					"varname" : "approximator.match"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "enabled", 1 ], [ "mode", "momentary" ], [ "label", "Clear" ], [ "value", 0 ] ],
					"filename" : "../../Shared/Interface/Button/ButtonControl.js",
					"id" : "clear-control",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 220.0, 230.0, 90.0, 30.0 ],
					"presentation" : 0,
					"presentation_rect" : [ 100.0, 0.0, 90.0, 30.0 ],
					"varname" : "approximator.clear"
				}

			}
, 			{
				"box" : 				{
					"id" : "clear-prepend",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 220.0, 270.0, 95.0, 22.0 ],
					"text" : "prepend clear"
				}

			}
, 			{
				"box" : 				{
					"id" : "match-prepend",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 120.0, 270.0, 95.0, 22.0 ],
					"text" : "prepend match"
				}

			}
, 			{
				"box" : 				{
					"id" : "controller",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 3,
					"outlettype" : [ "", "", "" ],
					"patching_rect" : [ 120.0, 330.0, 320.0, 22.0 ],
					"saved_object_attributes" : 					{
						"filename" : "Controllers/consolidator.approximator.controller.js",
						"parameter_enable" : 0
					}
,
					"text" : "js Controllers/consolidator.approximator.controller.js"
				}

			}
, 			{
				"box" : 				{
					"id" : "thispatcher",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 421.0, 370.0, 80.0, 22.0 ],
					"save" : [ "#N", "thispatcher", ";", "#Q", "end", ";" ],
					"text" : "thispatcher"
				}

			}
, 			{
				"box" : 				{
					"id" : "bus-receive",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 130.0, 110.0, 145.0, 22.0 ],
					"text" : "r ---message.bus.out"
				}

			}
, 			{
				"box" : 				{
					"id" : "eq-state-receive",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 280.0, 110.0, 110.0, 22.0 ],
					"text" : "r ---state.eq"
				}

			}
, 			{
				"box" : 				{
					"id" : "processor-state-receive",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 400.0, 110.0, 135.0, 22.0 ],
					"text" : "r ---state.processor"
				}

			}
, 			{
				"box" : 				{
					"id" : "controller-bus-send",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 120.0, 370.0, 118.0, 22.0 ],
					"text" : "s ---message.bus.in"
				}

			}
, 			{
				"box" : 				{
					"id" : "native",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "", "", "" ],
					"patching_rect" : [ 280.0, 160.0, 300.0, 22.0 ],
					"text" : "consolidator.approximator"
				}

			}
, 			{
				"box" : 				{
					"id" : "native-bus-send",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 280.0, 200.0, 145.0, 22.0 ],
					"text" : "s ---message.bus.in"
				}

			}
, 			{
				"box" : 				{
					"id" : "status-print",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 270.5, 370.0, 145.0, 22.0 ],
					"text" : "print approximator.status"
				}

			}
, 			{
				"box" : 				{
					"id" : "debug-print",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 561.0, 200.0, 120.0, 22.0 ],
					"text" : "print approximator"
				}

			}
, 			{
				"box" : 				{
					"id" : "fit-curve-receive",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 440.0, 270.0, 145.0, 22.0 ],
					"text" : "r ---analyzer.curves"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "native", 0 ],
					"source" : [ "bus-receive", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "clear-prepend", 0 ],
					"source" : [ "clear-control", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 0 ],
					"source" : [ "clear-prepend", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller-bus-send", 0 ],
					"source" : [ "controller", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "status-print", 0 ],
					"source" : [ "controller", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "thispatcher", 0 ],
					"source" : [ "controller", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "native", 0 ],
					"source" : [ "eq-state-receive", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 1 ],
					"source" : [ "fit-curve-receive", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "match-prepend", 0 ],
					"source" : [ "match-control", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 0 ],
					"source" : [ "match-prepend", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 1 ],
					"source" : [ "native", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "debug-print", 0 ],
					"source" : [ "native", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "native-bus-send", 0 ],
					"source" : [ "native", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "native", 0 ],
					"source" : [ "processor-state-receive", 0 ]
				}

			}
 ],
		"oscreceiveudpport" : 0
	}

}
