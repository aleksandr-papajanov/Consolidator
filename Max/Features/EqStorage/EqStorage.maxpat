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
		"rect" : [ 639.0, 250.0, 900.0, 600.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 10.0, 10.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"id" : "obj-message-bus-send-9",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 360.0, 130.0, 145.0, 22.0 ],
					"text" : "s ---message.bus.in"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-message-bus-receive-9",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 360.0, 50.0, 145.0, 22.0 ],
					"text" : "r ---message.bus.out"
				}

			}
, 			{
				"box" : 				{
					"id" : "storage",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 3,
					"outlettype" : [ "", "", "" ],
					"patching_rect" : [ 220.0, 90.0, 159.0, 22.0 ],
					"saved_object_attributes" : 					{
						"filename" : "consolidator.eqstorage.js",
						"parameter_enable" : 0
					}
,
					"text" : "js consolidator.eqstorage.js",
					"varname" : "storage"
				}

			}
, 			{
				"box" : 				{
					"id" : "controller",
					"maxclass" : "newobj",
					"numinlets" : 3,
					"numoutlets" : 3,
					"outlettype" : [ "", "", "" ],
					"patching_rect" : [ 30.0, 140.0, 209.0, 22.0 ],
					"saved_object_attributes" : 					{
						"filename" : "consolidator.eqstorage.controller.js",
						"parameter_enable" : 0
					}
,
					"text" : "js consolidator.eqstorage.controller.js",
					"varname" : "controller"
				}

			}
, 			{
				"box" : 				{
					"filename" : "consolidator.eqstorage.banklistview.js",
					"id" : "list",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 220.0, 180.0, 220.0, 110.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 220.0, 110.0 ]
				}

			}
,			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "count", 5 ], [ "allowEmptySelection", 1 ], [ "enabled", 1 ], [ "labels", "Add", "Remove", "Bypass", "Solo", "Join" ], [ "buttonModes", "momentary", "momentary", "toggle", "toggle", "momentary" ], [ "layout", "horizontal" ], [ "selectionMode", "custom" ] ],
					"filename" : "ButtonGroupControl.js",
					"id" : "bank-actions",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 220.0, 310.0, 220.0, 28.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 112.0, 220.0, 28.0 ],
					"varname" : "eqstorage.actions"
				}

			}
,			{
				"box" : 				{
					"id" : "thispatcher",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 460.0, 350.0, 80.0, 22.0 ],
					"save" : [ "#N", "thispatcher", ";", "#Q", "end", ";" ],
					"text" : "thispatcher"
				}

			}
,			{
				"box" : 				{
					"id" : "actions-prepend",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 240.0, 350.0, 88.0, 22.0 ],
					"text" : "prepend action"
				}

			}
, 			{
				"box" : 				{
					"id" : "addButton",
					"maxclass" : "live.text",
					"mode" : 0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"parameter_enable" : 1,
					"patching_rect" : [ 30.0, 0.0, 44.0, 15.0 ],
					"presentation" : 0,
					"presentation_rect" : [ 0.0, 110.0, 44.0, 15.0 ],
					"saved_attribute_attributes" : 					{
						"valueof" : 						{
							"parameter_enum" : [ "val1", "val2" ],
							"parameter_longname" : "live.text",
							"parameter_mmax" : 1,
							"parameter_modmode" : 0,
							"parameter_shortname" : "live.text",
							"parameter_type" : 2
						}

					}
,
					"text" : "Add",
					"varname" : "live.text"
				}

			}
, 			{
				"box" : 				{
					"id" : "removeButton",
					"maxclass" : "live.text",
					"mode" : 0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"parameter_enable" : 1,
					"patching_rect" : [ 90.0, 0.0, 44.0, 15.0 ],
					"presentation" : 0,
					"presentation_rect" : [ 0.0, 125.0, 44.0, 15.0 ],
					"saved_attribute_attributes" : 					{
						"valueof" : 						{
							"parameter_enum" : [ "val1", "val2" ],
							"parameter_longname" : "live.text[2]",
							"parameter_mmax" : 1,
							"parameter_modmode" : 0,
							"parameter_shortname" : "live.text[2]",
							"parameter_type" : 2
						}

					}
,
					"text" : "Remove",
					"varname" : "live.text[2]"
				}

			}
, 			{
				"box" : 				{
					"id" : "add",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "add" ],
					"patching_rect" : [ 30.0, 30.0, 35.0, 22.0 ],
					"text" : "t add"
				}

			}
, 			{
				"box" : 				{
					"id" : "remove",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "remove" ],
					"patching_rect" : [ 90.0, 30.0, 55.0, 22.0 ],
					"text" : "t remove"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "controller", 0 ],
					"source" : [ "add", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "add", 0 ],
					"source" : [ "addButton", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "list", 0 ],
					"source" : [ "controller", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "storage", 0 ],
					"midpoints" : [ 39.5, 172.0, 9.5, 172.0, 9.5, 80.0, 229.5, 80.0 ],
					"source" : [ "controller", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 0 ],
					"midpoints" : [ 229.5, 309.0, 19.5, 309.0, 19.5, 130.0, 39.5, 130.0 ],
					"source" : [ "list", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "storage", 1 ],
					"source" : [ "obj-message-bus-receive-9", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 0 ],
					"source" : [ "remove", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "remove", 0 ],
					"source" : [ "removeButton", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 1 ],
					"source" : [ "storage", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 2 ],
					"source" : [ "storage", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-message-bus-send-9", 0 ],
					"source" : [ "storage", 2 ]
				}

			}
,			{
				"patchline" : 				{
					"destination" : [ "actions-prepend", 0 ],
					"source" : [ "bank-actions", 0 ]
				}

			}
,			{
				"patchline" : 				{
					"destination" : [ "controller", 0 ],
					"source" : [ "actions-prepend", 0 ]
				}

			}
,			{
				"patchline" : 				{
					"destination" : [ "thispatcher", 0 ],
					"source" : [ "controller", 2 ]
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
