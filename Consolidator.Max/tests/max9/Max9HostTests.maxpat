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
      700,
      320
    ],
    "boxes": [
      {
        "box": {
          "id": "runner",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            30,
            30,
            220,
            22
          ],
          "text": "v8 Max9HostTestRunner.js",
          "varname": "max9_host_runner",
          "saved_object_attributes": {
            "filename": "Max9HostTestRunner.js",
            "parameter_enable": 0
          }
        }
      },
      {
        "box": {
          "id": "router",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "outlettype": [],
          "patching_rect": [
            30,
            70,
            220,
            22
          ],
          "text": "v8 ../Project:/js/PanelBindingHostV8.js",
          "varname": "panel_router",
          "saved_object_attributes": {
            "filename": "../Project:/js/PanelBindingHostV8.js",
            "parameter_enable": 0
          }
        }
      },
      {
        "box": {
          "filename": "V8UiTestProbe.js",
          "id": "probe",
          "maxclass": "v8ui",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            30,
            110,
            220,
            22
          ],
          "parameter_enable": 0,
          "varname": "ui_probe"
        }
      },
      {
        "box": {
          "id": "loadbang",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            "bang"
          ],
          "patching_rect": [
            30,
            190,
            60,
            22
          ],
          "text": "loadbang"
        }
      },
      {
        "box": {
          "id": "out",
          "maxclass": "outlet",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            30,
            260,
            30,
            30
          ]
        }
      },
      {
        "box": {
          "id": "router-message",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            280,
            70,
            110,
            22
          ],
          "text": "0 ui_probe run"
        }
      },
      {
        "box": {
          "id": "router-compile",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            280,
            40,
            55,
            22
          ],
          "text": "compile"
        }
      }
    ],
    "lines": [
      {
        "patchline": {
          "source": [
            "runner",
            0
          ],
          "destination": [
            "out",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "probe",
            0
          ],
          "destination": [
            "out",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "loadbang",
            0
          ],
          "destination": [
            "router-message",
            0
          ],
          "order": 1
        }
      },
      {
        "patchline": {
          "source": [
            "loadbang",
            0
          ],
          "destination": [
            "router-compile",
            0
          ],
          "order": 0
        }
      },
      {
        "patchline": {
          "source": [
            "router-compile",
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
            "router-message",
            0
          ],
          "destination": [
            "router",
            0
          ]
        }
      }
    ]
  }
}
