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
		"rect" : [ 294.0, 424.0, 1000.0, 780.0 ],
		"gridsize" : [ 8.0, 8.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/Dial/DialControl.js",
					"id" : "control",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 80.0, 72.0 ],
					"varname" : "output_gain"
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
					"patching_rect" : [ 0.0, 232.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "router",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 272.0, 198.0, 22.0 ],
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
					"patching_rect" : [ 0.0, 96.0, 145.0, 22.0 ],
					"text" : "prepend output_gain"
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
					"patching_rect" : [ 0.0, 136.0, 30.0, 30.0 ]
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "prefix", 0 ],
					"source" : [ "control", 0 ]
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
					"source" : [ "prefix", 0 ]
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
