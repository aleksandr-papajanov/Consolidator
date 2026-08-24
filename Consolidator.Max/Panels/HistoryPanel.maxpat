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
      0.0,
      0.0,
      280.0,
      240.0
    ],
    "boxes": [
      {
        "box": {
          "id": "history_panel",
          "maxclass": "jsui",
          "filename": "Project:/js/Controls/History/HistoryPanel.js",
          "varname": "history_panel",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            0.0,
            0.0,
            280.0,
            220.0
          ],
          "presentation": 1,
          "presentation_rect": [
            0.0,
            0.0,
            280.0,
            220.0
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
            0.0,
            230.0,
            30.0,
            30.0
          ]
        }
      },
      {
        "box": {
          "id": "router",
          "maxclass": "newobj",
          "text": "js Project:/js/PanelBindingHost.js",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            50.0,
            230.0,
            210.0,
            22.0
          ],
          "saved_object_attributes": {
            "filename": "Project:/js/PanelBindingHost.js",
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
            0.0,
            270.0,
            160.0,
            22.0
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
            0.0,
            305.0,
            30.0,
            30.0
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
