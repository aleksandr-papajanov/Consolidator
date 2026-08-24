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
		"gridsize" : [ 8.0, 8.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Analyzer/AnalyzerControl.js",
					"id" : "detector",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 200.0, 112.0 ],
					"varname" : "compressor_detector"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Dial/DialControl.js",
					"id" : "threshold",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 112.0, 64.0, 56.0 ],
					"varname" : "compressor_threshold"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Dial/DialControl.js",
					"id" : "ratio",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 64.0, 112.0, 64.0, 56.0 ],
					"varname" : "compressor_ratio"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Dial/DialControl.js",
					"id" : "attack",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 128.0, 112.0, 64.0, 56.0 ],
					"varname" : "compressor_attack"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Dial/DialControl.js",
					"id" : "release",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 192.0, 112.0, 64.0, 56.0 ],
					"varname" : "compressor_release"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Dial/DialControl.js",
					"id" : "gain",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 256.0, 112.0, 64.0, 56.0 ],
					"varname" : "compressor_gain"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Dial/DialControl.js",
					"id" : "mix",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 320.0, 112.0, 64.0, 56.0 ],
					"varname" : "compressor_mix"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Button/ButtonControl.js",
					"id" : "bypass",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 200.0, 0.0, 96.0, 24.0 ],
					"varname" : "compressor_bypass"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Button/ButtonControl.js",
					"id" : "solo",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 200.0, 24.0, 96.0, 24.0 ],
					"varname" : "compressor_solo"
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
					"text" : "prepend compressor_threshold"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-ratio",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 216.0, 488.0, 160.0, 22.0 ],
					"text" : "prepend compressor_ratio"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-attack",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 216.0, 520.0, 170.0, 22.0 ],
					"text" : "prepend compressor_attack"
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
					"text" : "prepend compressor_release"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-gain",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 216.0, 584.0, 155.0, 22.0 ],
					"text" : "prepend compressor_gain"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-mix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 216.0, 616.0, 145.0, 22.0 ],
					"text" : "prepend compressor_mix"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-bypass",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 216.0, 648.0, 170.0, 22.0 ],
					"text" : "prepend compressor_bypass"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-solo",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 216.0, 680.0, 150.0, 22.0 ],
					"text" : "prepend compressor_solo"
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
					"id" : "router",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 8.0, 368.0, 198.0, 22.0 ],
					"saved_object_attributes" : 					{
						"filename" : "Project:/js/PanelBindingHost.js",
						"parameter_enable" : 0
					}
,
					"text" : "js Project:/js/PanelBindingHost.js"
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
					"destination" : [ "p-attack", 0 ],
					"source" : [ "attack", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-bypass", 0 ],
					"source" : [ "bypass", 0 ]
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
					"destination" : [ "p-gain", 0 ],
					"source" : [ "gain", 0 ]
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
					"destination" : [ "p-mix", 0 ],
					"source" : [ "mix", 0 ]
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
					"source" : [ "p-bypass", 0 ]
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
					"source" : [ "p-solo", 0 ]
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
					"destination" : [ "out", 0 ],
					"source" : [ "prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-ratio", 0 ],
					"source" : [ "ratio", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-release", 0 ],
					"source" : [ "release", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-solo", 0 ],
					"source" : [ "solo", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-threshold", 0 ],
					"source" : [ "threshold", 0 ]
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
