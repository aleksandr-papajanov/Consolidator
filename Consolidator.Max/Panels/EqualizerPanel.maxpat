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
      448,
      154,
      1000,
      780
    ],
    "openinpresentation": 1,
    "gridsize": [
      13,
      13
    ],
    "boxes": [
      {
        "box": {
          "filename": "AnalyzerControl.js",
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
            416,
            143
          ],
          "presentation": 1,
          "presentation_rect": [
            0,
            0,
            403,
            130
          ],
          "textfile": {
            "filename": "AnalyzerControl.js",
            "flags": 0,
            "embed": 0,
            "autowatch": 1
          },
          "varname": "equalizer_analyzer"
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
          "filename": "PanelBindingHostV8.js",
          "id": "router",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            0,
            398,
            203,
            22
          ],
          "saved_object_attributes": {
            "parameter_enable": 0
          },
          "text": "v8 Project:/js/PanelBindingHostV8.js",
          "textfile": {
            "filename": "PanelBindingHostV8.js",
            "flags": 0,
            "embed": 0,
            "autowatch": 1
          }
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
            "prefix",
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

