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
		"rect" : [ 454.0, 441.0, 1000.0, 780.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 13.0, 13.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Button/ButtonControl.js",
					"id" : "bypass",
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 221.0, 169.0, 104.0, 24.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 104.0, 24.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Controls/Button/ButtonControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "equalizer_bypass"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Button/ButtonControl.js",
					"id" : "solo",
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 403.0, 169.0, 104.0, 24.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 247.0, 0.0, 104.0, 24.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Controls/Button/ButtonControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "equalizer_solo"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Analyzer/AnalyzerControl.js",
					"id" : "analyzer",
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 432.0, 144.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 351.0, 169.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Controls/Analyzer/AnalyzerControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "equalizer_analyzer"
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
					"filename" : "Project:/js/PanelBindingHostV8.js",
					"id" : "router",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 0.0, 398.0, 203.0, 22.0 ],
					"saved_object_attributes" : 					{
						"parameter_enable" : 0
					}
,
					"text" : "v8 Project:/js/PanelBindingHostV8.js",
					"textfile" : 					{
						"filename" : "Project:/js/PanelBindingHostV8.js",
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
					"patching_rect" : [ 221.0, 208.0, 160.0, 22.0 ],
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
					"patching_rect" : [ 403.0, 208.0, 145.0, 22.0 ],
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
