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
      476,
      266,
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
          "filename": "Project:/js/Controls/BankManager/BankManagerControl.js",
          "id": "control",
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
            280,
            168
          ],
          "presentation": 1,
          "presentation_rect": [
            0,
            0,
            270,
            165
          ],
          "varname": "bank_manager"
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
            255,
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
            300,
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
            180,
            150,
            22
          ],
          "text": "prepend bank_manager"
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
            0,
            215,
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
            "control",
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
