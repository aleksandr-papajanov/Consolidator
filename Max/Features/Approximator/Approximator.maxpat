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
		"rect" : [ 127.0, 85.0, 1212.0, 875.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 15.0, 15.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"id" : "obj-fit-trigger",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "bang", "" ],
					"patching_rect" : [ 15.0, 165.0, 35.0, 22.0 ],
					"text" : "sel 1"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-fit-message",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 195.0, 225.0, 35.0, 22.0 ],
					"text" : "fit"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-local-command",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 3,
					"outlettype" : [ "", "", "" ],
					"patching_rect" : [ 85.0, 228.0, 334.0, 22.0 ],
					"saved_object_attributes" : 					{
						"filename" : "consolidator.approximator.controller.js",
						"parameter_enable" : 0
					}
,
					"text" : "js consolidator.approximator.controller.js"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-ui-script",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 615.0, 270.0, 73.0, 22.0 ],
					"save" : [ "#N", "thispatcher", ";", "#Q", "end", ";" ],
					"text" : "thispatcher"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-bus-send",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 75.0, 270.0, 145.0, 22.0 ],
					"text" : "s ---message.bus.in"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-bus-receive",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 240.0, 30.0, 145.0, 22.0 ],
					"text" : "r ---message.bus.out"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-difference-receive",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 390.0, 60.0, 190.0, 22.0 ],
					"text" : "r ---approximator.difference.inlet"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-native",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 3,
					"outlettype" : [ "", "", "" ],
					"patching_rect" : [ 240.0, 135.0, 319.0, 22.0 ],
					"text" : "consolidator.approximator"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-native-command-send",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 240.0, 165.0, 145.0, 22.0 ],
					"text" : "s ---message.bus.in"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-debug",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 540.0, 165.0, 105.0, 22.0 ],
					"text" : "print approximator"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-fit-button",
					"maxclass" : "live.text",
					"mode" : 1,
					"outputmode" : 0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"parameter_enable" : 1,
					"patching_rect" : [ 15.0, 120.0, 56.0, 24.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 56.0, 24.0 ],
					"saved_attribute_attributes" : 					{
						"valueof" : 						{
							"parameter_enum" : [ "val1", "val2" ],
							"parameter_longname" : "fit_button",
							"parameter_mmax" : 1,
							"parameter_modmode" : 0,
							"parameter_shortname" : "fit_button",
							"parameter_type" : 2
						}

					}
,
					"text" : "Fit",
					"varname" : "fit_button"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-listen-button",
					"maxclass" : "live.text",
					"mode" : 1,
					"outputmode" : 0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"parameter_enable" : 1,
					"patching_rect" : [ 105.0, 120.0, 56.0, 24.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 64.0, 0.0, 56.0, 24.0 ],
					"rounded" : 20.0,
					"saved_attribute_attributes" : 					{
						"valueof" : 						{
							"parameter_enum" : [ "val1", "val2" ],
							"parameter_longname" : "listen_button",
							"parameter_mmax" : 1,
							"parameter_modmode" : 0,
							"parameter_shortname" : "listen_button",
							"parameter_type" : 2
						}

					}
,
					"text" : "Listen",
					"texton" : "Listening",
					"varname" : "listen_button"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-listen-prepend",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 105.0, 165.0, 90.0, 22.0 ],
					"text" : "prepend listen"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-clear-message",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 60.0, 165.0, 42.0, 22.0 ],
					"text" : "clear"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-status-route",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 390.0, 270.0, 90.0, 22.0 ],
					"text" : "route status"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "obj-local-command", 0 ],
					"source" : [ "obj-fit-message", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-native", 0 ],
					"source" : [ "obj-bus-receive", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-local-command", 0 ],
					"source" : [ "obj-clear-message", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-native", 1 ],
					"source" : [ "obj-difference-receive", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-fit-trigger", 0 ],
					"source" : [ "obj-fit-button", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-fit-message", 0 ],
					"source" : [ "obj-fit-trigger", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-listen-prepend", 0 ],
					"source" : [ "obj-listen-button", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-local-command", 0 ],
					"source" : [ "obj-listen-prepend", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-bus-send", 0 ],
					"source" : [ "obj-local-command", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-status-route", 0 ],
					"source" : [ "obj-local-command", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-ui-script", 0 ],
					"source" : [ "obj-local-command", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-debug", 0 ],
					"order" : 0,
					"source" : [ "obj-native", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-local-command", 1 ],
					"order" : 1,
					"source" : [ "obj-native", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-local-command", 1 ],
					"source" : [ "obj-native", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-native-command-send", 0 ],
					"source" : [ "obj-native", 0 ]
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
