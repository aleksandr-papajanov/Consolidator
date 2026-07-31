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
      760.0,
      260.0
    ],
    "boxes": [
      {
        "box": {
          "id": "input",
          "maxclass": "inlet",
          "index": 1,
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            20.0,
            20.0,
            30.0,
            30.0
          ],
          "comment": "Host events and snapshots"
        }
      },
      {
        "box": {
          "id": "router",
          "maxclass": "newobj",
          "text": "js consolidator.statetransport.js",
          "numinlets": 2,
          "numoutlets": 9,
          "outlettype": [
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            ""
          ],
          "patching_rect": [
            80.0,
            25.0,
            210.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "events",
          "maxclass": "newobj",
          "text": "s ---message.bus.out",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            20.0,
            100.0,
            135.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "eq",
          "maxclass": "newobj",
          "text": "s ---state.eq",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            165.0,
            100.0,
            100.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "dsp",
          "maxclass": "newobj",
          "text": "s ---state.dsp",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            275.0,
            100.0,
            105.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "device",
          "maxclass": "newobj",
          "text": "s ---state.device",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            545.0,
            100.0,
            115.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "processor",
          "maxclass": "newobj",
          "text": "s ---state.processor",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            670.0,
            75.0,
            135.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "analyzer",
          "maxclass": "newobj",
          "text": "s ---state.analyzer",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            670.0,
            100.0,
            125.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "parameter",
          "maxclass": "newobj",
          "text": "s ---dsp.parameter",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            805.0,
            100.0,
            130.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "eq-preview",
          "maxclass": "newobj",
          "text": "r ---link.control.analyzer",
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            300.0,
            25.0,
            145.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "analyzer-ui",
          "maxclass": "newobj",
          "text": "s ---analyzer.ui",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            670.0,
            130.0,
            120.0,
            22.0
          ]
        }
      }
    ],
    "lines": [
      {
        "patchline": {
          "source": [
            "input",
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
            "router",
            0
          ],
          "destination": [
            "events",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "router",
            1
          ],
          "destination": [
            "eq",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "router",
            2
          ],
          "destination": [
            "dsp",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "router",
            4
          ],
          "destination": [
            "device",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "router",
            5
          ],
          "destination": [
            "processor",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "router",
            6
          ],
          "destination": [
            "analyzer",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "router",
            7
          ],
          "destination": [
            "parameter",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "eq-preview",
            0
          ],
          "destination": [
            "router",
            1
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "router",
            8
          ],
          "destination": [
            "analyzer-ui",
            0
          ]
        }
      }
    ],
    "dependency_cache": [
      {
        "name": "consolidator.statetransport.js",
        "patcherrelativepath": ".",
        "type": "TEXT",
        "implicit": 1
      }
    ]
  }
}
