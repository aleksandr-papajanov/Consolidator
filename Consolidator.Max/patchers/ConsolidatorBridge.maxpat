{
  "patcher": {
    "modernui": 1,
    "classnamespace": "box",
    "rect": [
      134,
      134,
      1180,
      760
    ],
    "openinpresentation": 1,
    "gridsize": [
      8,
      8
    ],
    "boxes": [
      {
        "box": {
          "comment": "Native control outlet",
          "id": "native-control-in",
          "index": 1,
          "maxclass": "inlet",
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            30,
            30,
            30,
            30
          ]
        }
      },
      {
        "box": {
          "comment": "Native analysis outlet",
          "id": "analysis-in",
          "index": 2,
          "maxclass": "inlet",
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            70,
            30,
            30,
            30
          ]
        }
      },
      {
        "box": {
          "id": "host",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 2,
          "outlettype": [
            "",
            ""
          ],
          "patching_rect": [
            220,
            30,
            310,
            22
          ],
          "saved_object_attributes": {
            "filename": "Project:/js/ConsolidatorUiHost.js",
            "parameter_enable": 0
          },
          "text": "v8 Project:/js/ConsolidatorUiHost.js bridge.local"
        }
      },
      {
        "box": {
          "id": "live-ready",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 3,
          "outlettype": [
            "bang",
            "int",
            "int"
          ],
          "patching_rect": [
            30,
            190,
            105,
            22
          ],
          "text": "live.thisdevice"
        }
      },
      {
        "box": {
          "id": "live-ready-message",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            150,
            190,
            75,
            22
          ],
          "text": "live_ready"
        }
      },
      {
        "box": {
          "id": "live-instance-host",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            560,
            30,
            250,
            22
          ],
          "saved_object_attributes": {
            "filename": "Project:/js/LiveInstanceHost.js",
            "parameter_enable": 0
          },
          "text": "v8 Project:/js/LiveInstanceHost.js"
        }
      },
      {
        "box": {
          "id": "panel-broadcast",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 6,
          "outlettype": [
            "",
            "",
            "",
            "",
            "",
            ""
          ],
          "patching_rect": [
            220,
            160,
            120,
            22
          ],
          "text": "t l l l l l l"
        }
      },
      {
        "box": {
          "args": [
            "bank_manager"
          ],
          "bgmode": 0,
          "border": 0,
          "clickthrough": 0,
          "enablehscroll": 0,
          "enablevscroll": 0,
          "id": "bank-panel",
          "lockeddragscroll": 0,
          "lockedsize": 0,
          "maxclass": "bpatcher",
          "name": "Project:/Panels/BankManagerPanel.maxpat",
          "numinlets": 1,
          "numoutlets": 1,
          "offset": [
            0,
            0
          ],
          "outlettype": [
            ""
          ],
          "patching_rect": [
            30,
            230,
            220,
            220
          ],
          "presentation": 1,
          "presentation_rect": [
            80,
            0,
            280,
            168
          ],
          "viewvisibility": 1
        }
      },
      {
        "box": {
          "args": [
            "equalizer"
          ],
          "bgmode": 0,
          "border": 0,
          "clickthrough": 0,
          "enablehscroll": 0,
          "enablevscroll": 0,
          "id": "eq-panel",
          "lockeddragscroll": 0,
          "lockedsize": 0,
          "maxclass": "bpatcher",
          "name": "Project:/Panels/EqualizerPanel.maxpat",
          "numinlets": 1,
          "numoutlets": 1,
          "offset": [
            0,
            0
          ],
          "outlettype": [
            ""
          ],
          "patching_rect": [
            270,
            230,
            430,
            300
          ],
          "presentation": 1,
          "presentation_rect": [
            360,
            0,
            432,
            168
          ],
          "viewvisibility": 1
        }
      },
      {
        "box": {
          "args": [
            "compressor"
          ],
          "bgmode": 0,
          "border": 0,
          "clickthrough": 0,
          "enablehscroll": 0,
          "enablevscroll": 0,
          "id": "compressor-panel",
          "lockeddragscroll": 0,
          "lockedsize": 0,
          "maxclass": "bpatcher",
          "name": "Project:/Panels/CompressorPanel.maxpat",
          "numinlets": 1,
          "numoutlets": 1,
          "offset": [
            0,
            0
          ],
          "outlettype": [
            ""
          ],
          "patching_rect": [
            720,
            230,
            210,
            315
          ],
          "presentation": 1,
          "presentation_rect": [
            792,
            0,
            376,
            168
          ],
          "viewvisibility": 1
        }
      },
      {
        "box": {
          "args": [
            "saturator"
          ],
          "bgmode": 0,
          "border": 0,
          "clickthrough": 0,
          "enablehscroll": 0,
          "enablevscroll": 0,
          "id": "saturator-panel",
          "lockeddragscroll": 0,
          "lockedsize": 0,
          "maxclass": "bpatcher",
          "name": "Project:/Panels/SaturatorPanel.maxpat",
          "numinlets": 1,
          "numoutlets": 1,
          "offset": [
            0,
            0
          ],
          "outlettype": [
            ""
          ],
          "patching_rect": [
            950,
            230,
            210,
            300
          ],
          "presentation": 1,
          "presentation_rect": [
            1168,
            0,
            288,
            168
          ],
          "viewvisibility": 1
        }
      },
      {
        "box": {
          "args": [
            "input"
          ],
          "bgmode": 0,
          "border": 0,
          "clickthrough": 0,
          "enablehscroll": 0,
          "enablevscroll": 0,
          "id": "input-panel",
          "lockeddragscroll": 0,
          "lockedsize": 0,
          "maxclass": "bpatcher",
          "name": "Project:/Panels/InputGainPanel.maxpat",
          "numinlets": 1,
          "numoutlets": 1,
          "offset": [
            0,
            0
          ],
          "outlettype": [
            ""
          ],
          "patching_rect": [
            30,
            560,
            110,
            80
          ],
          "presentation": 1,
          "presentation_rect": [
            0,
            0,
            80,
            80
          ],
          "viewvisibility": 1
        }
      },
      {
        "box": {
          "args": [
            "output"
          ],
          "bgmode": 0,
          "border": 0,
          "clickthrough": 0,
          "enablehscroll": 0,
          "enablevscroll": 0,
          "id": "output-panel",
          "lockeddragscroll": 0,
          "lockedsize": 0,
          "maxclass": "bpatcher",
          "name": "Project:/Panels/OutputGainPanel.maxpat",
          "numinlets": 1,
          "numoutlets": 1,
          "offset": [
            0,
            0
          ],
          "outlettype": [
            ""
          ],
          "patching_rect": [
            150,
            560,
            110,
            80
          ],
          "presentation": 1,
          "presentation_rect": [
            0,
            88,
            80,
            72
          ],
          "viewvisibility": 1
        }
      },
      {
        "box": {
          "comment": "Commands to native external",
          "id": "outlet-native",
          "index": 1,
          "maxclass": "outlet",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            1000,
            650,
            30,
            30
          ]
        }
      },
      {
        "box": {
          "comment": "Optional UI transport diagnostics",
          "id": "outlet-diagnostic",
          "index": 2,
          "maxclass": "outlet",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            1050,
            650,
            30,
            30
          ]
        }
      },
      {
        "box": {
          "id": "control-list-normalizer",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            560,
            70,
            80,
            22
          ],
          "text": "prepend 0"
        }
      },
      {
        "box": {
          "id": "intent-list-normalizer",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            650,
            70,
            80,
            22
          ],
          "text": "prepend 0"
        }
      },
      {
        "box": {
          "id": "presentation-list-normalizer",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            220,
            120,
            80,
            22
          ],
          "text": "prepend 0"
        }
      }
    ],
    "lines": [
      {
        "patchline": {
          "destination": [
            "intent-list-normalizer",
            0
          ],
          "source": [
            "bank-panel",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "intent-list-normalizer",
            0
          ],
          "source": [
            "compressor-panel",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "intent-list-normalizer",
            0
          ],
          "source": [
            "eq-panel",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "outlet-diagnostic",
            0
          ],
          "order": 0,
          "source": [
            "host",
            1
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "outlet-native",
            0
          ],
          "source": [
            "host",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "presentation-list-normalizer",
            0
          ],
          "order": 1,
          "source": [
            "host",
            1
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "intent-list-normalizer",
            0
          ],
          "source": [
            "input-panel",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "live-ready-message",
            0
          ],
          "source": [
            "live-ready",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "live-instance-host",
            0
          ],
          "source": [
            "live-ready",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "control-list-normalizer",
            0
          ],
          "source": [
            "live-instance-host",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "host",
            0
          ],
          "source": [
            "live-ready-message",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "control-list-normalizer",
            0
          ],
          "source": [
            "native-control-in",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "control-list-normalizer",
            0
          ],
          "source": [
            "analysis-in",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "intent-list-normalizer",
            0
          ],
          "source": [
            "output-panel",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "bank-panel",
            0
          ],
          "source": [
            "panel-broadcast",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "compressor-panel",
            0
          ],
          "source": [
            "panel-broadcast",
            2
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "eq-panel",
            0
          ],
          "source": [
            "panel-broadcast",
            1
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "input-panel",
            0
          ],
          "source": [
            "panel-broadcast",
            4
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "output-panel",
            0
          ],
          "source": [
            "panel-broadcast",
            5
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "saturator-panel",
            0
          ],
          "source": [
            "panel-broadcast",
            3
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "intent-list-normalizer",
            0
          ],
          "source": [
            "saturator-panel",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "control-list-normalizer",
            0
          ],
          "destination": [
            "host",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "intent-list-normalizer",
            0
          ],
          "destination": [
            "host",
            1
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "presentation-list-normalizer",
            0
          ],
          "destination": [
            "panel-broadcast",
            0
          ]
        }
      }
    ],
    "saved_attribute_attributes": {
      "default_plcolor": {
        "expression": ""
      }
    },
    "oscreceiveudpport": 0
  }
}
