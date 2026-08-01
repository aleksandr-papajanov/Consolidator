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
		"rect" : [ 679.0, 131.0, 600.0, 780.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 10.0, 10.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"border" : 0,
					"filename" : "./consolidator.processorcontrols.detectorcurve.js",
					"id" : "saturator-detector-curve",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 90.0, 110.0, 60.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 90.0, 110.0, 60.0 ],
					"varname" : "saturator.detectorCurve"
				}

			}
, 			{
				"box" : 				{
					"id" : "saturator-detector-curve-command",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 175.0, 120.0, 22.0 ],
					"text" : "prepend saturator"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "primaryIndicator", 0 ], [ "levelMatchEnabled", 1 ], [ "onsetMatchEnabled", 1 ], [ "primaryValue", 0 ], [ "secondaryIndicator", 0 ], [ "tertiaryValue", 0.02 ], [ "tertiaryIndicator", 0 ], [ "activityEnabled", 1 ], [ "valueCount", 3 ], [ "listenEnabled", 0 ], [ "secondaryValue", 0.5 ], [ "enabled", 1 ] ],
					"filename" : "../../Shared/Interface/Dial/DialControl.js",
					"id" : "obj-8",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 20.0, 20.0, 70.0, 60.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 20.0, 20.0, 70.0, 60.0 ],
					"varname" : "saturator.saturationOutput"
				}

			}
, 			{
				"box" : 				{
					"id" : "saturator-controller",
					"maxclass" : "newobj",
					"numinlets" : 3,
					"numoutlets" : 4,
					"outlettype" : [ "", "", "", "" ],
					"patching_rect" : [ 240.0, 380.0, 297.0, 22.0 ],
					"saved_object_attributes" : 					{
						"filename" : "Controllers/consolidator.saturator.controller.js",
						"parameter_enable" : 0
					}
,
					"text" : "js Controllers/consolidator.saturator.controller.js"
				}

			}
, 			{
				"box" : 				{
					"id" : "saturator-bus-in",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 480.0, 340.0, 130.0, 22.0 ],
					"text" : "r ---state.processor"
				}

			}
, 			{
				"box" : 				{
					"id" : "saturator-bus-out",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 240.0, 420.0, 120.0, 22.0 ],
					"text" : "s ---message.bus.in"
				}

			}
, 			{
				"box" : 				{
					"id" : "saturator-thispatcher",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 360.0, 420.0, 80.0, 22.0 ],
					"save" : [ "#N", "thispatcher", ";", "#Q", "end", ";" ],
					"text" : "thispatcher"
				}

			}
, 			{
				"box" : 				{
					"id" : "saturator-print",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 480.0, 420.0, 140.0, 22.0 ],
					"text" : "print saturator.controls"
				}

			}
, 			{
				"box" : 				{
					"id" : "saturator-prefix-input-output",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 250.0, 197.0, 22.0 ],
					"text" : "prepend saturator saturation-output"
				}

			}
, 			{
				"box" : 				{
					"id" : "saturator-telemetry",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 480.0, 300.0, 145.0, 22.0 ],
					"text" : "r ---processor.telemetry"
				}

			}
, 			{
				"box" : 				{
					"id" : "saturator-target-level",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 480.0, 270.0, 145.0, 22.0 ],
					"text" : "r ---input.gain.target"
				}

			}
, 			{
				"box" : 				{
					"id" : "saturator-processor-limits",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 480.0, 300.0, 170.0, 22.0 ],
					"text" : "r ---link.control.processor"
				}

			}
, 			{
				"box" : 				{
					"id" : "saturator-gesture-send",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 240.0, 450.0, 175.0, 22.0 ],
					"text" : "s ---link.parameter.gesture"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "saturator-prefix-input-output", 0 ],
					"source" : [ "obj-8", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-controller", 1 ],
					"source" : [ "saturator-bus-in", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-bus-out", 0 ],
					"source" : [ "saturator-controller", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-gesture-send", 0 ],
					"source" : [ "saturator-controller", 3 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-print", 0 ],
					"source" : [ "saturator-controller", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-thispatcher", 0 ],
					"source" : [ "saturator-controller", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-detector-curve-command", 0 ],
					"source" : [ "saturator-detector-curve", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-controller", 0 ],
					"source" : [ "saturator-detector-curve-command", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-controller", 0 ],
					"source" : [ "saturator-prefix-input-output", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-controller", 0 ],
					"source" : [ "saturator-processor-limits", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-controller", 2 ],
					"source" : [ "saturator-target-level", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-controller", 2 ],
					"source" : [ "saturator-telemetry", 0 ]
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
