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
      100.0,
      100.0,
      620.0,
      420.0
    ],
    "openinpresentation": 1,
    "gridsize": [
      10.0,
      10.0
    ],
    "boxes": [
      {
        "box": {
          "border": 0,
          "filename": "consolidator.routing.control.js",
          "id": "routing-control",
          "maxclass": "jsui",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "parameter_enable": 0,
          "patching_rect": [
            0.0,
            0.0,
            120.0,
            80.0
          ],
          "presentation": 1,
          "presentation_rect": [
            0.0,
            0.0,
            120.0,
            80.0
          ],
          "varname": "routing.control"
        }
      },
      {
        "box": {
          "id": "controller",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 3,
          "outlettype": [
            "",
            "",
            ""
          ],
          "patching_rect": [
            20.0,
            150.0,
            250.0,
            22.0
          ],
          "saved_object_attributes": {
            "filename": "consolidator.routing.controller.js",
            "parameter_enable": 0
          },
          "text": "js consolidator.routing.controller.js"
        }
      },
      {
        "box": {
          "id": "command-route",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 3,
          "outlettype": [
            "",
            "",
            ""
          ],
          "patching_rect": [
            300.0,
            150.0,
            145.0,
            22.0
          ],
          "text": "route source channel"
        }
      },
      {
        "box": {
          "id": "type-message",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            300.0,
            190.0,
            55.0,
            22.0
          ],
          "text": "type $1"
        }
      },
      {
        "box": {
          "id": "channel-message",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            370.0,
            190.0,
            70.0,
            22.0
          ],
          "text": "channel $1"
        }
      },
      {
        "box": {
          "id": "routing",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 5,
          "outlettype": [
            "",
            "",
            "",
            "",
            ""
          ],
          "patching_rect": [
            300.0,
            240.0,
            210.0,
            22.0
          ],
          "text": "live.routing"
        }
      },
      {
        "box": {
          "id": "source-value-prepend",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            280.0,
            280.0,
            135.0,
            22.0
          ],
          "text": "prepend source_value"
        }
      },
      {
        "box": {
          "id": "channel-value-prepend",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            420.0,
            280.0,
            140.0,
            22.0
          ],
          "text": "prepend channel_value"
        }
      },
      {
        "box": {
          "id": "source-options-prepend",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            280.0,
            310.0,
            145.0,
            22.0
          ],
          "text": "prepend source_options"
        }
      },
      {
        "box": {
          "id": "channel-options-prepend",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            430.0,
            310.0,
            150.0,
            22.0
          ],
          "text": "prepend channel_options"
        }
      },
      {
        "box": {
          "id": "debug",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            120.0,
            190.0,
            125.0,
            22.0
          ],
          "text": "print routing"
        }
      },
      {
        "box": {
          "id": "device",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 3,
          "outlettype": [
            "bang",
            "int",
            "int"
          ],
          "patching_rect": [
            300.0,
            30.0,
            85.0,
            22.0
          ],
          "text": "live.thisdevice"
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
            400.0,
            30.0,
            60.0,
            22.0
          ],
          "text": "loadbang"
        }
      },
      {
        "box": {
          "id": "deferlow",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            "bang"
          ],
          "patching_rect": [
            400.0,
            65.0,
            60.0,
            22.0
          ],
          "text": "deferlow"
        }
      },
      {
        "box": {
          "id": "initialize",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            300.0,
            100.0,
            150.0,
            22.0
          ],
          "text": "port audio_inputs, index 1"
        }
      }
    ],
    "lines": [
      {
        "patchline": {
          "destination": [
            "controller",
            0
          ],
          "source": [
            "routing-control",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "command-route",
            0
          ],
          "source": [
            "controller",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "type-message",
            0
          ],
          "source": [
            "command-route",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "channel-message",
            0
          ],
          "source": [
            "command-route",
            1
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "routing",
            0
          ],
          "source": [
            "type-message",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "routing",
            0
          ],
          "source": [
            "channel-message",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "source-value-prepend",
            0
          ],
          "source": [
            "routing",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "channel-value-prepend",
            0
          ],
          "source": [
            "routing",
            1
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "source-options-prepend",
            0
          ],
          "source": [
            "routing",
            2
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "channel-options-prepend",
            0
          ],
          "source": [
            "routing",
            3
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "controller",
            1
          ],
          "source": [
            "source-value-prepend",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "controller",
            1
          ],
          "source": [
            "channel-value-prepend",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "controller",
            1
          ],
          "source": [
            "source-options-prepend",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "controller",
            1
          ],
          "source": [
            "channel-options-prepend",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "routing-control",
            0
          ],
          "source": [
            "controller",
            1
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "debug",
            0
          ],
          "source": [
            "controller",
            2
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "initialize",
            0
          ],
          "source": [
            "device",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "deferlow",
            0
          ],
          "source": [
            "loadbang",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "initialize",
            0
          ],
          "source": [
            "deferlow",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "routing",
            0
          ],
          "source": [
            "initialize",
            0
          ]
        }
      }
    ],
    "dependency_cache": [
      {
        "name": "consolidator.routing.controller.js",
        "bootpath": "D:/Projects/Ableton/Consolidator/Max/Features/Routing",
        "patcherrelativepath": ".",
        "type": "TEXT",
        "implicit": 1
      },
      {
        "name": "consolidator.routing.control.js",
        "bootpath": "D:/Projects/Ableton/Consolidator/Max/Features/Routing",
        "patcherrelativepath": ".",
        "type": "TEXT",
        "implicit": 1
      }
    ]
  }
}
