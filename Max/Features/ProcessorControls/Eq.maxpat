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
		"gridsize" : [ 10.0, 10.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "labels", "B", "R" ], [ "loadingIndex", 0 ], [ "layout", "horizontal" ], [ "selectionMode", "custom" ], [ "buttonModes", "toggle", "momentary", "toggle" ], [ "enabled", 1 ], [ "allowEmptySelection", 1 ], [ "count", 2 ] ],
					"filename" : "ButtonGroupControl.js",
					"id" : "obj-eq-gain-control",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 133.0, 460.0, 45.454546809196472, 22.909091591835022 ],
					"presentation" : 1,
					"presentation_rect" : [ 7.272726595401764, 143.529416084289551, 45.454546809196472, 22.909091591835022 ],
					"varname" : "eq.filter.9.control"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "secondaryValue", 0 ], [ "primaryValue", 0.5 ], [ "secondaryIndicator", 0 ], [ "tertiaryIndicator", 0 ], [ "enabled", 1 ], [ "tertiaryValue", 0 ], [ "valueCount", 1 ], [ "primaryIndicator", 0 ] ],
					"filename" : "DialControl.js",
					"id" : "obj-eq-gain-dial",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 120.0, 410.0, 70.0, 60.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 103.529414415359497, 60.0, 50.106947779655457 ],
					"varname" : "eq.filter.9.dial"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "labels", "B", "S", "R" ], [ "loadingIndex", 0 ], [ "layout", "horizontal" ], [ "selectionMode", "custom" ], [ "buttonModes", "toggle", "toggle", "momentary" ], [ "enabled", 1 ], [ "allowEmptySelection", 1 ], [ "count", 3 ] ],
					"filename" : "ButtonGroupControl.js",
					"id" : "obj-6",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 120.0, 290.0, 80.0, 30.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 60.0, 30.0 ],
					"varname" : "eq.global"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-5",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 430.0, 800.0, 32.0, 22.0 ],
					"text" : "print"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "labels", "B", "R" ], [ "loadingIndex", 0 ], [ "layout", "horizontal" ], [ "selectionMode", "custom" ], [ "buttonModes", "toggle", "momentary", "toggle" ], [ "enabled", 1 ], [ "allowEmptySelection", 1 ], [ "count", 2 ] ],
					"filename" : "ButtonGroupControl.js",
					"id" : "obj-1",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 133.0, 380.0, 45.454546809196472, 22.909091591835022 ],
					"presentation" : 1,
					"presentation_rect" : [ 7.647059142589569, 80.000001668930054, 45.454546809196472, 22.909091591835022 ],
					"varname" : "eq.filter.1.control"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "secondaryValue", 0.526885804063566 ], [ "primaryValue", 0.5 ], [ "secondaryIndicator", 0 ], [ "tertiaryIndicator", 0 ], [ "enabled", 1 ], [ "tertiaryValue", 0 ], [ "valueCount", 2 ], [ "primaryIndicator", 0 ] ],
					"filename" : "DialControl.js",
					"id" : "obj-2",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 120.0, 330.0, 70.0, 60.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 40.0, 60.0, 49.893046379089355 ],
					"varname" : "eq.filter.1.dial"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "labels", "B", "R" ], [ "loadingIndex", 0 ], [ "layout", "horizontal" ], [ "selectionMode", "custom" ], [ "buttonModes", "toggle", "momentary", "toggle" ], [ "enabled", 1 ], [ "allowEmptySelection", 1 ], [ "count", 2 ] ],
					"filename" : "ButtonGroupControl.js",
					"id" : "obj-15",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 370.0, 380.0, 45.454546809196472, 22.909091591835022 ],
					"presentation" : 1,
					"presentation_rect" : [ 188.823537290096283, 80.000001668930054, 45.454546809196472, 22.909091591835022 ],
					"varname" : "eq.filter.3.control"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "secondaryValue", 0.5 ], [ "primaryValue", 0.49 ], [ "secondaryIndicator", 0 ], [ "tertiaryIndicator", 0 ], [ "enabled", 1 ], [ "tertiaryValue", 0 ], [ "valueCount", 2 ], [ "primaryIndicator", 0 ] ],
					"filename" : "DialControl.js",
					"id" : "obj-16",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 357.0, 330.0, 70.0, 60.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 180.000007510185242, 40.0, 63.101611196994781, 49.893046379089355 ],
					"varname" : "eq.filter.3.dial"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "labels", "B", "R" ], [ "loadingIndex", 0 ], [ "layout", "horizontal" ], [ "selectionMode", "custom" ], [ "buttonModes", "toggle", "momentary", "toggle" ], [ "enabled", 1 ], [ "allowEmptySelection", 1 ], [ "count", 2 ] ],
					"filename" : "ButtonGroupControl.js",
					"id" : "obj-17",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 330.0, 460.0, 45.454546809196472, 22.909091591835022 ],
					"presentation" : 1,
					"presentation_rect" : [ 157.754006087779999, 143.101605772972107, 45.454546809196472, 22.909091591835022 ],
					"varname" : "eq.filter.6.control"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "secondaryValue", 0.413427121767652 ], [ "primaryValue", 0.5 ], [ "secondaryIndicator", 0 ], [ "tertiaryIndicator", 0 ], [ "enabled", 1 ], [ "tertiaryValue", 0.56457503405358 ], [ "valueCount", 3 ], [ "primaryIndicator", 0 ] ],
					"filename" : "DialControl.js",
					"id" : "obj-18",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 317.0, 410.0, 70.0, 60.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 149.732615947723389, 103.529414415359497, 60.000005841255188, 50.106947779655457 ],
					"varname" : "eq.filter.6.dial"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "labels", "B", "R" ], [ "loadingIndex", 0 ], [ "layout", "horizontal" ], [ "selectionMode", "custom" ], [ "buttonModes", "toggle", "momentary", "toggle" ], [ "enabled", 1 ], [ "allowEmptySelection", 1 ], [ "count", 2 ] ],
					"filename" : "ButtonGroupControl.js",
					"id" : "obj-11",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 290.0, 380.0, 45.454546809196472, 22.909091591835022 ],
					"presentation" : 1,
					"presentation_rect" : [ 127.058828830718994, 80.000001668930054, 45.454546809196472, 22.909091591835022 ],
					"varname" : "eq.filter.4.control"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "secondaryValue", 0.525461227180653 ], [ "primaryValue", 0.5 ], [ "secondaryIndicator", 0 ], [ "tertiaryIndicator", 0 ], [ "enabled", 1 ], [ "tertiaryValue", 1 ], [ "valueCount", 3 ], [ "primaryIndicator", 0 ] ],
					"filename" : "DialControl.js",
					"id" : "obj-12",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 277.0, 330.0, 70.0, 60.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 120.000005006790161, 40.0, 60.213907241821289, 49.893046379089355 ],
					"varname" : "eq.filter.4.dial"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "labels", "B", "R" ], [ "loadingIndex", 0 ], [ "layout", "horizontal" ], [ "selectionMode", "custom" ], [ "buttonModes", "toggle", "momentary", "toggle" ], [ "enabled", 1 ], [ "allowEmptySelection", 1 ], [ "count", 2 ] ],
					"filename" : "ButtonGroupControl.js",
					"id" : "obj-13",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 250.0, 460.0, 45.454546809196472, 22.909091591835022 ],
					"presentation" : 1,
					"presentation_rect" : [ 97.860959708690643, 143.101605772972107, 45.454546809196472, 22.909091591835022 ],
					"varname" : "eq.filter.5.control"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "secondaryValue", 0.706713560883826 ], [ "primaryValue", 0.5 ], [ "secondaryIndicator", 0 ], [ "tertiaryIndicator", 0 ], [ "enabled", 1 ], [ "tertiaryValue", 1 ], [ "valueCount", 3 ], [ "primaryIndicator", 0 ] ],
					"filename" : "DialControl.js",
					"id" : "obj-14",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 237.0, 410.0, 70.0, 60.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 89.839569568634033, 103.529414415359497, 60.213907241821289, 50.106947779655457 ],
					"varname" : "eq.filter.5.dial"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "labels", "B", "R" ], [ "loadingIndex", 0 ], [ "layout", "horizontal" ], [ "selectionMode", "custom" ], [ "buttonModes", "toggle", "momentary", "toggle" ], [ "enabled", 1 ], [ "allowEmptySelection", 1 ], [ "count", 2 ] ],
					"filename" : "ButtonGroupControl.js",
					"id" : "obj-9",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 210.0, 380.0, 45.454546809196472, 22.909091591835022 ],
					"presentation" : 1,
					"presentation_rect" : [ 68.823532283306122, 80.000001668930054, 45.454546809196472, 22.909091591835022 ],
					"varname" : "eq.filter.2.control"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"embedstate" : [ [ "secondaryValue", 0.5 ], [ "primaryValue", 0.495 ], [ "secondaryIndicator", 0 ], [ "tertiaryIndicator", 0 ], [ "enabled", 1 ], [ "tertiaryValue", 0 ], [ "valueCount", 2 ], [ "primaryIndicator", 0 ] ],
					"filename" : "DialControl.js",
					"id" : "obj-10",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 197.0, 330.0, 70.0, 60.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 60.000002503395081, 40.0, 60.106953620910645, 49.893046379089355 ],
					"varname" : "eq.filter.2.dial"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-controller",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 4,
					"outlettype" : [ "", "", "", "" ],
					"patching_rect" : [ 270.0, 770.0, 179.0, 22.0 ],
					"saved_object_attributes" : 					{
						"filename" : "consolidator.eq.controller.js",
						"parameter_enable" : 0
					}
