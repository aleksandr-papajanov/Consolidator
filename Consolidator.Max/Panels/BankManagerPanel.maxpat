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
		"rect" : [ 661.0, 158.0, 685.0, 780.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 13.0, 13.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"filename" : "Project:/js/Controls/BankManager/BankManagerControl.js",
					"border" : 0,
					"id" : "control",
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 351.0, 169.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 351.0, 169.0 ],
					"textfile" : 					{
						"filename" : "Project:/js/Controls/BankManager/BankManagerControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "bank_manager"
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
					"patching_rect" : [ 0.0, 255.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"filename" : "Project:/js/PanelBindingHostV8.js",
					"id" : "router",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 0.0, 300.0, 203.0, 22.0 ],
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
					"patching_rect" : [ 0.0, 180.0, 150.0, 22.0 ],
					"text" : "prepend bank_manager"
				}

			}
, 			{
				"box" : 				{
					"comment" : "",
					"id" : "panel-out",
					"index" : 2,
					"maxclass" : "outlet",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 45.0, 215.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "out",
					"index" : 1,
					"maxclass" : "outlet",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 0.0, 215.0, 30.0, 30.0 ]
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
					"destination" : [ "panel-out", 0 ],
					"source" : [ "control", 1 ]
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
