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
		"rect" : [ 133.0, 150.0, 1000.0, 780.0 ],
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
					"patching_rect" : [ 0.0, 0.0, 200.0, 112.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 143.0, 0.0, 117.0, 78.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Features/Analyzer/Controls/AnalyzerControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "compressor_detector"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Shared/Controls/Dial/DialControl.js",
					"id" : "attack",
					"jsarguments" : [ "ATTACK", 0.0, 1.0, 0, 100.0, 1, "%" ],
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 112.0, 64.0, 56.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 13.0, 0.0, 65.0, 65.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Shared/Controls/Dial/DialControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "compressor_attack"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Shared/Controls/Dial/DialControl.js",
					"id" : "sustain",
					"jsarguments" : [ "SUSTAIN", 0.0, 1.0, 0, 100.0, 1, "%" ],
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 64.0, 112.0, 64.0, 56.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 78.0, 0.0, 65.0, 65.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Shared/Controls/Dial/DialControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "compressor_sustain"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Shared/Controls/Dial/DialControl.js",
					"id" : "compression",
					"jsarguments" : [ "COMPRESSION", 0.0, 1.0, 0, 100.0, 1, "%" ],
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 128.0, 112.0, 64.0, 56.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 13.0, 65.0, 65.0, 65.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Shared/Controls/Dial/DialControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "compressor_compression"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Shared/Controls/MultiValueToggle/MultiValueToggleControl.js",
					"id" : "character",
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 192.0, 112.0, 64.0, 56.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 78.0, 65.0, 65.0, 65.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Shared/Controls/MultiValueToggle/MultiValueToggleControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "compressor_character"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Shared/Controls/Dial/DialControl.js",
					"id" : "output",
					"jsarguments" : [ "OUTPUT", -36.0, 36.0, 0, 1.0, 1, "dB" ],
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 256.0, 112.0, 64.0, 56.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 325.0, 0.0, 65.0, 65.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Shared/Controls/Dial/DialControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "compressor_output"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Shared/Controls/Toggle/ToggleControl.js",
					"id" : "parallel",
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 320.0, 112.0, 64.0, 56.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 143.0, 91.0, 117.0, 26.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Shared/Controls/Toggle/ToggleControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "compressor_parallel"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-threshold",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 216.0, 456.0, 180.0, 22.0 ],
					"text" : "prepend compressor_attack"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-ratio",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 216.0, 488.0, 163.0, 22.0 ],
					"text" : "prepend compressor_sustain"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-attack",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 216.0, 520.0, 193.0, 22.0 ],
					"text" : "prepend compressor_compression"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-release",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 216.0, 552.0, 175.0, 22.0 ],
					"text" : "prepend compressor_character"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-gain",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 216.0, 584.0, 159.0, 22.0 ],
					"text" : "prepend compressor_output"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-mix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 216.0, 616.0, 164.0, 22.0 ],
					"text" : "prepend compressor_parallel"
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
					"patching_rect" : [ 8.0, 328.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Hosts/PanelBindingHostV8.js",
					"id" : "router",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 8.0, 368.0, 203.0, 22.0 ],
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
					"id" : "prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 216.0, 424.0, 190.0, 22.0 ],
					"text" : "prepend compressor_detector"
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
					"patching_rect" : [ 72.0, 568.0, 30.0, 30.0 ]
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "p-threshold", 0 ],
					"source" : [ "attack", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-release", 0 ],
					"source" : [ "character", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-attack", 0 ],
					"source" : [ "compression", 0 ]
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
					"destination" : [ "router", 0 ],
					"source" : [ "in", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-gain", 0 ],
					"source" : [ "output", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "out", 0 ],
					"source" : [ "p-attack", 0 ]
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
					"source" : [ "p-ratio", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "out", 0 ],
					"source" : [ "p-release", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "out", 0 ],
					"source" : [ "p-threshold", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-mix", 0 ],
					"source" : [ "parallel", 0 ]
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
					"destination" : [ "p-ratio", 0 ],
					"source" : [ "sustain", 0 ]
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
