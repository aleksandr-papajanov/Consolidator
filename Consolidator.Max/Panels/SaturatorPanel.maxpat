{
  "patcher": {
    "fileversion": 1,
    "appversion": {
      "major": 9,
      "minor": 0,
      "revision": 9,
      "architecture": "x64",
      "modernui": 1
    },
    "classnamespace": "box",
    "rect": [
      441,
      476,
      1000,
      780
    ],
    "openinpresentation" : 1,
    "gridsize" : [ 13.0, 13.0 ],
    "boxes": [
      {
        "box": {
          "filename": "Project:/js/Controls/Analyzer/AnalyzerControl.js",
          "id": "detector",
          "maxclass": "v8ui",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "parameter_enable": 0,
          "patching_rect": [
            0,
            0,
            192,
            112
          ],
          "presentation": 1,
          "presentation_rect": [
            0,
            0,
            192,
            112
          ],
          "varname": "saturator_detector"
        }
      },
      {
        "box": {
          "filename": "Project:/js/Controls/Dial/DialControl.js",
          "id": "drive",
          "maxclass": "v8ui",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "parameter_enable": 0,
          "patching_rect": [
            0,
            112,
            64,
            56
          ],
          "presentation": 1,
          "presentation_rect": [
            0,
            112,
            64,
            56
          ],
          "varname": "saturator_drive"
        }
      },
      {
        "box": {
          "filename": "Project:/js/Controls/Dial/DialControl.js",
          "id": "gain",
          "maxclass": "v8ui",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "parameter_enable": 0,
          "patching_rect": [
            64,
            112,
            64,
            56
          ],
          "presentation": 1,
          "presentation_rect": [
            64,
            112,
            64,
            56
          ],
          "varname": "saturator_gain"
        }
      },
      {
        "box": {
          "filename": "Project:/js/Controls/Dial/DialControl.js",
          "id": "mix",
          "maxclass": "v8ui",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "parameter_enable": 0,
          "patching_rect": [
            128,
            112,
            64,
            56
          ],
          "presentation": 1,
          "presentation_rect": [
            128,
            112,
            64,
            56
          ],
          "varname": "saturator_mix"
        }
      },
      {
        "box": {
          "filename": "Project:/js/Controls/Dial/DialControl.js",
          "id": "detector-amount",
          "maxclass": "v8ui",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "parameter_enable": 0,
          "patching_rect": [
            192,
            112,
            64,
            56
          ],
          "presentation": 1,
          "presentation_rect": [
            192,
            112,
            64,
            56
          ],
          "varname": "saturator_detector_amount"
        }
      },
      {
        "box": {
          "filename": "Project:/js/Controls/Button/ButtonControl.js",
          "id": "bypass",
          "maxclass": "v8ui",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "parameter_enable": 0,
          "patching_rect": [
            192,
            0,
            96,
            24
          ],
          "presentation": 1,
          "presentation_rect": [
            192,
            0,
            96,
            24
          ],
          "varname": "saturator_bypass"
        }
      },
      {
        "box": {
          "filename": "Project:/js/Controls/Button/ButtonControl.js",
          "id": "solo",
          "maxclass": "v8ui",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "parameter_enable": 0,
          "patching_rect": [
            192,
            24,
            96,
            24
          ],
          "presentation": 1,
          "presentation_rect": [
            192,
            24,
            96,
            24
          ],
          "varname": "saturator_solo"
        }
      },
      {
        "box": {
          "id": "p-drive",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            320,
            232,
            150,
            22
          ],
          "text": "prepend saturator_drive"
        }
      },
      {
        "box": {
          "id": "p-gain",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            320,
            264,
            145,
            22
          ],
          "text": "prepend saturator_gain"
        }
      },
      {
        "box": {
          "id": "p-mix",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            320,
            296,
            140,
            22
          ],
          "text": "prepend saturator_mix"
        }
      },
      {
        "box": {
          "id": "p-detector-amount",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            320,
            328,
            220,
            22
          ],
          "text": "prepend saturator_detector_amount"
        }
      },
      {
        "box": {
          "id": "p-bypass",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            320,
            360,
            160,
            22
          ],
          "text": "prepend saturator_bypass"
        }
      },
      {
        "box": {
          "id": "p-solo",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            320,
            392,
            145,
            22
          ],
          "text": "prepend saturator_solo"
        }
      },
      {
        "box": {
          "comment": "",
          "id": "in",
          "index": 1,
          "maxclass": "inlet",
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            0,
            360,
            30,
            30
          ]
        }
      },
      {
        "box": {
          "id": "router",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            0,
            406,
            198,
            22
          ],
          "saved_object_attributes": {
            "filename": "Project:/js/PanelBindingHostV8.js",
            "parameter_enable": 0
          },
          "text": "v8 Project:/js/PanelBindingHostV8.js"
        }
      },
      {
        "box": {
          "id": "prefix",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            320,
            200,
            190,
            22
          ],
          "text": "prepend saturator_detector"
        }
      },
      {
        "box": {
          "comment": "",
          "id": "out",
          "index": 1,
          "maxclass": "outlet",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            160,
            328,
            30,
            30
          ]
        }
      }
    ],
    "lines": [
      {
        "patchline": {
          "destination": [
            "p-bypass",
            0
          ],
          "source": [
            "bypass",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "prefix",
            0
          ],
          "source": [
            "detector",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "p-detector-amount",
            0
          ],
          "source": [
            "detector-amount",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "p-drive",
            0
          ],
          "source": [
            "drive",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "p-gain",
            0
          ],
          "source": [
            "gain",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "router",
            0
          ],
          "source": [
            "in",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "p-mix",
            0
          ],
          "source": [
            "mix",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "out",
            0
          ],
          "source": [
            "p-bypass",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "out",
            0
          ],
          "source": [
            "p-detector-amount",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "out",
            0
          ],
          "source": [
            "p-drive",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "out",
            0
          ],
          "source": [
            "p-gain",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "out",
            0
          ],
          "source": [
            "p-mix",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "out",
            0
          ],
          "source": [
            "p-solo",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "out",
            0
          ],
          "source": [
            "prefix",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "p-solo",
            0
          ],
          "source": [
            "solo",
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
