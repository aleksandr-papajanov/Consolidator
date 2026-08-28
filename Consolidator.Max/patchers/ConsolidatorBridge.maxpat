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
		"rect" : [ 134.0, 134.0, 1180.0, 760.0 ],
		"openinpresentation" : 1,
		"gridsize" : [ 8.0, 8.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"bgmode" : 0,
					"border" : 0,
					"clickthrough" : 0,
					"enablehscroll" : 0,
					"enablevscroll" : 0,
					"id" : "obj-1",
					"lockeddragscroll" : 0,
					"lockedsize" : 0,
					"maxclass" : "bpatcher",
					"name" : "HistoryPanel.maxpat",
					"numinlets" : 1,
					"numoutlets" : 1,
					"offset" : [ 0.0, 0.0 ],
					"outlettype" : [ "" ],
					"patching_rect" : [ 354.0, 583.0, 286.0, 192.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 0.0, 0.0, 224.0, 168.0 ],
					"viewvisibility" : 1
				}

			}
, 			{
				"box" : 				{
					"comment" : "Native control outlet",
					"id" : "native-control-in",
					"index" : 1,
					"maxclass" : "inlet",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 30.0, 30.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"comment" : "Native analysis outlet",
					"id" : "analysis-in",
					"index" : 2,
					"maxclass" : "inlet",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 70.0, 30.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"filename" : "ConsolidatorUiHost.js",
					"id" : "host",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 220.0, 30.0, 310.0, 22.0 ],
					"saved_object_attributes" : 					{
						"parameter_enable" : 0
					}
,
					"text" : "v8 Project:/js/ConsolidatorUiHost.js bridge.local",
					"textfile" : 					{
						"filename" : "ConsolidatorUiHost.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}

				}

			}
, 			{
				"box" : 				{
					"id" : "live-ready",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "bang", "int", "int" ],
					"patching_rect" : [ 30.0, 190.0, 105.0, 22.0 ],
					"text" : "live.thisdevice"
				}

			}
, 			{
				"box" : 				{
					"id" : "live-ready-message",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 150.0, 190.0, 75.0, 22.0 ],
					"text" : "live_ready"
				}

			}
, 			{
				"box" : 				{
					"filename" : "LiveInstanceHost.js",
					"id" : "live-instance-host",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 560.0, 30.0, 250.0, 22.0 ],
					"saved_object_attributes" : 					{
						"parameter_enable" : 0
					}
,
					"text" : "v8 Project:/js/LiveInstanceHost.js",
					"textfile" : 					{
						"filename" : "LiveInstanceHost.js",
						"flags" : 0,
						"embed" : 0,
						"autowatch" : 1
					}

				}

			}
, 			{
				"box" : 				{
					"id" : "panel-broadcast",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 6,
					"outlettype" : [ "", "", "", "", "", "" ],
					"patching_rect" : [ 220.0, 160.0, 120.0, 22.0 ],
					"text" : "t l l l l l l"
				}

			}
, 			{
				"box" : 				{
					"args" : [ "bank_manager" ],
					"bgmode" : 0,
					"border" : 0,
					"clickthrough" : 0,
					"enablehscroll" : 0,
					"enablevscroll" : 0,
					"id" : "bank-panel",
					"lockeddragscroll" : 0,
					"lockedsize" : 0,
					"maxclass" : "bpatcher",
					"name" : "BankManagerPanel.maxpat",
					"numinlets" : 1,
					"numoutlets" : 1,
					"offset" : [ 0.0, 0.0 ],
					"outlettype" : [ "" ],
					"patching_rect" : [ 30.0, 230.0, 220.0, 220.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 304.0, 0.0, 280.0, 168.0 ],
					"viewvisibility" : 1
				}

			}
, 			{
				"box" : 				{
					"args" : [ "equalizer" ],
					"bgmode" : 0,
					"border" : 0,
					"clickthrough" : 0,
					"enablehscroll" : 0,
					"enablevscroll" : 0,
					"id" : "eq-panel",
					"lockeddragscroll" : 0,
					"lockedsize" : 0,
					"maxclass" : "bpatcher",
					"name" : "EqualizerPanel.maxpat",
					"numinlets" : 1,
					"numoutlets" : 1,
					"offset" : [ 0.0, 0.0 ],
					"outlettype" : [ "" ],
					"patching_rect" : [ 270.0, 230.0, 430.0, 300.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 584.0, 0.0, 432.0, 168.0 ],
					"viewvisibility" : 1
				}

			}
, 			{
				"box" : 				{
					"args" : [ "compressor" ],
					"bgmode" : 0,
					"border" : 0,
					"clickthrough" : 0,
					"enablehscroll" : 0,
					"enablevscroll" : 0,
					"id" : "compressor-panel",
					"lockeddragscroll" : 0,
					"lockedsize" : 0,
					"maxclass" : "bpatcher",
					"name" : "CompressorPanel.maxpat",
					"numinlets" : 1,
					"numoutlets" : 1,
					"offset" : [ 0.0, 0.0 ],
					"outlettype" : [ "" ],
					"patching_rect" : [ 720.0, 230.0, 210.0, 315.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 1016.0, 0.0, 376.0, 168.0 ],
					"viewvisibility" : 1
				}

			}
