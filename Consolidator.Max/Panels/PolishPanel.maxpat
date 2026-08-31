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
					"filename" : "DialControl.js",
					"id" : "thick",
					"jsarguments" : [ "THICK", 0.0, 1.0, 0, 100.0, 1, "%" ],
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 64.0, 64.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 117.0, 0.0, 78.0, 78.0 ],
					"textfile" : 					{
						"filename" : "DialControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "polish_thick"
				}

			}
, 			{
				"box" : 				{
					"filename" : "DialControl.js",
					"id" : "air",
					"jsarguments" : [ "AIR", 0.0, 1.0, 0, 100.0, 1, "%" ],
					"maxclass" : "v8ui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 0.0, 0.0, 64.0, 64.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 208.0, 0.0, 78.0, 78.0 ],
					"textfile" : 					{
						"filename" : "DialControl.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}
,
					"varname" : "polish_air"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-thick",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 0.0, 119.0, 22.0 ],
					"text" : "prepend polish_thick"
				}

			}
, 			{
				"box" : 				{
					"id" : "p-air",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 0.0, 0.0, 107.0, 22.0 ],
					"text" : "prepend polish_air"
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
					"filename" : "PanelBindingHostV8.js",
					"id" : "router",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 0.0, 0.0, 203.0, 22.0 ],
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
					"patching_rect" : [ 809.0, 1072.0, 128.0, 128.0 ],
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
					"destination" : [ "p-air", 0 ],
					"source" : [ "air", 0 ]
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
					"source" : [ "p-air", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "out", 0 ],
					"source" : [ "p-thick", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "p-thick", 0 ],
					"source" : [ "thick", 0 ]
				}

			}
 ],
		"oscreceiveudpport" : 0
	}

}
