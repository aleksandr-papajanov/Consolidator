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
		"rect" : [ 477.0, 263.0, 360.0, 260.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 10.0, 10.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"comment" : "Optional local commands: set_gain <0..1>, set_target <0..1>, enabled <0|1>",
					"id" : "control-inlet",
					"index" : 1,
					"maxclass" : "inlet",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 20.0, 20.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "primaryIndicator", 0 ], [ "onsetMatchEnabled", 0 ], [ "primaryValue", 0.5 ], [ "secondaryIndicator", 0 ], [ "tertiaryValue", 0 ], [ "tertiaryIndicator", 0 ], [ "activityEnabled", 0 ], [ "valueCount", 2 ], [ "listenEnabled", 0 ], [ "secondaryValue", 0.7 ], [ "enabled", 1 ], [ "levelMatchEnabled", 1 ] ],
					"filename" : "../../Shared/Interface/Dial/DialControl.js",
					"id" : "gain-dial",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 70.0, 70.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 70.0, 70.0 ],
					"varname" : "gain.dial"
				}

			}
, 			{
				"box" : 				{
					"id" : "controller",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 5,
					"outlettype" : [ "", "", "", "", "" ],
					"patching_rect" : [ 80.0, 140.0, 265.0, 22.0 ],
					"saved_object_attributes" : 					{
						"filename" : "Controllers/consolidator.gain.controller.js",
						"parameter_enable" : 0
					}
,
					"text" : "js Controllers/consolidator.gain.controller.js #1"
				}

			}
, 			{
				"box" : 				{
					"id" : "telemetry",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 180.0, 20.0, 145.0, 22.0 ],
					"text" : "r ---processor.telemetry"
				}

			}
, 			{
				"box" : 				{
					"id" : "message-bus-out",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 180.0, 55.0, 135.0, 22.0 ],
					"text" : "r ---state.processor"
				}

			}
, 			{
				"box" : 				{
					"id" : "processor-limits",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 180.0, 90.0, 165.0, 22.0 ],
					"text" : "r ---link.control.processor"
				}

			}
, 			{
				"box" : 				{
					"id" : "message-bus-in",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 80.0, 200.0, 130.0, 22.0 ],
					"text" : "s ---message.bus.in"
				}

			}
, 			{
				"box" : 				{
					"id" : "debug",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 225.0, 200.0, 70.0, 22.0 ],
					"text" : "print gain"
				}

			}
, 			{
				"box" : 				{
					"id" : "target-send",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 225.0, 225.0, 135.0, 22.0 ],
					"text" : "s ---input.gain.target"
				}

			}
, 			{
				"box" : 				{
					"id" : "gesture-send",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 80.0, 230.0, 175.0, 22.0 ],
					"text" : "s ---link.parameter.gesture"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "controller", 0 ],
					"source" : [ "control-inlet", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "debug", 0 ],
					"source" : [ "controller", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "gain-dial", 0 ],
					"source" : [ "controller", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "gesture-send", 0 ],
					"source" : [ "controller", 4 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "message-bus-in", 0 ],
					"source" : [ "controller", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "target-send", 0 ],
					"source" : [ "controller", 3 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 1 ],
					"source" : [ "gain-dial", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 0 ],
					"source" : [ "message-bus-out", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 0 ],
					"source" : [ "processor-limits", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 0 ],
					"source" : [ "telemetry", 0 ]
				}

			}
 ],
		"saved_attribute_attributes" : 		{
			"default_plcolor" : 			{
				"expression" : ""
			}

		}
,
		"oscreceiveudpport" : 0
	}

}
