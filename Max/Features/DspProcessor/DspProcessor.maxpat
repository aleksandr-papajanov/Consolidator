{
  "patcher": {
    "fileversion": 1,
    "appversion": { "major": 9, "minor": 0, "revision": 9, "architecture": "x64", "modernui": 1 },
    "classnamespace": "box",
    "rect": [100.0, 100.0, 520.0, 260.0],
    "boxes": [
      { "box": { "id": "inputL", "maxclass": "inlet", "index": 1, "numinlets": 0, "numoutlets": 1, "outlettype": ["signal"], "patching_rect": [30.0, 30.0, 30.0, 30.0] } },
      { "box": { "id": "inputR", "maxclass": "inlet", "index": 2, "numinlets": 0, "numoutlets": 1, "outlettype": ["signal"], "patching_rect": [90.0, 30.0, 30.0, 30.0] } },
      { "box": { "id": "bus", "maxclass": "newobj", "text": "r ---state.dsp", "numinlets": 0, "numoutlets": 1, "outlettype": [""], "patching_rect": [180.0, 30.0, 110.0, 22.0] } },
      { "box": { "id": "processor", "maxclass": "newobj", "text": "consolidator.dspprocessor", "numinlets": 3, "numoutlets": 7, "outlettype": ["signal", "signal", "signal", "signal", "", "", ""], "patching_rect": [30.0, 105.0, 180.0, 22.0] } },
      { "box": { "id": "controller", "maxclass": "newobj", "text": "js consolidator.dspprocessor.controller.js", "numinlets": 1, "numoutlets": 1, "outlettype": [""], "patching_rect": [240.0, 105.0, 225.0, 22.0] } },
      { "box": { "id": "print", "maxclass": "newobj", "text": "print consolidator.dspprocessor", "numinlets": 1, "numoutlets": 0, "patching_rect": [240.0, 150.0, 180.0, 22.0] } },
      { "box": { "id": "telemetrySend", "maxclass": "newobj", "text": "s ---processor.telemetry", "numinlets": 1, "numoutlets": 0, "patching_rect": [240.0, 190.0, 145.0, 22.0] } },
      { "box": { "id": "outputL", "maxclass": "outlet", "index": 1, "numinlets": 1, "numoutlets": 0, "patching_rect": [30.0, 190.0, 30.0, 30.0] } },
      { "box": { "id": "outputR", "maxclass": "outlet", "index": 2, "numinlets": 1, "numoutlets": 0, "patching_rect": [90.0, 190.0, 30.0, 30.0] } },
      { "box": { "id": "eqInputL", "maxclass": "outlet", "index": 3, "numinlets": 1, "numoutlets": 0, "patching_rect": [150.0, 190.0, 30.0, 30.0] } },
      { "box": { "id": "eqInputR", "maxclass": "outlet", "index": 4, "numinlets": 1, "numoutlets": 0, "patching_rect": [210.0, 190.0, 30.0, 30.0] } }
    ],
    "lines": [
      { "patchline": { "source": ["inputL", 0], "destination": ["processor", 0] } },
      { "patchline": { "source": ["inputR", 0], "destination": ["processor", 1] } },
      { "patchline": { "source": ["bus", 0], "destination": ["processor", 2] } },
      { "patchline": { "source": ["processor", 0], "destination": ["outputL", 0] } },
      { "patchline": { "source": ["processor", 1], "destination": ["outputR", 0] } },
      { "patchline": { "source": ["processor", 2], "destination": ["eqInputL", 0] } },
      { "patchline": { "source": ["processor", 3], "destination": ["eqInputR", 0] } },
      { "patchline": { "source": ["processor", 4], "destination": ["controller", 0] } },
      { "patchline": { "source": ["processor", 5], "destination": ["controller", 0] } },
      { "patchline": { "source": ["processor", 6], "destination": ["telemetrySend", 0] } },
      { "patchline": { "source": ["controller", 0], "destination": ["print", 0] } }
    ]
  }
}