, 			{
				"box" : 				{
					"args" : [ "saturator" ],
					"bgmode" : 0,
					"border" : 0,
					"clickthrough" : 0,
					"enablehscroll" : 0,
					"enablevscroll" : 0,
					"id" : "saturator-panel",
					"lockeddragscroll" : 0,
					"lockedsize" : 0,
					"maxclass" : "bpatcher",
					"name" : "SaturatorPanel.maxpat",
					"numinlets" : 1,
					"numoutlets" : 1,
					"offset" : [ 0.0, 0.0 ],
					"outlettype" : [ "" ],
					"patching_rect" : [ 950.0, 230.0, 210.0, 300.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 1392.0, 0.0, 288.0, 168.0 ],
					"viewvisibility" : 1
				}

			}
, 			{
				"box" : 				{
					"args" : [ "input" ],
					"bgmode" : 0,
					"border" : 0,
					"clickthrough" : 0,
					"enablehscroll" : 0,
					"enablevscroll" : 0,
					"id" : "input-panel",
					"lockeddragscroll" : 0,
					"lockedsize" : 0,
					"maxclass" : "bpatcher",
					"name" : "InputGainPanel.maxpat",
					"numinlets" : 1,
					"numoutlets" : 1,
					"offset" : [ 0.0, 0.0 ],
					"outlettype" : [ "" ],
					"patching_rect" : [ 30.0, 560.0, 110.0, 80.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 224.0, 0.0, 80.0, 80.0 ],
					"viewvisibility" : 1
				}

			}
, 			{
				"box" : 				{
					"args" : [ "output" ],
					"bgmode" : 0,
					"border" : 0,
					"clickthrough" : 0,
					"enablehscroll" : 0,
					"enablevscroll" : 0,
					"id" : "output-panel",
					"lockeddragscroll" : 0,
					"lockedsize" : 0,
					"maxclass" : "bpatcher",
					"name" : "OutputGainPanel.maxpat",
					"numinlets" : 1,
					"numoutlets" : 1,
					"offset" : [ 0.0, 0.0 ],
					"outlettype" : [ "" ],
					"patching_rect" : [ 150.0, 560.0, 110.0, 80.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 224.0, 88.0, 80.0, 72.0 ],
					"viewvisibility" : 1
				}

			}
, 			{
				"box" : 				{
					"comment" : "Commands to native external",
					"id" : "outlet-native",
					"index" : 1,
					"maxclass" : "outlet",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 1000.0, 650.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"comment" : "Optional UI transport diagnostics",
					"id" : "outlet-diagnostic",
					"index" : 2,
					"maxclass" : "outlet",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 1050.0, 650.0, 30.0, 30.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "control-list-normalizer",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 560.0, 70.0, 80.0, 22.0 ],
					"text" : "prepend 0"
				}

			}
, 			{
				"box" : 				{
					"id" : "intent-list-normalizer",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 650.0, 70.0, 80.0, 22.0 ],
					"text" : "prepend 0"
				}

			}
, 			{
				"box" : 				{
					"id" : "presentation-list-normalizer",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 220.0, 120.0, 80.0, 22.0 ],
					"text" : "prepend 0"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "control-list-normalizer", 0 ],
					"source" : [ "analysis-in", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "intent-list-normalizer", 0 ],
					"source" : [ "bank-panel", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "intent-list-normalizer", 0 ],
					"source" : [ "compressor-panel", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "host", 0 ],
					"source" : [ "control-list-normalizer", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "intent-list-normalizer", 0 ],
					"source" : [ "eq-panel", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "outlet-diagnostic", 0 ],
					"order" : 0,
					"source" : [ "host", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "outlet-native", 0 ],
					"source" : [ "host", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "presentation-list-normalizer", 0 ],
					"order" : 1,
					"source" : [ "host", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "intent-list-normalizer", 0 ],
					"source" : [ "input-panel", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "host", 1 ],
					"source" : [ "intent-list-normalizer", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "control-list-normalizer", 0 ],
					"source" : [ "live-instance-host", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "live-instance-host", 0 ],
					"order" : 0,
					"source" : [ "live-ready", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "live-ready-message", 0 ],
					"order" : 1,
					"source" : [ "live-ready", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "host", 0 ],
					"source" : [ "live-ready-message", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "control-list-normalizer", 0 ],
					"source" : [ "native-control-in", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "intent-list-normalizer", 0 ],
					"source" : [ "output-panel", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "bank-panel", 0 ],
					"source" : [ "panel-broadcast", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "compressor-panel", 0 ],
					"source" : [ "panel-broadcast", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "eq-panel", 0 ],
					"source" : [ "panel-broadcast", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "input-panel", 0 ],
					"source" : [ "panel-broadcast", 4 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "output-panel", 0 ],
					"source" : [ "panel-broadcast", 5 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "saturator-panel", 0 ],
					"source" : [ "panel-broadcast", 3 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "panel-broadcast", 0 ],
					"source" : [ "presentation-list-normalizer", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "intent-list-normalizer", 0 ],
					"source" : [ "saturator-panel", 0 ]
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
