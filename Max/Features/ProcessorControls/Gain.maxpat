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
    "rect": [100.0, 100.0, 360.0, 260.0],
    "openinpresentation": 1,
    "gridsize": [10.0, 10.0],
    "boxes": [
      {
        "box": {
          "id": "control-inlet",
          "maxclass": "inlet",
          "index": 1,
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [""],
          "comment": "Optional local commands: set_gain <0..1>, set_target <0..1>, rms <dB>, enabled <0|1>",
          "patching_rect": [20.0, 20.0, 30.0, 30.0]
        }
      },
      {
        "box": {
          "id": "gain-dial",
          "maxclass": "jsui",
          "filename": "DialControl.js",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [""],
          "parameter_enable": 0,
          "border": 0,
          "embedstate": [
            ["enabled", 1],
            ["valueCount", 2],
            ["primaryValue", 0.5],
            ["secondaryValue", 0.7]
          ],
          "patching_rect": [80.0, 20.0, 90.0, 80.0],
          "presentation": 1,
          "presentation_rect": [0.0, 0.0, 90.0, 80.0],
          "varname": "gain.dial"
        }
      },
      {
        "box": {
          "id": "controller",
          "maxclass": "newobj",
          "text": "js consolidator.gain.controller.js #1",
          "numinlets": 2,
          "numoutlets": 5,
          "outlettype": ["", "", "", "", ""],
          "patching_rect": [80.0, 140.0, 220.0, 22.0]
        }
      },
      {
        "box": {
          "id": "telemetry",
          "maxclass": "newobj",
          "text": "r ---processor.telemetry",
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [""],
          "patching_rect": [180.0, 20.0, 145.0, 22.0]
        }
      },
      {
        "box": {
          "id": "message-bus-out",
          "maxclass": "newobj",
          "text": "r ---state.processor",
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [""],
          "patching_rect": [180.0, 55.0, 135.0, 22.0]
        }
      },
      {
        "box": {
          "id": "definitions-receive",
          "maxclass": "newobj",
          "text": "r ---state.definitions",
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [""],
          "patching_rect": [180.0, 75.0, 145.0, 22.0]
        }
      },
      {
        "box": {
          "id": "processor-limits",
          "maxclass": "newobj",
          "text": "r ---link.control.state",
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [""],
          "patching_rect": [180.0, 90.0, 165.0, 22.0]
        }
      },
      {
        "box": {
          "id": "message-bus-in",
          "maxclass": "newobj",
          "text": "s ---message.bus.in",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [80.0, 200.0, 130.0, 22.0]
        }
      },
      {
        "box": {
          "id": "debug",
          "maxclass": "newobj",
          "text": "print gain",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [225.0, 200.0, 70.0, 22.0]
        }
      },
      {
        "box": {
          "id": "target-send",
          "maxclass": "newobj",
          "text": "s ---input.gain.target",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [225.0, 225.0, 135.0, 22.0]
        }
      },
      {
        "box": {
          "id": "gesture-send",
          "maxclass": "newobj",
          "text": "s ---link.parameter.gesture",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [80.0, 230.0, 175.0, 22.0]
        }
      }
    ],
    "lines": [
      {
        "patchline": {
          "source": ["control-inlet", 0],
          "destination": ["controller", 0]
        }
      },
      {
        "patchline": {
          "source": ["telemetry", 0],
          "destination": ["controller", 0]
        }
      },
      {
        "patchline": {
          "source": ["message-bus-out", 0],
          "destination": ["controller", 0]
        }
      },
      {
        "patchline": {
          "source": ["definitions-receive", 0],
          "destination": ["controller", 0]
        }
      },
      {
        "patchline": {
          "source": ["processor-limits", 0],
          "destination": ["controller", 0]
        }
      },
      {
        "patchline": {
          "source": ["gain-dial", 0],
          "destination": ["controller", 1]
        }
      },
      {
        "patchline": {
          "source": ["controller", 0],
          "destination": ["message-bus-in", 0]
        }
      },
      {
        "patchline": {
          "source": ["controller", 1],
          "destination": ["gain-dial", 0]
        }
      },
      {
        "patchline": {
          "source": ["controller", 2],
          "destination": ["debug", 0]
        }
      },
      {
        "patchline": {
          "source": ["controller", 3],
          "destination": ["target-send", 0]
        }
      },
      {
        "patchline": {
          "source": ["controller", 4],
          "destination": ["gesture-send", 0]
        }
      }
    ],
    "dependency_cache": [
      {
        "name": "DialControl.js",
        "bootpath": "D:/Projects/Ableton/Consolidator/Max/Features/Interface/JS",
        "type": "TEXT",
        "implicit": 1
      },
      {
        "name": "consolidator.gain.controller.js",
        "patcherrelativepath": ".",
        "type": "TEXT",
        "implicit": 1
      },
      {
        "name": "TargetLevelIndicator.js",
        "bootpath": "D:/Projects/Ableton/Consolidator/Max/Features/Shared/JS",
        "type": "TEXT",
        "implicit": 1
      }
    ]
  }
}
