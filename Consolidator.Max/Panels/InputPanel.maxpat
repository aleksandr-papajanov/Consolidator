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
		"rect" : [ 59.0, 107.0, 1000.0, 780.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 13.0, 13.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"filename" : "Project:/js/Features/Analyzer/Controls/AnalyzerControl.js",
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
						"filename" : "Project:/js/Features/Analyzer/Controls/AnalyzerControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "input_detector"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Shared/Controls/Dial/DialControl.js",
					"id" : "level",
					"jsarguments" : [ "LEVEL", -36.0, 36.0, 0, 1.0, 1, "dB" ],
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 64.0, 64.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 13.0, 0.0, 65.0, 65.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Shared/Controls/Dial/DialControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "input_level"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Shared/Controls/Dial/DialControl.js",
					"id" : "target",
					"jsarguments" : [ "TARGET", -36.0, 0.0, 0, 1.0, 1, "dB" ],
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 64.0, 64.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 78.0, 0.0, 65.0, 65.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Shared/Controls/Dial/DialControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "input_target"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Shared/Controls/Toggle/ToggleControl.js",
					"id" : "leveler",
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 64.0, 64.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 143.0, 91.0, 117.0, 26.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Shared/Controls/Toggle/ToggleControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "input_leveler"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Shared/Controls/Dial/DialControl.js",
					"id" : "width",
					"jsarguments" : [ "WIDTH", 0.0, 1.0, 0, 100.0, 1, "%" ],
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 64.0, 64.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 325.0, 0.0, 65.0, 65.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Shared/Controls/Dial/DialControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "input_width"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-detector",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 0.0, 160.0, 22.0 ],
					"text" : "prepend input_detector"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-level",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 0.0, 113.0, 22.0 ],
					"text" : "prepend input_level"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-target",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 0.0, 121.0, 22.0 ],
					"text" : "prepend input_target"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-leveler",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 0.0, 124.0, 22.0 ],
					"text" : "prepend input_leveler"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-width",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 0.0, 117.0, 22.0 ],
					"text" : "prepend input_width"
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
					"patching_rect" : [ 0.0, 0.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Hosts/PanelBindingHostV8.js",
					"id" : "router",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 0.0, 0.0, 203.0, 22.0 ],
					"saved_object_attributes" : 					{
						"parameter_enable" : 0
					}
,
					"text" : "v8 Project:/js/Hosts/PanelBindingHostV8.js",
					"textfile" : 					{
						"filename" : "Project:/js/Hosts/PanelBindingHostV8.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}

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
					"patching_rect" : [ 0.0, 0.0, 30.0, 30.0 ]
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
					"patching_rect" : [ 790.0, 1229.0, 128.0, 128.0 ],
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
					"destination" : [ "p-detector", 0 ],
					"source" : [ "detector", 0 ]
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
					"destination" : [ "p-level", 0 ],
					"source" : [ "level", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-leveler", 0 ],
					"source" : [ "leveler", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "out", 0 ],
					"source" : [ "p-detector", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "out", 0 ],
					"source" : [ "p-level", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "out", 0 ],
					"source" : [ "p-leveler", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "out", 0 ],
					"source" : [ "p-target", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "out", 0 ],
					"source" : [ "p-width", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-target", 0 ],
					"source" : [ "target", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-width", 0 ],
					"source" : [ "width", 0 ]
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
