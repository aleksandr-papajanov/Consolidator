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
		"rect" : [ 34.0, 77.0, 1612.0, 891.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 13.0, 13.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"filename" : "AnalyzerControl.js",
					"id" : "detector",
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 192.0, 112.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 143.0, 0.0, 117.0, 78.0 ],
					"textfile" : 					{
						"filename" : "AnalyzerControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "saturator_detector"
				}

			}
, 			{
				"box" : 				{
					"filename" : "DialControl.js",
					"id" : "drive",
					"jsarguments" : [ "DRIVE", 0.0, 1.0, 0, 100.0, 1, "%" ],
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 112.0, 64.0, 56.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 13.0, 0.0, 65.0, 65.0 ],
					"textfile" : 					{
						"filename" : "DialControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "saturator_drive"
				}

			}
, 			{
				"box" : 				{
					"filename" : "DialControl.js",
					"id" : "curve",
					"jsarguments" : [ "CURVE", 0.0, 1.0, 0, 100.0, 1, "%" ],
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 64.0, 112.0, 64.0, 56.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 78.0, 0.0, 65.0, 65.0 ],
					"textfile" : 					{
						"filename" : "DialControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "saturator_curve"
				}

			}
, 			{
				"box" : 				{
					"filename" : "ToggleControl.js",
					"id" : "split",
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 128.0, 112.0, 64.0, 56.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 143.0, 91.0, 117.0, 26.0 ],
					"textfile" : 					{
						"filename" : "ToggleControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "saturator_split"
				}

			}
, 			{
				"box" : 				{
					"filename" : "DialControl.js",
					"id" : "output",
					"jsarguments" : [ "OUTPUT", -36.0, 36.0, 0, 1.0, 1, "dB" ],
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 192.0, 112.0, 64.0, 56.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 325.0, 0.0, 65.0, 65.0 ],
					"textfile" : 					{
						"filename" : "DialControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "saturator_output"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-drive",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 320.0, 232.0, 150.0, 22.0 ],
					"text" : "prepend saturator_drive"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-gain",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 320.0, 264.0, 145.0, 22.0 ],
					"text" : "prepend saturator_curve"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-mix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 320.0, 296.0, 140.0, 22.0 ],
					"text" : "prepend saturator_split"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-detector-amount",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 320.0, 328.0, 220.0, 22.0 ],
					"text" : "prepend saturator_output"
				}

			}
, 			{
				"box" : 				{
					"comment" : "",
					"id" : "in",
					"index" : 1,
					"maxclass" : "inlet",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 360.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"filename" : "PanelBindingHostV8.js",
					"id" : "router",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 0.0, 406.0, 203.0, 22.0 ],
					"saved_object_attributes" : 					{
						"parameter_enable" : 0
					}
,
					"text" : "v8 Project:/js/PanelBindingHostV8.js",
					"textfile" : 					{
						"filename" : "PanelBindingHostV8.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}

				}

			}
, 			{
				"box" : 				{
					"id" : "prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 320.0, 200.0, 190.0, 22.0 ],
					"text" : "prepend saturator_detector"
				}

			}
, 			{
				"box" : 				{
					"comment" : "",
					"id" : "out",
					"index" : 1,
					"maxclass" : "outlet",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 160.0, 328.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"angle" : 270.0,
					"bgcolor" : [ 0.117647058823529, 0.117647058823529, 0.117647058823529, 0.0 ],
					"bordercolor" : [ 0.509803921568627, 0.741176470588235, 0.772549019607843, 0.0 ],
					"id" : "obj-5",
					"maxclass" : "panel",
					"mode" : 0,
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 764.0, 1071.0, 128.0, 128.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 403.0, 130.0 ],
					"proportion" : 0.39,
					"saved_attribute_attributes" : 					{
						"bgfillcolor" : 						{
							"expression" : ""
						}

					}

				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "p-gain", 0 ],
					"source" : [ "curve", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "prefix", 0 ],
					"source" : [ "detector", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-drive", 0 ],
					"source" : [ "drive", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "router", 0 ],
					"source" : [ "in", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-detector-amount", 0 ],
					"source" : [ "output", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "out", 0 ],
					"source" : [ "p-detector-amount", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "out", 0 ],
					"source" : [ "p-drive", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "out", 0 ],
					"source" : [ "p-gain", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "out", 0 ],
					"source" : [ "p-mix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "out", 0 ],
					"source" : [ "prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-mix", 0 ],
					"source" : [ "split", 0 ]
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
