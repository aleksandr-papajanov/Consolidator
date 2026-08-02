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
		"rect" : [ 874.0, 185.0, 584.0, 412.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 10.0, 10.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"id" : "global-receive",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 180.0, 20.0, 140.0, 22.0 ],
					"text" : "r consolidator.host.bus"
				}

			}

, 			{
				"box" : 				{
					"id" : "runtime-events-receive",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 340.0, 45.0, 165.0, 22.0 ],
					"text" : "r ---message.bus.out"
				}

			}
, 			{
				"box" : 				{
					"id" : "coordinator-receive",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 340.0, 70.0, 175.0, 22.0 ],
					"text" : "r ---bankmanager.coordinator"
				}

			}
, 			{
				"box" : 				{
					"border" : 0,
					"filename" : "consolidator.bankmanager.js",
					"id" : "manager",
					"maxclass" : "jsui",
					"numinlets" : 2,
					"numoutlets" : 3,
					"outlettype" : [ "", "", "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 20.0, 55.0, 380.0, 165.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 230.0, 170.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "local-send",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 235.0, 130.0, 22.0 ],
					"text" : "s ---message.bus.in"
				}

			}
, 			{
				"box" : 				{
					"id" : "global-send",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 180.0, 235.0, 145.0, 22.0 ],
					"text" : "s consolidator.host.bus"
				}

			}
, 			{
				"box" : 				{
					"id" : "processor-limits-send",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 340.0, 235.0, 170.0, 22.0 ],
					"text" : "s ---link.control.processor"
				}

			}

, 			{
				"box" : 				{
					"id" : "device",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "bang", "int", "int" ],
					"patching_rect" : [ 20.0, 270.0, 90.0, 22.0 ],
					"text" : "live.thisdevice"
				}

			}
, 			{
				"box" : 				{
					"id" : "initialize-defer",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 120.0, 270.0, 60.0, 22.0 ],
					"text" : "deferlow"
				}

			}
, 			{
				"box" : 				{
					"id" : "initialize",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 190.0, 270.0, 60.0, 22.0 ],
					"text" : "initialize"
				}

			}
, 			{
				"box" : 				{
					"id" : "freebang",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 260.0, 300.0, 65.0, 22.0 ],
					"text" : "freebang"
				}

			}
, 			{
				"box" : 				{
					"id" : "leave",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 335.0, 270.0, 40.0, 22.0 ],
					"text" : "leave"
				}

			}
, 			{
				"box" : 				{
					"id" : "eq-preview-route",
					"maxclass" : "newobj",
					"numinlets" : 3,
					"numoutlets" : 4,
					"outlettype" : [ "", "", "", "" ],
					"patching_rect" : [ 340.0, 265.0, 140.0, 22.0 ],
					"text" : "route eq_preview filter_limits link_color"
				}

			}
, 			{
				"box" : 				{
					"id" : "eq-preview-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 445.0, 265.0, 117.0, 22.0 ],
					"text" : "prepend eq_preview"
				}

			}
, 			{
				"box" : 				{
					"id" : "eq-preview-send",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 570.0, 265.0, 165.0, 22.0 ],
					"text" : "s ---link.control.analyzer"
				}

			}
, 			{
				"box" : 				{
					"id" : "filter-limits-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 445.0, 290.0, 120.0, 22.0 ],
					"text" : "prepend filter_limits"
				}

			}
,			{
				"box" : {
					"id" : "link-color-prefix",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 445.0, 315.0, 105.0, 22.0 ],
					"text" : "prepend link_color"
				}

			}
,			{
				"box" : {
					"id" : "analyzer-ui-send",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 570.0, 315.0, 105.0, 22.0 ],
					"text" : "s ---analyzer.ui"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "initialize-defer", 0 ],
					"source" : [ "device", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "eq-preview-send", 0 ],
					"source" : [ "eq-preview-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "eq-preview-prefix", 0 ],
					"source" : [ "eq-preview-route", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "filter-limits-prefix", 0 ],
					"source" : [ "eq-preview-route", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "processor-limits-send", 0 ],
					"source" : [ "eq-preview-route", 3 ]
				}

			}
,			{
				"patchline" : {
					"destination" : [ "link-color-prefix", 0 ],
					"source" : [ "eq-preview-route", 2 ]
				}

			}
,			{
				"patchline" : {
					"destination" : [ "analyzer-ui-send", 0 ],
					"source" : [ "link-color-prefix", 0 ]
				}

			}
,			{
				"patchline" : {
					"destination" : [ "processor-limits-send", 0 ],
					"source" : [ "link-color-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "eq-preview-send", 0 ],
					"source" : [ "filter-limits-prefix", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "leave", 0 ],
					"source" : [ "freebang", 0 ]
				}

			}

, 			{
				"patchline" : 				{
					"destination" : [ "manager", 0 ],
					"source" : [ "runtime-events-receive", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "manager", 0 ],
					"source" : [ "coordinator-receive", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "manager", 1 ],
					"source" : [ "global-receive", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "manager", 0 ],
					"source" : [ "initialize", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "initialize", 0 ],
					"source" : [ "initialize-defer", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "manager", 0 ],
					"source" : [ "leave", 0 ]
				}

			}

, 			{
				"patchline" : 				{
					"destination" : [ "eq-preview-route", 0 ],
					"source" : [ "manager", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "global-send", 0 ],
					"source" : [ "manager", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "local-send", 0 ],
					"source" : [ "manager", 0 ]
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
