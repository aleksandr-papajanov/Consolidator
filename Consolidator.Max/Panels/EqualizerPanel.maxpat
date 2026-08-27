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
      668,
      221,
      1000,
      780
    ],
    "gridsize": [
      8,
      8
    ],
    "boxes": [
      {
        "box": {
          "filename": "Project:/js/Controls/Analyzer/AnalyzerControl.js",
          "id": "analyzer",
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
            432,
            144
          ],
          "presentation": 1,
          "presentation_rect": [
            0,
            0,
            432,
            144
          ],
          "varname": "equalizer_analyzer"
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
            224,
            144,
            104,
            24
          ],
          "presentation": 1,
          "presentation_rect": [
            224,
            144,
            104,
            24
          ],
          "varname": "equalizer_bypass"
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
            328,
            144,
            104,
            24
          ],
          "presentation": 1,
          "presentation_rect": [
            328,
            144,
            104,
            24
          ],
          "varname": "equalizer_solo"
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
            352,
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
            398,
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
            0,
            208,
            180,
            22
          ],
          "text": "prepend equalizer_analyzer"
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
            224,
            208,
            160,
            22
          ],
          "text": "prepend equalizer_bypass"
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
            328,
            240,
            145,
            22
          ],
          "text": "prepend equalizer_solo"
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
            224,
            312,
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
            "prefix",
            0
          ],
          "source": [
            "analyzer",
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
