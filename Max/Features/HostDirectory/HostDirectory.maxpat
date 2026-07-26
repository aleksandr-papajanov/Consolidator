{
  "patcher": {
    "fileversion": 1,
    "appversion": { "major": 9, "minor": 0, "revision": 9, "architecture": "x64", "modernui": 1 },
    "classnamespace": "box",
    "rect": [100.0, 100.0, 370.0, 140.0],
    "openinpresentation": 1,
    "boxes": [
      { "box": { "id": "directory", "maxclass": "jsui", "filename": "consolidator.hostdirectory.js", "numinlets": 1, "numoutlets": 1, "outlettype": [""], "patching_rect": [0.0, 0.0, 370.0, 140.0], "presentation": 1, "presentation_rect": [0.0, 0.0, 370.0, 140.0], "parameter_enable": 0, "varname": "host.directory" } },
      { "box": { "id": "global-in", "maxclass": "newobj", "text": "r consolidator.host.bus", "numinlets": 0, "numoutlets": 1, "outlettype": [""], "patching_rect": [20.0, 170.0, 140.0, 22.0] } },
      { "box": { "id": "global-out", "maxclass": "newobj", "text": "s consolidator.host.bus", "numinlets": 1, "numoutlets": 0, "patching_rect": [180.0, 170.0, 140.0, 22.0] } },
      { "box": { "id": "this-device", "maxclass": "newobj", "text": "live.thisdevice", "numinlets": 1, "numoutlets": 3, "outlettype": ["bang", "int", "int"], "patching_rect": [20.0, 210.0, 85.0, 22.0] } },
      { "box": { "id": "defer", "maxclass": "newobj", "text": "deferlow", "numinlets": 1, "numoutlets": 1, "outlettype": ["bang"], "patching_rect": [120.0, 210.0, 60.0, 22.0] } },
      { "box": { "id": "refresh", "maxclass": "message", "text": "refresh", "numinlets": 2, "numoutlets": 1, "outlettype": [""], "patching_rect": [195.0, 210.0, 50.0, 22.0] } }
    ],
    "lines": [
      { "patchline": { "source": ["global-in", 0], "destination": ["directory", 0] } },
      { "patchline": { "source": ["directory", 0], "destination": ["global-out", 0] } },
      { "patchline": { "source": ["this-device", 0], "destination": ["defer", 0] } },
      { "patchline": { "source": ["defer", 0], "destination": ["refresh", 0] } },
      { "patchline": { "source": ["refresh", 0], "destination": ["directory", 0] } }
    ],
    "dependency_cache": [
      { "name": "consolidator.hostdirectory.js", "bootpath": "D:/Projects/Ableton/Consolidator/Max/Features/HostDirectory", "patcherrelativepath": ".", "type": "TEXT", "implicit": 1 }
    ]
  }
}
