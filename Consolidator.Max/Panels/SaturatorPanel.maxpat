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
		"rect" : [ 441.0, 476.0, 1000.0, 780.0 ],
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
					"patching_rect" : [ 0.0, 0.0, 192.0, 112.0 ],
					"varname" : "saturator_detector"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Dial/DialControl.js",
					"id" : "drive",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 112.0, 64.0, 56.0 ],
					"varname" : "saturator_drive"
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
					"patching_rect" : [ 64.0, 112.0, 64.0, 56.0 ],
					"varname" : "saturator_gain"
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
					"patching_rect" : [ 128.0, 112.0, 64.0, 56.0 ],
					"varname" : "saturator_mix"
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Dial/DialControl.js",
					"id" : "detector-amount",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 192.0, 112.0, 64.0, 56.0 ],
					"varname" : "saturator_detector_amount"
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
					"patching_rect" : [ 192.0, 0.0, 96.0, 24.0 ],
					"varname" : "saturator_bypass"
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
					"patching_rect" : [ 192.0, 24.0, 96.0, 24.0 ],
					"varname" : "saturator_solo"
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
					"text" : "prepend saturator_gain"
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
					"text" : "prepend saturator_mix"
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
					"text" : "prepend saturator_detector_amount"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-bypass",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 320.0, 360.0, 160.0, 22.0 ],
					"text" : "prepend saturator_bypass"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-solo",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 320.0, 392.0, 145.0, 22.0 ],
					"text" : "prepend saturator_solo"
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
					"id" : "router",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 406.0, 198.0, 22.0 ],
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
 ],
		"lines" : [ 			{
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
					"destination" : [ "p-detector-amount", 0 ],
					"source" : [ "detector-amount", 0 ]
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
					"source" : [ "p-bypass", 0 ]
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