,
					"text" : "js consolidator.eq.controller.js"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-message-bus-out",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 430.0, 730.0, 145.0, 22.0 ],
					"text" : "r ---state.eq"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-message-bus-in",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 270.0, 880.0, 145.0, 22.0 ],
					"text" : "s ---message.bus.in"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-thispatcher",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 350.0, 800.0, 75.0, 22.0 ],
					"save" : [ "#N", "thispatcher", ";", "#Q", "end", ";" ],
					"text" : "thispatcher"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-loadbang",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 290.0, 840.0, 60.0, 22.0 ],
					"text" : "loadbang"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-global-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 120.0, 540.0, 101.0, 22.0 ],
					"text" : "prepend eqglobal"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-dial1-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 140.0, 620.0, 85.0, 22.0 ],
					"text" : "prepend dial 1"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-dial2-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 420.0, 490.0, 85.0, 22.0 ],
					"text" : "prepend dial 3"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-dial3-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 360.0, 620.0, 85.0, 22.0 ],
					"text" : "prepend dial 6"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-dial4-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 470.0, 620.0, 85.0, 22.0 ],
					"text" : "prepend dial 4"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-dial5-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 250.0, 620.0, 85.0, 22.0 ],
					"text" : "prepend dial 5"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-dial6-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 200.0, 490.0, 85.0, 22.0 ],
					"text" : "prepend dial 2"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-dial9-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 580.0, 620.0, 85.0, 22.0 ],
					"text" : "prepend dial 9"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-bypass9-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 580.0, 650.0, 104.0, 22.0 ],
					"text" : "prepend control 9"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-bypass1-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 140.0, 650.0, 104.0, 22.0 ],
					"text" : "prepend control 1"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-bypass2-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 420.0, 520.0, 104.0, 22.0 ],
					"text" : "prepend control 3"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-bypass3-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 360.0, 650.0, 104.0, 22.0 ],
					"text" : "prepend control 6"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-bypass4-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 470.0, 650.0, 104.0, 22.0 ],
					"text" : "prepend control 4"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-bypass5-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 250.0, 650.0, 104.0, 22.0 ],
					"text" : "prepend control 5"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-bypass6-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 200.0, 520.0, 104.0, 22.0 ],
					"text" : "prepend control 2"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-link-control-state",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 590.0, 730.0, 145.0, 22.0 ],
					"text" : "r ---link.control.state"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-gesture-send",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 430.0, 880.0, 175.0, 22.0 ],
					"text" : "s ---link.parameter.gesture"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-eq-definitions-receive",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 430.0, 700.0, 145.0, 22.0 ],
					"text" : "r ---state.definitions"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-bypass1-prefix", 0 ],
					"source" : [ "obj-1", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-dial6-prefix", 0 ],
					"source" : [ "obj-10", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-bypass4-prefix", 0 ],
					"source" : [ "obj-11", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-dial4-prefix", 0 ],
					"source" : [ "obj-12", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-bypass5-prefix", 0 ],
					"source" : [ "obj-13", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-dial5-prefix", 0 ],
					"source" : [ "obj-14", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-bypass2-prefix", 0 ],
					"source" : [ "obj-15", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-dial2-prefix", 0 ],
					"source" : [ "obj-16", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-bypass3-prefix", 0 ],
					"source" : [ "obj-17", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-dial3-prefix", 0 ],
					"source" : [ "obj-18", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-dial1-prefix", 0 ],
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-global-prefix", 0 ],
					"source" : [ "obj-6", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-bypass6-prefix", 0 ],
					"source" : [ "obj-9", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-bypass1-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-bypass2-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-bypass3-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-bypass4-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-bypass5-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-bypass6-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-bypass9-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-5", 0 ],
					"source" : [ "obj-eq-controller", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-message-bus-in", 0 ],
					"source" : [ "obj-eq-controller", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-thispatcher", 0 ],
					"source" : [ "obj-eq-controller", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-dial1-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-dial2-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-dial3-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-dial4-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-dial5-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-dial6-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-dial9-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-bypass9-prefix", 0 ],
					"source" : [ "obj-eq-gain-control", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-dial9-prefix", 0 ],
					"source" : [ "obj-eq-gain-dial", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-global-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-message-bus-in", 0 ],
					"source" : [ "obj-eq-loadbang", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 1 ],
					"source" : [ "obj-eq-message-bus-out", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 0 ],
					"source" : [ "obj-eq-link-control-state", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-gesture-send", 0 ],
					"source" : [ "obj-eq-controller", 3 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-eq-controller", 1 ],
					"source" : [ "obj-eq-definitions-receive", 0 ]
				}

			}
 ],
		"oscreceiveudpport" : 0
	}

}
