{
  "patcher": {
    "fileversion": 1,
    "appversion": {
      "major": 9,
      "minor": 0,
      "revision": 9,
      "architecture": "x64"
    },
    "classnamespace": "box",
    "rect": [
      0,
      0,
      280,
      240
    ],
    "boxes": [
      {
        "box": {
          "id": "history_panel",
          "maxclass": "v8ui",
          "filename": "Project:/js/Controls/History/HistoryPanel.js",
          "varname": "history_panel",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            0,
            0,
            280,
            220
          ],
          "presentation": 1,
          "presentation_rect": [
            0,
            0,
            280,
            220
          ]
        }
      },
      {
        "box": {
          "id": "in",
          "maxclass": "inlet",
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            0,
            230,
            30,
            30
          ]
        }
      },
      {
        "box": {
          "id": "router",
          "maxclass": "newobj",
          "text": "v8 Project:/js/PanelBindingHostV8.js",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            50,
            230,
            210,
            22
          ],
          "saved_object_attributes": {
            "filename": "Project:/js/PanelBindingHostV8.js",
            "parameter_enable": 0
          }
        },
        "comment": "Routes history_panel presentation messages."
      },
      {
        "box": {
          "id": "prefix",
          "maxclass": "newobj",
          "text": "prepend history_panel",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            0,
            270,
            160,
            22
          ]
        }
      },
      {
        "box": {
          "id": "out",
          "maxclass": "outlet",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            0,
            305,
            30,
            30
          ]
        }
      }
    ],
    "lines": [
      {
        "patchline": {
          "source": [
            "in",
            0
          ],
          "destination": [
            "router",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "history_panel",
            0
          ],
          "destination": [
            "prefix",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "prefix",
            0
          ],
          "destination": [
            "out",
            0
          ]
        }
      }
    ]
  }
}
