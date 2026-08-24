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
		"rect" : [ 668.0, 221.0, 1000.0, 780.0 ],
		"gridsize" : [ 8.0, 8.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Analyzer/AnalyzerControl.js",
					"id" : "analyzer",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 432.0, 144.0 ],
					"varname" : "equalizer_analyzer"
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
					"patching_rect" : [ 224.0, 144.0, 104.0, 24.0 ],
					"varname" : "equalizer_bypass"
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
					"patching_rect" : [ 328.0, 144.0, 104.0, 24.0 ],
					"varname" : "equalizer_solo"
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
					"patching_rect" : [ 0.0, 352.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "router",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 398.0, 198.0, 22.0 ],
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
					"patching_rect" : [ 0.0, 208.0, 180.0, 22.0 ],
					"text" : "prepend equalizer_analyzer"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-bypass",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 224.0, 208.0, 160.0, 22.0 ],
					"text" : "prepend equalizer_bypass"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-solo",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 328.0, 240.0, 145.0, 22.0 ],
					"text" : "prepend equalizer_solo"
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
					"patching_rect" : [ 224.0, 312.0, 30.0, 30.0 ]
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "prefix", 0 ],
					"source" : [ "analyzer", 0 ]
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
					"destination" : [ "router", 0 ],
					"source" : [ "in", 0 ]
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
					"source" : [ "p-solo", 0 ]
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
					"destination" : [ "p-solo", 0 ],
					"source" : [ "solo", 0 ]
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
