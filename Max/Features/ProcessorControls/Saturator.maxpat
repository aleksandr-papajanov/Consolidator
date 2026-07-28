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
		"rect" : [ 519.0, 124.0, 600.0, 780.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 10.0, 10.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"border" : 0,
					"filename" : "consolidator.processorcontrols.detectorcurve.js",
					"id" : "saturator-detector-curve",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 90.0, 50.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 90.0, 50.0 ],
					"varname" : "saturator.detectorCurve"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "tertiaryIndicator", 0 ], [ "tertiaryValue", 0 ], [ "valueCount", 2 ], [ "enabled", 1 ], [ "primaryIndicator", 0.43551350983974 ], [ "secondaryValue", 0.5 ], [ "primaryValue", 0.5 ], [ "secondaryIndicator", -0.031877848458844 ] ],
					"filename" : "DialControl.js",
					"id" : "obj-8",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 140.0, 50.0, 70.0, 60.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 140.0, 50.0, 70.0, 60.0 ],
					"varname" : "saturator.saturationOutput"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "tertiaryIndicator", 0 ], [ "tertiaryValue", 0.342302673496236 ], [ "valueCount", 3 ], [ "enabled", 1 ], [ "primaryIndicator", 0 ], [ "secondaryValue", 0.566323334778673 ], [ "primaryValue", 0.5 ], [ "secondaryIndicator", 0 ] ],
					"filename" : "DialControl.js",
					"id" : "obj-9",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 70.0, 120.0, 60.0, 50.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 70.0, 120.0, 60.0, 50.0 ],
					"varname" : "saturator.detector.2"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "buttonModes", "toggle", "momentary", "toggle" ], [ "layout", "horizontal" ], [ "allowEmptySelection", 1 ], [ "enabled", 1 ], [ "count", 3 ], [ "loadingIndex", 0 ], [ "selectionMode", "custom" ], [ "labels", "B", "R", "L" ] ],
					"filename" : "ButtonGroupControl.js",
					"id" : "obj-11",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 50.0, 90.0, 30.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 50.0, 90.0, 30.0 ],
					"varname" : "saturator.control"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "tertiaryIndicator", 0 ], [ "tertiaryValue", 0.342302673496236 ], [ "valueCount", 3 ], [ "enabled", 1 ], [ "primaryIndicator", 0 ], [ "secondaryValue", 0.566323334778673 ], [ "primaryValue", 0.5 ], [ "secondaryIndicator", 0 ] ],
					"filename" : "DialControl.js",
					"id" : "obj-13",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 120.0, 60.0, 50.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 120.0, 60.0, 50.0 ],
					"varname" : "saturator.detector.1"
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
						"filename" : "consolidator.processorcontrols.controller.js",
						"parameter_enable" : 0
					}
,
					"text" : "js consolidator.processorcontrols.controller.js saturator"
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
					"id" : "saturator-prefix-control",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 290.0, 150.0, 22.0 ],
					"text" : "prepend saturator control"
				}

			}
, 			{
				"box" : 				{
					"id" : "saturator-prefix-input-output",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 250.0, 180.0, 22.0 ],
					"text" : "prepend saturator saturation-output"
				}

			}
, 			{
				"box" : 				{
					"id" : "saturator-prefix-detector-1",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 30.0, 260.0, 180.0, 22.0 ],
					"text" : "prepend saturator detector 1"
				}

			}
, 			{
				"box" : 				{
					"id" : "saturator-prefix-detector-2",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 160.0, 140.0, 180.0, 22.0 ],
					"text" : "prepend saturator detector 2"
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
					"text" : "r ---link.control.state"
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
, 			{
				"box" : 				{
					"id" : "saturator-definitions-receive",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 480.0, 330.0, 145.0, 22.0 ],
					"text" : "r ---state.definitions"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "saturator-prefix-control", 0 ],
					"source" : [ "obj-11", 0 ]
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
					"destination" : [ "saturator-prefix-detector-1", 0 ],
					"source" : [ "obj-13", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-prefix-input-output", 0 ],
					"source" : [ "obj-8", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-prefix-detector-2", 0 ],
					"source" : [ "obj-9", 0 ]
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
					"destination" : [ "saturator-controller", 0 ],
					"source" : [ "saturator-prefix-control", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-controller", 0 ],
					"source" : [ "saturator-prefix-detector-1", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-controller", 0 ],
					"source" : [ "saturator-prefix-detector-2", 0 ]
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
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-gesture-send", 0 ],
					"source" : [ "saturator-controller", 3 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-controller", 1 ],
					"source" : [ "saturator-definitions-receive", 0 ]
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
