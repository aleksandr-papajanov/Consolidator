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
		"gridsize" : [ 15.0, 15.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"id" : "obj-65",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 540.0, 405.0, 180.0, 22.0 ],
					"text" : "s ---approximator.difference.inlet"
				}

			}
, 			{
				"box" : 				{
					"comment" : "",
					"id" : "obj-4",
					"index" : 4,
					"maxclass" : "inlet",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "signal" ],
					"patching_rect" : [ 690.0, 240.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"comment" : "",
					"id" : "obj-3",
					"index" : 3,
					"maxclass" : "inlet",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "signal" ],
					"patching_rect" : [ 565.625, 240.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"comment" : "",
					"id" : "obj-2",
					"index" : 2,
					"maxclass" : "inlet",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "signal" ],
					"patching_rect" : [ 440.3125, 240.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"comment" : "",
					"id" : "obj-1",
					"index" : 1,
					"maxclass" : "inlet",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "signal" ],
					"patching_rect" : [ 315.0, 240.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "busReceive",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 816.249999999999886, 240.0, 145.0, 22.0 ],
					"text" : "r ---message.bus.out"
				}

			}
, 			{
				"box" : 				{
					"id" : "analyzer",
					"linecount" : 2,
					"maxclass" : "newobj",
					"numinlets" : 5,
					"numoutlets" : 7,
					"outlettype" : [ "", "", "", "", "", "", "" ],
					"patching_rect" : [ 315.0, 330.0, 520.249999999999886, 36.0 ],
					"text" : "consolidator.analyzer"
				}

			}
, 			{
				"box" : 				{
					"id" : "controller",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 315.0, 630.0, 218.0, 22.0 ],
					"saved_object_attributes" : 					{
						"filename" : "consolidator.analyzer.controller.js",
						"parameter_enable" : 0
					}
,
					"text" : "js consolidator.analyzer.controller.js",
					"varname" : "controller"
				}

			}
, 			{
				"box" : 				{
					"id" : "eqCurveSend",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 735.0, 405.0, 180.0, 22.0 ],
					"text" : "s ---approximator.eqcurve.inlet"
				}

			}
, 			{
				"box" : 				{
					"filename" : "consolidator.analyzer.spectrumview.js",
					"id" : "spectrumView",
					"maxclass" : "jsui",
					"numinlets" : 5,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 315.0, 465.0, 420.0, 150.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 420.0, 150.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "busSend",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 315.0, 630.0, 135.0, 22.0 ],
					"text" : "s ---message.bus.in"
				}

			}
, 			{
				"box" : 				{
					"id" : "debug",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 816.0, 374.0, 148.0, 22.0 ],
					"text" : "print consolidator.analyzer"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "debug", 0 ],
					"source" : [ "analyzer", 6 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "eqCurveSend", 0 ],
					"order" : 0,
					"source" : [ "analyzer", 5 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-65", 0 ],
					"order" : 0,
					"source" : [ "analyzer", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "spectrumView", 4 ],
					"order" : 1,
					"source" : [ "analyzer", 4 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "spectrumView", 3 ],
					"source" : [ "analyzer", 3 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "spectrumView", 2 ],
					"order" : 1,
					"source" : [ "analyzer", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "spectrumView", 1 ],
					"source" : [ "analyzer", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "spectrumView", 0 ],
					"source" : [ "analyzer", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "analyzer", 4 ],
					"source" : [ "busReceive", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "analyzer", 0 ],
					"source" : [ "obj-1", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "analyzer", 1 ],
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "analyzer", 2 ],
					"source" : [ "obj-3", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "analyzer", 3 ],
					"source" : [ "obj-4", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 0 ],
					"source" : [ "spectrumView", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "busSend", 0 ],
					"source" : [ "controller", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 1 ],
					"source" : [ "analyzer", 6 ]
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
