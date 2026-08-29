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
      133,
      150,
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
            200,
            112
          ],
          "presentation": 1,
          "presentation_rect": [
            0,
            0,
            200,
            112
          ],
          "varname": "compressor_detector"
        }
      },
      {
        "box": {
          "filename": "Project:/js/Controls/Dial/DialControl.js",
          "id": "threshold",
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
          "varname": "compressor_threshold"
        }
      },
      {
        "box": {
          "filename": "Project:/js/Controls/Dial/DialControl.js",
          "id": "ratio",
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
          "varname": "compressor_ratio"
        }
      },
      {
        "box": {
          "filename": "Project:/js/Controls/Dial/DialControl.js",
          "id": "attack",
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
          "varname": "compressor_attack"
        }
      },
      {
        "box": {
          "filename": "Project:/js/Controls/Dial/DialControl.js",
          "id": "release",
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
          "varname": "compressor_release"
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
            256,
            112,
            64,
            56
          ],
          "presentation": 1,
          "presentation_rect": [
            256,
            112,
            64,
            56
          ],
          "varname": "compressor_gain"
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
            320,
            112,
            64,
            56
          ],
          "presentation": 1,
          "presentation_rect": [
            320,
            112,
            64,
            56
          ],
          "varname": "compressor_mix"
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
            200,
            0,
            96,
            24
          ],
          "presentation": 1,
          "presentation_rect": [
            200,
            0,
            96,
            24
          ],
          "varname": "compressor_bypass"
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
            200,
            24,
            96,
            24
          ],
          "presentation": 1,
          "presentation_rect": [
            200,
            24,
            96,
            24
          ],
          "varname": "compressor_solo"
        }
      },
      {
        "box": {
          "id": "p-threshold",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            216,
            456,
            180,
            22
          ],
          "text": "prepend compressor_threshold"
        }
      },
      {
        "box": {
          "id": "p-ratio",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            216,
            488,
            160,
            22
          ],
          "text": "prepend compressor_ratio"
        }
      },
      {
        "box": {
          "id": "p-attack",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            216,
            520,
            170,
            22
          ],
          "text": "prepend compressor_attack"
        }
      },
      {
        "box": {
          "id": "p-release",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            216,
            552,
            175,
            22
          ],
          "text": "prepend compressor_release"
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
            216,
            584,
            155,
            22
          ],
          "text": "prepend compressor_gain"
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
            216,
            616,
            145,
            22
          ],
          "text": "prepend compressor_mix"
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
            216,
            648,
            170,
            22
          ],
          "text": "prepend compressor_bypass"
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
            216,
            680,
            150,
            22
          ],
          "text": "prepend compressor_solo"
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
            8,
            328,
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
            8,
            368,
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
            216,
            424,
            190,
            22
          ],
          "text": "prepend compressor_detector"
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
            72,
            568,
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
            "p-attack",
            0
          ],
          "source": [
            "attack",
            0
          ]
        }
      },
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
            "p-attack",
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
            "p-ratio",
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
            "p-release",
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
            "p-threshold",
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
            "p-ratio",
            0
          ],
          "source": [
            "ratio",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "p-release",
            0
          ],
          "source": [
            "release",
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
      },
      {
        "patchline": {
          "destination": [
            "p-threshold",
            0
          ],
          "source": [
            "threshold",
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
