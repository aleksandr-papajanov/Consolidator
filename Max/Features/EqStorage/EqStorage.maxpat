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
		"rect" : [ 718.0, 143.0, 900.0, 600.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 15.0, 15.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"id" : "obj-message-bus-send-9",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 525.0, 255.0, 145.0, 22.0 ],
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
					"patching_rect" : [ 375.0, 30.0, 145.0, 22.0 ],
					"text" : "r ---message.bus.out"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-1",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 390.0, 540.0, 32.0, 22.0 ],
					"text" : "print"
				}

			}
, 			{
				"box" : 				{
					"id" : "storage",
					"maxclass" : "newobj",
					"numinlets" : 3,
					"numoutlets" : 7,
					"outlettype" : [ "", "", "", "", "", "", "" ],
					"patching_rect" : [ 255.0, 195.0, 148.0, 22.0 ],
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
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 255.0, 150.0, 217.0, 22.0 ],
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
					"comment" : "",
					"id" : "inputL",
					"index" : 1,
					"maxclass" : "inlet",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "signal" ],
					"patching_rect" : [ 270.0, 450.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"comment" : "",
					"id" : "inputR",
					"index" : 2,
					"maxclass" : "inlet",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "signal" ],
					"patching_rect" : [ 330.0, 450.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "eqchain",
					"maxclass" : "newobj",
					"numinlets" : 3,
					"numoutlets" : 3,
					"outlettype" : [ "signal", "signal", "" ],
					"patching_rect" : [ 270.0, 495.0, 139.0, 22.0 ],
					"text" : "consolidator.eqchain"
				}

			}
, 			{
				"box" : 				{
					"comment" : "",
					"id" : "outputL",
					"index" : 1,
					"maxclass" : "outlet",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 270.0, 540.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"comment" : "",
					"id" : "outputR",
					"index" : 2,
					"maxclass" : "outlet",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 330.0, 540.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"comment" : "dictionary <name> after an EqStorage mutation",
					"id" : "persistenceOutput",
					"index" : 3,
					"maxclass" : "outlet",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 461.5, 495.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"comment" : "dictionary <name> restored from root pattrstorage",
					"id" : "persistenceInput",
					"index" : 3,
					"maxclass" : "inlet",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 480.0, 135.0, 30.0, 30.0 ]
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
					"patching_rect" : [ 300.0, 255.0, 120.0, 110.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 90.0, 110.0 ]
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
					"patching_rect" : [ 255.0, 90.0, 44.0, 15.0 ],
					"presentation" : 1,
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
					"patching_rect" : [ 315.0, 90.0, 44.0, 15.0 ],
					"presentation" : 1,
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
					"patching_rect" : [ 255.0, 120.0, 35.0, 22.0 ],
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
					"patching_rect" : [ 315.0, 120.0, 55.0, 22.0 ],
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
					"destination" : [ "obj-1", 0 ],
					"source" : [ "eqchain", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "outputL", 0 ],
					"source" : [ "eqchain", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "outputR", 0 ],
					"source" : [ "eqchain", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "eqchain", 0 ],
					"source" : [ "inputL", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "eqchain", 1 ],
					"source" : [ "inputR", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 0 ],
					"midpoints" : [ 309.5, 393.0, 210.5, 393.0, 210.5, 183.0, 264.5, 183.0 ],
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
					"destination" : [ "storage", 2 ],
					"source" : [ "persistenceInput", 0 ]
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
					"destination" : [ "storage", 0 ],
					"source" : [ "controller", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 1 ],
					"source" : [ "storage", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "eqchain", 2 ],
					"source" : [ "storage", 3 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 2 ],
					"source" : [ "storage", 1 ]
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
					"destination" : [ "obj-message-bus-send-9", 0 ],
					"source" : [ "storage", 5 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-message-bus-send-9", 0 ],
					"source" : [ "storage", 4 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-message-bus-send-9", 0 ],
					"source" : [ "storage", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "persistenceOutput", 0 ],
					"source" : [ "storage", 6 ]
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
