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
		"rect" : [ 683.0, 141.0, 600.0, 780.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 10.0, 10.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"border" : 0,
					"filename" : "consolidator.processorcontrols.detectorcurve.js",
					"id" : "compressor-detector-curve",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 90.0, 50.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 90.0, 50.0 ],
					"varname" : "compressor.detectorCurve"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "tertiaryIndicator", 0 ], [ "tertiaryValue", 0 ], [ "valueCount", 2 ], [ "enabled", 1 ], [ "primaryIndicator", 0 ], [ "secondaryValue", 0.5 ], [ "primaryValue", 0.5 ], [ "secondaryIndicator", 0.084038818560153 ] ],
					"filename" : "DialControl.js",
					"id" : "obj-8",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 140.625, 9.6875, 70.0, 60.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 140.625, 9.6875, 70.0, 60.0 ],
					"varname" : "compressor.thresholdOutput"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "tertiaryIndicator", 0 ], [ "tertiaryValue", 0.342302673496236 ], [ "valueCount", 3 ], [ "enabled", 1 ], [ "primaryIndicator", 0 ], [ "secondaryValue", 0.566323334778673 ], [ "primaryValue", 0.5 ], [ "secondaryIndicator", 0 ] ],
					"filename" : "DialControl.js",
					"id" : "obj-3",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 70.3125, 120.625, 60.0, 50.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 70.3125, 120.625, 60.0, 50.0 ],
					"varname" : "compressor.detector.2"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "enabled", 1 ], [ "value", 1 ] ],
					"filename" : "SliderControl.js",
					"id" : "obj-7",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 140.625, 144.0625, 70.0, 27.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 140.625, 144.0625, 70.0, 27.0 ],
					"varname" : "compressor.mix"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "buttonModes", "toggle", "momentary", "toggle" ], [ "layout", "horizontal" ], [ "allowEmptySelection", 1 ], [ "enabled", 1 ], [ "count", 3 ], [ "loadingIndex", 0 ], [ "selectionMode", "custom" ], [ "labels", "B", "R", "L" ] ],
					"filename" : "ButtonGroupControl.js",
					"id" : "obj-1",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 50.0, 90.0, 30.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 50.0, 90.0, 30.0 ],
					"varname" : "compressor.control"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "tertiaryIndicator", 0 ], [ "tertiaryValue", 0.342302673496236 ], [ "valueCount", 3 ], [ "enabled", 1 ], [ "primaryIndicator", 0 ], [ "secondaryValue", 0.566323334778673 ], [ "primaryValue", 0.5 ], [ "secondaryIndicator", 0 ] ],
					"filename" : "DialControl.js",
					"id" : "obj-2",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 120.625, 60.0, 50.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 120.625, 60.0, 50.0 ],
					"varname" : "compressor.detector.1"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "tertiaryIndicator", 0 ], [ "tertiaryValue", 0 ], [ "valueCount", 2 ], [ "enabled", 1 ], [ "primaryIndicator", 0 ], [ "secondaryValue", 0.5 ], [ "primaryValue", 0.540691056606448 ], [ "secondaryIndicator", 0 ] ],
					"filename" : "DialControl.js",
					"id" : "obj-10",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 140.625, 80.0, 70.0, 60.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 140.625, 80.0, 70.0, 60.0 ],
					"varname" : "compressor.attackRelease"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-controller",
					"maxclass" : "newobj",
					"numinlets" : 3,
					"numoutlets" : 4,
					"outlettype" : [ "", "", "", "" ],
					"patching_rect" : [ 250.0, 490.9375, 313.0, 22.0 ],
					"saved_object_attributes" : 					{
						"filename" : "consolidator.processorcontrols.controller.js",
						"parameter_enable" : 0
					}
,
					"text" : "js consolidator.processorcontrols.controller.js compressor"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-bus-in",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 490.625, 450.3125, 130.0, 22.0 ],
					"text" : "r ---state.processor"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-bus-out",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 250.0, 530.0, 120.0, 22.0 ],
					"text" : "s ---message.bus.in"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-thispatcher",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 370.3125, 530.0, 80.0, 22.0 ],
					"save" : [ "#N", "thispatcher", ";", "#Q", "end", ";" ],
					"text" : "thispatcher"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-print",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 490.625, 530.0, 143.0, 22.0 ],
					"text" : "print compressor.controls"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-prefix-control",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 340.9375, 158.0, 22.0 ],
					"text" : "prepend compressor control"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-prefix-mix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 140.625, 250.3125, 141.0, 22.0 ],
					"text" : "prepend compressor mix"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-prefix-input-output",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 300.0, 250.3125, 185.0, 22.0 ],
					"text" : "prepend compressor threshold-output"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-prefix-attack-release",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 300.0, 280.0, 197.0, 22.0 ],
					"text" : "prepend compressor attack-release"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-prefix-detector-1",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 20.3125, 309.6875, 180.0, 22.0 ],
					"text" : "prepend compressor detector 1"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-prefix-detector-2",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 170.3125, 220.625, 180.0, 22.0 ],
					"text" : "prepend compressor detector 2"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-telemetry",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 490.625, 410.0, 145.0, 22.0 ],
					"text" : "r ---processor.telemetry"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-target-level",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 490.625, 380.0, 145.0, 22.0 ],
					"text" : "r ---input.gain.target"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-processor-limits",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 490.625, 410.0, 170.0, 22.0 ],
					"text" : "r ---link.control.state"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-gesture-send",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 250.0, 560.0, 175.0, 22.0 ],
					"text" : "s ---link.parameter.gesture"
				}

			}
, 			{
				"box" : 				{
					"id" : "compressor-definitions-receive",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 490.625, 440.0, 145.0, 22.0 ],
					"text" : "r ---state.definitions"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "compressor-controller", 1 ],
					"source" : [ "compressor-bus-in", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-controller", 0 ],
					"source" : [ "compressor-processor-limits", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-bus-out", 0 ],
					"source" : [ "compressor-controller", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-print", 0 ],
					"source" : [ "compressor-controller", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-thispatcher", 0 ],
					"source" : [ "compressor-controller", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-controller", 0 ],
					"source" : [ "compressor-prefix-attack-release", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-controller", 0 ],
					"source" : [ "compressor-prefix-control", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-controller", 0 ],
					"source" : [ "compressor-prefix-detector-1", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-controller", 0 ],
					"source" : [ "compressor-prefix-detector-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-controller", 0 ],
					"source" : [ "compressor-prefix-input-output", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-controller", 0 ],
					"source" : [ "compressor-prefix-mix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-controller", 2 ],
					"source" : [ "compressor-target-level", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-controller", 2 ],
					"source" : [ "compressor-telemetry", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-prefix-control", 0 ],
					"source" : [ "obj-1", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-prefix-attack-release", 0 ],
					"source" : [ "obj-10", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-prefix-detector-1", 0 ],
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-prefix-detector-2", 0 ],
					"source" : [ "obj-3", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-prefix-mix", 0 ],
					"source" : [ "obj-7", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-prefix-input-output", 0 ],
					"source" : [ "obj-8", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-gesture-send", 0 ],
					"source" : [ "compressor-controller", 3 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-controller", 1 ],
					"source" : [ "compressor-definitions-receive", 0 ]
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
