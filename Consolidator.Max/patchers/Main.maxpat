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
					"id" : "plugin",
					"maxclass" : "newobj",
					"numinlets" : 4,
					"numoutlets" : 4,
					"outlettype" : [ "signal", "signal", "signal", "signal" ],
					"patching_rect" : [ 113.0, 157.0, 212.0, 22.0 ],
					"text" : "plugin~ 1 2 3 4"
				}

			}
, 			{
				"box" : 				{
					"id" : "external",
					"maxclass" : "newobj",
					"numinlets" : 5,
					"numoutlets" : 6,
					"outlettype" : [ "", "", "signal", "signal", "signal", "signal" ],
					"patching_rect" : [ 48.0, 200.0, 277.0, 22.0 ],
					"saved_attribute_attributes" : 					{
						"valueof" : 						{
							"parameter_invisible" : 1,
							"parameter_longname" : "ConsolidatorExternal",
							"parameter_modmode" : 0,
							"parameter_shortname" : "ConsolidatorExternal",
							"parameter_type" : 3
						}

					}
,
					"saved_object_attributes" : 					{
						"parameter_enable" : 1,
						"parameter_mappable" : 0
					}
,
					"text" : "ConsolidatorExternal",
					"varname" : "ConsolidatorExternal"
				}

			}
, 			{
				"box" : 				{
					"bgmode" : 0,
					"border" : 0,
					"clickthrough" : 0,
					"enablehscroll" : 0,
					"enablevscroll" : 0,
					"id" : "bridge",
					"lockeddragscroll" : 0,
					"lockedsize" : 0,
					"maxclass" : "bpatcher",
					"name" : "ConsolidatorBridge.maxpat",
					"numinlets" : 2,
					"numoutlets" : 2,
					"offset" : [ 0.0, 0.0 ],
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 52.0, 286.0, 1074.0, 169.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 884.0, 169.0 ],
					"viewvisibility" : 1
				}

			}
, 			{
				"box" : 				{
					"id" : "plugout",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "signal", "signal" ],
					"patching_rect" : [ 150.800000000000011, 247.0, 71.0, 22.0 ],
					"text" : "plugout~"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "external", 0 ],
					"midpoints" : [ 61.5, 495.0, 15.5, 495.0, 15.5, 175.0, 57.5, 175.0 ],
					"source" : [ "bridge", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "bridge", 1 ],
					"source" : [ "external", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "bridge", 0 ],
					"source" : [ "external", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "plugout", 1 ],
					"source" : [ "external", 3 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "plugout", 0 ],
					"source" : [ "external", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "external", 4 ],
					"source" : [ "plugin", 3 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "external", 3 ],
					"source" : [ "plugin", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "external", 2 ],
					"source" : [ "plugin", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "external", 1 ],
					"source" : [ "plugin", 0 ]
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
