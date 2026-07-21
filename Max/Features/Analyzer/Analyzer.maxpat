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
		"rect" : [ 34.0, 77.0, 1212.0, 891.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 10.0, 10.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"id" : "viewTabs",
					"maxclass" : "live.tab",
					"num_lines_patching" : 1,
					"num_lines_presentation" : 1,
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "", "", "float" ],
					"parameter_enable" : 1,
					"patching_rect" : [ 1215.0, 450.0, 160.0, 22.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 410.0, 140.0, 170.0, 20.0 ],
					"saved_attribute_attributes" : 					{
						"valueof" : 						{
							"parameter_enum" : [ "Spectrum", "Analysis" ],
							"parameter_initial" : [ 0 ],
							"parameter_initial_enable" : 1,
							"parameter_longname" : "Analyzer View",
							"parameter_mmax" : 1,
							"parameter_modmode" : 0,
							"parameter_shortname" : "Analyzer View",
							"parameter_type" : 2,
							"parameter_unitstyle" : 9
						}

					}
,
					"varname" : "analyzer_view"
				}

			}
, 			{
				"box" : 				{
					"id" : "viewSelector",
					"maxclass" : "newobj",
					"numinlets" : 3,
					"numoutlets" : 3,
					"outlettype" : [ "bang", "bang", "" ],
					"patching_rect" : [ 1215.0, 495.0, 49.0, 22.0 ],
					"text" : "sel 0 1"
				}

			}
, 			{
				"box" : 				{
					"id" : "showSpectrum",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1170.0, 540.0, 292.0, 22.0 ],
					"text" : "script show spectrum_view, script hide analysis_view"
				}

			}
, 			{
				"box" : 				{
					"id" : "showAnalysis",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1170.0, 570.0, 292.0, 22.0 ],
					"text" : "script hide spectrum_view, script show analysis_view"
				}

			}
, 			{
				"box" : 				{
					"id" : "viewThispatcher",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 1215.0, 615.0, 74.0, 22.0 ],
					"save" : [ "#N", "thispatcher", ";", "#Q", "end", ";" ],
					"text" : "thispatcher"
				}

			}
, 			{
				"box" : 				{
					"id" : "viewDeviceReady",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "bang", "int", "int" ],
					"patching_rect" : [ 1395.0, 450.0, 83.0, 22.0 ],
					"text" : "live.thisdevice"
				}

			}
, 			{
				"box" : 				{
					"id" : "viewDeferlow",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1395.0, 495.0, 57.0, 22.0 ],
					"text" : "deferlow"
				}

			}
, 			{
				"box" : 				{
					"id" : "viewOutputValue",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1395.0, 540.0, 73.0, 22.0 ],
					"text" : "outputvalue"
				}

			}
, 			{
				"box" : 				{
					"id" : "telemetryReceive",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 990.0, 240.0, 145.0, 22.0 ],
					"text" : "r ---processor.telemetry"
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
					"numoutlets" : 8,
					"outlettype" : [ "", "", "", "", "", "", "", "" ],
					"patching_rect" : [ 315.0, 330.0, 120.249999999999886, 36.0 ],
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
					"filename" : "consolidator.analyzer.spectrum.js",
					"id" : "spectrum",
					"maxclass" : "jsui",
					"numinlets" : 6,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 315.0, 450.0, 648.0, 168.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 405.0, 165.0 ],
					"varname" : "spectrum_view"
				}

			}
, 			{
				"box" : 				{
					"filename" : "consolidator.analyzer.analysis.js",
					"hidden" : 1,
					"id" : "analysis",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 315.0, 675.0, 824.0, 168.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 405.0, 165.0 ],
					"varname" : "analysis_view"
				}

			}
, 			{
				"box" : 				{
					"filename" : "consolidator.analyzer.processormeters.js",
					"id" : "processorMeters",
					"maxclass" : "jsui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 990.0, 450.0, 176.0, 168.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 405.0, 0.0, 180.0, 165.0 ]
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
, 			{
				"box" : 				{
					"id" : "fitReferenceLeft",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 565.0, 195.0, 155.0, 22.0 ],
					"text" : "send~ ---fit.reference.left"
				}

			}
, 			{
				"box" : 				{
					"id" : "fitReferenceRight",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 735.0, 195.0, 160.0, 22.0 ],
					"text" : "send~ ---fit.reference.right"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "fitReferenceLeft", 0 ],
					"source" : [ "obj-3", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "fitReferenceRight", 0 ],
					"source" : [ "obj-4", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "analysis", 0 ],
					"source" : [ "analyzer", 7 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 1 ],
					"source" : [ "analyzer", 5 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "debug", 0 ],
					"source" : [ "analyzer", 6 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "spectrum", 4 ],
					"source" : [ "analyzer", 4 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "spectrum", 3 ],
					"source" : [ "analyzer", 3 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "spectrum", 2 ],
					"order" : 0,
					"source" : [ "analyzer", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "spectrum", 1 ],
					"source" : [ "analyzer", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "spectrum", 0 ],
					"source" : [ "analyzer", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "analyzer", 4 ],
					"order" : 1,
					"source" : [ "busReceive", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "spectrum", 5 ],
					"order" : 0,
					"source" : [ "busReceive", 0 ]
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
					"destination" : [ "viewThispatcher", 0 ],
					"source" : [ "showAnalysis", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "viewThispatcher", 0 ],
					"source" : [ "showSpectrum", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "controller", 0 ],
					"source" : [ "spectrum", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "processorMeters", 0 ],
					"source" : [ "telemetryReceive", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "viewOutputValue", 0 ],
					"source" : [ "viewDeferlow", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "viewDeferlow", 0 ],
					"source" : [ "viewDeviceReady", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "viewTabs", 0 ],
					"source" : [ "viewOutputValue", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "showAnalysis", 0 ],
					"source" : [ "viewSelector", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "showSpectrum", 0 ],
					"source" : [ "viewSelector", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "viewSelector", 0 ],
					"source" : [ "viewTabs", 0 ]
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
