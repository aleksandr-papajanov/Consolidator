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
		"rect" : [ 381.0, 132.0, 360.0, 180.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 15.0, 15.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "loadingIndex", 0 ], [ "selectionMode", "custom" ], [ "count", 2 ], [ "buttonModes", "momentary", "momentary" ], [ "labels", "Undo", "Redo" ], [ "layout", "horizontal" ], [ "enabled", 1 ], [ "allowEmptySelection", 1 ] ],
					"filename" : "../../Shared/Interface/ButtonGroup/ButtonGroupControl.js",
					"id" : "history-actions",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 20.0, 20.0, 130.0, 24.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 90.0, 30.0 ],
					"varname" : "history.actions"
				}

			}
, 			{
				"box" : 				{
					"id" : "controller",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 20.0, 100.0, 248.0, 22.0 ],
					"saved_object_attributes" : 					{
						"filename" : "Controllers/consolidator.history.controller.js",
						"parameter_enable" : 0
					}
,
					"text" : "js Controllers/consolidator.history.controller.js"
				}

			}
, 			{
				"box" : 				{
					"id" : "bus-out",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 180.0, 20.0, 135.0, 22.0 ],
					"text" : "r ---message.bus.out"
				}

			}
, 			{
				"box" : 				{
					"id" : "bus-in",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 140.0, 130.0, 22.0 ],
					"text" : "s ---message.bus.in"
				}

			}
, 			{
				"box" : 				{
					"id" : "thispatcher",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 260.0, 100.0, 80.0, 22.0 ],
					"save" : [ "#N", "thispatcher", ";", "#Q", "end", ";" ],
					"text" : "thispatcher"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "controller", 1 ],
					"source" : [ "bus-out", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "bus-in", 0 ],
					"source" : [ "controller", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "thispatcher", 0 ],
					"source" : [ "controller", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 0 ],
					"source" : [ "history-actions", 0 ]
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
